#include "animation_rainbow.h"
#include "utils/rainbow.h"
#include "config/dice_variants.h"

using namespace Config;

namespace Animations
{
	/// <summary>
	/// constructor for rainbow animations
	/// Needs to have an associated preset passed in
	/// </summary>
	AnimationInstanceRainbow::AnimationInstanceRainbow(const AnimationRainbow* preset, const DataSet::AnimationBits* bits)
		: AnimationInstance(preset, bits) {
	}

	/// <summary>
	/// destructor
	/// </summary>
	AnimationInstanceRainbow::~AnimationInstanceRainbow() {
	}

	/// <summary>
	/// Small helper to return the expected size of the preset data
	/// </summary>
	int AnimationInstanceRainbow::animationSize() const {
		return sizeof(AnimationRainbow);
	}

	/// <summary>
	/// (re)Initializes the instance to animate leds. This can be called on a reused instance.
	/// </summary>
	void AnimationInstanceRainbow::start(int _startTime, uint8_t _remapFace, bool _loop) {
		AnimationInstance::start(_startTime, _remapFace, _loop);
	}

	/// <summary>
	/// Computes the list of LEDs that need to be on, and what their intensities should be.
	/// </summary>
	/// <param name="ms">The animation time (in milliseconds)</param>
	/// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of leds</param>
	/// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of leds</param>
	/// <returns>The number of leds/intensities added to the return array</returns>
	int AnimationInstanceRainbow::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
		auto l = DiceVariants::getLayout();
		int c = l->ledCount;

		auto preset = getPreset();

		// Compute color
		uint32_t color = 0;
		int fadeTime = preset->duration * preset->fade / (255 * 2);
		int time = (ms - startTime);

		int wheelPos = (time * preset->count * 255 / preset->duration) % 256;

		uint8_t intensity = preset->intensity;
		if (time <= fadeTime) {
			// Ramp up
			intensity = (uint8_t)(time * preset->intensity / fadeTime);
		} else if (time >= (preset->duration - fadeTime)) {
			// Ramp down
			intensity = (uint8_t)((preset->duration - time) * preset->intensity / fadeTime);
		}

		// Fill the indices and colors for the anim controller to know how to update leds
		int retCount = 0;
        if (preset->animFlags & AnimationFlags_Traveling) {
			for (int i = 0; i < c; ++i) {
				if ((preset->faceMask & (1 << i)) != 0)
				{
					retIndices[retCount] = i;
					retColors[retCount] = Rainbow::wheel((uint8_t)((wheelPos + i * 256 / c) % 256), intensity);
					retCount++;
				}
			}
		} else {
			// All leds same color
			color = Rainbow::wheel((uint8_t)wheelPos, intensity);
			retCount = setColor(color, preset->faceMask, retIndices, retColors);
		}
		return retCount;
	}

	/// <summary>
	/// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
	/// </summary>
	int AnimationInstanceRainbow::stop(int retIndices[]) {
		auto preset = getPreset();
		return setIndices(preset->faceMask, retIndices);
	}

	const AnimationRainbow* AnimationInstanceRainbow::getPreset() const {
        return static_cast<const AnimationRainbow*>(animationPreset);
    }
}
