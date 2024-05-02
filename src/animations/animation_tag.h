#pragma once

#include <stdint.h>

namespace Animations
{
    // Animation Tag is used to indicate what triggered an animation, in case we want to
    // prioritize certain animations over others, or cancel animations, etc...
    enum AnimationTag : uint8_t
    {
        AnimationTag_Unknown = 0,
        AnimationTag_Status,
        AnimationTag_Accelerometer,
        AnimationTag_BluetoothNotification,
        AnimationTag_BatteryNotification,
        AnimationTag_BluetoothMessage,
    };
}
