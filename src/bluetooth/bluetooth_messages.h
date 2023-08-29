#pragma once

#include <stdint.h>
#include "config/sdk_config.h"
#include "config/dice_variants.h"
#include "modules/accelerometer.h"
#include "config/settings.h"
#include "modules/battery_controller.h"
#include "core/int3.h"
#include "modules/accelerometer.h"

#define MAX_DATA_SIZE 100

#pragma pack(push, 1)

namespace Bluetooth
{

using BatteryState = Modules::BatteryController::BatteryState;
using BatteryControllerState = Modules::BatteryController::State;
using RollState = Modules::Accelerometer::RollState;

/// <summary>
///  Base class for messages from the die to the app
/// </summary>
struct Message
{
	enum MessageType : uint8_t
	{
		MessageType_None = 0,
		MessageType_WhoAreYou,
		MessageType_IAmADie,
		MessageType_RollState,
		MessageType_Telemetry,
		MessageType_BulkSetup,
		MessageType_BulkSetupAck,
		MessageType_BulkData,
		MessageType_BulkDataAck,
		MessageType_TransferProfile,
		MessageType_TransferProfileAck,
		MessageType_TransferProfileFinished,
		MessageType_Unused_001,
		MessageType_Unused_002,
		MessageType_Unused_003,
		MessageType_TransferInstantProfile,
		MessageType_TransferInstantProfileAck,
		MessageType_TransferInstantProfileFinished,
		MessageType_DebugLog,
		MessageType_PlayAnim,
		MessageType_Unused_007,
		MessageType_StopAnim,
		MessageType_RemoteAction,
		MessageType_RequestRollState,
		MessageType_RequestAnimSet,
		MessageType_RequestSettings,
		MessageType_RequestTelemetry,
		MessageType_ProgramDefaultProfile,
		MessageType_ProgramDefaultProfileFinished,
		MessageType_Blink,
		MessageType_BlinkAck,
		MessageType_Unused_008,
		MessageType_Unused_009,
		MessageType_RequestBatteryLevel,
		MessageType_BatteryLevel,
		MessageType_RequestRssi,
		MessageType_Rssi,
		MessageType_Calibrate,
		MessageType_CalibrateFace,
		MessageType_NotifyUser,
		MessageType_NotifyUserAck,
		MessageType_TestHardware,
		MessageType_TestLedLoopback,
		MessageType_LedLoopback,
		MessageType_SetTopLevelState,
		MessageType_ProgramDefaultParameters,
		MessageType_ProgramDefaultParametersFinished,
		MessageType_SetDesignAndColor,
		MessageType_SetDesignAndColorAck,
		MessageType_Unused_010,
		MessageType_Unused_011,
		MessageType_SetName,
		MessageType_SetNameAck,
		MessageType_Sleep,
		MessageType_ExitValidation,
		MessageType_Unused_012,
		MessageType_Unused_013,
		MessageType_Unused_014,
		MessageType_PlayInstantAnim,
		MessageType_StopAllAnims,
		MessageType_RequestTemperature,
		MessageType_Temperature,
		MessageType_EnableCharging,
		MessageType_DisableCharging,
		MessageType_Discharge,
		MessageType_BlinkId,
		MessageType_BlinkIdAck,
		MessageType_TransferTest,
		MessageType_TransferTestAck,
		MessageType_TransferTestFinished,

		// TESTING
		MessageType_TestBulkSend,
		MessageType_TestBulkReceive,
		MessageType_SetAllLEDsToColor,
		MessageType_AttractMode,
		MessageType_PrintNormals,
		MessageType_PrintA2DReadings,
		MessageType_LightUpFace,
		MessageType_SetLEDToColor,
		MessageType_PrintAnimControllerState,

		MessageType_Count,
	};

	MessageType type;

