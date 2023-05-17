#pragma once

#include <stdint.h>

namespace Animations
{
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