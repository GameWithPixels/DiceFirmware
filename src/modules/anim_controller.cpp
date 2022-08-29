#include "anim_controller.h"
#include "animations/animation.h"
#include "data_set/data_set.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/flash.h"
#include "utils/utils.h"
#include "utils/rainbow.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "config/dice_variants.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_log.h"
#include "accelerometer.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "leds.h"
#include "drivers_nrf/scheduler.h"

using namespace Animations;
using namespace Modules;
using namespace Config;
using namespace DriversNRF;
using namespace Bluetooth;

#define MAX_ANIMS 20

namespace Modules::AnimController
{
	static DelegateArray<AnimControllerClientMethod, 1> clients;

	// Our currently running animations
	static Animations::AnimationInstance *animations[MAX_ANIMS];
	static int animationCount = 0;

	// Some local functions
	void update(int ms);
	uint32_t getColorForAnim(void* token, uint32_t colorIndex);
	void onProgrammingEvent(void* context, Flash::ProgrammingEventType evt);
	void printAnimControllerState(void *context, const Message *msg);

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

	// Stop all currently running animations when going to sleep
	void onPowerEvent(void* context, nrf_pwr_mgmt_evt_t event) {
		if (event == NRF_PWR_MGMT_EVT_PREPARE_WAKEUP) {
			NRF_LOG_INFO("Stopping animations for sleep mode");
			stopAll();
		}
	}

