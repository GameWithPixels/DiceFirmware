#pragma once

#include "stdint.h"
#include "profile/profile_buffer.h"
#include "animation_parameters.h"
#include "animation_context_globals.h"

#pragma pack(push, 1)

namespace Animations
{
    // Defines a parameter override used during animation execution
    struct ParameterOverride
    {
        uint16_t index;          // Index in static buffer
        uint16_t overrideIndex;  // Index in override buffer
    };

    // Encompasses all information needed by an animation instance to compute led color values
    // This includes the animation buffer used to store colors and gradients pointed to by the
    // animation preset, as well as overrides to those values if applicable, and global values
    // such as current face index or battery level.
    struct AnimationContext
    {
        // Globals, such as current face, accelerometer, etc..
        const AnimationContextGlobals* globals;

        // Typically the profile programmed in flash memory
        Profile::BufferDescriptor buffer;

        // Typically ALSO the profile programmed in flash memory, but could be
        // a chunk in RAM if for instance an animation programmed in flash is triggered
        // with overrides from a bluetooth message.
        Profile::BufferDescriptor overrideBuffer;
        Profile::Array<ParameterOverride> overrides;
        
    private:
        template <typename T>
        Profile::BufferDescriptor getParameterBuffer(Profile::Pointer<T>& inOutPtr) const {
            return getParameterBuffer(inOutPtr.offset);
        }
        template <typename T>
        Profile::BufferDescriptor getParameterBuffer(Profile::Array<T>& inOutPtr) const {
            return getParameterBuffer(inOutPtr.offset);
        }
        Profile::BufferDescriptor getParameterBuffer(uint16_t& inOutOffset) const;

    public:
        AnimationContext(
            const AnimationContextGlobals* theGlobals,
            Profile::BufferDescriptor theBuffer,
            Profile::BufferDescriptor theOverrideBuffer,
            Profile::Array<ParameterOverride> theOverrides);

        // Get pointer from Ptr
        template <typename T>
        T const * getParameter(Profile::Pointer<T> ptr) const {
            auto buf = getParameterBuffer(ptr);
            return ptr.get(buf);
        }

        // Get pointer from Array
        template <typename T>
        T const * getParameterAt(Profile::Array<T> ptr, uint8_t index) const {
            auto buf = getParameterBuffer(ptr);
            return ptr.getAt(buf, index);
        }

        // public parameter evaluators
        uint16_t evaluateScalar(DScalarPtr scalar) const;
        uint32_t evaluateColor(DColorPtr color) const;
        uint16_t evaluateCurve(DScalarPtr curve, uint16_t param) const;
        uint32_t evaluateGradient(DColorPtr color, uint16_t param) const;
    };

}

#pragma pack(pop)
