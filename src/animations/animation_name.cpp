#include "animation_name.h"
#include "utils/utils.h"
#include "config/board_config.h"
#include "data_set/data_animation_bits.h"
#include "data_set/data_set.h"
#include "die.h"

namespace Animations
{
	/// <summary>
	/// constructor for simple animations
	/// Needs to have an associated preset passed in
	/// </summary>
	AnimationInstanceName::AnimationInstanceName(const AnimationName* preset, const DataSet::AnimationBits* bits)
		: AnimationInstance(preset, bits) {
	}

	/// <summary>
	/// destructor
	/// </summary>
	AnimationInstanceName::~AnimationInstanceName() {
	}

	/// <summary>
	/// Small helper to return the expected size of the preset data
	/// </summary>
	int AnimationInstanceName::animationSize() const {
		return sizeof(AnimationName);
	}

	/// <summary>
	/// (re)Initializes the instance to animate leds. This can be called on a reused instance.
	/// </summary>
	void AnimationInstanceName::start(int _startTime, uint8_t _remapFace, bool _loop) {
		AnimationInstance::start(_startTime, _remapFace, _loop);
        auto preset = getPreset();
        rgb = animationBits->getPaletteColor(preset->colorIndex);
	}

	/// <summary>
	/// Computes the list of LEDs that need to be on, and what their intensities should be.
	/// </summary>
	/// <param name="ms">The animation time (in milliseconds)</param>
	/// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of leds</param>
	/// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of leds</param>
	/// <returns>The number of leds/intensities added to the return array</returns>
	int AnimationInstanceName::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
        
        auto preset = getPreset();

        // Compute color
        uint32_t black = 0;
        uint32_t color = 0;
        int period = preset->duration / preset->count;
        int fadeTime = period * preset->fade / (255 * 2);
        int onOffTime = (period - fadeTime * 2) / 2;
        int time = (ms - startTime) % period;
        int count = (ms - startTime) / period;
        bool bitIsOne = Utils::inValidateMode() ? (count == 0) ? true : 
            ((Die::getDeviceID() >> (count-1)) & 1) == 1 :
            true;

        if (count == 0) {
            color = animationBits->getPaletteColor(2);
        } else if (time <= fadeTime && bitIsOne) {
            // Ramp up
            color = Utils::interpolateColors(black, 0, rgb, fadeTime, time);
        } else if (time <= fadeTime + onOffTime && bitIsOne) {
            color = rgb;
        } else if (time <= fadeTime * 2 + onOffTime && bitIsOne) {
            // Ramp down
            color = Utils::interpolateColors(rgb, fadeTime + onOffTime, black, fadeTime * 2 + onOffTime, time);
        } else {
            color = black;
        }

        // Fill the indices and colors for the anim controller to know how to update leds
        int retCount = 0;
        for (int i = 0; i < 20; ++i) {
            if ((preset->faceMask & (1 << i)) != 0)
            {
                retIndices[retCount] = i;
                retColors[retCount] = color;
                retCount++;
            }
        }
        return retCount;
	}

	/// <summary>
	/// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
	/// </summary>
	int AnimationInstanceName::stop(int retIndices[]) {
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

	const AnimationName* AnimationInstanceName::getPreset() const {
        return static_cast<const AnimationName*>(animationPreset);
    }

}