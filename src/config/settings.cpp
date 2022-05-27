#include "settings.h"
#include "drivers_nrf/flash.h"
#include "nrf_log.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "config/board_config.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_stack.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bulk_data_transfer.h"
#include "malloc.h"
#include "config/dice_variants.h"
#include "utils/utils.h"
#include "data_set/data_set.h"
#include "die.h"

#define SETTINGS_VALID_KEY (0x15E77165) // 1SETTINGS in leet speak ;)
#define SETTINGS_VERSION 3
#define SETTINGS_PAGE_COUNT 1

using namespace DriversNRF;
using namespace Bluetooth;
using namespace Config;
using namespace Modules;
using namespace DataSet;

namespace Config::SettingsManager
{
	static Settings const * settings = nullptr;

	void ProgramDefaultParametersHandler(void* context, const Message* msg);
	void SetDesignTypeAndColorHandler(void* context, const Message* msg);
	void SetNameHandler(void* context, const Message* msg);
	void SetDebugFlagsHandler(void* context, const Message* msg);
	
	#if BLE_LOG_ENABLED
	void PrintNormals(void* context, const Message* msg) {
		auto m = static_cast<const MessagePrintNormals*>(msg);
		int i = m->face;
		auto settings = getSettings();
		BLE_LOG_INFO("Face %d: %d, %d, %d", i, (int)(settings->faceNormals[i].x * 100), (int)(settings->faceNormals[i].y * 100), (int)(settings->faceNormals[i].z * 100));
	}
	#endif

	void init(SettingsWrittenCallback callback) {
		static SettingsWrittenCallback _callback; // Don't initialize this static inline because it would only do it on first call!
		_callback = callback;

		settings = (Settings const * const)Flash::getSettingsStartAddress();

		auto finishInit = [](bool success) {
			// Register as a handler to program settings
			MessageService::RegisterMessageHandler(Message::MessageType_ProgramDefaultParameters, nullptr, ProgramDefaultParametersHandler);
			MessageService::RegisterMessageHandler(Message::MessageType_SetDesignAndColor, nullptr, SetDesignTypeAndColorHandler);
			MessageService::RegisterMessageHandler(Message::MessageType_SetName, nullptr, SetNameHandler);
			
			#if BLE_LOG_ENABLED
			MessageService::RegisterMessageHandler(Message::MessageType_PrintNormals, nullptr, PrintNormals);
			#endif

			NRF_LOG_INFO("Settings initialized, size=%d bytes", sizeof(Settings));

			auto callBackCopy = _callback;
			_callback = nullptr;
			if (callBackCopy != nullptr) {
				callBackCopy(success);
			}
		};

		if (!checkValid()) {
			NRF_LOG_WARNING("Settings not found in flash, programming defaults");
			programDefaults(finishInit);
		} else {
    		finishInit(true);
		}
	}

	bool checkValid() {
		return (settings->headMarker == SETTINGS_VALID_KEY &&
			settings->version == SETTINGS_VERSION &&
			settings->tailMarker == SETTINGS_VALID_KEY);
	}

	Settings const * const getSettings() {
		if (!checkValid()) {
			return nullptr;
		} else {
			return settings;
		}
	}

	void ProgramDefaultParametersHandler(void* context, const Message* msg) {
		programDefaultParameters([] (bool result) {
			// Ignore result for now
			Bluetooth::MessageService::SendMessage(Message::MessageType_ProgramDefaultParametersFinished);
		});
	}

	void SetDesignTypeAndColorHandler(void* context, const Message* msg) {
		auto designMsg = (const MessageSetDesignAndColor*)msg;
		NRF_LOG_INFO("Received request to set design to %d", designMsg->designAndColor);
		programDesignAndColor(designMsg->designAndColor, [](bool result) {
			MessageService::SendMessage(Message::MessageType_SetDesignAndColorAck);
		});
	}

	void SetNameHandler(void* context, const Message* msg) {
		auto nameMsg = (const MessageSetName*)msg;
		NRF_LOG_INFO("Received request to rename die to %s", nameMsg->name);
		programName(nameMsg->name, [](bool result) {
			MessageService::SendMessage(Message::MessageType_SetNameAck);
		});
	}

