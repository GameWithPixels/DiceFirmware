#include "anim_controller.h"
#include "animations/animation.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/flash.h"
#include "utils/utils.h"
#include "utils/rainbow.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_log.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "leds.h"
#include "drivers_nrf/scheduler.h"
#include "core/delegate_array.h"
#include "animations/animation_instance_allocator.h"
#include "profile/profile_static.h"
#include "accelerometer.h"

using namespace Animations;
using namespace Modules;
using namespace Config;
using namespace DriversNRF;
using namespace Bluetooth;

#define MAX_ANIMS 20
#define FORCE_FADE_OUT_DURATION_MS 500

namespace Modules::AnimController
{
    static DelegateArray<AnimControllerClientMethod, 1> clients;

    // Our currently running animations
    static Animations::AnimationInstance *animations[MAX_ANIMS];
    static int animationCount = 0;

    enum State
    {
        State_Unknown = 0,
        State_Initializing,
        State_Off,
        State_On,
    };
    State currentState = State_Unknown;

    // Our animation globals used by animation instances
    AnimationContextGlobals globals;

    // Some local functions
    void update(int ms);
    void onProgrammingEvent(void* context, Flash::ProgrammingEventType evt);
    void printAnimControllerStateHandler(const Message *msg);

    // Update timer
    APP_TIMER_DEF(animControllerTimer);
    // To be passed to the timer
    static int animControllerTicks = 0;
    void animationControllerUpdate(void* param)
    {
        animControllerTicks++;
        int timeMs = animControllerTicks * ANIM_FRAME_DURATION;
        update(timeMs);
    }

    /// <summary>
    /// Kick off the animation controller, registering it with the Timer system
    /// </summary>
    void init()
    {
        currentState = State_Initializing;
        Flash::hookProgrammingEvent(onProgrammingEvent, nullptr);
        MessageService::RegisterMessageHandler(Message::MessageType_PrintAnimControllerState, printAnimControllerStateHandler);
        Timers::createTimer(&animControllerTimer, APP_TIMER_MODE_REPEATED, animationControllerUpdate);

        NRF_LOG_DEBUG("Anim Controller init");

        currentState = State_Off;
        start();
    }

#pragma GCC diagnostic push "-Wstack-usage="
#pragma GCC diagnostic ignored "-Wstack-usage="

    /// <summary>
    /// Update all currently running animations, and performing housekeeping when necessary
    /// </summary>
    /// <param name="ms">Current global time in milliseconds</param>
    void update(int ms)
    {
        const auto ledCount = BoardManager::getBoard()->ledCount;
        const auto faceCount = DiceVariants::getLayout()->faceCount;

        // Update our globals
        globals.ledCount = ledCount;
        globals.currentFace = Accelerometer::currentFace();
        globals.normalizedCurrentFace = globals.currentFace * 0xFFFF / faceCount;

        if (animationCount > 0) {
            // Notify clients for feeding or not feeding PowerManager
            Scheduler::push(nullptr, 0, [](void *p_event_data, uint16_t event_size) {
                for (int i = 0; i < clients.Count(); ++i) {
                  clients[i].handler(clients[i].token);
                }
            });

            // clear the global color array
            uint32_t allColors[MAX_COUNT];
            for (int j = 0; j < ledCount; ++j) {
                allColors[j] = 0;
            }

            for (int i = 0; i < animationCount; ++i) {
                auto anim = animations[i];

                bool fade = anim->forceFadeTime != -1;

                const auto duration = anim->animationPreset->duration;
                int endTime = anim->startTime + duration;
                uint32_t fadePercentTimes1000 = 1000;
                if (anim->loopCount > 1 && ms > endTime) {
                    // Yes, update anim start time so next if statement updates the animation
                    anim->loopCount--;
                    anim->startTime += duration;
                    endTime += duration;
                } else if (fade) {
                    endTime = anim->forceFadeTime;
                    fadePercentTimes1000 = 1000 * (endTime - ms) / FORCE_FADE_OUT_DURATION_MS;
                }

                // Update normalized animation time
                globals.normalizedAnimationTime = 0xffff * (ms - anim->startTime) / duration;

                if (ms > endTime)
                {
                    // The animation is over, get rid of it!
                    AnimationInstanceAllocator::DestroyInstance(anim);

                    // Shift the other animations
                    for (int j = i; j < animationCount - 1; ++j)
                    {
                        animations[j] = animations[j + 1];
                    }

                    // Reduce the count
                    animationCount--;

                    // Decrement loop counter since we just replaced the current anim
                    i--;
                }
                else
                {
                    int canonIndices[MAX_COUNT * 4]; // Allow up to 4 tracks to target the same LED
                    int ledIndices[MAX_COUNT * 4];
                    uint32_t colors[MAX_COUNT * 4];

                    // Update the LEDs
                    int animTrackCount = anim->updateLEDs(ms, canonIndices, colors);

                    // This is a little bit of a hack, but to keep one single default profile
                    // for all dice types, filter the LEDs here to only keep one and make it the top face.
                    if (animTrackCount > 0 && anim->animationPreset->animFlags & AnimationFlags_HighestLed) {
                        // Override the faces so there is only the highest led color
                        animTrackCount = 1;
                        canonIndices[0] = faceCount - 1;
                    }

                    // Gamma correct and map face index to led index
                    //NRF_LOG_INFO("track_count = %d", animTrackCount);
                    if (anim->animationPreset->animFlags & AnimationFlags_UseLedIndices) {
                        // animation is working with led indices, not face indices
                        memcpy(ledIndices, canonIndices, animTrackCount * sizeof(int));
                    } else {
                        // Compute electrical indices from face indices
                        for (int j = 0; j < animTrackCount; ++j) {
                            ledIndices[j] = DiceVariants::animIndexToLEDIndex(canonIndices[j], anim->remapFace);
                        }
                    }

                    // Update color array
                    for (int j = 0; j < animTrackCount; ++j) {

                        // Fade out all LEDs if necessary
                        auto color = colors[j];
                        if (fade) {
                            color = Utils::scaleColor(color, fadePercentTimes1000);
                        }
                        
                        // Combine colors if necessary
                        //NRF_LOG_INFO("index: %d -> %08x", ledIndices[j], colors[j]);
                        allColors[ledIndices[j]] = Utils::addColors(allColors[ledIndices[j]], color);
                    }
                }
            }
            
            // And light up!
            uint8_t brightness = Profile::Static::getData()->getBrightness();
            for (int j = 0; j < ledCount; ++j) {
                allColors[j] = Utils::modulateColor(allColors[j], brightness);
            }
            LEDs::setPixelColors(allColors);
        }
    }

#pragma GCC diagnostic pop "-Wstack-usage="

