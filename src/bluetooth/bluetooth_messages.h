#pragma once

#include <stdint.h>
#include "config/sdk_config.h"
#include "config/dice_variants.h"
#include "modules/accelerometer.h"
#include "config/settings.h"
#include "config/dice_variants.h"
#include "modules/battery_controller.h"
#include "core/int3.h"
#include "modules/accelerometer.h"
#include "modules/user_mode_controller.h"
#include "pixel.h"
#include "die.h"

// FW version of last settings struct change
#define SETTINGS_VERSION 0x100

// Maximum size for messages (sort of)
#define MAX_DATA_SIZE 100

#pragma pack(push, 1)

namespace Bluetooth
{

using Colorway = Config::DiceVariants::Colorway;
using DieType = Config::DiceVariants::DieType;
using BatteryState = Modules::BatteryController::BatteryState;
using BatteryControllerState = Modules::BatteryController::State;
using RollState = Modules::Accelerometer::RollState;
using BatteryControllerMode = Modules::BatteryController::ControllerOverrideMode;
using RunMode = Pixel::RunMode;
using UserMode = Modules::UserModeController::UserMode;

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
        MessageType_Unused3,                            // Unused
        MessageType_Unused4,                            // Unused
        MessageType_Unused5,                            // Unused
        MessageType_DebugLog,
        MessageType_PlayAnim,
        MessageType_PlayAnimEvent,
        MessageType_StopAnim,
        MessageType_RemoteAction,
        MessageType_RequestRollState,
        MessageType_RequestAnimSet,
        MessageType_RequestSettings,
        MessageType_RequestTelemetry,
        MessageType_ProgramDefaultAnimSet,
        MessageType_ProgramDefaultAnimSetFinished,
        MessageType_Blink,
        MessageType_BlinkAck,
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
        MessageType_StoreValue,
        MessageType_StoreValueAck,
        MessageType_Unused2,                            // Unused
        MessageType_ProgramDefaultParameters,
        MessageType_ProgramDefaultParametersFinished,
        MessageType_SetDesignAndColor,
        MessageType_SetDesignAndColorAck,
        MessageType_SetCurrentBehavior,
        MessageType_SetCurrentBehaviorAck,
        MessageType_SetName,
        MessageType_SetNameAck,
        MessageType_PowerOperation,
        MessageType_ExitValidation,
        MessageType_TransferInstantAnimSet,
        MessageType_TransferInstantAnimSetAck,
        MessageType_TransferInstantAnimSetFinished,
        MessageType_PlayInstantAnim,
        MessageType_StopAllAnims,
        MessageType_RequestTemperature,
        MessageType_Temperature,
        MessageType_SetBatteryControllerMode,
        MessageType_Unused,                             // Unused
        MessageType_Discharge,
        MessageType_BlinkId,
        MessageType_BlinkIdAck,
        MessageType_TransferTest,
        MessageType_TransferTestAck,
        MessageType_TransferTestFinished,
        MessageType_ClearSettings,
        MessageType_ClearSettingsAck,
        MessageType_SetUserMode,
        MessageType_SetUserModeAck,

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

    // Returns empty string in release builds so to save space
    static const char *GetMessageTypeString(MessageType msgType);

protected:
    Message() : type(MessageType_None) {}
};

// Supported chip models
enum ChipModel : uint8_t
{
    ChipModel_Unknown = 0,
    ChipModel_nRF52810
};

template <typename _T>
struct Chunk
{
    uint8_t chunkSize = sizeof(_T);
};

struct VersionInfo : Chunk<VersionInfo>
{
    uint16_t firmwareVersion = FIRMWARE_VERSION; // From makefile
    uint32_t buildTimestamp = BUILD_TIMESTAMP;	 // From makefile

    // Version of the settings default data and data structure
    uint16_t settingsVersion = SETTINGS_VERSION;

    // API compatibility versions
    uint16_t compatStandardApiVersion = 0x100; // WhoAreYou, IAmADie, RollState, BatteryLevel, RequestRssi, Rssi, Blink, BlinkAck
    uint16_t compatExtendedApiVersion = 0x100; // Animations (including anim classes), profile
    uint16_t compatManagementApiVersion = 0x100; // The rest
};

struct DieInfo : Chunk<DieInfo>
{
    uint32_t pixelId;  // A unique identifier
    ChipModel chipModel;
    DieType dieType;
    uint8_t ledCount;  // Number of LEDs
    Colorway colorway; // Physical look
    RunMode runMode;   // Validation or user or attract mode at the moment
};

