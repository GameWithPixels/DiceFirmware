#include "blink.h"
#include "modules\anim_controller.h"
#include "utils\Utils.h"
#include "config/board_config.h"
#include "config/dice_variants.h"

namespace Animations
{
    static uint8_t colorBuffer[sizeof(DColorRGB)];

    void Blink::play(uint32_t color, uint16_t durationMs, uint8_t flashCount /*= 1*/, uint8_t fade /*= 0*/, uint32_t faceMask /*=ANIM_FACEMASK_ALL_LEDS*/, uint8_t loopCount /*= 1*/)
    {
        // Store color in palette
        // Note: the color is stored at the same memory location on each call, the most recent call
        // will override the color of any previous call for which the animation is still playing.
        auto c = reinterpret_cast<DColorRGB*>(colorBuffer);
        c->type = ColorType_RGB;
        c->rValue = Utils::getRed(color);
        c->gValue = Utils::getGreen(color);
        c->bValue = Utils::getBlue(color);
        Profile::Pointer<DColorRGB> cptr(0);

        Profile::BufferDescriptor buffer;
        buffer.start = colorBuffer;
        buffer.size = sizeof(colorBuffer);

        // Create a small anim on the spot
        blinkAnim.type = AnimationType_Flashes;
        blinkAnim.duration = durationMs;
        blinkAnim.faceMask = faceMask;
        blinkAnim.count = flashCount;
        blinkAnim.fade = fade;
        blinkAnim.color = cptr;

        Modules::AnimController::stop(&blinkAnim);
        const auto remapFace = Config::DiceVariants::getLayout()->faceCount - 1;
        Modules::AnimController::PlayAnimationParameters params;
        params.remapFace = remapFace;
        params.loopCount = loopCount;
        params.tag = Animations::AnimationTag_BluetoothMessage;
        params.buffer = buffer;
        Modules::AnimController::play(&blinkAnim, params);
    }
}
