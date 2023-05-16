#include "pixel.h"
#include "validation_manager.h"
#include "config/board_config.h"
#include "nrf_log.h"
#include "animations/animation.h"
#include "modules/anim_controller.h"
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
#define VALIDATION_MIN_VBAT_TIMES_1000 3500 // millivolts
namespace Modules::ValidationManager
{
    static AnimationBlinkId nameAnim;
    static bool isPlaying;

    void stopNameAnim();
    void startNameAnim();
    void onConnectionEvent(void *token, bool connected);
    void exitValidationModeHandler(const Message *msg);

    void GoToSysOffCallback(void* ignore);

    // Initializes validation animation objects and hooks AnimController callback
    void init()
    {
        isPlaying = false;

        // Name animation object
        nameAnim.type = Animation_BlinkId;
        nameAnim.framesPerBlink = 3; // 3 animation frames per blink
        nameAnim.setDuration(1000);
        nameAnim.brightness = 0x10;

        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_ExitValidation, exitValidationModeHandler);

        // Hook local connection function to BLE connection events
        Bluetooth::Stack::hook(onConnectionEvent, nullptr);

        NRF_LOG_DEBUG("Validation Manager init");
    }

    // Function for playing validation animations
    void onPixelInitialized()
    {
        // Trigger a callback to turn die off
        Timers::setDelayedCallback(GoToSysOffCallback, nullptr, VALIDATION_MODE_SLEEP_DELAY_MS);

        // Play preamble/name animation
        startNameAnim();
    }

    bool checkMinVBat()
    {
        return Battery::checkVBatTimes1000() > VALIDATION_MIN_VBAT_TIMES_1000;
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
        AnimController::play(&nameAnim, nullptr, 0, true); // Loop forever! (until timeout)
        isPlaying = true;
    }

    // Function for name animation behavior on BLE connection event
    void onConnectionEvent(void *token, bool connected)
    {
        if (connected)
        {
            stopNameAnim(); // Stop animation on connect
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