	/// <summary>
	/// Kick off the animation controller, registering it with the Timer system
	/// </summary>
	void init()
	{
		Flash::hookProgrammingEvent(onProgrammingEvent, nullptr);
		MessageService::RegisterMessageHandler(Message::MessageType_PrintAnimControllerState, nullptr, printAnimControllerState);
		Timers::createTimer(&animControllerTimer, APP_TIMER_MODE_REPEATED, animationControllerUpdate);
		PowerManager::hook(onPowerEvent, nullptr);

		NRF_LOG_INFO("Anim Controller Initialized");

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
			uint32_t allColors[MAX_LED_COUNT];
			for (int j = 0; j < c; ++j) {
				allColors[j] = 0;
			}

			for (int i = 0; i < animationCount; ++i)
			{
				auto anim = animations[i];
				int animTime = ms - anim->startTime;
				if (anim->loop && animTime > anim->animationPreset->duration)
				{
					// Yes, update anim start time so next if statement updates the animation
					anim->startTime += anim->animationPreset->duration;
					animTime = ms - anim->startTime;
				}

				if (animTime > anim->animationPreset->duration)
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
					int canonIndices[MAX_LED_COUNT * 4]; // Allow up to 4 tracks to target the same LED
					int ledIndices[MAX_LED_COUNT * 4];
					uint32_t colors[MAX_LED_COUNT * 4];

					// Update the leds
					int animTrackCount = anim->updateLEDs(ms, canonIndices, colors);

					// Gamma correct and map face index to led index
					//NRF_LOG_INFO("track_count = %d", animTrackCount);
					for (int j = 0; j < animTrackCount; ++j) {
						//colors[j] = Utils::gamma(colors[j]);
                        ledIndices[j] = b->animIndexToLEDIndex(canonIndices[j], anim->remapFace);
					}

					// Update color array
					for (int j = 0; j < animTrackCount; ++j) {
						
						// Combine colors if necessary
						//NRF_LOG_INFO("index: %d -> %08x", ledIndices[j], colors[j]);
						allColors[ledIndices[j]] = Utils::addColors(allColors[ledIndices[j]], colors[j]);
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
		Timers::stopTimer(animControllerTimer);
		// Clear all data
		stopAll();
		NRF_LOG_INFO("Stopped anim controller");
	}

	void start()
	{
		NRF_LOG_INFO("Starting anim controller");
		Timers::startTimer(animControllerTimer, ANIM_FRAME_DURATION, NULL);
	}

	/// <summary>
	/// Add an animation to the list of running animations
	/// </summary>
	void play(int animIndex, uint8_t remapFace, bool loop)
	{
		// Find the preset for this animation Index
		auto animationPreset = DataSet::getAnimation(animIndex);
		play(animationPreset, DataSet::getAnimationBits(), remapFace, loop);
	}

	void play(const Animation* animationPreset, const DataSet::AnimationBits* animationBits, uint8_t remapFace, bool loop)
	{
		#if (NRF_LOG_DEFAULT_LEVEL == 4)
		NRF_LOG_DEBUG("Playing Anim!");
		NRF_LOG_DEBUG("  Track count: %d", anim->trackCount);
		for (int t = 0; t < anim->trackCount; ++t) {
			auto& track = anim->GetTrack(t);
			NRF_LOG_DEBUG("  Track %d:", t);
			NRF_LOG_DEBUG("  Track Offset %d:", anim->tracksOffset + t);
			NRF_LOG_DEBUG("  LED index %d:", track.ledIndex);
			NRF_LOG_DEBUG("  RGB Track Offset %d:", track.trackOffset);
			auto& rgbTrack = track.getLEDTrack();
			NRF_LOG_DEBUG("  RGB Keyframe count: %d", rgbTrack.keyFrameCount);
			for (int k = 0; k < rgbTrack.keyFrameCount; ++k) {
				auto& keyframe = rgbTrack.getRGBKeyframe(k);
				int time = keyframe.time();
				uint32_t color = keyframe.color(0);
				NRF_LOG_DEBUG("    Offset %d: %d -> %06x", (rgbTrack.keyframesOffset + k), time, color);
			}
		}
		#endif

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
			// Replace a previous animation
			stopAtIndex(prevAnimIndex);
			animations[prevAnimIndex]->startTime = ms;
		}
		else if (animationCount < MAX_ANIMS)
		{
			// Add a new animation
			animations[animationCount] = Animations::createAnimationInstance(animationPreset, animationBits);
			animations[animationCount]->start(ms, remapFace, loop);
			animationCount++;
		}
		// Else there is no more room
	}

	/// <summary>
	/// Forcibly stop a currently running animation
	/// </summary>
	void stop(int animIndex, uint8_t remapFace)	{

		// Find the preset for this animation Index
		auto animationPreset = DataSet::getAnimation(animIndex);

		stop(animationPreset, remapFace);
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
		auto b = BoardManager::getBoard();

		// Found the animation, start by killing the leds it controls
		int canonIndices[MAX_LED_COUNT];
		int ledIndices[MAX_LED_COUNT];
		uint32_t zeros[MAX_LED_COUNT];
		memset(zeros, 0, sizeof(uint32_t) * MAX_LED_COUNT);
		auto anim = animations[animIndex];
		int ledCount = anim->stop(canonIndices);
		for (int i = 0; i < ledCount; ++i) {
            ledIndices[i] = b->animIndexToLEDIndex(canonIndices[i], anim->remapFace);
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

	void printAnimControllerState(void* context, const Message* msg) {
		NRF_LOG_INFO("Anim Controller has %d animations", animationCount);
		for (int i = 0; i < animationCount; ++i) {
			AnimationInstance* anim = animations[i];
			NRF_LOG_INFO("Anim %d is of type %d, duration %d", i, anim->animationPreset->type, anim->animationPreset->duration);
			NRF_LOG_INFO("StartTime %d, remapFace %d, loop %d", anim->startTime, anim->remapFace, anim->loop ? 1: 0);
		}
	}

	/// <summary>
	/// Method used by clients to request timer callbacks
	/// </summary>
	void hook(AnimControllerClientMethod method, void* parameter)
	{
		if (!clients.Register(parameter, method))
		{
			NRF_LOG_ERROR("Failed to register AnimationController hook because client array is full");
		}
	}

	/// <summary>
	/// Method used by clients to stop getting callbacks
	/// </summary>
	void unHook(AnimControllerClientMethod method)
	{
		clients.UnregisterWithHandler(method);
	}
}
