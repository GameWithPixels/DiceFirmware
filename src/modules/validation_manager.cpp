#include "pixel.h"
#include "validation_manager.h"
#include "config/board_config.h"
#include "config/value_store.h"
#include "nrf_log.h"
#include "animations/animation.h"
#include "modules/anim_controller.h"
#include "modules/behavior_controller.h"
#include "animations/animation_blinkid.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_stack.h"
#include "drivers_nrf/power_manager.h"
#include "data_set/data_set.h"
#include "nrf_nvmc.h"
#include "drivers_nrf/timers.h"
#include "drivers_hw/battery.h"

using namespace Animations;
using namespace Modules;
using namespace Config;
using namespace DriversNRF;
using namespace DriversHW;
using namespace Bluetooth;

#define VALIDATION_MODE_SLEEP_DELAY_MS 25000 // milliseconds

namespace Modules::ValidationManager
{
    static AnimationBlinkId blinkId;
    static bool isPlaying = false;

    void stopNameAnim();
    void startNameAnim();
    void onConnectionEvent(void *token, bool connected);
    void exitValidationModeHandler(const Message *msg);

    void GoToSysOffCallback(void* ignore);

    // Initializes validation animation objects and hooks AnimController callback
    void init()
    {
        // Has board been validated? If so this is probably a casted die
        const bool isCastedDie =
            ValueStore::readValue(ValueStore::ValueType_ValidationTimestampBoard) != 0xFFFFFFFF;

        // Name animation object
        blinkId.type = Animation_BlinkId;
        blinkId.framesPerBlink = 3; // blink duration = 3 x 33 ms
        blinkId.setDuration(1000);
        blinkId.brightness = isCastedDie ? 0x80 : 0x10; // Higher brightness for casted dice

        Bluetooth::MessageService::RegisterMessageHandler(
            Message::MessageType_ExitValidation, exitValidationModeHandler);

        NRF_LOG_DEBUG("Validation Manager init for %s", isCastedDie ? "casted die" : "bare board");
    }

    // Function for playing validation animations
    void onPixelInitialized()
    {
        // Hook local connection function to BLE connection events
        Bluetooth::Stack::hook(onConnectionEvent, nullptr);

        // Trigger a callback to turn die off
        Timers::setDelayedCallback(GoToSysOffCallback, nullptr, VALIDATION_MODE_SLEEP_DELAY_MS);

        // Play preamble/name animation
        startNameAnim();
    }

    // Stop playing name animation
    void stopNameAnim()
    {
        NRF_LOG_DEBUG("Stopping name animation");
        AnimController::stopAll();
        isPlaying = false;
    }

    // Start playing name animation
    void startNameAnim()
    {
        NRF_LOG_DEBUG("Starting name animation");
         // Loop animation for as long as we can
        AnimController::play(&blinkId, nullptr, 0, 0xff);
        isPlaying = true;
    }

    // Function for name animation behavior on BLE connection event
    void onConnectionEvent(void *token, bool connected)
    {
        if (connected)
        {
            stopNameAnim(); // Stop animation on connect
            BehaviorController::forceCheckBatteryState();
            Timers::cancelDelayedCallback(GoToSysOffCallback, nullptr);
        }
        else
        {
            Timers::setDelayedCallback(GoToSysOffCallback, nullptr, VALIDATION_MODE_SLEEP_DELAY_MS);
            if (!isPlaying)
                startNameAnim(); // Resume animation on disconnect
        }
    }

    // Clear bits to signal exit of validation mode, go to sleep
    void exitValidationModeHandler(const Message *msg)
    {
        leaveValidation();
        PowerManager::reset();
    }

    void leaveValidation()
    {
        Pixel::setCurrentRunMode(Pixel::RunMode_User);
    }

    // Check if system is in validation mode
    bool inValidation()
    {
        auto mode = Pixel::getCurrentRunMode();
        return mode == Pixel::RunMode_Validation;
    }

    void GoToSysOffCallback(void* ignore)
    {
        PowerManager::goToSystemOff();
    }

}
