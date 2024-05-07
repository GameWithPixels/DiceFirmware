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

        int retCount = 0;
        for (int i = 0; i < ledCount; ++i) {
            // TODO !!
            ((AnimationContextGlobals*)context->globals)->animatedLED = i;
            ((AnimationContextGlobals*)context->globals)->normalizedAnimatedLED = i * 0xFFFF / ledCount;
            const uint32_t intensity = context->evaluateScalar(preset->intensity);

            if (intensity > 0) {
                // Compute color
                const uint32_t color =
                    ((preset->colorFlags & (uint8_t)AnimationFlashesFlags_CaptureColor) != 0) ?
                    capturedColor :
                    context->evaluateColor(preset->color);

                // Set LED color
                retIndices[retCount] = i;
                retColors[retCount] = Utils::modulateColor(color, intensity / 256);
                retCount++;
            }
        }

        return retCount;
    }

    const AnimationFlashes* AnimationFlashesInstance::getPreset() const {
        return static_cast<const AnimationFlashes*>(animationPreset);
    }

}
