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

	// Some local functions
	void update(int ms);
	uint32_t getColorForAnim(void* token, uint32_t colorIndex);
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

	/// <summary>
	/// Update all currently running animations, and performing housekeeping when necessary
	/// </summary>
	/// <param name="ms">Current global time in milliseconds</param>
	void update(int ms)
	{
		auto b = BoardManager::getBoard();
		int c = b->ledCount;

		if (animationCount > 0) {
			// Notify clients for feeding or not feeding PowerManager
			Scheduler::push(nullptr, 0, [](void *p_event_data, uint16_t event_size) {
				for (int i = 0; i < clients.Count(); ++i) {
              	clients[i].handler(clients[i].token);
            	}
			});

			// clear the global color array
			uint32_t allColors[MAX_COUNT];
			for (int j = 0; j < c; ++j) {
				allColors[j] = 0;
			}

			for (int i = 0; i < animationCount; ++i) {
				auto anim = animations[i];

				bool loop = anim->loop;
				bool fade = anim->forceFadeTime != -1;

				int endTime = anim->startTime + anim->animationPreset->duration;
				uint32_t fadePercentTimes1000 = 1000;
				if (loop && ms > endTime) {
					// Yes, update anim start time so next if statement updates the animation
					anim->startTime += anim->animationPreset->duration;
					endTime += anim->animationPreset->duration;
				} else if (fade) {
					endTime = anim->forceFadeTime;
					fadePercentTimes1000 = 1000 * (endTime - ms) / FORCE_FADE_OUT_DURATION_MS;
				}
				
				if (ms > endTime)
				{
					// The animation is over, get rid of it!
					Animations::destroyAnimationInstance(anim);

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

					// Update the leds
					int animTrackCount = anim->updateLEDs(ms, canonIndices, colors);

					// Gamma correct and map face index to led index
					//NRF_LOG_INFO("track_count = %d", animTrackCount);
					for (int j = 0; j < animTrackCount; ++j) {
						//colors[j] = Utils::gamma(colors[j]);
                        ledIndices[j] = DiceVariants::animIndexToLEDIndex(canonIndices[j], anim->remapFace);
					}

					// Update color array
					for (int j = 0; j < animTrackCount; ++j) {

						// Fade out all leds if necessary
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
			LEDs::setPixelColors(allColors);
		}
	}

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

	void play(const Animation* animationPreset, const DataSet::AnimationBits* animationBits, uint8_t remapFace, bool loop, Animations::AnimationTag tag)
	{
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
			// Add a new animation
			animations[animationCount] = Animations::createAnimationInstance(animationPreset, animationBits);
			animations[animationCount]->setTag(tag);
			animations[animationCount]->start(ms, remapFace, loop);
			animationCount++;
		}
		// Else there is no more room
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
			Animations::destroyAnimationInstance(prevAnimInstance);
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
			Animations::destroyAnimationInstance(animations[i]);
		}
		animationCount = 0;
		LEDs::clear();
	}

	/// <summary>
	/// Helper function to clear anim LED turned on by a current animation
	/// </summary>
	void stopAtIndex(int animIndex)
	{
		// Found the animation, start by killing the leds it controls
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
			NRF_LOG_DEBUG("StartTime %d, remapFace %d, loop %d", anim->startTime, anim->remapFace, anim->loop ? 1: 0);
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
