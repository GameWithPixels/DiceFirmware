#pragma once

#include "stdint.h"
#include "animations/Animation.h"

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
	namespace ValidManager
	{
		void init();
		void onDiceInitialized();
	}
}