	Message(MessageType msgType) : type(msgType) {}

protected:
	Message() : type(MessageType_None) {}
};


/// <summary>
/// Identifies the dice
/// </summary>
struct MessageIAmADie
	: public Message
{
	uint8_t ledCount; // Number of LEDs
	Config::DiceVariants::DesignAndColor designAndColor; // Physical look
	uint8_t padding;
	uint32_t dataSetHash;
	uint32_t pixelId; // A unique identifier
	uint16_t availableFlash; // How much room available for data
	uint32_t buildTimestamp;

	// Roll state
	uint8_t rollState;
	uint8_t rollFace; // This is the current face index

	// Battery level
	uint8_t batteryLevelPercent;
	BatteryState batteryState;

	MessageIAmADie() : Message(Message::MessageType_IAmADie) {}
};

/// <summary>
/// Describes a state change detection message
/// </summary>
struct MessageRollState
	: public Message
{
	uint8_t state;
	uint8_t face; // Current face index

	MessageRollState() : Message(Message::MessageType_RollState) {}
};

/// <summary>
/// Describes an acceleration readings message (for telemetry)
/// </summary>
struct MessageTelemetry
	: public Message
{
	// Accelerometer
	Core::int3 acc;
	int faceConfidenceTimes1000;
	uint32_t time;
	RollState rollState;
	uint8_t face;

	// Battery controller data
	uint8_t batteryLevelPercent;
	BatteryState batteryState;
	BatteryControllerState batteryControllerState;
	uint8_t voltageTimes50;
	uint8_t vCoilTimes50;

	// RSSI
	int8_t rssi;
	uint8_t channelIndex;

	// Temperatures
	int16_t mcuTempTimes100;
	int16_t batteryTempTimes100;

	// Battery charger low level state
	uint8_t internalChargeState;
	uint8_t forceDisableChargingState;

	// LEDs
	uint8_t ledCurrent;

	MessageTelemetry() : Message(Message::MessageType_Telemetry) {}
};

struct MessageBulkSetup
	: Message
{
	uint16_t size;

	MessageBulkSetup() : Message(Message::MessageType_BulkSetup) {}
};

struct MessageBulkData
	: Message
{
	uint8_t size;
	uint16_t offset;
	uint8_t data[MAX_DATA_SIZE];

	MessageBulkData() : Message(Message::MessageType_BulkData) {}
};

struct MessageBulkDataAck
	: Message
{
	uint16_t offset;

	MessageBulkDataAck() : Message(Message::MessageType_BulkDataAck) {}
};

struct MessageTransferProfile
	: Message
{
	uint16_t profileSize;
	uint32_t profileHash;

	MessageTransferProfile() : Message(Message::MessageType_TransferProfile) {}
};

enum TransferProfileAckType : uint8_t
{
	TransferProfileAck_Download = 0,
	TransferProfileAck_UpToDate,
	TransferProfileAck_NoMemory,
};

struct MessageTransferProfileAck
	: Message
{
	TransferProfileAckType result;

	MessageTransferProfileAck() : Message(Message::MessageType_TransferProfileAck) {}
};

enum TransferProfileFinishedType : uint8_t
{
	TransferProfileFinished_Success = 0,
	TransferProfileFinished_Error,
};

struct MessageTransferProfileFinished
	: Message
{
	TransferProfileFinishedType result;

	MessageTransferProfileFinished() : Message(Message::MessageType_TransferProfileFinished) {}
};

struct MessageTransferInstantProfile
	: Message
{
	uint16_t profileSize;
	uint32_t profileHash;

	MessageTransferInstantProfile() : Message(Message::MessageType_TransferInstantProfile) {}
};

struct MessageTransferInstantProfileAck
	: Message
{
	TransferProfileAckType result;

	MessageTransferInstantProfileAck() : Message(Message::MessageType_TransferInstantProfileAck) {}
};

struct MessageTransferInstantProfileFinished
	: Message
{
	TransferProfileFinishedType result;

	MessageTransferInstantProfileFinished() : Message(Message::MessageType_TransferInstantProfileFinished) {}
};

struct MessageDebugLog
	: public Message
{
	char text[MAX_DATA_SIZE];

	MessageDebugLog() : Message(Message::MessageType_DebugLog) {}
};

struct MessagePlayAnim
	: public Message
{
	uint8_t animation;
	uint8_t remapFace;  // Assumes that an animation was made for face 20
	uint8_t loop; 		// 1 == loop, 0 == once

	MessagePlayAnim() : Message(Message::MessageType_PlayAnim) {}
};

struct MessageRemoteAction
	: public Message
{
	// uint8_t remoteActionType;
	uint16_t actionId;

	MessageRemoteAction() : Message(Message::MessageType_RemoteAction) {}
};

struct MessageStopAnim
	: public Message
{
	uint8_t animation;
	uint8_t remapFace;  // Assumes that an animation was made for face 20

	MessageStopAnim() : Message(Message::MessageType_StopAnim) {}
};

enum TelemetryRequestMode
{
	TelemetryRequestMode_Off = 0,
	TelemetryRequestMode_Once = 1,
	TelemetryRequestMode_Repeat = 2,
};

struct MessageRequestTelemetry
	: public Message
{
	TelemetryRequestMode requestMode;
	uint16_t minInterval; // Milliseconds, 0 for no cap on rate

	MessageRequestTelemetry() : Message(Message::MessageType_RequestTelemetry) {}
};

struct MessageBlink
	: public Message
{
	uint8_t count;
	uint16_t duration;
	uint32_t color;
	uint32_t faceMask;
	uint8_t fade;
	uint8_t loop; // 1 == loop, 0 == once

	MessageBlink() : Message(Message::MessageType_Blink) {}
};

struct MessageSetAllLEDsToColor
	: public Message
{
	uint32_t color;

	MessageSetAllLEDsToColor() : Message(Message::MessageType_SetAllLEDsToColor) {}
};

struct MessageBatteryLevel
	: public Message
{
	uint8_t levelPercent;
	BatteryState state;

	MessageBatteryLevel() : Message(Message::MessageType_BatteryLevel) {}
};

struct MessageRequestRssi
	: public Message
{
	TelemetryRequestMode requestMode;
	uint16_t minInterval; // Milliseconds, 0 for no cap on rate

	MessageRequestRssi() : Message(Message::MessageType_RequestRssi) {}
};

struct MessageRssi
	: public Message
{
	int8_t rssi;

	MessageRssi() : Message(Message::MessageType_Rssi) {}
};

struct MessageLedLoopback
	: public Message
{
	uint8_t value;

	MessageLedLoopback() : Message(Message::MessageType_LedLoopback) {}
};

struct MessageSetDesignAndColor
	: public Message
{
	Config::DiceVariants::DesignAndColor designAndColor;

	MessageSetDesignAndColor() : Message(Message::MessageType_SetDesignAndColor) {}
};

struct MessageSetName
	: public Message
{
	char name[MAX_NAME_LENGTH + 1];

	MessageSetName() : Message(Message::MessageType_SetName) {}
};

struct MessageNotifyUser
	: public Message
{
	uint8_t timeout_s;
	uint8_t ok; // Boolean
	uint8_t cancel; // Boolean
	char text[MAX_DATA_SIZE - 4];
	MessageNotifyUser() : Message(Message::MessageType_NotifyUser) {
		timeout_s = 30;
		ok = 1;
		cancel = 0;
		text[0] = '\0';
	}
};

struct MessageNotifyUserAck
	: public Message
{
	uint8_t okCancel; // Boolean

	MessageNotifyUserAck() : Message(Message::MessageType_NotifyUserAck) {}
};

struct MessageSetTopLevelState
	: public Message
{
	uint8_t state; // See TopLevelState enum

	MessageSetTopLevelState() : Message(MessageType_SetTopLevelState) {}
};

struct MessageCalibrateFace
	: public Message
{
	uint8_t face;

	MessageCalibrateFace() : Message(MessageType_CalibrateFace) {}
};

struct MessagePrintNormals
	: public Message
{
	uint8_t face;

	MessagePrintNormals() : Message(MessageType_PrintNormals) {}
};

struct MessageLightUpFace
	: public Message
{
	uint8_t face; // face to light up
	uint8_t opt_remapFace; // "up" face, 0 is default (no remapping), 0xFF to use current up face
	uint32_t color;

	// For reference, the transformation is:
	// animFaceIndex
	//	-> rotatedOutsideAnimFaceIndex (based on remapFace and remapping table, i.e. what actual face should light up to "retarget" the animation around the current up face)
	//		-> ledIndex (based on pcb face to led mapping, i.e. to account for the fact that the LEDs are not accessed in the same order as the number of the faces)

	MessageLightUpFace() : Message(MessageType_LightUpFace) {}
};

struct MessageSetLEDToColor
	: public Message
{
	uint8_t ledIndex; // Starts at 0
	uint32_t color;
	MessageSetLEDToColor() : Message(Message::MessageType_SetLEDToColor) {}
};

struct MessagePlayInstantAnim
	: public Message
{
	uint8_t animation;
	uint8_t faceIndex;	// Assumes that an animation was made for face 20
	uint8_t loop; 		// 1 == loop, 0 == once

	MessagePlayInstantAnim() : Message(Message::MessageType_PlayInstantAnim) {}
};

struct MessageTemperature
	: public Message
{
	int16_t mcuTempTimes100;
	int16_t batteryTempTimes100;

	MessageTemperature() : Message(Message::MessageType_Temperature) {}
};

struct MessageDischarge
	: public Message
{
	uint8_t currentMA; // Current in mA, rounded up to nearest 10mA, or 0 to reset

	MessageDischarge() : Message(Message::MessageType_Discharge) {}
};

struct MessageBlinkId
	: public Message
{
	uint8_t brightness;
	uint8_t loop; // 1 == loop, 0 == once

	MessageBlinkId() : Message(Message::MessageType_BlinkId) {}
};

}

#pragma pack(pop)
