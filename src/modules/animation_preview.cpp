#include "animation_preview.h"
#include "animations/animation.h"
#include "animations/blink.h"
#include "animations/animations/animation_blinkid.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bulk_data_transfer.h"
#include "profile/profile_instant.h"
#include "anim_controller.h"
#include "accelerometer.h"
#include "utils/utils.h"
#include "malloc.h"
#include "nrf_log.h"

using namespace Bluetooth;
using namespace Profile;
using namespace Animations;

namespace Modules::AnimationPreview
{
    void PlayInstantAnimHandler(const Message *msg);
    void BlinkLEDsHandler(const Message *msg);
    void BlinkIdHandler(const Message *msg);

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_PlayInstantAnim, PlayInstantAnimHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_Blink, BlinkLEDsHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_BlinkId, BlinkIdHandler);
        NRF_LOG_DEBUG("Animation Preview init");
    }

    void PlayInstantAnimHandler(const Message *msg) {
        if (Profile::Instant::CheckValid()) {
            auto playAnimMessage = (const MessagePlayInstantAnim*)msg;
            int animIndex = playAnimMessage->animation;
            if (animIndex < Profile::Instant::getData()->getAnimationCount()) {
                NRF_LOG_DEBUG("Playing instant animation %d", animIndex);
                auto animationPreset = Profile::Instant::getData()->getAnimation(animIndex);
                AnimController::PlayAnimationParameters params;
                params.remapFace = playAnimMessage->faceIndex;
                params.loopCount = playAnimMessage->loopCount;
                params.tag = Animations::AnimationTag_BluetoothMessage;
                AnimController::play(animationPreset, params);
            } else {
                NRF_LOG_ERROR("Invalid animation index for current instant profile data");
            }
        } else {
            NRF_LOG_ERROR("Instant profile data not initialized");
        }
    }

    void BlinkLEDsHandler(const Message* msg) {
        auto *message = (const MessageBlink *)msg;
        NRF_LOG_DEBUG("Received request to blink the LEDs %d times with duration of %d ms", message->count, message->duration);

        // Create and initialize animation data
        // We keep the data in a static variable so it stays valid after this call returns
        // Note: we keep the data in a static variable so it stays valid after this call returns
        static Blink blink;
        blink.play(message->color, message->duration, message->count, message->fade, message->faceMask, message->loopCount);

        MessageService::SendMessage(Message::MessageType_BlinkAck);
    }

    void BlinkIdHandler(const Message* msg) {
        auto *message = (const MessageBlinkId *)msg;
        NRF_LOG_DEBUG("Received request to blink id with brightness=%d and loopCount=%d", message->brightness, message->loopCount);

        // Create and initialize animation data
        // Note: we keep the data in a static variable so it stays valid after this call returns
        static AnimationBlinkId blinkId;
        blinkId.type = AnimationType_BlinkID;
        blinkId.framesPerBlink = 3; // 3 animation frames per blink
        blinkId.setDuration(1000);
        blinkId.brightness = message->brightness;

        // Stop previous instance in case it was still playing
        Modules::AnimController::stop(&blinkId);

        // And play new animation
        Modules::AnimController::PlayAnimationParameters params;
        params.loopCount = message->loopCount;
        Modules::AnimController::play(&blinkId, params);

        MessageService::SendMessage(Message::MessageType_BlinkIdAck);
    }
}
