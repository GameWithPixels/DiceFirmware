#include "hardware_test.h"
#include "die_private.h"
#include "nrf_log.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_stack.h"
#include "modules/leds.h"
#include "utils/Utils.h"

using namespace Bluetooth;

namespace Modules::HardwareTest
{
    void HardwareTestHandler(const Message* msg);
    void LedLoopbackHandler(const Message *msg);
    void DischargeHandler(const Message *msg);

    static MessageLedLoopback messageLedLoopback;

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_TestLedLoopback, LedLoopbackHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_Discharge, DischargeHandler);
        #if defined(DEBUG)
        //MessageService::RegisterMessageHandler(Message::MessageType_TestHardware, nullptr, HardwareTestHandler);
        #endif
        NRF_LOG_DEBUG("Hardware Test init");
    }

    void LedLoopbackHandler(const Message *msg) {
        messageLedLoopback.value = 1;
        MessageService::SendMessage(&messageLedLoopback);
    }

    void DischargeHandler(const Message *msg) {

        // We measured the following current consumptions
        // 1 x (111, 111, 111) -> 26 mA
        // 2 x (111, 111, 111) ->  37 mA
        // 4 x (111, 111, 111) ->  55 mA
        // 8 x (111, 111, 111) ->  86 mA
        // 12 x (111, 111, 111) ->  112 mA
        // 20 x (111, 111, 111) ->  152 mA

        // Used google sheets to extract a simply polynomial:
        // count = -0.769 + 0.0558 * I + 0.00053 * I * I

        static auto stopDischarge = []() {
            // Unhook connection event callback
            Bluetooth::Stack::unHookWithParam((void*)0x1E511E51); // No meaning, just a token to make unhooking easy. It does look like 'TESTTEST'

            // Turn off LEDs
            uint32_t zeros[MAX_COUNT];
            for (int px = 0; px < MAX_COUNT; ++px) {
                zeros[px] = 0;
            }
            LEDs::setPixelColors(zeros);

            // Return the die to its standard state
            Die::enterStandardState();
        };

        auto dischargeMsg = (const MessageDischarge*)msg;
        if (dischargeMsg->currentMA > 0) {
            // Put the die in test mode
            Die::enterTestingState();

            // Just to be safe, add disconnection handler to reset the state
            Bluetooth::Stack::hook([](void* param, bool connected) {
                // On disconnect, stop discharge and unhook, etc...
                stopDischarge();
            }, (void*)0x1E511E51); // No meaning, just a token to make unhooking easy. It does look like 'TESTTEST'

            // Turn LEDs on according to the message
            // count = -0.769 + 0.0558 * I + 0.00053 * I * I
            int count = (-76900 + 5580 * dischargeMsg->currentMA + 53 * dischargeMsg->currentMA * dischargeMsg->currentMA) / 100000; // using fixed point math
            if (count < 1)
                count = 1;
            if (count > MAX_COUNT)
                count = MAX_COUNT;
            uint32_t colors[MAX_COUNT];
            uint32_t c = Utils::toColor(112,112,112); // This colors consumes ~10mA per LED
            int px = 0;
            for (; px < count; ++px) { colors[px] = c; }
            for (; px < MAX_COUNT; ++px) { colors[px] = 0; }
            LEDs::setPixelColors(colors);
        } else {
            stopDischarge();
        }
    }
}
