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
		case MessageType_Sleep:
			return "Sleep";
        case MessageType_ExitValidation:
			return "ExitValidation";
		default:
			return "<missing>";
		}
	#else
		return "";
	#endif
	}

    uint16_t Message::GetMessageSize(MessageType msgType) {
        switch (msgType) {
		case MessageType_IAmADie:
            return sizeof(MessageIAmADie);
		case MessageType_RollState:
            return sizeof(MessageRollState);
		case MessageType_BulkSetup:
            return sizeof(MessageBulkSetup);
		case MessageType_BulkData:
            return sizeof(MessageBulkData);
		case MessageType_BulkDataAck:
            return sizeof(MessageBulkDataAck);
		case MessageType_TransferAnimSet:
            return sizeof(MessageTransferAnimSet);
		case MessageType_TransferAnimSetAck:
            return sizeof(MessageTransferAnimSetAck);
		case MessageType_TransferTestAnimSet:
            return sizeof(MessageTransferTestAnimSet);
		case MessageType_TransferTestAnimSetAck:
            return sizeof(MessageTransferTestAnimSetAck);
		case MessageType_DebugLog:
            return sizeof(MessageDebugLog);
		case MessageType_PlayAnim:
            return sizeof(MessagePlayAnim);
		case MessageType_PlayAnimEvent:
            return sizeof(MessagePlayAnimEvent);
		case MessageType_StopAnim:
            return sizeof(MessageStopAnim);
		case MessageType_PlaySound:
            return sizeof(MessagePlaySound);
		case MessageType_RequestTelemetry:
            return sizeof(MessageRequestTelemetry);
		case MessageType_ProgramDefaultAnimSet:
            return sizeof(MessageProgramDefaultAnimSet);
		case MessageType_Blink:
            return sizeof(MessageBlink);
		case MessageType_DefaultAnimSetColor:
            return sizeof(MessageDefaultAnimSetColor);
		case MessageType_BatteryLevel:
            return sizeof(MessageBatteryLevel);
		case MessageType_Rssi:
            return sizeof(MessageRssi);
		case MessageType_CalibrateFace:
            return sizeof(MessageCalibrateFace);
		case MessageType_NotifyUser:
            return sizeof(MessageNotifyUser);
		case MessageType_NotifyUserAck:
            return sizeof(MessageNotifyUserAck);
		case MessageType_SetDesignAndColor:
            return sizeof(MessageSetDesignAndColor);
		case MessageType_SetCurrentBehavior:
            return sizeof(MessageSetCurrentBehavior);
		case MessageType_SetName:
            return sizeof(MessageSetName);
		case MessageType_Sleep:
            return sizeof(MessageSleep);
		case MessageType_ExitValidation:
            return sizeof(MessageExitValidation);
		case MessageType_TransferInstantAnimSet:
            return sizeof(MessageTransferInstantAnimSet);
		case MessageType_TransferInstantAnimSetAck:
            return sizeof(MessageTransferInstantAnimSetAck);
		case MessageType_PlayInstantAnim:
            return sizeof(MessagePlayInstantAnim);
		case MessageType_SetAllLEDsToColor:
            return sizeof(MessageSetAllLEDsToColor);
		case MessageType_PrintNormals:
            return sizeof(MessagePrintNormals);
		case MessageType_LightUpFace:
            return sizeof(MessageLightUpFace);
		case MessageType_SetLEDToColor:
            return sizeof(MessageSetLEDToColor);
        default:
            return sizeof(Message);
        }
    }
}
