#pragma once

#include "stdint.h"
#include "stddef.h"
#include "core/int3.h"
#include "dice_variants.h"

#define MAX_COUNT 22		// Max LED count so far is 21 (on PD6)
							// but we want room for one more 'fake' LED to test LED return
#define MAX_NAME_LENGTH 24

namespace Config
{
	// Flags for various debugging options
	enum class DebugFlags : uint32_t
	{
		None = 0,
		LEDsStayOff = 1,			 // Prevent LEDs for lighting up
		OnBootToggleLEDsStayOff = 2, // Toggle above flag on firmware boot
		LoopCycleAnimation = 4,		 // Light up LEDs one by one, forever
	};

	struct Settings
	{
		// Indicates whether there is valid data
		uint32_t headMarker;
		int version;

		// Physical Appearance
		DiceVariants::DesignAndColor designAndColor;

		char name[MAX_NAME_LENGTH + 1]; // One extra byte for the zero terminator

		// Face detector
		int sigmaDecayTimes1000;
		int startMovingThresholdTimes1000;
		int stopMovingThresholdTimes1000;
		int faceThresholdTimes1000;
		int fallingThresholdTimes1000;
		int shockThresholdTimes1000;
		int accDecayTimes1000;

		// Calibration data
		Core::int3 faceNormals[MAX_COUNT];

		// Indicates whether there is valid data
		uint32_t tailMarker;
	};

	namespace SettingsManager
	{
		typedef void (*InitCallback)();
		typedef void (*SettingsWrittenCallback)(bool success);

		void init(InitCallback callback);
		bool checkValid();
		Config::Settings const * const getSettings();

		void setDefaults(Settings& outSettings);
		void programDefaults(SettingsWrittenCallback callback);
		void programDefaultParameters(SettingsWrittenCallback callback);
		void programCalibrationData(const Core::int3* newNormals, int count, SettingsWrittenCallback callback);
		void programDesignAndColor(DiceVariants::DesignAndColor design, SettingsWrittenCallback callback);
		void programName(const char* newName, SettingsWrittenCallback callback);
	}
}
