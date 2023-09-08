#pragma once

#include "stddef.h"
#include "svcs_board_config.h"

namespace Config::BoardManager
{
    void init();
    void setNTC_ID_VDD(bool set);
    const Board* getBoard();
}
