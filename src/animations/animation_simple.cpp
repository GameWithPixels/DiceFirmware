#include "animation_simple.h"
#include "utils/utils.h"
#include "config/board_config.h"
#include "data_set/data_animation_bits.h"
#include "data_set/data_set.h"

namespace Animations
{
	/// <summary>
	/// constructor for simple animations
	/// Needs to have an associated preset passed in
	/// </summary>
	AnimationInstanceSimple::AnimationInstanceSimple(const AnimationSimple* preset, const DataSet::AnimationBits* bits)
		: AnimationInstance(preset, bits) {
	}

	/// <summary>
	/// destructor
	/// </summary>
	AnimationInstanceSimple::~AnimationInstanceSimple() {
	}

	/// <summary>
	/// Small helper to return the expected size of the preset data
	/// </summary>
	int AnimationInstanceSimple::animationSize() const {
		return sizeof(AnimationSimple);
	}

	/// <summary>
	/// (re)Initializes the instance to animate LEDs. This can be called on a reused instance.
	/// </summary>
	void AnimationInstanceSimple::start(int _startTime, uint8_t _remapFace, bool _loop) {
		AnimationInstance::start(_startTime, _remapFace, _loop);
        auto preset = getPreset();
        rgb = animationBits->getPaletteColor(preset->colorIndex);
	}

	/// <summary>
	/// Computes the list of LEDs that need to be on, and what their intensities should be.
	/// </summary>
	/// <param name="ms">The animation time (in milliseconds)</param>
	/// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of LEDs</param>
	/// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of LEDs</param>
	/// <returns>The number of LEDs/intensities added to the return array</returns>
	int AnimationInstanceSimple::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
        
        auto preset = getPreset();

        // Compute color
        uint32_t black = 0;
        uint32_t color = 0;
        int period = preset->duration / preset->count;
        int fadeTime = period * preset->fade / (255 * 2);
        int onOffTime = (period - fadeTime * 2) / 2;
        int time = (ms - startTime) % period;

        if (time <= fadeTime) {
            // Ramp up
            color = Utils::interpolateColors(black, 0, rgb, fadeTime, time);
        } else if (time <= fadeTime + onOffTime) {
            color = rgb;
        } else if (time <= fadeTime * 2 + onOffTime) {
            // Ramp down
            color = Utils::interpolateColors(rgb, fadeTime + onOffTime, black, fadeTime * 2 + onOffTime, time);
        } else {
            color = black;
        }

		// Fill the indices and colors for the anim controller to know how to update LEDs
		return setColor(color, preset->faceMask, retIndices, retColors);
	}

	/// <summary>
	/// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
	/// </summary>
	int AnimationInstanceSimple::stop(int retIndices[]) {
        auto preset = getPreset();
		return setIndices(preset->faceMask, retIndices);
	}

	const AnimationSimple* AnimationInstanceSimple::getPreset() const {
        return static_cast<const AnimationSimple*>(animationPreset);
    }

}
