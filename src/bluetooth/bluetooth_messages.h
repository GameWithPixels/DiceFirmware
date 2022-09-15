#pragma once

#include <stdint.h>
#include "config/sdk_config.h"
#include "config/dice_variants.h"
#include "modules/accelerometer.h"
#include "config/settings.h"

#define MAX_DATA_SIZE 100

#pragma pack(push, 1)

namespace Bluetooth
{
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
		MessageType_TransferAnimSet,
		MessageType_TransferAnimSetAck,
		MessageType_TransferAnimSetFinished,
		MessageType_TransferSettings,
		MessageType_TransferSettingsAck,
		MessageType_TransferSettingsFinished,
		MessageType_TransferTestAnimSet,
		MessageType_TransferTestAnimSetAck,
		MessageType_TransferTestAnimSetFinished,
		MessageType_DebugLog,
		MessageType_PlayAnim,
		MessageType_PlayAnimEvent,
		MessageType_StopAnim,
		MessageType_PlaySound,
		MessageType_RequestRollState,
		MessageType_RequestAnimSet,
		MessageType_RequestSettings,
		MessageType_RequestTelemetry,
		MessageType_ProgramDefaultAnimSet,
		MessageType_ProgramDefaultAnimSetFinished,
		MessageType_Blink,
		MessageType_BlinkFinished,
		MessageType_RequestDefaultAnimSetColor,
		MessageType_DefaultAnimSetColor,
		MessageType_RequestBatteryLevel,
		MessageType_BatteryLevel,
		MessageType_RequestRssi,
		MessageType_Rssi,
		MessageType_Calibrate,
		MessageType_CalibrateFace,
		MessageType_NotifyUser,
		MessageType_NotifyUserAck,
		MessageType_TestHardware,
		MessageType_SetTopLevelState,
		MessageType_Unused1,
		MessageType_Unused2,
		MessageType_ProgramDefaultParameters,
		MessageType_ProgramDefaultParametersFinished,
		MessageType_SetDesignAndColor,
		MessageType_SetDesignAndColorAck,
		MessageType_SetCurrentBehavior,
		MessageType_SetCurrentBehaviorAck,
		MessageType_SetName,
		MessageType_SetNameAck,
		MessageType_Sleep,
		MessageType_ExitValidation,
		MessageType_TransferInstantAnimSet,
		MessageType_TransferInstantAnimSetAck,
		MessageType_TransferInstantAnimSetFinished,
		MessageType_PlayInstantAnim,
		MessageType_StopAllAnims,

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

	inline Message(MessageType msgType) : type(msgType) {}

	// Returns empty string in release builds so to save space
	static const char *GetMessageTypeString(MessageType msgType);

protected:
	inline Message() : type(MessageType_None) {}
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
	inline MessageIAmADie() : Message(Message::MessageType_IAmADie) {}
};

/// <summary>
/// Describes a state change detection message
/// </summary>
struct MessageRollState
	: public Message
{
	uint8_t state;
	uint8_t face;

	inline MessageRollState() : Message(Message::MessageType_RollState) {}
};

/// <summary>
/// Describes an acceleration readings message (for telemetry)
/// </summary>
struct MessageTelemetry
	: public Message
{
	Modules::Accelerometer::AccelFrame accelFrame;

	inline MessageTelemetry() : Message(Message::MessageType_Telemetry) {}
};

struct MessageBulkSetup
	: Message
{
	uint16_t size;

	inline MessageBulkSetup() : Message(Message::MessageType_BulkSetup) {}
};

struct MessageBulkData
	: Message
{
	uint8_t size;
	uint16_t offset;
	uint8_t data[MAX_DATA_SIZE];

	inline MessageBulkData() : Message(Message::MessageType_BulkData) {}
};

struct MessageBulkDataAck
	: Message
{
	uint16_t offset;
	inline MessageBulkDataAck() : Message(Message::MessageType_BulkDataAck) {}
};

struct MessageTransferAnimSet
	: Message
{
	uint16_t paletteSize;
	uint16_t rgbKeyFrameCount;
	uint16_t rgbTrackCount;
	uint16_t keyFrameCount;
	uint16_t trackCount;

	uint16_t animationCount;
	uint16_t animationSize;

	uint16_t conditionCount;
	uint16_t conditionSize;
	uint16_t actionCount;
	uint16_t actionSize;
	uint16_t ruleCount;

	inline MessageTransferAnimSet() : Message(Message::MessageType_TransferAnimSet) {}
};

struct MessageTransferAnimSetAck
	: Message
{
	uint8_t result;
	inline MessageTransferAnimSetAck() : Message(Message::MessageType_TransferAnimSetAck) {}
};

struct MessageTransferTestAnimSet
	: Message
{
	uint16_t paletteSize;
	uint16_t rgbKeyFrameCount;
	uint16_t rgbTrackCount;
	uint16_t keyFrameCount;
	uint16_t trackCount;

	uint16_t animationSize;

	uint32_t hash;

	inline MessageTransferTestAnimSet() : Message(Message::MessageType_TransferTestAnimSet) {}
};

enum TransferInstantAnimSetAckType : uint8_t
{
	TransferInstantAnimSetAck_Download = 0,
	TransferInstantAnimSetAck_UpToDate,
	TransferInstantAnimSetAck_NoMemory
};

struct MessageTransferTestAnimSetAck
	: Message
{
	TransferInstantAnimSetAckType ackType;
	inline MessageTransferTestAnimSetAck() : Message(Message::MessageType_TransferTestAnimSetAck) {}
};

struct MessageDebugLog
	: public Message
{
	char text[MAX_DATA_SIZE];

	inline MessageDebugLog() : Message(Message::MessageType_DebugLog) {}
};