    /// <summary>
    /// Stop updating animations
    /// </summary>
    void stop()
    {
        switch (currentState) {
            case State_On:
                Timers::stopTimer(animControllerTimer);
                // Clear all data
                stopAll();
                NRF_LOG_DEBUG("Stopped anim controller");
                currentState = State_Off;
                break;
            default:
                NRF_LOG_WARNING("Anim Controller in invalid state to stop");
                break;
        }
    }

    void start()
    {
        switch (currentState) {
            case State_Off:
                NRF_LOG_DEBUG("Starting anim controller");
                Timers::startTimer(animControllerTimer, ANIM_FRAME_DURATION, NULL);
                currentState = State_On;
                break;
            default:
                NRF_LOG_WARNING("Anim Controller in invalid state to start");
                break;
        }
    }

    PlayAnimationParameters::PlayAnimationParameters()
        : buffer(Profile::Static::getData()->getBuffer())
        , overrideBuffer(Profile::BufferDescriptor::nullDescriptor)
        , overrides(Profile::Array<Animations::ParameterOverride>::emptyArray())
        , tag(Animations::AnimationTag_Unknown)
        , remapFace(0)
        , loopCount(1)
    {
    }

    void play(const Animations::Animation* animationPreset, const PlayAnimationParameters& parameters) {

        // Determine face
        uint8_t remapFace = 0;
        switch (parameters.remapFace) {
            case FACE_INDEX_CURRENT_FACE:
                remapFace = globals.currentFace;
                break;
            case FACE_INDEX_HIGHEST_FACE:
                remapFace = DiceVariants::getLayout()->faceCount-1;
                break;
            default:
                remapFace = parameters.remapFace;
                break;
        }

        // Is there already an animation for this?
        int prevAnimIndex = 0;
        for (; prevAnimIndex < animationCount; ++prevAnimIndex)
        {
            auto prevAnim = animations[prevAnimIndex];
            if (prevAnim->animationPreset == animationPreset && prevAnim->remapFace == remapFace)
            {
                break;
            }
        }

        int ms = animControllerTicks * ANIM_FRAME_DURATION;
        if (prevAnimIndex < animationCount)
        {
            // Fade out the previous animation pretty quickly
            animations[prevAnimIndex]->forceFadeOut(ms + FORCE_FADE_OUT_DURATION_MS);
        }
        
        if (animationCount < MAX_ANIMS)
        {
            AnimationInstanceAllocator allocator(&globals, parameters.buffer, parameters.overrideBuffer, parameters.overrides);
            const auto anim = allocator.CreateInstance(animationPreset);
            if (anim) {
                // Add a new animation
                animations[animationCount] = anim;
                animations[animationCount]->setTag(parameters.tag);
                animations[animationCount]->start(ms, remapFace, parameters.loopCount);
                animationCount++;
            }
        }
        // Else there is no more room
    }

