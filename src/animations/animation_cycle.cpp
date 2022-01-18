#include "animation_cycle.h"
#include "utils/utils.h"
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
        int time = (ms - startTime) % period;
        int fadeTime = period * preset->fade / (255 * 2);
        int faceIndex = (ms - startTime) / (period * preset->count);

        uint32_t color;
        if (preset->rainbow) {
            // Rainbow animation
            uint8_t intensity = 255;
            if (time <= fadeTime) {
                // Ramp up
                intensity = (uint8_t)(time * 255 / fadeTime);
            } else if (time >= (period - fadeTime)) {
                // Ramp down
                intensity = (uint8_t)((period - time) * 255 / fadeTime);
            }

            int wheelPos = time * 255 / period;
            color = Rainbow::wheel((uint8_t)wheelPos, intensity);
        } else {
            // RBG animation
            int redTime = period / 3;
            int greenTime = redTime + redTime;
            if (time <= redTime) {
                color = 0xFF0000;
            } else if (time <= greenTime) {
                color = 0x00FF00;
            } else {
                color = 0x0000FF;
            }

            uint32_t black = 0;
            if (time <= fadeTime) {
                color = Utils::interpolateColors(black, 0, color, fadeTime, time);
            } else if (time > (period - fadeTime)) {
                color = Utils::interpolateColors(color, period - fadeTime, black, period, time);
            }
        }

        uint8_t ledsOrder[] = {
            13, // Led closest to chip and serial connector, index = 0
            9,
            0,
            5,
            17,
            11, // Index = 5
            7,
            18,
            15,
            16,
            8, // Index = 10
            12,
            1,
            4,
            3,
            19, // Index = 15
            10,
            6,
            2,
            14,
        };

        // Fill the indices and colors for the anim controller to know how to update leds
        int retCount = 0;
        for (int k = 0; k < 20; ++k) {
            int i = ledsOrder[k];
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
