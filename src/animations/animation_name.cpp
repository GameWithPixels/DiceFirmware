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
        counter = 0;
        last_bit = 0;
        skip = false;
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
        uint8_t preamble = preset->preamble_count;
        uint32_t brightness = (uint32_t)preset->brightness;
        int period = preset->duration / (NAME_COUNT + preamble);
        int time = (ms - startTime) % period;
        int count = (ms - startTime) / period;
        bool bitIsOne = ((Die::getDeviceID() >> (count-(preamble+1))) & 1) == 1;    // is current bit 1?


        // Update color counter for bit 0
        if (count == (preamble + 1) && bitIsOne && !skip) {
            counter = (counter + 1) % 3;
            skip = true;
        }
        // Update color counter for bits > 0
        else if (count - (preamble + 1) > last_bit && count > preamble) {
            counter = (counter + 1) % 3;
            last_bit = count - (preamble + 1);
            if (bitIsOne) {
                counter = (counter + 1) % 3;
            }
        }

        // Preamble black (even counts)
        if (count < (preamble + 1) && count % 2 == 0)
        {
            color = black;
        }
        // Preamble white (odd counts)
        else if (count < (preamble + 1))
        {
            color |= (brightness | brightness << 8 | brightness << 16);
        }
        // Bit colors (preamble < count < 32)
        else if (count - (preamble + 1) < 32) 
        {
            switch (counter) {
                case 0:
                    color |= (brightness << 16);    // R (counter == 0)
                    break;
                case 1:
                    color |= (brightness << 8);     // G (counter == 1)
                    break;
                case 2:
                    color |= brightness;            // B (counter == 2)
                    break;
            }
        } 

        // Fill the indices and colors for the anim controller to know how to update leds
        int retCount = 0;
        for (int i = 0; i < 20; ++i) {
            if ((NAME_FACEMASK & (1 << i)) != 0)
            {
                retIndices[retCount] = i;
                retColors[retCount] = color;
                retCount++;
            }
        }

        // Reset fields at end of name
        if (count*period + time + 33 > preset->duration) {
            counter = 0;
            last_bit = 0;
            skip = false;
        }

        return retCount;
	}

	/// <summary>
	/// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
	/// </summary>
	int AnimationInstanceName::stop(int retIndices[]) {
        int retCount = 0;
        for (int i = 0; i < 20; ++i) {
            if ((NAME_FACEMASK & (1 << i)) != 0)
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