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

    void Blink::play(uint32_t color, uint16_t durationMs, uint8_t flashCount /*= 1*/, uint8_t fade /*= 0*/, uint32_t faceMask /*=ANIM_FACEMASK_ALL_LEDS*/, uint8_t loopCount /*= 1*/)
    {
        // Create a small anim on the spot
        // blinkAnim.type = AnimationType_Simple;
        // blinkAnim.duration = durationMs;
        // blinkAnim.faceMask = faceMask;
        // blinkAnim.count = flashCount;
        // blinkAnim.fade = fade;
        // blinkAnim.color = color;

        // Modules::AnimController::stop(&blinkAnim);
        // const auto remapFace = Config::DiceVariants::getTopFace();
        // Modules::AnimController::PlayAnimationParameters params;
        // params.remapFace = remapFace;
        // params.loopCount = loopCount;
        // params.tag = Animations::AnimationTag_BluetoothMessage;
        // Modules::AnimController::play(&blinkAnim, params);
    }
}