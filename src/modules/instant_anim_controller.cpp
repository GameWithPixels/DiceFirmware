#include "instant_anim_controller.h"
#include "animations/animation.h"
#include "data_set/data_animation_bits.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bulk_data_transfer.h"
#include "anim_controller.h"
#include "behaviors/action.h"
#include "accelerometer.h"
#include "utils/utils.h"
#include "malloc.h"
#include "nrf_log.h"

using namespace Bluetooth;
using namespace DataSet;

namespace Modules::InstantAnimationController
{
    static AnimationBits animationBits;
    static void *animationsData;
    static uint32_t animationsDataHash;

    void ReceiveInstantAnimSetHandler(const Message *msg);
    void PlayInstantAnimHandler(const Message *msg);

    void clearData() {
        free(animationsData);
        animationsData = nullptr;
        animationsDataHash = 0;
    }

    void init()
    {
        MessageService::RegisterMessageHandler(Message::MessageType_TransferInstantAnimSet, ReceiveInstantAnimSetHandler);
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_PlayInstantAnim, PlayInstantAnimHandler);
        animationsData = nullptr;
        clearData();

        NRF_LOG_DEBUG("Instant Animation Controller init");
    }

    void ReceiveInstantAnimSetHandler(const Message *msg)
    {
        NRF_LOG_INFO("Received request to download instant animation");
        const MessageTransferInstantAnimSet *message = (const MessageTransferInstantAnimSet *)msg;

        if (animationsData == nullptr || animationsDataHash != message->hash) {
            // Stop playing animations as we are about to delete their data
            for (uint32_t i = 0; i < animationBits.animationCount; ++i) {
                AnimController::stop(animationBits.getAnimation(i), 255);
            }

            // We should download the data
            clearData();

            NRF_LOG_DEBUG("Animations Data to be received:");
            NRF_LOG_DEBUG("Palette: %d * %d", message->paletteSize, sizeof(uint8_t));
            NRF_LOG_DEBUG("RGB Keyframes: %d * %d", message->rgbKeyFrameCount, sizeof(RGBKeyframe));
            NRF_LOG_DEBUG("RGB Tracks: %d * %d", message->rgbTrackCount, sizeof(RGBTrack));
            NRF_LOG_DEBUG("Keyframes: %d * %d", message->keyFrameCount, sizeof(Keyframe));
            NRF_LOG_DEBUG("Tracks: %d * %d", message->trackCount, sizeof(Track));
            NRF_LOG_DEBUG("Animations: %d", message->animationCount);
            NRF_LOG_DEBUG("Hash: 0x%04x", message->hash);

            int paletteBufferSize = Utils::roundUpTo4(message->paletteSize);
            int animationOffsetsBufferSize = Utils::roundUpTo4(message->animationCount * 2);

            int bufferSize =
                paletteBufferSize +
                message->rgbKeyFrameCount * sizeof(RGBKeyframe) +
                message->rgbTrackCount * sizeof(RGBTrack) +
                message->keyFrameCount * sizeof(Keyframe) +
                message->trackCount * sizeof(Track) +
                animationOffsetsBufferSize +
                message->animationSize;

            // Allocate anim data
            animationsData = malloc(bufferSize);
            if (animationsData != nullptr) {

                // Setup pointers
                NRF_LOG_DEBUG("Animations bufferSize: %d", bufferSize);
                uint32_t address = (uint32_t)animationsData;
                animationBits.palette = (const uint8_t*)address;
                animationBits.paletteSize = message->paletteSize;
                address += paletteBufferSize;

                animationBits.rgbKeyframes = (const RGBKeyframe*)address;
                animationBits.rgbKeyFrameCount = message->rgbKeyFrameCount;
                address += message->rgbKeyFrameCount * sizeof(RGBKeyframe);

                animationBits.rgbTracks = (const RGBTrack*)address;
                animationBits.rgbTrackCount = message->rgbTrackCount;
                address += message->rgbTrackCount * sizeof(RGBTrack);

                animationBits.keyframes = (const Keyframe*)address;
                animationBits.keyFrameCount = message->keyFrameCount;
                address += message->keyFrameCount * sizeof(Keyframe);

                animationBits.tracks = (const Track*)address;
                animationBits.trackCount = message->trackCount;
                address += message->trackCount * sizeof(Track);

                animationBits.animationOffsets = (const uint16_t*)address;
                animationBits.animationCount = message->animationCount;
                address += animationOffsetsBufferSize;

                animationBits.animations = (const uint8_t*)address;
                animationBits.animationsSize = message->animationSize;

                // Send Ack and receive data
                MessageTransferInstantAnimSetAck ackMsg;
                ackMsg.ackType = TransferInstantAnimSetAck_Download;
                MessageService::SendMessage(&ackMsg);

                // Receive all the buffers directly to flash
                ReceiveBulkData::receive(nullptr,
                    [](void* context, uint16_t size) -> uint8_t* {
                        // Regardless of the size passed in, we return the pre-allocated animation data buffer
                        return (uint8_t*)animationsData;
                    },
                    [](void* context, bool result, uint8_t* data, uint16_t size) {
                    if (result) {
                        animationsDataHash = Utils::computeHash((uint8_t*)animationsData, size);
                        MessageService::SendMessage(Message::MessageType_TransferInstantAnimSetFinished);
                    }
                    else {
                        NRF_LOG_ERROR("Failed to download instant animation");
                        clearData();
                    }
                });
            }
            else {
                // No memory
                MessageTransferInstantAnimSetAck ackMsg;
                ackMsg.ackType = TransferInstantAnimSetAck_NoMemory;
                MessageService::SendMessage(&ackMsg);
            }
        }
        else {
            // The animation data is valid and matches the app data
            MessageTransferInstantAnimSetAck ackMsg;
            ackMsg.ackType = TransferInstantAnimSetAck_UpToDate;
            MessageService::SendMessage(&ackMsg);
        }
    }

    void PlayInstantAnimHandler(const Message *msg) 
    {
        const MessagePlayInstantAnim *message = (const MessagePlayInstantAnim *)msg;
        NRF_LOG_INFO("Received request to play instant animation %d", message->animation);

        if (animationsData != nullptr && message->animation < animationBits.getAnimationCount()) {
            // TODO may crash if we are still downloading an animation
            auto animation = animationBits.getAnimation(message->animation);
            uint8_t faceIndex = message->faceIndex == FACE_INDEX_CURRENT_FACE
                ? Accelerometer::currentFace() : message->faceIndex;
            AnimController::play(animation, &animationBits, faceIndex, message->loopCount);
        }
        else if (animationsData == nullptr) {
            NRF_LOG_DEBUG("No instant animation in memory");
        }
        else {
            NRF_LOG_DEBUG("Animation index out of bounds %d >= %d", message->animation, animationBits.getAnimationCount());
        }
    }
}
