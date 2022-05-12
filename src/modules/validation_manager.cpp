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

#include "utils/Utils.h"

using namespace Animations;
using namespace Modules;
using namespace Config;
using namespace DriversNRF;
using namespace Bluetooth;

namespace Modules::ValidationManager
{
    static AnimationName nameAnim;
    static bool isPlaying;

    void skipFeed(void *token);
    void stopNameAnim();
    void startNameAnim();
    void onConnection(void *token, bool connected);
    void exitValidationMode(void *token, const Message *msg);

    // Initializes validation animation objects and hooks AnimController callback
    void init()
    {
        isPlaying = false;

        // Name animation object
        nameAnim.type = Animation_Name;
        nameAnim.framesPerBlink = 3; // 3 animation frames per blink
        nameAnim.setDuration(1000);
        nameAnim.brightness = 0x10;

        NRF_LOG_INFO("Validation Manager Initialized");
    }

    // Function for playing validation animations
    void onDiceInitialized()
    {
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_ExitValidation, nullptr, exitValidationMode);

        // Hook local connection function to BLE connection events
        Bluetooth::Stack::hook(onConnection, nullptr);

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
        }
        else
        {
            if (!isPlaying)
                startNameAnim(); // Resume animation on disconnect
        }
    }

    // Clear bits to signal exit of valdidation mode, go to sleep
    void exitValidationMode(void *token, const Message *msg)
    {
        Utils::leaveValidation();
        PowerManager::goToSystemOff();
    }
}
