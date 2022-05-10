#pragma once

#include "stdint.h"
#include "animations/Animation.h"

// Frame duration = time between each animation update, in ms.
#define ANIM_FRAME_DURATION 33

namespace Animations
{
	struct Animation;
}

namespace Modules
{
	/// <summary>
	/// Manages a set of running animations, talking to the LED controller
	/// to tell it what LEDs must have what intensity at what time.
	/// </summary>
	namespace AnimController
	{
		void stopAtIndex(int animIndex);
		void removeAtIndex(int animIndex);

		void init();
		void stop();
		void start();

		void play(int animIndex, uint8_t remapFace = 0, bool loop = false);
		void play(const Animations::Animation* animationPreset, const DataSet::AnimationBits* animationBits, uint8_t remapFace = 0, bool loop = false);
		void stop(int animIndex, uint8_t remapFace = 0);
		void stop(const Animations::Animation* animationPreset, uint8_t remapFace = 0);
		void stopAll();

		// Notification management
		typedef void(*AnimControllerClientMethod)(void* param);
		void hook(AnimControllerClientMethod method, void* param);
		void unHook(AnimControllerClientMethod client);
	}
}

