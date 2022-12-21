#pragma once

#include <stdint.h>

namespace Die
{
    enum TopLevelState
    {
        TopLevel_Unknown = 0,
        TopLevel_SoloPlay,      // Playing animations as a result of landing on faces
        TopLevel_Animator,      // LED Animator
        TopLevel_LowPower,      // Die is low on power
    };

    void init();
	void initMainLogic();
    void initDieLogic();

	uint32_t getDeviceID();
	uint32_t getBuildTimestamp();
    void update();

	TopLevelState getCurrentState();
}

