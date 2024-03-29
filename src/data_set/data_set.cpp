#include "data_set.h"
#include "data_set_data.h"
#include "utils/utils.h"
#include "drivers_nrf/flash.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/watchdog.h"
#include "drivers_nrf/power_manager.h"
#include "modules/accelerometer.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "data_animation_bits.h"
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


namespace DataSet
{
    void ReceiveDataSetHandler(const Bluetooth::Message* msg);
    void ProgramDefaultAnimSetHandler(const Message* msg);
    uint32_t computeDataSetSize();
    uint32_t computeDataSetHash();

    // The animation set always points at a specific address in memory
    Data const * data = nullptr;

    // A simple hash value of the dataset data
    uint32_t size = 0;
    uint32_t hash = 0;

    uint32_t availableDataSize() {
        return Flash::getFlashEndAddress() - Flash::getDataSetDataAddress();
    }

    uint32_t dataSize() {
        return size;
    }

    uint32_t dataHash() {
        return hash;
    }

    void init(InitCallback callback) {
        static InitCallback _callback; // Don't initialize this static inline because it would only do it on first call!
        _callback = callback;
        data = (Data const *)Flash::getDataSetAddress();

        // NRF_LOG_INFO("INITTTTTTTTTTTTTTTTTTTT");
        // auto ptr = (uint8_t *)Flash::getDataSetAddress();
        // uint32_t i0 = *(uint32_t *)(ptr + 0);
        // uint32_t i1 = *(uint32_t *)(ptr + 1);
        // uint32_t i2 = *(uint32_t *)(ptr + 2);
        // uint32_t i3 = *(uint32_t *)(ptr + 3);
        // NRF_LOG_INFO("0:%x, 1:%x, 2:%x, 3:%x", i0, i1, i2, i3);
        // uint16_t s0 = *(uint16_t *)(ptr + 0);
        // uint16_t s1 = *(uint16_t *)(ptr + 1);
        // uint16_t s2 = *(uint16_t *)(ptr + 2);
        // uint16_t s3 = *(uint16_t *)(ptr + 3);
        // NRF_LOG_INFO("0:%x, 1:%x, 2:%x, 3:%x", s0, s1, s2, s3);

        // This gets called after the animation set has been initialized
        auto finishInit = [] (bool success) {
            APP_ERROR_CHECK(success ? NRF_SUCCESS : NRF_ERROR_INTERNAL);

            size = computeDataSetSize();
            hash = computeDataSetHash();

            MessageService::RegisterMessageHandler(Message::MessageType_TransferAnimSet, ReceiveDataSetHandler);
            MessageService::RegisterMessageHandler(Message::MessageType_ProgramDefaultAnimSet, ProgramDefaultAnimSetHandler);
            NRF_LOG_INFO("DataSet init, size: 0x%x, hash: 0x%08x", size, hash);
            auto callBackCopy = _callback;
            _callback = nullptr;
            if (callBackCopy != nullptr) {
                callBackCopy();
            }
        };

        //ProgramDefaultDataSet();
        if (!CheckValid()) {
            NRF_LOG_INFO("DataSet not valid!");
            ProgramDefaultDataSet(*SettingsManager::getSettings(), finishInit);
        } else {
            finishInit(true);
        }
        //printAnimationInfo();
    }

    /// <summary>
    /// Checks whether the animation set in flash is valid or garbage data
    /// </summary>
    bool CheckValid() {
        return data->headMarker == ANIMATION_SET_VALID_KEY &&
            data->version == ANIMATION_SET_VERSION &&
            data->tailMarker == ANIMATION_SET_VALID_KEY;
    }

    const AnimationBits* getAnimationBits() {
        return &(data->animationBits);
    }

    AnimationInstance* createAnimationInstance(int animationIndex) {
        // Grab the preset data
        const Animation* preset = DataSet::getAnimation(animationIndex);
        return createAnimationInstance(preset, DataSet::getAnimationBits());
    }

    const Animation* getAnimation(int animationIndex) {
        assert(CheckValid());
        return data->animationBits.getAnimation(animationIndex);
    }

    uint16_t getAnimationCount() {
        assert(CheckValid());
        return data->animationBits.getAnimationCount();
    }

    const Condition* getCondition(int conditionIndex) {
        assert(CheckValid());
        if (conditionIndex >= 0 && (uint32_t)conditionIndex < data->conditionCount) {
            auto conditionPtr = (const uint8_t *)data->conditions + data->conditionsOffsets[conditionIndex];
            return (const Condition*)conditionPtr;
        }
        return nullptr;
    }

    uint16_t getConditionCount() {
        assert(CheckValid());
        return data->conditionCount;
    }

    const Action* getAction(int actionIndex) {
        assert(CheckValid());
        if (actionIndex >= 0 && (uint32_t)actionIndex < data->actionCount) {
            auto actionPtr = (const uint8_t*)data->actions + data->actionsOffsets[actionIndex];
            return (const Action*)actionPtr;
        }
        return nullptr;
    }

