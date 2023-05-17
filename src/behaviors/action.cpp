#include "action.h"
#include "data_set/data_set.h"
#include "modules/anim_controller.h"
#include "bluetooth/bluetooth_stack.h"
#include "bluetooth/bluetooth_message_service.h"
#include "nrf_log.h"

using namespace Modules;
using namespace Bluetooth;

namespace Behaviors
{
    void triggerActions(int actionOffset, int actionCount, Animations::AnimationTag tag) {
        for (int index = actionOffset; index < actionOffset + actionCount; ++index) {
            // Fetch the action from the dataset
            auto action = DataSet::getAction(index);
            switch (action->type) {
                case Action_PlayAnimation:
                    {
                        auto playAnimAction = static_cast<const ActionPlayAnimation*>(action);
                        if (playAnimAction->animIndex < DataSet::getAnimationCount()) {
                            // if faceIndex is getAnimationCount, ignore the value and get it from accelerometer, otherwise keep it
                            uint8_t faceIndex = playAnimAction->faceIndex == FACE_INDEX_CURRENT_FACE ?
                                Accelerometer::currentFace() : playAnimAction->faceIndex;
                            NRF_LOG_INFO("Playing anim %d on face %d, animFaceIndex: %d", playAnimAction->animIndex, faceIndex, playAnimAction->faceIndex);
                            auto animationPreset = DataSet::getAnimation(playAnimAction->animIndex);
                            AnimController::play(animationPreset, DataSet::getAnimationBits(), faceIndex, false, tag); // FIXME, handle remapFace and loopCount properly
                        } else {
                            NRF_LOG_ERROR("Invalid animation index %d", playAnimAction->animIndex);
                        }
                    }
                    break;
                case Action_RunOnDevice:
                    {
                        auto runOnDeviceAction = static_cast<const ActionRunOnDevice*>(action);
                        if (MessageService::canSend())
                        {
                            NRF_LOG_INFO("Sending message for remote action %08x", runOnDeviceAction->actionId);
                            MessageRemoteAction remoteAction;
                            // remoteAction.remoteActionType = runOnDeviceAction->remoteActionType;
                            remoteAction.actionId = runOnDeviceAction->actionId;
                            MessageService::SendMessage(&remoteAction);
                        }
                        else
                        {
                            NRF_LOG_INFO("(Ignored) Remote action %08x", runOnDeviceAction->actionId);
                        }
                    }
                    break;
                default:
                    NRF_LOG_ERROR("Unknown action type %d for action index %d", action->type, index);
                    break;
            }
        }
    }
}
