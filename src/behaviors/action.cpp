#include "action.h"
#include "animations/animation.h"
#include "modules/anim_controller.h"
#include "bluetooth/bluetooth_stack.h"
#include "bluetooth/bluetooth_message_service.h"
#include "nrf_log.h"

using namespace Modules;
using namespace Bluetooth;

namespace Behaviors
{
    void triggerAction(Profile::BufferDescriptor buffer, ActionPtr action, Animations::AnimationTag tag) {
        // Fetch the action from the dataset
        auto baseAction = action.get(buffer);
        switch (baseAction->type) {
            case Action_PlayAnimation:
                {
                    auto playAnimAction = static_cast<const ActionPlayAnimation*>(baseAction);
                    auto animationPreset = playAnimAction->animation.get(buffer);
                    // if faceIndex is getAnimationCount, ignore the value and get it from accelerometer, otherwise keep it
                    uint8_t faceIndex = playAnimAction->faceIndex;
                    NRF_LOG_INFO("Playing anim %d on face %d, animFaceIndex: %d", playAnimAction->animation.offset, faceIndex, playAnimAction->faceIndex);
                    // Play the animation with overrides
                    AnimController::PlayAnimationParameters params;
                    params.buffer = buffer;
                    params.overrideBuffer = buffer;
                    params.overrides = playAnimAction->overrides;
                    params.remapFace = faceIndex;
                    params.loopCount = playAnimAction->loopCount;
                    params.tag = tag;
                    AnimController::play(animationPreset, params);
                }
                break;
            case Action_RunOnDevice:
                {
                    auto runOnDeviceAction = static_cast<const ActionRunOnDevice*>(baseAction);
                    NRF_LOG_INFO("Sending message for remote action %08x", runOnDeviceAction->actionId);
                    MessageRemoteAction remoteAction;
                    // remoteAction.remoteActionType = runOnDeviceAction->remoteActionType;
                    remoteAction.actionId = runOnDeviceAction->actionId;
                    if (!MessageService::SendMessage(&remoteAction))
                    {
                        NRF_LOG_WARNING("Remote action %08x not send!", runOnDeviceAction->actionId);
                    }
                }
                break;
            default:
                NRF_LOG_ERROR("Unknown action type %d for action index %d", baseAction->type, index);
                break;
        }
    }
}
