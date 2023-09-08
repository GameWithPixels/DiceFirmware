#include "bluetooth_message_service.h"
#include "bluetooth_stack.h"
#include "bluetooth_message_queue.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_log.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "drivers_nrf/scheduler.h"
#include "nrf_delay.h"

#include "drivers_nrf/watchdog.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/timers.h"

#include "core/queue.h"

#define MESSAGE_QUEUE_SIZE 160

using namespace DriversNRF;
using namespace Core;

namespace Bluetooth::MessageService
{
    void BLEObserver(ble_evt_t const * p_ble_evt, void * p_context);

    NRF_SDH_BLE_OBSERVER(GenericServiceObserver, 3, BLEObserver, nullptr);

    MessageQueue<MESSAGE_QUEUE_SIZE> SendQueue;
    MessageQueue<MESSAGE_QUEUE_SIZE> ReceiveQueue;

    uint16_t service_handle;
    ble_gatts_char_handles_t rx_handles;
    ble_gatts_char_handles_t tx_handles;

	MessageHandler messageHandlers[Message::MessageType_Count];

    Stack::SendResult send(const uint8_t* data, uint16_t size);
    bool SendMessage(Message::MessageType msgType);
    bool SendMessage(const Message* msg, int msgSize);

    void onMessageReceived(const uint8_t* data, uint16_t len);
    void update();

    void init() {
        // Clear message handle array
    	memset(messageHandlers, 0, sizeof(MessageHandler) * Message::MessageType_Count);

        ret_code_t            err_code;
        ble_uuid_t            ble_uuid;
        ble_uuid128_t         nus_base_uuid = GENERIC_DATA_SERVICE_UUID;
        ble_add_char_params_t add_char_params;

        // Add a custom base UUID.
        uint8_t uuid_type;
        err_code = sd_ble_uuid_vs_add(&nus_base_uuid, &uuid_type);
        APP_ERROR_CHECK(err_code);

        ble_uuid.type = uuid_type;
        ble_uuid.uuid = GENERIC_DATA_SERVICE_UUID_SHORT;

        // Add the service.
        err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                            &ble_uuid,
                                            &service_handle);
        APP_ERROR_CHECK(err_code);

        // Add the RX Characteristic.
        memset(&add_char_params, 0, sizeof(add_char_params));
        add_char_params.uuid                     = GENERIC_DATA_RX_CHARACTERISTIC;
        add_char_params.uuid_type                = uuid_type;
        add_char_params.max_len                  = NRF_SDH_BLE_GATT_MAX_MTU_SIZE;
        add_char_params.init_len                 = sizeof(uint8_t);
        add_char_params.is_var_len               = true;
        add_char_params.char_props.write         = 1;
        add_char_params.char_props.write_wo_resp = 1;

        add_char_params.read_access  = SEC_OPEN;
        add_char_params.write_access = SEC_OPEN;

        err_code = characteristic_add(service_handle, &add_char_params, &rx_handles);
        APP_ERROR_CHECK(err_code);

        // Add the TX Characteristic.
        memset(&add_char_params, 0, sizeof(add_char_params));
        add_char_params.uuid              = GENERIC_DATA_TX_CHARACTERISTIC;
        add_char_params.uuid_type         = uuid_type;
        add_char_params.max_len           = NRF_SDH_BLE_GATT_MAX_MTU_SIZE;
        add_char_params.init_len          = sizeof(uint8_t);
        add_char_params.is_var_len        = true;
        add_char_params.char_props.notify = 1;

        add_char_params.read_access       = SEC_OPEN;
        add_char_params.write_access      = SEC_OPEN;
        add_char_params.cccd_write_access = SEC_OPEN;

        err_code = characteristic_add(service_handle, &add_char_params, &tx_handles);
        APP_ERROR_CHECK(err_code);

