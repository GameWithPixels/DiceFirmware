#pragma once

#include "stdint.h"
#include "profile/profile_buffer.h"
#include "animation_context.h"

namespace Animations
{
    class AnimationContext;
    struct LEDPattern;
    struct Animation;
    struct ParameterOverrides;

    struct AnimationInstance;
    struct LEDPatternInstance;

    class AnimationInstanceAllocator
    {
        // used during a call to ComputeSize and CreateInstance
        const AnimationContextGlobals* globals;
        Profile::BufferDescriptor contextBuffer;
        Profile::BufferDescriptor contextOverrideBuffer;
        Profile::Array<ParameterOverride> contextOverrides;

        AnimationContext* context;
        uint8_t* instanceBuffer;
        uint8_t* instanceCurrent;
        uint8_t instanceBufferSize;

        template<typename T> T* instanceBufferAllocate() {
            T* ret = new (instanceCurrent) T();
            instanceCurrent += sizeof(T);
            return ret;
        }

        uint16_t getLEDPatternInstanceSize(LEDPattern const * pattern) const;
        uint16_t getAnimationInstanceSize(Animation const * animation) const;

        LEDPatternInstance* CreateLEDPatternInstance(LEDPattern const * pattern);
        AnimationInstance* CreateAnimationInstance(Animation const * animation);

    public:
        AnimationInstanceAllocator(
            const AnimationContextGlobals* theGlobals,
            Profile::BufferDescriptor theBuffer,
            Profile::BufferDescriptor theOverrideBuffer,
            Profile::Array<ParameterOverride> theOverrides);

        // If no overrides, use this version
        AnimationInstanceAllocator(const AnimationContextGlobals* theGlobals, Profile::BufferDescriptor theBuffer);

        AnimationInstance* CreateInstance(Animation const* animation);
        static void DestroyInstance(AnimationInstance* instance);
    };
}
