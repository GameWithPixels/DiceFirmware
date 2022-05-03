#include "animation_name.h"
#include "utils/utils.h"
#include "config/board_config.h"
#include "data_set/data_animation_bits.h"
#include "data_set/data_set.h"
#include "die.h"
#include "nrf_log.h"

namespace Animations
{
	/// <summary>
	/// constructor for simple animations
	/// Needs to have an associated preset passed in
	/// </summary>
	AnimationInstanceName::AnimationInstanceName(const AnimationName* preset, const DataSet::AnimationBits* bits)
		: AnimationInstance(preset, bits)
    {
	}

	/// <summary>
	/// destructor
	/// </summary>
	AnimationInstanceName::~AnimationInstanceName()
    {
	}

	/// <summary>
	/// Small helper to return the expected size of the preset data
	/// </summary>
	int AnimationInstanceName::animationSize() const
    {
		return sizeof(AnimationName);
	}

	/// <summary>
	/// (re)Initializes the instance to animate leds. This can be called on a reused instance.
	/// </summary>
	void AnimationInstanceName::start(int _startTime, uint8_t _remapFace, bool _loop)
    {
		AnimationInstance::start(_startTime, _remapFace, _loop);
    }

	/// <summary>
	/// Computes the list of LEDs that need to be on, and what their intensities should be.
	/// </summary>
	/// <param name="ms">The animation time (in milliseconds)</param>
	/// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of leds</param>
	/// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of leds</param>
	/// <returns>The number of leds/intensities added to the return array</returns>
	int AnimationInstanceName::updateLEDs(int ms, int retIndices[], uint32_t retColors[])
    {
        auto preset = getPreset();

        // Compute color
        uint32_t color = 0;
        const uint32_t brightness = (uint32_t)preset->brightness;
        const uint8_t preamble = preset->preambleCount;
        const int numTicks = preamble + 8 * sizeof(Die::getDeviceID());
        const int period = preset->duration / numTicks; // "tick" must always be less than "numTicks", otherwise we'll blink an extra bit/color
        const int tick = (ms - startTime) / period;

        // -- Note --
        // (ms - startTime) starts at 33 and ends = preset->duration

        // White preamble
        if (tick < preamble)
        {
            color = brightness | brightness << 8 | brightness << 16;

            // NRF_LOG_INFO("white - (%d / %d) - id=%x", ms - startTime, preset->duration, Die::getDeviceID());
        }
        // Bit color
        else if (tick < numTicks) // We want to skip the last tick if that gets past the last bit
        {
            // Compute the color of the current bit
            auto value = Die::getDeviceID();
            int rgbIndex = -1;
            for (int i = preamble; i <= tick; ++i)
            {
                rgbIndex += 1 + (value & 1);
                value >>= 1;
            }

            // rgbIndex = 0 => red, 1 => green, 2 => blue
            rgbIndex %= 3;
            color = brightness << (16 - 8 * rgbIndex);

            // NRF_LOG_INFO("#%d (%d) - bit = %d - color = %d", tick - preamble, ms - startTime, (Die::getDeviceID() >> (tick - preamble)) & 1, rgbIndex);
        }

        // Fill the indices and colors for the anim controller to know how to update leds
        int retCount = 0;
        if (color != 0)
        {
            for (int i = 0; i < 20; ++i)
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
	int AnimationInstanceName::stop(int retIndices[])
    {
        int retCount = 0;
        for (int i = 0; i < 20; ++i)
        {
            retIndices[retCount] = i;
            retCount++;
        }
        return retCount;
	}

	const AnimationName* AnimationInstanceName::getPreset() const
    {
        return static_cast<const AnimationName*>(animationPreset);
    }
}
