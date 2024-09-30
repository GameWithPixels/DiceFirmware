#pragma once
#include "bluetooth_messages.h"

#ifndef BLE_LOG_ENABLED
#define BLE_LOG_ENABLED 0
#endif

namespace Bluetooth::MessageService
{
    // Base UUID: a6b9xxxx-7a5a-43f2-a962-350c8edc9b5b
    #define GENERIC_DATA_SERVICE_UUID {{ 0x5B, 0x9B, 0xDC, 0x8E, 0x0C, 0x35, 0x62, 0xA9, 0xF2, 0x43, 0x5A, 0x7A, 0x00, 0x00, 0xB9, 0xA6 }};
    #define GENERIC_DATA_SERVICE_UUID_SHORT 0x0001
    #define GENERIC_DATA_TX_CHARACTERISTIC 0x0002
    #define GENERIC_DATA_RX_CHARACTERISTIC 0x0003

    void init();
    bool isConnected();

    bool needUpdate();
    void update();

    bool SendMessage(Message::MessageType msgType);
    bool SendMessage(const Message* msg, int msgSize);

    template <typename Msg>
    bool SendMessage(const Msg* msg) {
        return SendMessage(msg, sizeof(Msg));
    }

    // Our bluetooth message handlers
    typedef void (*MessageHandler)(const Message* message);

    void RegisterMessageHandler(Message::MessageType msgType, MessageHandler handler);
    void UnregisterMessageHandler(Message::MessageType msgType);

    typedef void (*NotifyUserCallback)(bool result);
    void NotifyUser(const char* text, bool ok, bool cancel, uint8_t timeout_s, NotifyUserCallback callback);

#if BLE_LOG_ENABLED
    void DebugLog_0(const char* text);
    void DebugLog_1(const char* text, uint32_t arg0);
    void DebugLog_2(const char* text, uint32_t arg0, uint32_t arg1);
    void DebugLog_3(const char* text, uint32_t arg0, uint32_t arg1, uint32_t arg2);
    void DebugLog_4(const char* text, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3);
    void DebugLog_5(const char* text, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);
    void DebugLog_6(const char* text, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);

    #define BLE_LOG_INFO(...) BLE_LOG_INTERNAL( __VA_ARGS__)
    #define BLE_LOG_INTERNAL_X(N, ...) CONCAT_2(BLE_LOG_INTERNAL_, N) (__VA_ARGS__)
    #define BLE_LOG_INTERNAL_0(str) Bluetooth::MessageService::DebugLog_0(str)
    #define BLE_LOG_INTERNAL_1(str, arg0) Bluetooth::MessageService::DebugLog_1(str, (uint32_t)(arg0))
    #define BLE_LOG_INTERNAL_2(str, arg0, arg1) Bluetooth::MessageService::DebugLog_2(str, (uint32_t)(arg0), (uint32_t)(arg1))
    #define BLE_LOG_INTERNAL_3(str, arg0, arg1, arg2) Bluetooth::MessageService::DebugLog_3(str, (uint32_t)(arg0), (uint32_t)(arg1), (uint32_t)(arg2))
    #define BLE_LOG_INTERNAL_4(str, arg0, arg1, arg2, arg3) Bluetooth::MessageService::DebugLog_4(str, (uint32_t)(arg0), (uint32_t)(arg1), (uint32_t)(arg2), (uint32_t)(arg3))
    #define BLE_LOG_INTERNAL_5(str, arg0, arg1, arg2, arg3, arg4) Bluetooth::MessageService::DebugLog_5(str, (uint32_t)(arg0), (uint32_t)(arg1), (uint32_t)(arg2), (uint32_t)(arg3), (uint32_t)(arg4))
    #define BLE_LOG_INTERNAL_6(str, arg0, arg1, arg2, arg3, arg4, arg5) Bluetooth::MessageService::DebugLog_6(str, (uint32_t)(arg0), (uint32_t)(arg1), (uint32_t)(arg2), (uint32_t)(arg3), (uint32_t)(arg4), (uint32_t)(arg5))

    #define BLE_LOG_INTERNAL(...) BLE_LOG_INTERNAL_X(NUM_VA_ARGS_LESS_1(__VA_ARGS__), __VA_ARGS__)
#else
    #define BLE_LOG_INFO(...) ;
#endif
}
