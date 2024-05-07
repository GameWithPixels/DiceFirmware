#include "animation.h"
#include "assert.h"
#include "../utils/utils.h"
#include "nrf_log.h"
#include "config/board_config.h"

#define MAX_LEVEL (256)

using namespace Utils;
using namespace Config;

namespace Animations
{
    /// Dims the passed in color by the passed in intensity (normalized 0 - 255)
    /// </summary>
    uint32_t scaleColor(uint32_t refColor, uint8_t intensity)
    {
        uint8_t r = getRed(refColor);
        uint8_t g = getGreen(refColor);
        uint8_t b = getBlue(refColor);
        return toColor(r * intensity / MAX_LEVEL, g * intensity / MAX_LEVEL, b * intensity / MAX_LEVEL);
    }

    void AnimationInstance::setTag(AnimationTag _tag) {
        tag = _tag;
    }

    void AnimationInstance::start(int _startTime, uint8_t _remapFace, uint8_t _loopCount) {
        startTime = _startTime;
        remapFace = _remapFace;
        forceFadeTime = -1;
        loopCount = _loopCount;
    }

    int AnimationInstance::stop(int retIndices[]) {
        // TODO track which LEDs where turned on by the animation
        return setIndices(ANIM_FACEMASK_ALL_LEDS, retIndices);
    }

    int AnimationInstance::setColor(uint32_t color, uint32_t faceMask, int retIndices[], uint32_t retColors[]) {
        const auto ledCount = BoardManager::getBoard()->ledCount;
        int retCount = 0;
        for (int i = 0; i < ledCount; ++i) {
            if ((faceMask & (1 << i)) != 0) {
                retIndices[retCount] = i;
                retColors[retCount] = color;
                retCount++;
            }
        }
        return retCount;
    }

    int AnimationInstance::setIndices(uint32_t faceMask, int retIndices[]) {
        const auto ledCount = BoardManager::getBoard()->ledCount;
        int retCount = 0;
        for (int i = 0; i < ledCount; ++i) {
            if ((faceMask & (1 << i)) != 0) {
                retIndices[retCount] = i;
                retCount++;
            }
        }
        return retCount;
    }

    void AnimationInstance::forceFadeOut(int fadeOutTime) {
        loopCount = 1;
        if (forceFadeTime < startTime + animationPreset->duration) {
            forceFadeTime = fadeOutTime;
        }
        // Otherwise the anim will end sooner than the force fade out time, so
        // making it not loop is enough.
    }
}
