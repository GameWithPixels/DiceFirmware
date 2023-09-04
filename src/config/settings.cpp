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
#include "pixel.h"
#include "app_error.h"

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

	void ProgramDefaultParametersHandler(const Message* msg);
	void SetDesignTypeAndColorHandler(const Message* msg);
	void SetNameHandler(const Message* msg);
	void SetDebugFlagsHandler(const Message* msg);
	
	#if BLE_LOG_ENABLED
	void PrintNormals(const Message* msg) {
		auto m = static_cast<const MessagePrintNormals*>(msg);
		int i = m->face;
		auto settings = getSettings();
		BLE_LOG_INFO("Face %d: %d, %d, %d", i, (int)(settings->faceNormals[i].x * 100), (int)(settings->faceNormals[i].y * 100), (int)(settings->faceNormals[i].z * 100));
	}
	#endif

	void init(InitCallback callback) {
		static InitCallback _callback; // Don't initialize this static inline because it would only do it on first call!
		_callback = callback;

		settings = (Settings const * const)Flash::getSettingsStartAddress();

		auto finishInit = [](bool success) {
	        APP_ERROR_CHECK(success ? NRF_SUCCESS : NRF_ERROR_INTERNAL);

			// Register as a handler to program settings
			MessageService::RegisterMessageHandler(Message::MessageType_ProgramDefaultParameters, ProgramDefaultParametersHandler);
			MessageService::RegisterMessageHandler(Message::MessageType_SetDesignAndColor, SetDesignTypeAndColorHandler);
			MessageService::RegisterMessageHandler(Message::MessageType_SetName, SetNameHandler);
			
			#if BLE_LOG_ENABLED
			MessageService::RegisterMessageHandler(Message::MessageType_PrintNormals, PrintNormals);
			#endif

			NRF_LOG_INFO("Settings init, size: %d", sizeof(Settings));

			auto callBackCopy = _callback;
			_callback = nullptr;
			if (callBackCopy != nullptr) {
				callBackCopy();
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

	void ProgramDefaultParametersHandler(const Message* msg) {
		programDefaultParameters([] (bool result) {
			// Ignore result for now
			Bluetooth::MessageService::SendMessage(Message::MessageType_ProgramDefaultParametersFinished);
		});
	}

	void SetDesignTypeAndColorHandler(const Message* msg) {
		auto designMsg = (const MessageSetDesignAndColor*)msg;
		NRF_LOG_DEBUG("Received request to set design to %d", designMsg->designAndColor);
		programDesignAndColor(designMsg->designAndColor, [](bool result) {
			MessageService::SendMessage(Message::MessageType_SetDesignAndColorAck);
		});
	}

	void SetNameHandler(const Message* msg) {
		auto nameMsg = (const MessageSetName*)msg;
		NRF_LOG_DEBUG("Received request to rename die to %s", nameMsg->name);
		programName(nameMsg->name, [](bool result) {
			MessageService::SendMessage(Message::MessageType_SetNameAck);
		});
	}

	void setDefaultParameters(Settings& outSettings) {
        // Generate our name
		const char pixel[] = "Pixel";
		static_assert(sizeof(pixel) + 9 < sizeof(outSettings.name));
		strcpy(outSettings.name, pixel);
		uint32_t uniqueId = Pixel::getDeviceID();
    	for (int i = 0; i < 8; ++i) {
			const char value = (uniqueId >> ((7 - i) << 2)) & 0xf;
			outSettings.name[i + sizeof(pixel) - 1] = value + (value < 10 ? '0' : 'a' - 10);
		}
		outSettings.settingsTimeStamp = Pixel::getBuildTimestamp();
		outSettings.dieType = DiceVariants::estimateDieTypeFromBoard();
		outSettings.name[8 + sizeof(pixel)] = '\0';
		outSettings.designAndColor = DiceVariants::DesignAndColor::DesignAndColor_Unknown;
		outSettings.customDesignAndColorName[0] = '\0';
		outSettings.sigmaDecayTimes1000 = 500;
		outSettings.startMovingThresholdTimes1000 = 5000;
		outSettings.stopMovingThresholdTimes1000 = 500;
		outSettings.faceThresholdTimes1000 = 980;
		outSettings.fallingThresholdTimes1000 = 100;
		outSettings.shockThresholdTimes1000 = 7500;
		outSettings.accDecayTimes1000 = 900;
	}

	void setDefaultCalibrationData(Settings& outSettings) {
		// Copy normals from defaults
        auto layout = DiceVariants::getLayout();
		int ledCount = layout->ledCount;
		const Core::int3* defaultNormals = layout->baseNormals;
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

		// // Copy over everything
		// memcpy(&settingsCopy, settings, sizeof(Settings));

		// // Change normals
		// setDefaultParameters(settingsCopy);

		// Reprogram settings
		DataSet::ProgramDefaultDataSet(settingsCopy, callback);
	}

	void programCalibrationData(const Core::int3* newNormals, int count, SettingsWrittenCallback callback) {

		// Grab current settings
		Settings settingsCopy;

		// Begin by resetting our new settings
		setDefaults(settingsCopy);

		// Copy over everything
		memcpy(&settingsCopy, settings, sizeof(Settings));

		// Change normals
		memcpy(&(settingsCopy.faceNormals[0]), newNormals, count * sizeof(Core::int3));

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
			NRF_LOG_DEBUG("DesignAndColor already set to %s", design);
			callback(true);
		}
	}

	void programName(const char* newName, SettingsWrittenCallback callback) {

		if (strncmp(settings->name, newName, MAX_NAME_LENGTH)) {

			// Grab current settings
			Settings settingsCopy;
			memcpy(&settingsCopy, settings, sizeof(Settings));

			// Update name
			strncpy(settingsCopy.name, newName, MAX_NAME_LENGTH);
			settingsCopy.name[MAX_NAME_LENGTH] = 0; // Make sure we always have a null terminated string
			NRF_LOG_INFO("Setting name to %s", settingsCopy.name);

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
			NRF_LOG_DEBUG("Name already set to %s", newName);
			callback(true);
		}
	}
}
