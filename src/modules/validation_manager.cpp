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

#if !defined(FIRMWARE_VERSION)
    #warning Firmware version not defined
    #define FIRMWARE_VERSION "Unknown"
#endif

namespace Modules::ValidManager
{
	static AnimationName nameAnim;
	bool isPlaying;

	void skipFeed(void* token);
	void stopNameAnim();
	void startNameAnim();
	void onConnection(void* token, bool connected);
    void exitValidationMode(void* token, const Message* msg);
    void WhoAreYouHandler(void* token, const Message* message);
    void EnterSleepMode(void* token, const Message* msg);

	// Initializes validation animation objects and hooks AnimController callback
	void init() 
	{
		// Determine which board is running
		int ledCount = BoardManager::getBoard()->ledCount;
		isPlaying = false;

		// Name animation object
        nameAnim.type = Animation_Name;
        switch (ledCount) 	// Change duration and count by board type
        {
		    // D20
            case 20:
                nameAnim.preamble_count = 4;
                nameAnim.duration = 1188;
			    nameAnim.brightness = 15;
                break;
			
		    // D6
		    case 6:
                nameAnim.preamble_count = 6;
                nameAnim.duration = 1254;
			    nameAnim.brightness = 4;
                break;
        }

		// Assign callback for AnimController to skip PowerManager::feed
		AnimController::hook(skipFeed, nullptr);

		NRF_LOG_INFO("Validation Manager Initialized");
	}

	// Function for playing validation animations
	void onDiceInitialized() 
	{
		// May want other validation function calls here as well
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_WhoAreYou, nullptr, WhoAreYouHandler);
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_ExitValidation, nullptr, exitValidationMode);
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_Sleep, nullptr, EnterSleepMode);

		// Hook local connection function to BLE connection events
		Bluetooth::Stack::hook(onConnection, nullptr);

		// Play preamble/name animation
        startNameAnim();
	}

	// Callback for explicitly skipping PowerManager::feed call in AnimController
	void skipFeed(void* token)
	{
		return;
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
		AnimController::play(&nameAnim, nullptr, 0, true); 		// Loop forever! (until timeout)
		isPlaying = true;
	}

	// Function for name animation behavior on BLE connection event
	void onConnection(void* token, bool connected) 
    {
        if (connected) 
		{
            stopNameAnim();		// Stop animation on connect
        } 
		else 
		{
            if (!isPlaying) startNameAnim();	// Resume animation on disconnect
        }
    }

    // Clear bits to signal exit of valdidation mode, go to sleep
    void exitValidationMode(void* token, const Message* msg)
    {
        SettingsManager::programDefaults(nullptr);
        Utils::leaveValidation();
        PowerManager::goToSystemOff();
    }

    void WhoAreYouHandler(void* token, const Message* message) {
        // Central asked for the die state, return it!
        Bluetooth::MessageIAmADie identityMessage;
        identityMessage.deviceId = Die::getDeviceID();
        identityMessage.dataSetHash = DataSet::dataHash();
        strncpy(identityMessage.versionInfo, FIRMWARE_VERSION, VERSION_INFO_SIZE);
        identityMessage.faceCount = (uint8_t)BoardManager::getBoard()->ledCount;
        identityMessage.designAndColor = SettingsManager::getSettings()->designAndColor;
        identityMessage.flashSize = DataSet::availableDataSize();
        Bluetooth::MessageService::SendMessage(&identityMessage);
    }

    void EnterSleepMode(void* token, const Message* msg) {
        PowerManager::goToSystemOff();
    }
}
