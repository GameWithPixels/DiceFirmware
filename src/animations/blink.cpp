#include "blink.h"
#include "modules\anim_controller.h"
#include "utils\Utils.h"
#include "config/board_config.h"
#include "config/dice_variants.h"

namespace Animations
{
    Blink::Blink()
    {
    }

    void Blink::play(uint32_t color, uint16_t durationMs, uint8_t flashCount /*= 1*/, uint8_t fade /*= 0*/, uint32_t faceMask /*=ANIM_FACEMASK_ALL_LEDS*/, bool loop /*= false*/)
    {
        // Store color in palette
        // Note: the color is stored at the same memory location on each call, the most recent call
        // will override the color of any previous call for which the animation is still playing.

        // Create a small anim on the spot
        // blinkAnim.type = AnimationType_Simple;
        // blinkAnim.duration = durationMs;
        // blinkAnim.faceMask = faceMask;
        // blinkAnim.count = flashCount;
        // blinkAnim.fade = fade;
        // //blinkAnim.color = color;

        // Modules::AnimController::stop(&blinkAnim);
        // const auto remapFace = Config::DiceVariants::getLayout()->faceCount - 1;
        // Modules::AnimController::PlayAnimationParameters params;
        // params.remapFace = remapFace;
        // params.loop = loop;
        // params.tag = Animations::AnimationTag_BluetoothMessage;
        // Modules::AnimController::play(&blinkAnim, params);
    }
}