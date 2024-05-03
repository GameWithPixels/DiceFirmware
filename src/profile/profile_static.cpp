#include "profile_static.h"
#include "profile_buffer.h"
#include "profile_data.h"
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

namespace Profile::Static
{
    void ReceiveProfileHandler(const Bluetooth::Message* msg);
    void ProgramDefaultProfile(Flash::ProgramFlashNotification callback);
    void ProgramDefaultProfileHandler(const Message* msg);

    // This points to the profile data in flash
    Data staticProfileData;

    uint32_t getSize() {
        return staticProfileData.getSize();
    }

    uint32_t getHash() {
        return staticProfileData.getHash();
    }

    BufferDescriptor getBuffer() {
        return staticProfileData.getBuffer();
    }

    const Data* getData() {
        return &staticProfileData;
    }

    bool CheckValid() {
        return staticProfileData.checkValid();
    }

    void init(InitCallback callback) {
        static InitCallback _callback; // Don't initialize this static inline because it would only do it on first call!
        _callback = callback;

        // This gets called after the animation set has been initialized
        static auto finishInit = [] (bool success) {
            APP_ERROR_CHECK(success ? NRF_SUCCESS : NRF_ERROR_INTERNAL);

            MessageService::RegisterMessageHandler(Message::MessageType_TransferProfile, ReceiveProfileHandler);
            MessageService::RegisterMessageHandler(Message::MessageType_ProgramDefaultProfile, ProgramDefaultProfileHandler);
            NRF_LOG_INFO("Profile init, size: %d, hash: 0x%08x", getSize(), getHash());

            auto callBackCopy = _callback;
            _callback = nullptr;
            if (callBackCopy != nullptr) {
                callBackCopy();
            }
        };

        if (refreshData()) {
            finishInit(true);
        } else {
            NRF_LOG_INFO("Profile not valid, programming default");
            ProgramDefaultProfile([](bool result) {
                finishInit(result);
            });
        }
    }

    bool refreshData() {
        // Assume the profile is in flash already, and check/initialize it
        uint32_t headerAddress = Flash::getProfileAddress();
        auto header = reinterpret_cast<Header const *>(headerAddress);
        return staticProfileData.init(header);
    }

    void ReceiveProfileHandler(const Message* msg) {
        NRF_LOG_INFO("Received request to download new profile");
        const MessageTransferProfile* message = (const MessageTransferProfile*)msg;

        if (message->mode != MessageTransferProfileMode_Persistent) {
            return;
        }

        if (message->hash == getHash()) {
            // Up to date
            MessageTransferProfileAck ack;
            ack.result = TransferProfileAck_UpToDate;
            MessageService::SendMessage(&ack);
        } else {
            // Store the address and size
            static auto receiveToFlash = [](Flash::ProgramFlashFuncCallback callback) {
                MessageTransferProfileAck ack;
                ack.result = TransferProfileAck_Download;
                MessageService::SendMessage(&ack);

                // Transfer data
                Bluetooth::ReceiveBulkData::receiveToFlash(Flash::getProfileAddress(), nullptr, callback);
            };

            static auto onProgramFinished = [](bool result) {
                if (result) {
                    result = refreshData();
                    if (result) {
                        NRF_LOG_DEBUG("Dataset size=%d, hash=0x%08x", getSize(), getHash());
                    } else {
                        NRF_LOG_ERROR("Error after programming dataset: size=%d, hash=0x%08x", getSize(), getHash());
                    }
                    MessageTransferProfileFinished finMsg;
                    finMsg.result = result ? TransferProfileFinished_Success : TransferProfileFinished_Error;
                    MessageService::SendMessage(&finMsg);
                }
            };

            if (!Flash::programProfile(message->dataSize, receiveToFlash, onProgramFinished)) {
                // Don't send data please
                MessageTransferProfileAck ack;
                ack.result = TransferProfileAck_NoMemory;
                MessageService::SendMessage(&ack);
            }
        }
    }

    void ProgramDefaultProfile(Flash::ProgramFlashNotification callback) {
        // Reprogram the default dataset
        static Flash::ProgramFlashNotification _callback;
        static uint32_t defaultProfileSize = 0;
        static uint8_t* defaultProfile = nullptr;

        static auto programProfile = [](Flash::ProgramFlashFuncCallback funcCallback) {
            // Write the profile
            Flash::write(nullptr, Flash::getProfileAddress(), defaultProfile, defaultProfileSize, funcCallback);
        };

        _callback = callback;
        defaultProfile = Data::createDefaultProfile(&defaultProfileSize);
        Flash::programProfile(defaultProfileSize, programProfile, [](bool success) {
            Data::destroyDefaultProfile(defaultProfile);
            defaultProfile = nullptr;
            defaultProfileSize = 0;
            if (success) {
                success = refreshData();
            }
            _callback(success);
        });
    }

    void ProgramDefaultProfileHandler(const Message* msg) {
        static auto thisCallback = [](bool result) {
            Bluetooth::MessageService::SendMessage(Message::MessageType_ProgramDefaultProfileFinished);
        };

        ProgramDefaultProfile(thisCallback);
    }

    uint32_t availableDataSize() {
        return Flash::getUsableBytes() - sizeof(Config::Settings);
    }
}
