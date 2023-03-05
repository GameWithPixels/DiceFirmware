#pragma once

#include <stdint.h>

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

    /// <summary>
    /// Action to play an animation, really! 
    /// </summary>
    struct ActionPlayAnimation
        : Action
    {
        uint8_t animIndex;
        uint8_t faceIndex;
        uint8_t loopCount;
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

    // This method will execute the passed in action from the dataset
    void triggerActions(int actionOffset, int actionCount);
}

#pragma pack(pop)
