#include "animation_flashes.h"
#include "animation_context.h"
#include "utils/utils.h"
#include "settings.h"

namespace Animations
{
    /// <summary>
    /// (re)Initializes the instance to animate LEDs. This can be called on a reused instance.
    /// </summary>
    void AnimationFlashesInstance::start(int _startTime, uint8_t _remapFace, uint8_t _loopCount) {
        AnimationInstance::start(_startTime, _remapFace, _loopCount);
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

        uint32_t colors[MAX_COUNT];
        context->evaluateColorVector(preset->colors, colors);

        int retCount = 0;
        for (int i = 0; i < ledCount; ++i) {
            // Set LED color
            retIndices[retCount] = i;
            retColors[retCount] = colors[i];
            retCount++;
        }

        return retCount;
    }

    const AnimationFlashes* AnimationFlashesInstance::getPreset() const {
        return static_cast<const AnimationFlashes*>(animationPreset);
    }
}
