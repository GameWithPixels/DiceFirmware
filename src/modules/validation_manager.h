#pragma once

#include "stdint.h"
#include "animations/Animation.h"

namespace Animations
{
    struct Animation;
}

namespace Modules
{
    /// <summary>
    /// Manages validation structures and functions including
    /// a name blinking animation and the function that plays
    /// this animation after the system boots.
    /// </summary>
    namespace ValidationManager
    {
        void init();
        void onDiceInitialized();

        void leaveValidation();
        bool inValidation();
    }
}
