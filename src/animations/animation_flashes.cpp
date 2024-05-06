#include "animation_flashes.h"
#include "animation_context.h"
#include "utils/utils.h"

namespace Animations
{
    /// <summary>
    /// (re)Initializes the instance to animate LEDs. This can be called on a reused instance.
    /// </summary>
    void AnimationFlashesInstance::start(int _startTime, uint8_t _remapFace, uint8_t _loopCount) {
        AnimationInstance::start(_startTime, _remapFace, _loopCount);

        // If applicable, capture the color once
        auto preset = getPreset();
        if ((preset->colorFlags & (uint8_t)AnimationFlashesFlags_CaptureColor) != 0) {
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
    int AnimationFlashesInstance::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
        const auto ledCount = context->globals->ledCount;
        auto preset = getPreset();

        const int period = preset->duration / preset->count;
        const int fadeTime = period * preset->fade / (255 * 2);
        const int onOffTime = (period - fadeTime * 2) / 2;
        const int time = (ms - startTime) % period;

        int retCount = 0;
        for (int i = 0; i < ledCount; ++i) {
            if ((preset->faceMask & (1 << i)) != 0) {
                // TODO !!
                ((AnimationContextGlobals*)context->globals)->animatedLED = i;

                // Compute color
                const uint32_t presetColor =
                    ((preset->colorFlags & (uint8_t)AnimationFlashesFlags_CaptureColor) != 0) ?
                    capturedColor :
                    context->evaluateColor(preset->color);

                // Fade in & out
                const uint32_t black = 0;
                uint32_t color = 0;
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
                color = presetColor;

                // Scale color intensity
                color = Utils::scaleColor(color, preset->intensity * 1000 / 255);

                // Set LED color
                retColors[retCount] = color;
                retCount++;
            }
        }
        return retCount;
    }

    /// <summary>
    /// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
    /// </summary>
    int AnimationFlashesInstance::stop(int retIndices[]) {
        auto preset = getPreset();
        return setIndices(preset->faceMask, retIndices);
    }

    const AnimationFlashes* AnimationFlashesInstance::getPreset() const {
        return static_cast<const AnimationFlashes*>(animationPreset);
    }

}