struct MessagePlayAnim
	: public Message
{
	uint8_t animation;
	uint8_t remapFace;  // Assumes that an animation was made for face 20
	uint8_t loop; 		// 1 == loop, 0 == once

	inline MessagePlayAnim() : Message(Message::MessageType_PlayAnim) {}
};

struct MessagePlaySound
	: public Message
{
	uint16_t clipId;

	inline MessagePlaySound() : Message(Message::MessageType_PlaySound) {}
};

struct MessagePlayAnimEvent
	: public Message
{
	uint8_t evt;
	uint8_t remapFace;
	uint8_t loop;

	inline MessagePlayAnimEvent() : Message(Message::MessageType_PlayAnimEvent) {}
};

struct MessageStopAnim
	: public Message
{
	uint8_t animation;
	uint8_t remapFace;  // Assumes that an animation was made for face 20

	inline MessageStopAnim() : Message(Message::MessageType_StopAnim) {}
};

struct MessageRequestTelemetry
	: public Message
{
	uint8_t activate; // Boolean

	inline MessageRequestTelemetry() : Message(Message::MessageType_RequestTelemetry) {}
};

struct MessageProgramDefaultAnimSet
	: public Message
{
	uint32_t color;

	inline MessageProgramDefaultAnimSet() : Message(Message::MessageType_ProgramDefaultAnimSet) {}
};

struct MessageBlink
	: public Message
{
	uint8_t flashCount;
	uint16_t duration;
	uint32_t color;
	uint8_t fade;

	inline MessageBlink() : Message(Message::MessageType_Blink) {}
};

struct MessageDefaultAnimSetColor
	: public Message
{
	uint32_t color;
	inline MessageDefaultAnimSetColor() : Message(Message::MessageType_DefaultAnimSetColor) {}
};

struct MessageSetAllLEDsToColor
	: public Message
{
	uint32_t color;
	inline MessageSetAllLEDsToColor() : Message(Message::MessageType_SetAllLEDsToColor) {}
};

struct MessageBatteryLevel
	: public Message
{
	float level;
	float voltage;
	uint8_t charging;
	inline MessageBatteryLevel() : Message(Message::MessageType_BatteryLevel) {}
};

struct MessageRssi
	: public Message
{
	int8_t rssi;
	uint8_t channelIndex;
	inline MessageRssi() : Message(Message::MessageType_Rssi) {}
};

struct MessageSetDesignAndColor
	: public Message
{
	Config::DiceVariants::DesignAndColor designAndColor;
	inline MessageSetDesignAndColor() : Message(Message::MessageType_SetDesignAndColor) {}
};

struct MessageSetCurrentBehavior
	: public Message
{
	uint8_t currentBehavior;
	inline MessageSetCurrentBehavior() : Message(Message::MessageType_SetCurrentBehavior) {}
};

struct MessageSetName
	: public Message
{
	char name[MAX_NAME_LENGTH + 1];
	inline MessageSetName() : Message(Message::MessageType_SetName) {}
};

struct MessageNotifyUser
	: public Message
{
	uint8_t timeout_s;
	uint8_t ok; // Boolean
	uint8_t cancel; // Boolean
	char text[MAX_DATA_SIZE - 4];
	inline MessageNotifyUser() : Message(Message::MessageType_NotifyUser) {
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
	inline MessageNotifyUserAck() : Message(Message::MessageType_NotifyUserAck) {}
};

struct MessageSetTopLevelState
	: public Message
{
	uint8_t state; // See TopLevelState enum
	inline MessageSetTopLevelState() : Message(MessageType_SetTopLevelState) {}
};

struct MessageCalibrateFace
	: public Message
{
	uint8_t face;
	inline MessageCalibrateFace() : Message(MessageType_CalibrateFace) {}
};

struct MessagePrintNormals
	: public Message
{
	uint8_t face;
	inline MessagePrintNormals() : Message(MessageType_PrintNormals) {}
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

	inline MessageLightUpFace() : Message(MessageType_LightUpFace) {}
};

struct MessageSetLEDToColor
	: public Message
{
	uint8_t ledIndex; // Starts at 0
	uint32_t color;
	inline MessageSetLEDToColor() : Message(Message::MessageType_SetLEDToColor) {}
};

struct MessageSleep
	: public Message
{
	inline MessageSleep() : Message(Message::MessageType_Sleep) {}
};

struct MessageExitValidation
	: public Message
{
	inline MessageExitValidation() : Message(Message::MessageType_ExitValidation) {}
};

struct MessageTransferInstantAnimSet
	: Message
{
	uint16_t paletteSize;
	uint16_t rgbKeyFrameCount;
	uint16_t rgbTrackCount;
	uint16_t keyFrameCount;
	uint16_t trackCount;

	uint16_t animationCount;
	uint16_t animationSize;

	uint32_t hash;

	inline MessageTransferInstantAnimSet() : Message(Message::MessageType_TransferInstantAnimSet) {}
};

struct MessageTransferInstantAnimSetAck
	: Message
{
	TransferInstantAnimSetAckType ackType;
	inline MessageTransferInstantAnimSetAck() : Message(Message::MessageType_TransferInstantAnimSetAck) {}
};

struct MessagePlayInstantAnim
	: public Message
{
	uint8_t animation;
	uint8_t faceIndex;	// Assumes that an animation was made for face 20
	uint8_t loop; 		// 1 == loop, 0 == once

	inline MessagePlayInstantAnim() : Message(Message::MessageType_PlayInstantAnim) {}
};

struct MessageStopAllAnims
	: public Message
{
	inline MessageStopAllAnims() : Message(Message::MessageType_StopAllAnims) {}
};

}

#pragma pack(pop)
