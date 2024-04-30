#include "animation_simple.h"
#include "utils/utils.h"
#include "utils/Rainbow.h"
#include "config/board_config.h"

namespace Animations
{
    /// <summary>
    /// (re)Initializes the instance to animate LEDs. This can be called on a reused instance.
    /// </summary>
    void AnimationSimpleInstance::start(int _startTime, uint8_t _remapFace, uint8_t _loopCount) {
        AnimationInstance::start(_startTime, _remapFace, _loopCount);

        // If applicable, capture the color once
        auto preset = getPreset();
        if ((preset->colorFlags & AnimationSimpleFlags_CaptureColor) != 0) {
            capturedColor = context->evaluateColor(preset->color);
        }
    }

    /// <summary>
    /// Computes the list of LEDs that need to be on, and what their intensities should be.
    /// </summary>
    /// <param name="ms">The animation time (in milliseconds)</param>
    /// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of LEDs</param>
    /// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of LEDs</param>
    /// <returns>The number of LEDs/intensities added to the return array</returns>
    int AnimationSimpleInstance::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
        
        auto preset = getPreset();

        // Compute color
        uint32_t black = 0;
        uint32_t color = 0;
        uint32_t presetColor = 0;
        if ((preset->colorFlags & AnimationSimpleFlags_CaptureColor) != 0) {
            presetColor = capturedColor;
        } else {
            presetColor = context->evaluateColor(preset->color);
        }
        int period = preset->duration / preset->count;
        int fadeTime = period * preset->fade / (255 * 2);
        int onOffTime = (period - fadeTime * 2) / 2;
        int time = (ms - startTime) % period;

        if (time <= fadeTime) {
            // Ramp up
            color = Utils::interpolateColors(black, 0, presetColor, fadeTime, time);
        } else if (time <= fadeTime + onOffTime) {
            color = presetColor;
        } else if (time <= fadeTime * 2 + onOffTime) {
            // Ramp down
            color = Utils::interpolateColors(presetColor, fadeTime + onOffTime, black, fadeTime * 2 + onOffTime, time);
        } else {
            color = black;
        }

        uint8_t intensity = preset->intensity;
        // Fill the indices and colors for the anim controller to know how to update LEDs
        return setColor(Utils::scaleColor(color, intensity * 1000 / 255), preset->faceMask, retIndices, retColors);
    }

    /// <summary>
    /// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
    /// </summary>
    int AnimationSimpleInstance::stop(int retIndices[]) {
        auto preset = getPreset();
        return setIndices(preset->faceMask, retIndices);
    }

    const AnimationSimple* AnimationSimpleInstance::getPreset() const {
        return static_cast<const AnimationSimple*>(animationPreset);
    }

}
