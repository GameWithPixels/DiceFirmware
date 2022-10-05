#include "blink.h"
#include "modules\anim_controller.h"
#include "utils\Utils.h"
#include "nrf_log.h"

namespace Animations
{
    Blink::Blink()
    {
        animBits.Clear();
        animBits.palette = animPalette;
        animBits.paletteSize = 3;
    }

    void Blink::play(uint32_t color, uint16_t durationMs, uint8_t flashCount /*= 1*/, uint8_t fade /*= 0*/, uint32_t faceMask /*=ANIM_FACEMASK_ALL_LEDS*/)
    {
        // Store color in palette
        // Note: the color is stored at the same memory location on each call, the most recent call
        // will override the color of any previous call for which the animation is still playing.
        animPalette[0] = Utils::getRed(color);
        animPalette[1] = Utils::getGreen(color);
        animPalette[2] = Utils::getBlue(color);

        // Create a small anim on the spot
        blinkAnim.type = Animation_Simple;
        blinkAnim.duration = durationMs;
        blinkAnim.faceMask = faceMask;
        blinkAnim.count = flashCount;
        blinkAnim.fade = fade;
        blinkAnim.colorIndex = 0;

        Modules::AnimController::stop(&blinkAnim);
        Modules::AnimController::play(&blinkAnim, &animBits);
    }
}