	void setDefaultParameters(Settings& outSettings) {
        // Generate our name
        outSettings.name[0] = '\0';
		uint32_t uniqueId = Die::getDeviceID();
		for (int i = 0; i < 7; ++i) {
            outSettings.name[i] = '0' + uniqueId % 10;
            uniqueId /= 10;
        }
        outSettings.name[7] = '\0';
		outSettings.designAndColor = DiceVariants::DesignAndColor::DesignAndColor_Generic;
		outSettings.jerkClamp = 10.f;
		outSettings.sigmaDecay = 0.5f;
		outSettings.startMovingThreshold = 5.0f;
		outSettings.stopMovingThreshold = 0.5f;
		outSettings.faceThreshold = 0.98f;
		outSettings.fallingThreshold = 0.1f;
		outSettings.shockThreshold = 7.5f;
		outSettings.batteryLow = 3.0f;
		outSettings.batteryHigh = 4.0f;
		outSettings.accDecay = 0.9f;
		outSettings.heatUpRate = 0.0004f;
		outSettings.coolDownRate = 0.995f;
	}

	void setDefaultCalibrationData(Settings& outSettings) {
		// Copy normals from defaults
        auto board = BoardManager::getBoard();
		int ledCount = board->ledCount;
		const Core::float3* defaultNormals = board->layout.baseNormals;
		for (int i = 0; i < ledCount; ++i) {
			outSettings.faceNormals[i] = defaultNormals[i];
		}
	}


	void setDefaults(Settings& outSettings) {
		outSettings.headMarker = SETTINGS_VALID_KEY;
		outSettings.version = SETTINGS_VERSION;
		setDefaultParameters(outSettings);
		setDefaultCalibrationData(outSettings);
		outSettings.tailMarker = SETTINGS_VALID_KEY;
	}

	void programDefaults(SettingsWrittenCallback callback) {
		Settings defaults;
		setDefaults(defaults);
		DataSet::ProgramDefaultDataSet(defaults, callback);
	}

	void programDefaultParameters(SettingsWrittenCallback callback) {

		// Grab current settings
		Settings settingsCopy;

		// Begin by resetting our new settings
		setDefaults(settingsCopy);

		// Copy over everything
		memcpy(&settingsCopy, settings, sizeof(Settings));

		// Change normals
		setDefaultParameters(settingsCopy);

		// Reprogram settings
		DataSet::ProgramDefaultDataSet(settingsCopy, callback);
	}

	void programCalibrationData(const Core::float3* newNormals, int count, SettingsWrittenCallback callback) {

		// Grab current settings
		Settings settingsCopy;

		// Begin by resetting our new settings
		setDefaults(settingsCopy);

		// Copy over everything
		memcpy(&settingsCopy, settings, sizeof(Settings));

		// Change normals
		memcpy(&(settingsCopy.faceNormals[0]), newNormals, count * sizeof(Core::float3));

		// Reprogram settings
		DataSet::ProgramDefaultDataSet(settingsCopy, callback);
	}

	void programDesignAndColor(DiceVariants::DesignAndColor design, SettingsWrittenCallback callback) {

		if (settings->designAndColor != design) {

			// Grab current settings
			Settings settingsCopy;
			memcpy(&settingsCopy, settings, sizeof(Settings));

			// Update design and color
			settingsCopy.designAndColor = design;

			// Reprogram settings
			DataSet::ProgramDefaultDataSet(settingsCopy, callback);
		}
		else {
			NRF_LOG_INFO("DesignAndColor already set to %s", design);
			callback(true);
		}
	}

	void programName(const char* newName, SettingsWrittenCallback callback) {

		if (strncmp(settings->name, newName, sizeof(settings->name))) {

			// Grab current settings
			Settings settingsCopy;
			memcpy(&settingsCopy, settings, sizeof(Settings));

			// Update name
			strncpy(settingsCopy.name, newName, sizeof(settingsCopy.name));

			// Reprogram settings
			static SettingsWrittenCallback programNameCallback = nullptr;
			programNameCallback = callback;
			DataSet::ProgramDefaultDataSet(settingsCopy, [] (bool success) {
				// We want to reset once disconnected so to apply the name change
				Bluetooth::Stack::resetOnDisconnect();
				auto callback = programNameCallback;
				programNameCallback = nullptr;
				if (callback)
					callback(success);
			});
		}
		else {
			NRF_LOG_INFO("Name already set to %s", newName);
			callback(true);
		}
	}
}