struct CustomDesignAndColorName : Chunk<CustomDesignAndColorName>
{
    // Set only when designAndColor is custom
    char name[MAX_CUSTOM_DESIGN_COLOR_LENGTH]; // No need to make room for null terminator
};

struct DieName : Chunk<DieName>
{
    char name[MAX_NAME_LENGTH]; // No need to make room for null terminator
};

struct SettingsInfo : Chunk<SettingsInfo>
{
    uint32_t profileDataHash;
    uint32_t availableFlash;   // Amount of available flash to store data
    uint32_t totalUsableFlash; // Total amount of flash that can be used to store data
};

struct StatusInfo : Chunk<StatusInfo>
{
    // Battery info
    uint8_t batteryLevelPercent;
    BatteryState batteryState;

    // Roll info
    uint8_t rollState;
    uint8_t rollFace; // This is the current face index
};

/// <summary>
/// Identifies the dice
/// </summary>
struct MessageIAmADie
    : Message
{
#if LEGACY_IMADIE_MESSAGE
    uint8_t ledCount; // Number of LEDs
    Colorway colorway; // Physical look
    DieType dieType;
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
#else
    VersionInfo versionInfo;
    DieInfo dieInfo;
    CustomDesignAndColorName customDesignAndColorName;
    DieName dieName;
    SettingsInfo settingsInfo;
    StatusInfo statusInfo;
#endif

    MessageIAmADie() : Message(Message::MessageType_IAmADie){}
};

/// <summary>
/// Describes a state change detection message
/// </summary>
struct MessageRollState
    : Message
{
    uint8_t state;
    uint8_t face; // Current face index

    MessageRollState() : Message(Message::MessageType_RollState) {}
};

/// <summary>
/// Describes an acceleration readings message (for telemetry)
/// </summary>
struct MessageTelemetry
    : Message
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
    BatteryControllerMode batteryControllerMode;

    // LEDs
    uint8_t ledCurrent;

    // More battery states, added in FW 2025-01-03
    uint8_t vCoilMinTimes50;
    uint8_t vCoilMaxTimes50;

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

    uint8_t brightness;

    MessageTransferAnimSet() : Message(Message::MessageType_TransferAnimSet) {}
};

struct MessageTransferAnimSetAck
    : Message
{
    uint8_t result;

    MessageTransferAnimSetAck() : Message(Message::MessageType_TransferAnimSetAck) {}
};

struct MessageDebugLog
    : Message
{
    char text[MAX_DATA_SIZE];

    MessageDebugLog() : Message(Message::MessageType_DebugLog) {}
};

struct MessagePlayAnim
    : Message
{
    uint8_t animation;
    uint8_t remapFace;	// The animations are designed assuming that the higher face value is up
    uint8_t loopCount;

    MessagePlayAnim() : Message(Message::MessageType_PlayAnim) {}
};

struct MessageRemoteAction
    : Message
{
    // uint8_t remoteActionType;
    uint16_t actionId;

    MessageRemoteAction() : Message(Message::MessageType_RemoteAction) {}
};

struct MessagePlayAnimEvent
    : Message
{
    uint8_t evt;
    uint8_t remapFace;
    uint8_t loopCount;

    MessagePlayAnimEvent() : Message(Message::MessageType_PlayAnimEvent) {}
};

struct MessageStopAnim
    : Message
{
    uint8_t animation;
    uint8_t remapFace;  // Assumes that an animation was made for face 20

    MessageStopAnim() : Message(Message::MessageType_StopAnim) {}
};

enum TelemetryRequestMode : uint8_t
{
    TelemetryRequestMode_Off = 0,
    TelemetryRequestMode_Once = 1,
    TelemetryRequestMode_Repeat = 2,
};

struct MessageRequestTelemetry
    : Message
{
    TelemetryRequestMode requestMode;
    uint16_t minInterval; // Milliseconds, 0 for no cap on rate

    MessageRequestTelemetry() : Message(Message::MessageType_RequestTelemetry) {}
};

struct MessageBlink
    : Message
{
    uint8_t count;
    uint16_t duration;
    uint32_t color;
    uint32_t faceMask;
    uint8_t fade;
    uint8_t loopCount;

    MessageBlink() : Message(Message::MessageType_Blink) {}
};

struct MessageDefaultAnimSetColor
    : Message
{
    uint32_t color;

    MessageDefaultAnimSetColor() : Message(Message::MessageType_DefaultAnimSetColor) {}
};

struct MessageSetAllLEDsToColor
    : Message
{
    uint32_t color;

    MessageSetAllLEDsToColor() : Message(Message::MessageType_SetAllLEDsToColor) {}
};

struct MessageBatteryLevel
    : Message
{
    uint8_t levelPercent;
    BatteryState state;

    MessageBatteryLevel() : Message(Message::MessageType_BatteryLevel) {}
};

