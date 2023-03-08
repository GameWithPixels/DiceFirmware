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

#if defined(DEBUG)
    // APP_TIMER_DEF(ledsTimer);
    // APP_TIMER_DEF(chargingTimer);

    // void HardwareTestHandler(void* context, const Message* msg) {
    //     NRF_LOG_INFO("Starting Hardware Test");

    //     // Reprogram default anim settings
    //     DataSet::ProgramDefaultDataSet(*SettingsManager::getSettings(), [] (bool result) {

    //         // Good, move onto the next test
    //         MessageService::NotifyUser("Put the die down!", true, true, 30, [](bool okCancel)
    //         {
    //             if (okCancel) {
    //                 // Check that interrupt pin is low
    //                 if (Accelerometer::checkIntPin()) {
    //                     NRF_LOG_INFO("Good int pin");
    //                     // Good, try interrupt
    //                     MessageService::NotifyUser("Now pick the die up!", false, false, 10, nullptr);

    //                     NRF_LOG_INFO("Setting up interrupt");

    //                     // Set interrupt pin
    //                     GPIOTE::enableInterrupt(
    //                         BoardManager::getBoard()->accInterruptPin,
    //                         NRF_GPIO_PIN_NOPULL,
    //                         NRF_GPIOTE_POLARITY_LOTOHI,
    //                         [](uint32_t pin, nrf_gpiote_polarity_t action)
    //                         {
    //                             GPIOTE::disableInterrupt(BoardManager::getBoard()->accInterruptPin);
                                
    //                             // Don't do a lot of work in interrupt context
    //                             Scheduler::push(nullptr, 0, [](void * p_event_data, uint16_t event_size)
    //                             {
    //                                 NRF_LOG_INFO("Interrupt triggered");
    //                                 // Acc seems to work well,

    //                                 // Turn all LEDs on repeatedly!
    //                                 Timers::createTimer(&ledsTimer, APP_TIMER_MODE_REPEATED, [](void* ctx)
    //                                 {
    //                                     AnimController::play(DataSet::getAnimationCount() - 3);
    //                                 });
    //                                 Timers::startTimer(ledsTimer, 1000, nullptr);
    //                                 AnimController::play(DataSet::getAnimationCount() - 3);

    //                                 MessageService::NotifyUser("Check all leds", true, true, 30, [](bool okCancel)
    //                                 {
    //                                     Timers::stopTimer(ledsTimer);

    //                                     // LEDs seem good, test charging
    //                                     if (okCancel) {
    //                                         char buffer[100]; buffer[0] = '\0';
    //                                         const char* stateString = BatteryController::getBatteryStateString(BatteryController::getCurrentChargeState());
    //                                         float vbat = Battery::checkVBat();
    //                                         sprintf(buffer, "Battery %s, " SPRINTF_FLOAT_MARKER "V. place on charger!", stateString, SPRINTF_FLOAT(vbat));
    //                                         MessageService::NotifyUser(buffer, false, false, 30, nullptr);

    //                                         // Register a handler with the battery controller
    //                                         BatteryController::hook([](void* ignore, BatteryController::BatteryState newState) {
    //                                             if (newState == BatteryController::BatteryState_Charging) {

    //                                                 // Good! unhook from the controller now
    //                                                 BatteryController::unHookWithParam((void*)(0x12345678));
    //                                                 Timers::stopTimer(chargingTimer);

    //                                                 // Done!
    //                                                 MessageService::NotifyUser("Test complete!", true, false, 10, nullptr);
    //                                             }
    //                                         }, (void*)(0x12345678));

    //                                         // Turn all LEDs on repeatedly!
    //                                         Timers::createTimer(&chargingTimer, APP_TIMER_MODE_SINGLE_SHOT, [](void* ctx)
    //                                         {
    //                                             BatteryController::unHookWithParam((void*)(0x12345678));
    //                                             MessageService::NotifyUser("No charging detected!", true, false, 10, nullptr);

    //                                             // Done!
    //                                         });
    //                                         Timers::startTimer(chargingTimer, 30000, nullptr);
    //                                     }
    //                                 });
    //                             });
    //                         });

    //                     Accelerometer::enableInterrupt();
    //                 } else {
    //                     NRF_LOG_INFO("Bad int pin");
    //                     MessageService::NotifyUser("Bad Int. pin.", true, false, 10, nullptr);
    //                 }
    //             }
    //         });
    //     });
    // }
#endif
}
