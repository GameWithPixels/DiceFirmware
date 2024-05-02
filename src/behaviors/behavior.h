#pragma once

#include <stdint.h>
#include "profile/profile_buffer.h"
#include "condition.h"
#include "action.h"

#pragma pack(push, 1)

namespace Behaviors
{
    /// <summary>
    /// A behavior is made of rules, and this is what a rule is:
    /// a pairing of a condition and an actions. We are using indices and not pointers
    /// because this stuff is stored in flash and so we don't really know what the pointers are.
    /// </summary>
    struct Rule
    {
        Profile::Pointer<Condition> condition;
        Profile::Array<Profile::Pointer<Action>> actions;
    };

    /// <summary>
    /// A behavior is a set of condition->animation pairs, that's it!
    /// </summary>
    struct Behavior
    {
        Profile::Array<Rule> rules;
    };
}

#pragma pack(pop)