struct MessageRequestRssi
    : Message
{
    TelemetryRequestMode requestMode;
    uint16_t minInterval; // Milliseconds, 0 for no cap on rate

    MessageRequestRssi() : Message(Message::MessageType_RequestRssi) {}
};

struct MessageRssi
    : Message
{
    int8_t rssi;

    MessageRssi() : Message(Message::MessageType_Rssi) {}
};

struct MessageSetDesignAndColor
    : Message
{
    DieType dieType;
    Colorway colorway;

    MessageSetDesignAndColor() : Message(Message::MessageType_SetDesignAndColor) {}
};

struct MessageSetCurrentBehavior
    : Message
{
    uint8_t currentBehavior;

    MessageSetCurrentBehavior() : Message(Message::MessageType_SetCurrentBehavior) {}
};

struct MessageSetName
    : Message
{
    char name[MAX_NAME_LENGTH + 1];

    MessageSetName() : Message(Message::MessageType_SetName) {}
};

enum PowerOperation : uint8_t
{
    PowerMode_TurnOff = 0,
    PowerMode_Reset = 1,
    PowerOperation_Sleep = 2,
};

struct MessagePowerOperation
    : Message
{
    PowerOperation operation;

    MessagePowerOperation() : Message(Message::MessageType_PowerOperation) {}
};

struct MessageNotifyUser
    : Message
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
    : Message
{
    uint8_t okCancel; // Boolean

    MessageNotifyUserAck() : Message(Message::MessageType_NotifyUserAck) {}
};

struct MessageStoreValue
    : public Message
{
    uint32_t value;

    MessageStoreValue() : Message(Message::MessageType_StoreValue) {}
};

enum StoreValueResult : uint8_t
{
    StoreValueResult_Success = 0,
    StoreValueResult_UnknownError,
    StoreValueResult_StoreFull,
    StoreValueResult_InvalidRange,
    StoreValueResult_NotPermitted,
};

struct MessageStoreValueAck
    : public Message
{
    StoreValueResult result;
    uint8_t index;

    MessageStoreValueAck() : Message(Message::MessageType_StoreValueAck) {}
};

struct MessageSetUserMode
    : Message
{
    UserMode userMode; // See TopLevelState enum

    MessageSetUserMode() : Message(MessageType_SetUserMode) {}
};

struct MessageCalibrateFace
    : Message
{
    uint8_t face;

    MessageCalibrateFace() : Message(MessageType_CalibrateFace) {}
};

struct MessagePrintNormals
    : Message
{
    uint8_t face;

    MessagePrintNormals() : Message(MessageType_PrintNormals) {}
};

struct MessageLightUpFace
    : Message
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
    : Message
{
    uint8_t ledIndex; // Starts at 0
    uint32_t color;
    MessageSetLEDToColor() : Message(Message::MessageType_SetLEDToColor) {}
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

    MessageTransferInstantAnimSet() : Message(Message::MessageType_TransferInstantAnimSet) {}
};

enum TransferInstantAnimSetAckType : uint8_t
{
    TransferInstantAnimSetAck_Download = 0,
    TransferInstantAnimSetAck_UpToDate,
    TransferInstantAnimSetAck_NoMemory
};

struct MessageTransferInstantAnimSetAck
    : Message
{
    TransferInstantAnimSetAckType ackType;

    MessageTransferInstantAnimSetAck() : Message(Message::MessageType_TransferInstantAnimSetAck) {}
};

struct MessagePlayInstantAnim
    : Message
{
    uint8_t animation;
    uint8_t faceIndex;	// Assumes that an animation was made for face 20
    uint8_t loopCount;

    MessagePlayInstantAnim() : Message(Message::MessageType_PlayInstantAnim) {}
};

struct MessageTemperature
    : Message
{
    int16_t mcuTempTimes100;
    int16_t batteryTempTimes100;

    MessageTemperature() : Message(Message::MessageType_Temperature) {}
};

struct MessageSetBatteryControllerMode
    : Message
{
    BatteryControllerMode mode;

    MessageSetBatteryControllerMode() : Message(Message::MessageType_SetBatteryControllerMode) {}
};

struct MessageDischarge
    : Message
{
    uint8_t currentMA; // Current in mA, rounded up to nearest 10mA, or 0 to reset

    MessageDischarge() : Message(Message::MessageType_Discharge) {}
};

struct MessageBlinkId
    : Message
{
    uint8_t brightness;
    uint8_t loopCount;

    MessageBlinkId() : Message(Message::MessageType_BlinkId) {}
};

}

#pragma pack(pop)
