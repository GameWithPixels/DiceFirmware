#include "discharge_controller.h"
#include "nrf_log.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_stack.h"
#include "modules/leds.h"
#include "utils/Utils.h"
#include "modules/behavior_controller.h"
#include "modules/anim_controller.h"

using namespace Bluetooth;
using namespace Modules;

namespace Modules::DischargeController
{
    void DischargeHandler(const Message *msg);
    int currentMA;

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_Discharge, DischargeHandler);
        currentMA = 0;
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
            BehaviorController::EnableAccelerometerRules();
            AnimController::start();

            currentMA = 0;
        };

        auto dischargeMsg = (const MessageDischarge*)msg;
        if (dischargeMsg->currentMA > 0) {

            // Store the current being discharged
            currentMA = dischargeMsg->currentMA;

            // Put the die in special mode
            BehaviorController::DisableAccelerometerRules();
            AnimController::stop();

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

    int getDischargeCurrent() {
        return currentMA;
    }

    bool isDischarging() {
        return currentMA > 0;
    }
}
