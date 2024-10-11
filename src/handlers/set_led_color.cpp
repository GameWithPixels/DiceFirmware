#include "set_led_color.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "utils/utils.h"
#include "nrf_log.h"
#include "modules/anim_controller.h"
#include "animations/animation.h"
#include "data_set/data_set.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "config/dice_variants.h"
#include "modules/leds.h"
#include "animations/blink.h"
#include "animations/animation_blinkid.h"

using namespace Modules;
using namespace Bluetooth;
using namespace Utils;
using namespace Animations;
using namespace Config;

namespace Handlers::SetLEDColor
{
    void SetLEDToColorHandler(const Message* msg);
    void SetAllLEDsToColorHandler(const Message* msg);
    void LightUpFaceHandler(const Message* msg);
    void BlinkLEDsHandler(const Message *msg);
    void BlinkIdHandler(const Message *msg);

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_SetLEDToColor, SetLEDToColorHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_SetAllLEDsToColor, SetAllLEDsToColorHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_LightUpFace, LightUpFaceHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_Blink, BlinkLEDsHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_BlinkId, BlinkIdHandler);
        NRF_LOG_DEBUG("LED Color tester init");
    }

    void SetLEDToColorHandler(const Message* msg) {
        auto colorMsg = (const MessageSetLEDToColor*)msg;
        uint8_t led = colorMsg->ledIndex;
        uint8_t r = Utils::getRed(colorMsg->color);
        uint8_t g = Utils::getGreen(colorMsg->color);
        uint8_t b = Utils::getBlue(colorMsg->color);
        uint32_t color = Utils::toColor(r, g, b);
        NRF_LOG_INFO("Setting LED %d to %06x -> %06x", led, colorMsg->color, color);
        LEDs::setPixelColor(led, color);
    }

    void SetAllLEDsToColorHandler(const Message* msg) {
        auto colorMsg = (const MessageSetAllLEDsToColor*)msg;
        uint8_t r = Utils::getRed(colorMsg->color);
        uint8_t g = Utils::getGreen(colorMsg->color);
        uint8_t b = Utils::getBlue(colorMsg->color);
        uint32_t color = Utils::toColor(r, g, b);
        NRF_LOG_INFO("Setting All LEDs to %06x -> %06x", colorMsg->color, color);
        LEDs::setAll(color);
    }

    void LightUpFaceHandler(const Message* msg) {
        // The transformation is:
        // animFaceIndex
        //	-> rotatedOutsideAnimFaceIndex (based on remapFace and remapping table, i.e. what actual face should light up to "retarget" the animation around the current up face)
        //		-> ledIndex (based on pcb face to led mapping, i.e. to account for the fact that the LEDs are not accessed in the same order as the number of the faces)

        auto l = SettingsManager::getLayout();

        auto lufmsg = static_cast<const MessageLightUpFace*>(msg);

        NRF_LOG_INFO("Light Up Face: face: %d, remapFace: %d, color: %08x", lufmsg->face, lufmsg->opt_remapFace, lufmsg->color);

        int remapFace = lufmsg->opt_remapFace;
        if (remapFace == FACE_INDEX_CURRENT_FACE) {
            remapFace = Accelerometer::currentFace();
        }

        uint16_t ledIndex = l->daisyChainIndexFromLEDIndexLookup[remapFace];

        NRF_LOG_INFO(" -> LED Index: %d, color: %08x", ledIndex, lufmsg->color);
        BLE_LOG_INFO("ledIndex: %d", ledIndex);

        LEDs::setPixelColor(ledIndex, lufmsg->color);
    }

    void BlinkLEDsHandler(const Message* msg) 
    {
        auto *message = (const MessageBlink *)msg;
        NRF_LOG_DEBUG("Received request to blink the LEDs %d times with duration of %d ms", message->count, message->duration);

        // Create and initialize animation data
        // We keep the data in a static variable so it stays valid after this call returns
        // Note: we keep the data in a static variable so it stays valid after this call returns
        static Blink blink;
        blink.play(message->color, message->duration, message->count, message->fade, message->faceMask, message->loopCount);

        MessageService::SendMessage(Message::MessageType_BlinkAck);
    }

    void BlinkIdHandler(const Message* msg)
    {
        auto *message = (const MessageBlinkId *)msg;
        NRF_LOG_DEBUG("Received request to blink id with brightness=%d and loopCount=%d", message->brightness, message->loopCount);

        // Create and initialize animation data
        // Note: we keep the data in a static variable so it stays valid after this call returns
        static AnimationBlinkId blinkId;
        blinkId.type = Animation_BlinkId;
        blinkId.framesPerBlink = 3; // 3 animation frames per blink
        blinkId.setDuration(1000);
        blinkId.brightness = message->brightness;

        // Stop previous instance in case it was still playing
        Modules::AnimController::stop(&blinkId);
        // And play new animation
        Modules::AnimController::play(&blinkId, nullptr, 0, message->loopCount);

        MessageService::SendMessage(Message::MessageType_BlinkIdAck);
    }

}
