#include "profile_instant.h"
#include "profile_buffer.h"
#include "utils/utils.h"
#include "animations/animation.h"
#include "drivers_nrf/flash.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/watchdog.h"
#include "drivers_nrf/power_manager.h"
#include "modules/accelerometer.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bulk_data_transfer.h"
#include "profile_data.h"
#include "malloc.h"
#include "assert.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "app_error.h"

using namespace Utils;
using namespace DriversNRF;
using namespace Bluetooth;
using namespace Config;
using namespace Modules;
using namespace Animations;
using namespace Behaviors;

#define INSTANT_PROFILE_ALLOC_SIZE 200

namespace Profile::Instant
{
    // The space reserved for the instant profile
    uint8_t __attribute__ ((section (".noinit"))) instantProfileBytes[INSTANT_PROFILE_ALLOC_SIZE];

    void ReceiveInstantProfileHandler(const Message *msg);

	// This points to the profile data in flash
	Data instantProfileData;

	uint32_t getSize() {
		return instantProfileData.getSize();
	}

	uint32_t getHash() {
		return instantProfileData.getHash();
	}

	BufferDescriptor getBuffer() {
		return instantProfileData.getBuffer();
	}

	const Data* getData() {
		return &instantProfileData;
	}

	bool CheckValid() {
		return instantProfileData.checkValid();
	}

	void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_TransferInstantProfile, ReceiveInstantProfileHandler);
		NRF_LOG_INFO("Instant Profile init");
	}

	bool refreshData() {
		// Assume the profile is in flash already, and check/initialize it
		auto header = reinterpret_cast<Header const *>(instantProfileBytes);
		return instantProfileData.init(header);
	}

	void ReceiveInstantProfileHandler(const Message* msg) {
		NRF_LOG_INFO("Received request to download new profile");
		auto message = (const MessageTransferInstantProfile*)msg;

		if (message->profileHash == getHash()) {
			// Up to date
			MessageTransferInstantProfileAck ack;
			ack.result = TransferProfileAck_UpToDate;
			MessageService::SendMessage(&ack);
		} else if (message->profileSize > INSTANT_PROFILE_ALLOC_SIZE) {
            // Don't send data please
            MessageTransferInstantProfileAck ack;
            ack.result = TransferProfileAck_NoMemory;
            MessageService::SendMessage(&ack);
        } else {
            // Send Ack and receive data
            MessageTransferInstantProfileAck ackMsg;
            ackMsg.result = TransferProfileAck_Download;
            MessageService::SendMessage(&ackMsg);

            // Receive all the buffers directly to ram
            Bluetooth::ReceiveBulkData::receive(nullptr,
                [](void* context, uint16_t size) -> uint8_t* {
                    // Regardless of the size passed in, we return the pre-allocated animation data buffer
                    return (uint8_t*)instantProfileBytes;
                },
                [](void* context, bool result, uint8_t* data, uint16_t size) {
                if (result) {
                    result = refreshData();
                    MessageTransferInstantProfileFinished finMsg;
                    finMsg.result = result ? TransferProfileFinished_Success : TransferProfileFinished_Error;
                    MessageService::SendMessage(&finMsg);
                } else {
                }
            });
        }
	}

	uint32_t availableDataSize() {
		return INSTANT_PROFILE_ALLOC_SIZE;
	}

}
