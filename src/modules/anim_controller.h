#pragma once

#include "stdint.h"
#include "animations/Animation.h"

// Frame duration = time between each animation update, in ms.
#define ANIM_FRAME_DURATION 33

namespace Animations
{
	struct Animation;
}

/// <summary>
/// Manages a set of running animations, talking to the LED controller
/// to tell it what LEDs must have what intensity at what time.
/// </summary>
namespace Modules::AnimController
{
	void stopAtIndex(int animIndex);
	void removeAtIndex(int animIndex);

	void init();
	void stop();
	void start();

	void play(const Animations::Animation* animationPreset, const DataSet::AnimationBits* animationBits, uint8_t remapFace = 0, bool loop = false, Animations::AnimationTag tag = Animations::AnimationTag_Unknown);
	void stop(const Animations::Animation* animationPreset, uint8_t remapFace = 0);
	void fadeOutAnimsWithTag(Animations::AnimationTag tagToStop);
	void stopAll();

	// Notification management
	typedef void(*AnimControllerClientMethod)(void* param);
	void hook(AnimControllerClientMethod method, void* param);
	void unHook(AnimControllerClientMethod client);
}
