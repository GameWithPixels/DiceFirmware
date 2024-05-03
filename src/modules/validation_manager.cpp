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

    void stopBlinkId();
    void startBlinkId();
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
        blinkId.type = AnimationType_BlinkID;
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
        startBlinkId();
    }

    // Stop playing name animation
    void stopBlinkId()
    {
        NRF_LOG_DEBUG("Stopping name animation");
        AnimController::stopAll();
        isPlaying = false;
    }

    // Start playing name animation
    void startBlinkId()
    {
        NRF_LOG_DEBUG("Starting name animation");
         // Loop animation for as long as we can
        AnimController::PlayAnimationParameters params;
        params.loopCount = 0xff;
        AnimController::play(&blinkId, params);
        isPlaying = true;
    }

    // Function for name animation behavior on BLE connection event
    void onConnectionEvent(void *token, bool connected)
    {
        if (connected)
        {
            stopBlinkId(); // Stop animation on connect
            BehaviorController::forceCheckBatteryState();
            Timers::cancelDelayedCallback(GoToSysOffCallback, nullptr);
        }
        else
        {
            Timers::setDelayedCallback(GoToSysOffCallback, nullptr, VALIDATION_MODE_SLEEP_DELAY_MS);
            if (!isPlaying)
                startBlinkId(); // Resume animation on disconnect
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
        // Use additional bit in UICR register to skip validation functions, perform normal functions
        nrf_nvmc_write_byte((uint32_t)&NRF_UICR->CUSTOMER[0], 0xFC);
    }

    // Check if system is in validation mode
    bool inValidation()
    {
        uint32_t *validation = (uint32_t *)&NRF_UICR->CUSTOMER[0];
        return (*validation == 0xFFFFFFFE);
    }

    void GoToSysOffCallback(void* ignore)
    {
        PowerManager::goToSystemOff();
    }

}