    void play(const Animation * animationPreset, uint8_t remapFace) {
        PlayAnimationParameters params;
        params.remapFace = remapFace;
        play(animationPreset, params);
    }

    void stop(const Animation* animationPreset, uint8_t remapFace) {

        // Find the animation with that preset and remap face
        int prevAnimIndex = 0;
        AnimationInstance* prevAnimInstance = nullptr;
        for (; prevAnimIndex < animationCount; ++prevAnimIndex)
        {
            auto instance = animations[prevAnimIndex];
            if (instance->animationPreset == animationPreset && (remapFace == 255 || instance->remapFace == remapFace))
            {
                prevAnimInstance = instance;
                break;
            }
        }

        if (prevAnimIndex < animationCount)
        {
            removeAtIndex(prevAnimIndex);

            // Delete the instance
            AnimationInstanceAllocator::DestroyInstance(prevAnimInstance);
        }
        // Else the animation isn't playing
    }

    void fadeOutAnimsWithTag(Animations::AnimationTag tagToStop, int fadeOutTimeMs) {

        // Is there already an animation for this?
        int ms = animControllerTicks * ANIM_FRAME_DURATION;
        for (int prevAnimIndex = 0; prevAnimIndex < animationCount; ++prevAnimIndex)
        {
            auto prevAnim = animations[prevAnimIndex];
            if (prevAnim->tag == tagToStop)
            {
                // Fade out the previous animation pretty quickly
                prevAnim->forceFadeOut(ms + fadeOutTimeMs);
            }
        }
    }

    /// <summary>
    /// Stop all currently running animations
    /// </summary>
    void stopAll()
    {
        for (int i = 0; i < animationCount; ++i)
        {
            // Delete the instance
            AnimationInstanceAllocator::DestroyInstance(animations[i]);
        }
        animationCount = 0;
        LEDs::clear();
    }

    /// <summary>
    /// Helper function to clear anim LED turned on by a current animation
    /// </summary>
    void stopAtIndex(int animIndex)
    {
        // Found the animation, start by killing the LEDs it controls
        int canonIndices[MAX_COUNT];
        int ledIndices[MAX_COUNT];
        uint32_t zeros[MAX_COUNT];
        memset(zeros, 0, sizeof(uint32_t) * MAX_COUNT);
        auto anim = animations[animIndex];
        int ledCount = anim->stop(canonIndices);
        for (int i = 0; i < ledCount; ++i) {
            ledIndices[i] = DiceVariants::animIndexToLEDIndex(canonIndices[i], anim->remapFace);
        }
        LEDs::setPixelColors(ledIndices, zeros, ledCount);
    }

    /// <summary>
    /// Helper method: Stop the animation at the given index. Used by Stop(IAnimation*)
    /// </summary>
    void removeAtIndex(int animIndex)
    {
        stopAtIndex(animIndex);

        // Shift the other animations
        for (; animIndex < animationCount - 1; ++animIndex)
        {
            animations[animIndex] = animations[animIndex + 1];
        }

        // Reduce the count
        animationCount--;
    }

    void onProgrammingEvent(void* context, Flash::ProgrammingEventType evt){
        if (evt == Flash::ProgrammingEventType_Begin) {
            stop();
        } else {
            start();
        }
    }

    void printAnimControllerStateHandler(const Message* msg) {
        NRF_LOG_DEBUG("Anim Controller has %d anims", animationCount);
        for (int i = 0; i < animationCount; ++i) {
            AnimationInstance* anim = animations[i];
            NRF_LOG_DEBUG("Anim %d is of type %d, duration %d", i, anim->animationPreset->type, anim->animationPreset->duration);
            NRF_LOG_DEBUG("StartTime %d, remapFace %d, loopCount %d", anim->startTime, anim->remapFace, anim->loopCount);
        }
    }

    /// <summary>
    /// Method used by clients to request timer callbacks
    /// </summary>
    void hook(AnimControllerClientMethod method, void* parameter) {
        if (!clients.Register(parameter, method)) {
            NRF_LOG_ERROR("Failed to register AnimationController hook because client array is full");
        }
    }

    /// <summary>
    /// Method used by clients to stop getting callbacks
    /// </summary>
    void unHook(AnimControllerClientMethod method) {
        clients.UnregisterWithHandler(method);
    }
}
