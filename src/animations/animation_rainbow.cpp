#include "animation_rainbow.h"
#include "animation_context.h"
#include "utils/rainbow.h"
#include "utils/Utils.h"

namespace Animations
{
    /// <summary>
    /// (re)Initializes the instance to animate LEDs. This can be called on a reused instance.
    /// </summary>
    void AnimationRainbowInstance::start(int _startTime, uint8_t _remapFace, uint8_t _loopCount) {
        AnimationInstance::start(_startTime, _remapFace, _loopCount);
    }

    /// <summary>
    /// Computes the list of LEDs that need to be on, and what their intensities should be.
    /// </summary>
    /// <param name="ms">The animation time (in milliseconds)</param>
    /// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of leds</param>
    /// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of leds</param>
    /// <returns>The number of LEDs/intensities added to the return array</returns>
    int AnimationRainbowInstance::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
        const auto ledCount = context->globals->ledCount;
        auto preset = getPreset();

        // Compute color
        uint32_t color = 0;
        const int fadeTime = preset->duration * preset->fade / (255 * 2);
        const int time = (ms - startTime);

        const int wheelPos = (time * preset->count * 255 / preset->duration) % 256;

        uint8_t intensity = preset->intensity;
        if (time <= fadeTime) {
            // Ramp up
            intensity = (uint8_t)(time * preset->intensity / fadeTime);
        } else if (time >= (preset->duration - fadeTime)) {
            // Ramp down
            intensity = (uint8_t)((preset->duration - time) * preset->intensity / fadeTime);
        }

        // Fill the indices and colors for the anim controller to know how to update LEDs
        int retCount = 0;
        if (preset->traveling) {
            for (int i = 0; i < ledCount; ++i) {
                if ((preset->faceMask & (1 << i)) != 0) {
                    retIndices[retCount] = i;
                    const int pos = wheelPos + i * 256 * preset->cyclesTimes16 / (ledCount * 16);
                    auto color = Rainbow::wheel((uint8_t)(pos % 256));
                    color = Utils::scaleColor(color, intensity * 1000 / 255);
                    retColors[retCount] = color;
                    retCount++;
                }
            }
        } else {
            // All LEDs same color
            color = Rainbow::wheel((uint8_t)wheelPos);
            color = Utils::scaleColor(color, intensity * 1000 / 255);
            retCount = setColor(color, preset->faceMask, retIndices, retColors);
        }
        return retCount;
    }
}