    uint16_t getActionCount() {
        assert(CheckValid());
        return data->actionCount;
    }

    const Rule* getRule(int ruleIndex) {
        assert(CheckValid());
        if (ruleIndex >= 0 && (uint32_t)ruleIndex < data->ruleCount) {
            return &data->rules[ruleIndex];
        }
        return nullptr;
    }

    uint16_t getRuleCount() {
        assert(CheckValid());
        return data->ruleCount;
    }

    // Behaviors
    const Behavior* getBehavior() {
        assert(CheckValid());
        return data->behavior;
    }

    uint8_t getBrightness() {
        assert(CheckValid());
        return data->brightness;
    }

    int offset = 0;

    void ReceiveDataSetHandler(const Message* msg) {
		NRF_LOG_DEBUG("Received request to download new animation set");
		const MessageTransferAnimSet* message = (const MessageTransferAnimSet*)msg;

        NRF_LOG_DEBUG("Animation Data to be received:");
        NRF_LOG_DEBUG("Palette: %d * %d", message->paletteSize, sizeof(uint8_t));
        NRF_LOG_DEBUG("RGB Keyframes: %d * %d", message->rgbKeyFrameCount, sizeof(RGBKeyframe));
        NRF_LOG_DEBUG("RGB Tracks: %d * %d", message->rgbTrackCount, sizeof(RGBTrack));
        NRF_LOG_DEBUG("Keyframes: %d * %d", message->keyFrameCount, sizeof(Keyframe));
        NRF_LOG_DEBUG("Tracks: %d * %d", message->trackCount, sizeof(Track));
        NRF_LOG_DEBUG("Animation Offsets: %d * %d", message->animationCount, sizeof(uint16_t));
        NRF_LOG_DEBUG("Animations: %d", message->animationSize);
        NRF_LOG_DEBUG("Conditions Offsets: %d * %d", message->conditionCount, sizeof(uint16_t));
        NRF_LOG_DEBUG("Conditions: %d", message->conditionSize);
        NRF_LOG_DEBUG("Actions Offsets: %d * %d", message->actionCount, sizeof(uint16_t));
        NRF_LOG_DEBUG("Actions: %d", message->actionSize);
        NRF_LOG_DEBUG("Rules: %d * %d", message->ruleCount, sizeof(Rule));
        NRF_LOG_DEBUG("Behavior: %d", sizeof(Behavior));

        // Store the address and size
        NRF_LOG_DEBUG("Setting up pointers");
        Data newData  __attribute__ ((aligned (4)));
        newData.headMarker = ANIMATION_SET_VALID_KEY;
        newData.version = ANIMATION_SET_VERSION;

        uint32_t address = Flash::getDataSetDataAddress();
        newData.animationBits.palette = (const uint8_t*)address;
        newData.animationBits.paletteSize = message->paletteSize;
        address += Utils::roundUpTo4(message->paletteSize * sizeof(uint8_t));

        newData.animationBits.rgbKeyframes = (const RGBKeyframe*)address;
        newData.animationBits.rgbKeyFrameCount = message->rgbKeyFrameCount;
        address += message->rgbKeyFrameCount * sizeof(RGBKeyframe);

        newData.animationBits.rgbTracks = (const RGBTrack*)address;
        newData.animationBits.rgbTrackCount = message->rgbTrackCount;
        address += message->rgbTrackCount * sizeof(RGBTrack);

        newData.animationBits.keyframes = (const Keyframe*)address;
        newData.animationBits.keyFrameCount = message->keyFrameCount;
        address += message->keyFrameCount * sizeof(Keyframe);

        newData.animationBits.tracks = (const Track*)address;
        newData.animationBits.trackCount = message->trackCount;
        address += message->trackCount * sizeof(Track);

        newData.animationBits.animationOffsets = (const uint16_t*)address;
        newData.animationBits.animationCount = message->animationCount;
        address += Utils::roundUpTo4(message->animationCount * sizeof(uint16_t)); // round to multiple of 4
        newData.animationBits.animations = (const uint8_t*)address;
        newData.animationBits.animationsSize = message->animationSize;
        address += message->animationSize;

        newData.conditionsOffsets = (const uint16_t*)address;
        newData.conditionCount = message->conditionCount;
        address += Utils::roundUpTo4(message->conditionCount * sizeof(uint16_t)); // round to multiple of 4
        newData.conditions = (const Condition*)address;
        newData.conditionsSize = message->conditionSize;
        address += message->conditionSize;

        newData.actionsOffsets = (const uint16_t*)address;
        newData.actionCount = message->actionCount;
        address += Utils::roundUpTo4(message->actionCount * sizeof(uint16_t)); // round to multiple of 4
        newData.actions = (const Action*)address;
        newData.actionsSize = message->actionSize;
        address += message->actionSize;

        newData.rules = (const Rule*)address;
        newData.ruleCount = message->ruleCount;
        address += message->ruleCount * sizeof(Rule);

        newData.behavior = (const Behavior*)address;
        address += sizeof(Behavior);

        newData.brightness = message->brightness;

        newData.tailMarker = ANIMATION_SET_VALID_KEY;

        static auto receiveToFlash = [](Flash::ProgramFlashFuncCallback callback) {
            MessageTransferAnimSetAck ack;
            ack.result = 1;
            MessageService::SendMessage(&ack);

            // Transfer data
            Bluetooth::ReceiveBulkData::receiveToFlash(Flash::getDataSetDataAddress(), nullptr, callback);
        };

        static auto onProgramFinished = [](bool result) {
            size = computeDataSetSize();
            hash = computeDataSetHash();

            //printAnimationInfo();
            NRF_LOG_INFO("Dataset size=0x%x, hash=0x%08x", size, hash);
            //NRF_LOG_INFO("Data addr: 0x%08x, data: 0x%08x", Flash::getDataSetAddress(), Flash::getDataSetDataAddress());
            MessageService::SendMessage(Message::MessageType_TransferAnimSetFinished);
        };

        if (!Flash::programFlash(newData, *SettingsManager::getSettings(), receiveToFlash, onProgramFinished)) {
            // Don't send data please
            MessageTransferAnimSetAck ack;
            ack.result = 0;
            MessageService::SendMessage(&ack);
        }
    }

