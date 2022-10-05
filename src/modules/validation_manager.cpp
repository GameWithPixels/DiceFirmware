#include "die.h"
#include "validation_manager.h"
#include "config/board_config.h"
#include "nrf_log.h"
#include "animations/animation.h"
#include "modules/anim_controller.h"
#include "animations/animation_name.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_stack.h"
#include "drivers_nrf/power_manager.h"
#include "data_set/data_set.h"
#include "nrf_nvmc.h"
#include "drivers_nrf/timers.h"

using namespace Animations;
using namespace Modules;
using namespace Config;
using namespace DriversNRF;
using namespace Bluetooth;

#define VALIDATION_MODE_SLEEP_DELAY_MS 20000 // milliseconds

namespace Modules::ValidationManager
{
    static AnimationName nameAnim;
    static bool isPlaying;

    void stopNameAnim();
    void startNameAnim();
    void onConnection(void *token, bool connected);
    void exitValidationMode(void *token, const Message *msg);

    void GoToSleepCallback(void* ignore);

    // Initializes validation animation objects and hooks AnimController callback
    void init()
    {
        isPlaying = false;

        // Name animation object
        nameAnim.type = Animation_Name;
        nameAnim.framesPerBlink = 3; // 3 animation frames per blink
        nameAnim.setDuration(1000);
        nameAnim.brightness = 0x10;

        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_ExitValidation, nullptr, exitValidationMode);

        // Hook local connection function to BLE connection events
        Bluetooth::Stack::hook(onConnection, nullptr);

        NRF_LOG_INFO("Validation Manager Initialized");
    }

    // Function for playing validation animations
    void onDiceInitialized()
    {
        // Trigger a callback to turn die off
        Timers::setDelayedCallback(GoToSleepCallback, nullptr, VALIDATION_MODE_SLEEP_DELAY_MS);

        // Play preamble/name animation
        startNameAnim();
    }

    // Stop playing name animation
    void stopNameAnim()
    {
        NRF_LOG_INFO("Stopping name animation");
        AnimController::stopAll();
        isPlaying = false;
    }

    // Start playing name animation
    void startNameAnim()
    {
        NRF_LOG_INFO("Starting name animation");
        AnimController::play(&nameAnim, nullptr, 0, true); // Loop forever! (until timeout)
        isPlaying = true;
    }

    // Function for name animation behavior on BLE connection event
    void onConnection(void *token, bool connected)
    {
        if (connected)
        {
            stopNameAnim(); // Stop animation on connect
            Timers::cancelDelayedCallback(GoToSleepCallback, nullptr);
        }
        else
        {
            Timers::setDelayedCallback(GoToSleepCallback, nullptr, VALIDATION_MODE_SLEEP_DELAY_MS);
            if (!isPlaying)
                startNameAnim(); // Resume animation on disconnect
        }
    }

    // Clear bits to signal exit of validation mode, go to sleep
    void exitValidationMode(void *token, const Message *msg)
    {
        leaveValidation();
        PowerManager::goToSystemOff();
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

    void GoToSleepCallback(void* ignore)
    {
        PowerManager::goToSystemOff();
    }

}