        NRF_LOG_DEBUG("Message Service init");
    }

    bool isConnected() {
        return Stack::isConnected();
    }

    bool needUpdate() {
        return ReceiveQueue.count() + SendQueue.count() > 0;
    }

    void update() {
        // Process received messages if possible
        while (ReceiveQueue.tryDequeue([] (const Message* msg, uint16_t msgSize) {
            // Cast the data
            auto handler = messageHandlers[(int)msg->type];
            if (handler != nullptr) {
                NRF_LOG_DEBUG("Calling message handler %08x", handler);
                handler(msg);
            }
            return true;
        })) {
            // No body to the loop, everything happens in the condition
        }

        // Send queued messages if possible
        if (SendQueue.count() > 0) {
            if (!Stack::isConnected()) {
                NRF_LOG_INFO("Disconnected, clearing messages send queue!");
                SendQueue.clear();
            } else {
                NRF_LOG_INFO("Message queue count: %d", SendQueue.count());
                SendQueue.tryDequeue([] (const Message* msg, uint16_t msgSize) {
                    auto ret = send((const uint8_t*)msg, msgSize) != Stack::SendResult_Busy;
                    if (ret) {
                        NRF_LOG_DEBUG("Queued Message of type %d of size %d SENT (Queue=%d)", msg->type, msgSize, SendQueue.count());
                    } else {
                        NRF_LOG_DEBUG("Queued Message of type %d of size %d NOT SENT (Stack Busy) (Queue=%d)", msg->type, msgSize, SendQueue.count());
                    }
                    return ret;
                });
            }
        }
    }

    void BLEObserver(ble_evt_t const * p_ble_evt, void * p_context) {
        switch (p_ble_evt->header.evt_id)
        {
            case BLE_GATTS_EVT_WRITE:
                {
                    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
                    if (p_evt_write->handle == rx_handles.value_handle)
                    {
                        NRF_LOG_DEBUG("Generic Service Message Received: %d bytes", p_evt_write->len);
                        NRF_LOG_HEXDUMP_DEBUG(p_evt_write->data, p_evt_write->len);
                        onMessageReceived(p_evt_write->data, p_evt_write->len);
                    }
                    // Else its not meant for us
                }
                break;

            case BLE_GATTS_EVT_HVN_TX_COMPLETE:
                break;

            default:
                // No implementation needed.
                break;
        }
    }

    Stack::SendResult send(const uint8_t* data, uint16_t size) {
        NRF_LOG_DEBUG("Generic Service Message Sending: %d bytes", size);
        NRF_LOG_HEXDUMP_DEBUG(data, size);
        return Stack::send(tx_handles.value_handle, data, size);
    }

    bool SendMessage(Message::MessageType msgType) {
        Message msg(msgType);
        return SendMessage(&msg, sizeof(Message));
    }

    bool SendMessage(const Message* msg, int msgSize) {
        bool ret = false;
        auto res = send((const uint8_t*)msg, msgSize);
        switch (res) {
            case Stack::SendResult_Ok:
                NRF_LOG_HEXDUMP_DEBUG((const void*)msg, msgSize);
                NRF_LOG_DEBUG("Message of type %d of size %d SENT IMMEDIATELY (Queue=%d)", msg->type, msgSize, SendQueue.count());
                ret = true;
                break;
            case Stack::SendResult_Busy:
                {
                    // Couldn't send right away, try to schedule it for later
                    ret = SendQueue.tryEnqueue(msg, msgSize);
                    if (ret) {
                        NRF_LOG_INFO("Message of type %d of size %d QUEUED (Queue=%d)", msg->type, msgSize, SendQueue.count());
                        // update() will be called on the next frame
                    } else {
                        NRF_LOG_ERROR("Message of type %d of size %d NOT QUEUED (%s)", msg->type, msgSize, "Queue full");
                    }
                }
                break;
            case Stack::SendResult_Error:
                // Any other error, don't know what to do, forget the message
                NRF_LOG_ERROR("Message of type %d of size %d NOT QUEUED (%s)", msg->type, msgSize, "Unknown Error");
                break;
            case Stack::SendResult_NotConnected:
                // Not connected, forget the message
                NRF_LOG_ERROR("Message of type %d of size %d NOT QUEUED (%s)", msg->type, msgSize, "Not Connected");
                break;
            case Stack::SendResult_NotReady:
                // Connected but not yet ready to send messages
                NRF_LOG_ERROR("Message of type %d of size %d NOT QUEUED (%s)", msg->type, msgSize, "Not ready");
                break;
            default:
                // Unhandled result, shouldn not happen
                NRF_LOG_ERROR("Message of type %d of size %d NOT QUEUED (%s)", msg->type, msgSize, "UNEXPECTED RESULT");
            }
        return ret;
    }

    void RegisterMessageHandler(Message::MessageType msgType, MessageHandler handler) {
        if (messageHandlers[msgType] != nullptr)
        {
            NRF_LOG_WARNING("Handler for message %d already set.", msgType);
        }
        else
        {
            messageHandlers[msgType] = handler;
            NRF_LOG_DEBUG("Setting message handler for %d to %08x", msgType, handler);
        }
    }

    void UnregisterMessageHandler(Message::MessageType msgType) {
        messageHandlers[msgType] = nullptr;
    }

    void onMessageReceived(const uint8_t* data, uint16_t len) {
        if (len >= sizeof(Message)) {
            auto msg = reinterpret_cast<const Message*>(data);
            if (msg->type >= Message::MessageType_WhoAreYou && msg->type < Message::MessageType_Count) {
                if (!ReceiveQueue.tryEnqueue(msg, len)) {
                    NRF_LOG_ERROR("Message of type %d NOT HANDLED (Scheduler full)", msg->type);
                } else {
                    // update() will be called on the next frame
                }
            } else {
                NRF_LOG_ERROR("Bad message type %d", msg->type);
            }
        } else {
            NRF_LOG_ERROR("Bad message length %d", len);
        }
    }

    static NotifyUserCallback currentCallback = nullptr;
    void NotifyUser(const char* text, bool ok, bool cancel, uint8_t timeout_s, NotifyUserCallback callback) {
		MessageNotifyUser notifyMsg;
        notifyMsg.ok = ok ? 1 : 0;
        notifyMsg.cancel = cancel ? 1 : 0;
        notifyMsg.timeout_s = timeout_s;
        strncpy(notifyMsg.text, text, sizeof(notifyMsg.text) - 1);
        notifyMsg.text[sizeof(notifyMsg.text) - 1] = '\0'; // Make sure we always have a null terminated string
        if ((ok || cancel) && callback != nullptr) {

            // This timer will trigger after the timeout period and unregister the event handler
    		APP_TIMER_DEF(notifyTimeout);
            Timers::createTimer(&notifyTimeout, APP_TIMER_MODE_SINGLE_SHOT, [](void* context) {
                MessageService::UnregisterMessageHandler(Message::MessageType_NotifyUserAck);
                ((NotifyUserCallback)context)(false);
            });

			Timers::startTimer(notifyTimeout, (uint32_t)timeout_s * 1000, (void*)callback);

            currentCallback = callback;
            MessageService::RegisterMessageHandler(Message::MessageType_NotifyUserAck, [] (const Message* msg) {

                MessageService::UnregisterMessageHandler(Message::MessageType_NotifyUserAck);

                // Stop the timer since we got a message back!
                Timers::stopTimer(notifyTimeout);
                MessageNotifyUserAck* ackMsg = (MessageNotifyUserAck*)msg;
                auto ccb = currentCallback;
                currentCallback = nullptr;
                ccb(ackMsg->okCancel != 0);
            });
        }

        // Kick things off by sending the notification
		MessageService::SendMessage(&notifyMsg);
    }

