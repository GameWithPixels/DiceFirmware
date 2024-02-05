#include "bluetooth_messages.h"

namespace Bluetooth
{
    const char* Message::GetMessageTypeString(Message::MessageType msgType)
    {
    #if defined(DEBUG)
        switch (msgType)
        {
        case MessageType_None:
            return "None";
        case MessageType_RollState:
            return "RollState";
        case MessageType_Telemetry:
            return "Telemetry";
        case MessageType_BulkSetup:
            return "BulkSetup";
        case MessageType_BulkSetupAck:
            return "BulkSetupAck";
        case MessageType_BulkData:
            return "BulkData";
        case MessageType_BulkDataAck:
            return "BulkDataAck";
        case MessageType_TransferAnimSet:
            return "TransferAnimSet";
        case MessageType_TransferAnimSetAck:
            return "TransferAnimSetAck";
        case MessageType_TransferSettings:
            return "TransferSettings";
        case MessageType_TransferSettingsAck:
            return "TransferSettingsAck";
        case MessageType_DebugLog:
            return "DebugLog";
        case MessageType_PlayAnim:
            return "PlayAnim";
        case MessageType_RequestRollState:
            return "RequestRollState";
        case MessageType_RequestAnimSet:
            return "RequestAnimSet";
        case MessageType_RequestSettings:
            return "RequestSettings";
        case MessageType_RequestTelemetry:
            return "RequestTelemetry";
        case MessageType_ProgramDefaultAnimSet:
            return "ProgramDefaultAnimSet";
        case MessageType_Blink:
            return "Blink";
        case MessageType_RequestDefaultAnimSetColor:
            return "RequestDefaultAnimSetColor";
        case MessageType_PowerOperation:
            return "PowerOperation";
        case MessageType_ExitValidation:
            return "ExitValidation";
        case MessageType_TransferInstantAnimSet:
            return "TransferInstantAnimSet";
        case MessageType_TransferInstantAnimSetAck:
            return "TransferInstantAnimSetAck";
        case MessageType_TransferInstantAnimSetFinished:
            return "TransferInstantAnimSetFinished";
        case MessageType_PlayInstantAnim:
            return "PlayInstantAnim";
        case MessageType_StopAllAnims:
            return "StopAllAnims";
        case MessageType_RequestTemperature:
            return "RequestTemperature";
        case MessageType_Temperature:
            return "Temperature";
        case MessageType_SetBatteryControllerMode:
            return "SetBatteryControllerMode";
        case MessageType_Discharge:
            return "Discharge";
        case MessageType_BlinkId:
            return "BlinkId";
        case MessageType_BlinkIdAck:
            return "BlinkIdAck";
        case MessageType_TransferTest:
            return "TransferTest";
        case MessageType_TransferTestAck:
            return "TransferTestAck";
        case MessageType_TransferTestFinished:
            return "TransferTestFinished";
        default:
            return "<missing>";
    }
    #else
        return "";
    #endif
    }
}