    void ProgramDefaultAnimSetHandler(const Message* msg) {
        // Reprogram the default dataset
        ProgramDefaultDataSet(*SettingsManager::getSettings(), [](bool success) {
            Bluetooth::MessageService::SendMessage(Message::MessageType_ProgramDefaultAnimSetFinished);
        });

    }

    uint32_t computeDataSetDataSize(const Data* newData) {
        return
            Utils::roundUpTo4(newData->animationBits.paletteSize * sizeof(uint8_t)) +
            newData->animationBits.rgbKeyFrameCount * sizeof(RGBKeyframe) +
            newData->animationBits.rgbTrackCount * sizeof(RGBTrack) +
            newData->animationBits.keyFrameCount * sizeof(Keyframe) +
            newData->animationBits.trackCount * sizeof(Track) +
            Utils::roundUpTo4(sizeof(uint16_t) * newData->animationBits.animationCount) + // round up to multiple of 4
            newData->animationBits.animationsSize +
            Utils::roundUpTo4(sizeof(uint16_t) * newData->conditionCount) + // round up to multiple of 4
            newData->conditionsSize +
            Utils::roundUpTo4(sizeof(uint16_t) * newData->actionCount) + // round up to multiple of 4
            newData->actionsSize +
            newData->ruleCount * sizeof(Rule) +
            sizeof(Behavior);
    }


    void printAnimationInfo() {
        Timers::pause();
        NRF_LOG_DEBUG("Palette: %d * %d", data->animationBits.paletteSize, sizeof(uint8_t));
        NRF_LOG_DEBUG("RGB Keyframes: %d * %d", data->animationBits.rgbKeyFrameCount, sizeof(RGBKeyframe));
        NRF_LOG_DEBUG("RGB Tracks: %d * %d", data->animationBits.rgbTrackCount, sizeof(RGBTrack));
        NRF_LOG_DEBUG("Keyframes: %d * %d", data->animationBits.keyFrameCount, sizeof(Keyframe));
        NRF_LOG_DEBUG("Tracks: %d * %d", data->animationBits.trackCount, sizeof(Track));
        NRF_LOG_DEBUG("Animation Offsets: %d * %d", data->animationBits.animationCount, sizeof(uint16_t));
        NRF_LOG_DEBUG("Animations: %d", data->animationBits.animationsSize);
        NRF_LOG_DEBUG("Conditions Offsets: %d * %d", data->conditionCount, sizeof(uint16_t));
        NRF_LOG_DEBUG("Conditions: %d", data->conditionsSize);
        NRF_LOG_DEBUG("Actions Offsets: %d * %d", data->actionCount, sizeof(uint16_t));
        NRF_LOG_DEBUG("Actions: %d", data->actionsSize);
        NRF_LOG_DEBUG("Rules: %d * %d", data->ruleCount, sizeof(Rule));
        NRF_LOG_DEBUG("Behaviors: %d", sizeof(Behavior));
        Timers::resume();
    }

    uint32_t computeDataSetSize() {
        // Compute the size of the needed buffer to store all that data!
        return computeDataSetDataSize(data);
    }

    uint32_t computeDataSetHash() {
        return Utils::computeHash((const uint8_t*)Flash::getDataSetDataAddress(), size);
    }

}