#if BLE_LOG_ENABLED

    void DebugLog_0(const char* text) {
        if (isConnected()) {
            MessageDebugLog msg;
            strncpy(msg.text, text, MAX_DATA_SIZE);
            SendMessage(&msg);
        }
    }

    void DebugLog_1(const char* text, uint32_t arg0) {
        if (isConnected()) {
            MessageDebugLog msg;
            snprintf(msg.text, MAX_DATA_SIZE, text, arg0);
            SendMessage(&msg);
        }
    }

    void DebugLog_2(const char* text, uint32_t arg0, uint32_t arg1) {
        if (isConnected()) {
            MessageDebugLog msg;
            snprintf(msg.text, MAX_DATA_SIZE, text, arg0, arg1);
            SendMessage(&msg);
        }
    }

    void DebugLog_3(const char* text, uint32_t arg0, uint32_t arg1, uint32_t arg2) {
        if (isConnected()) {
            MessageDebugLog msg;
            snprintf(msg.text, MAX_DATA_SIZE, text, arg0, arg1, arg2);
            SendMessage(&msg);
        }
    }

    void DebugLog_4(const char* text, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
        if (isConnected()) {
            MessageDebugLog msg;
            snprintf(msg.text, MAX_DATA_SIZE, text, arg0, arg1, arg2, arg3);
            SendMessage(&msg);
        }
    }

    void DebugLog_5(const char* text, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
        if (isConnected()) {
            MessageDebugLog msg;
            snprintf(msg.text, MAX_DATA_SIZE, text, arg0, arg1, arg2, arg3, arg4);
            SendMessage(&msg);
        }
    }

    void DebugLog_6(const char* text, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
        if (isConnected()) {
            MessageDebugLog msg;
            snprintf(msg.text, MAX_DATA_SIZE, text, arg0, arg1, arg2, arg3, arg4, arg5);
            SendMessage(&msg);
        }
    }

#endif
}
