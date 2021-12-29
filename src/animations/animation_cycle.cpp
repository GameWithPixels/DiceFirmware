#include "animation_cycle.h"
#include "utils/rainbow.h"

namespace Animations
{
    uint8_t countFaces(uint32_t faceMask) {
        uint8_t count = 0;
        for (int i = 0; i < 20; ++i) {
            count += faceMask & 1;
            faceMask >>= 1;
        }
        return count ? count : 1;
    }

	/// <summary>
	/// constructor for simple animations
	/// Needs to have an associated preset passed in
	/// </summary>
	AnimationInstanceCycle::AnimationInstanceCycle(const AnimationCycle* preset, const DataSet::AnimationBits* bits)
		: AnimationInstance(preset, bits), faceCount(countFaces(preset->faceMask)) {
    }

    /// <summary>
	/// destructor
	/// </summary>
	AnimationInstanceCycle::~AnimationInstanceCycle() {
	}

	/// <summary>
	/// Small helper to return the expected size of the preset data
	/// </summary>
	int AnimationInstanceCycle::animationSize() const {
		return sizeof(AnimationCycle);
	}

	/// <summary>
	/// (re)Initializes the instance to animate leds. This can be called on a reused instance.
	/// </summary>
	void AnimationInstanceCycle::start(int _startTime, uint8_t _remapFace, bool _loop) {
		AnimationInstance::start(_startTime, _remapFace, _loop);
        //rgb = animationBits->getPaletteColor(getPreset()->colorIndex);
    }

	/// <summary>
	/// Computes the list of LEDs that need to be on, and what their intensities should be.
	/// </summary>
	/// <param name="ms">The animation time (in milliseconds)</param>
	/// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of leds</param>
	/// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of leds</param>
	/// <returns>The number of leds/intensities added to the return array</returns>
	int AnimationInstanceCycle::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
        
        auto preset = getPreset();

        int period = preset->duration / preset->count / faceCount;
        int fadeTime = period * preset->fade / (255 * 2);
        int time = (ms - startTime) % period;
        int faceIndex = (ms - startTime) / (period * preset->count);

        uint8_t intensity = 255;
		if (time <= fadeTime) {
			// Ramp up
			intensity = (uint8_t)(time * 255 / fadeTime);
		} else if (time >= (period - fadeTime)) {
			// Ramp down
            intensity = (uint8_t)((period - time) * 255 / fadeTime);
        }

        int wheelPos = time * 255 / period;
        uint32_t color = Rainbow::wheel((uint8_t)wheelPos, intensity);

        // Fill the indices and colors for the anim controller to know how to update leds
        int retCount = 0;
        for (int i = 0; i < 20; ++i) {
            if ((preset->faceMask & (1 << i)) != 0) {
                if (faceIndex == 0) {
                    retIndices[retCount] = i;
                    retColors[retCount] = color;
                    retCount++;
                    break;
                }
                --faceIndex;
            }
        }
        return retCount;
    }

	/// <summary>
	/// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
	/// </summary>
	int AnimationInstanceCycle::stop(int retIndices[]) {
        auto preset = getPreset();
        int retCount = 0;
        for (int i = 0; i < 20; ++i) {
            if ((preset->faceMask & (1 << i)) != 0)
            {
                retIndices[retCount] = i;
                retCount++;
            }
        }
        return retCount;
	}

	const AnimationCycle* AnimationInstanceCycle::getPreset() const {
        return static_cast<const AnimationCycle*>(animationPreset);
    }
}
