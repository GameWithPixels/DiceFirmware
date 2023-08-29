#pragma once

#include <stdint.h>
#include "animations/animation.h"
#include "animations/animation_parameters.h"
#include "animations/animation_tag.h"
#include "profile/profile_buffer.h"

#pragma pack(push, 1)

#define FACE_INDEX_CURRENT_FACE 0xff

namespace Behaviors
{
    /// <summary>
    /// The different types of action we support. Yes, yes, it's only one right now :)
    /// </summary>
    enum ActionType : uint8_t
    {
        Action_Unknown = 0,
        Action_PlayAnimation,
        Action_RunOnDevice
    };

    /// <summary>
    /// Base struct for Actions. Stores the actual type so that we can cast the data
    /// to the proper derived type and access the parameters.
    /// </summary>
    struct Action
    {
        ActionType type;
    };
    typedef Profile::Pointer<Action> ActionPtr;

    /// <summary>
    /// Action to play an animation, really! 
    /// </summary>
    struct ActionPlayAnimation
        : Action
    {
        uint8_t faceIndex;
        uint8_t loopCount;
        Profile::Pointer<Animations::Animation> animation;
        Profile::Array<Animations::ParameterOverride> overrides;
    };

    /// <summary>
    /// Action to be run on a connected device
    /// </summary>
    struct ActionRunOnDevice
        : Action
    {
        uint8_t remoteActionType; // Type of remote action
        uint16_t actionId;        // The id of the remote action
    };

    void triggerAction(Profile::BufferDescriptor buffer, ActionPtr action, Animations::AnimationTag tag);
}

#pragma pack(pop)
