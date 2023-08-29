#include "led_color_tester.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "utils/utils.h"
#include "nrf_log.h"
#include "modules/anim_controller.h"
#include "animations/animation.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "config/dice_variants.h"
#include "leds.h"

using namespace Modules;
using namespace Bluetooth;
using namespace Utils;
using namespace Animations;
using namespace Config;

namespace Modules::LEDColorTester
{
    void SetLEDToColorHandler(const Message* msg);
    void SetAllLEDsToColorHandler(const Message* msg);
    void LightUpFaceHandler(const Message* msg);

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_SetLEDToColor, SetLEDToColorHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_SetAllLEDsToColor, SetAllLEDsToColorHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_LightUpFace, LightUpFaceHandler);
        NRF_LOG_DEBUG("LED Color tester ini");
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

		auto l = DiceVariants::getLayout();

        auto lufmsg = static_cast<const MessageLightUpFace*>(msg);

        NRF_LOG_INFO("Light Up Face: face: %d, remapFace: %d, color: %08x", lufmsg->face, lufmsg->opt_remapFace, lufmsg->color);

        int remapFace = lufmsg->opt_remapFace;
        if (remapFace == 0xFF) {
            remapFace = Accelerometer::currentFace();
        }

		uint16_t ledIndex = l->canonicalIndexToElectricalIndexLookup[remapFace];

        NRF_LOG_INFO(" -> LED Index: %d, color: %08x", ledIndex, lufmsg->color);
        BLE_LOG_INFO("ledIndex: %d", ledIndex);

        LEDs::setPixelColor(ledIndex, lufmsg->color);
    }

}
