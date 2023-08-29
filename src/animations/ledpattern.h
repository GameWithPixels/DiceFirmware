#pragma once

#include "profile/profile_buffer.h"
#include "animation_parameters.h"

#pragma pack(push, 1)


namespace Animations
{
    struct AnimationContext;

    // LED Patterns define when the LEDs turn on, without including any color or intensity information
    enum LEDPatternType
    {
        LEDPatternType_Unknown = 0,
        LEDPatternType_Keyframes,
        LEDPatternType_Noise,
    };

    // The LED Pattern drives when LEDs turn on/off for a specific die type
    // This struct is *polymorphic* (again using our fake polymorphism to avoid working with pointers)
    struct LEDPattern
    {
        LEDPatternType type;
    };
    typedef Profile::Pointer<LEDPattern> LEDPatternPtr;

    struct LEDPatternInstance
    {
        AnimationContext const * context;
        LEDPattern const * pattern;
		virtual int updateLEDs(int normalizedTime, int retIndices[], uint16_t retParameters[]) = 0;
    };

    // This implementation of the LEDPattern uses random noise to determine when LEDs turn on or off
    struct LEDPatternNoise
        : public LEDPattern
    {
        // Some noise parameters tbd!!!
    };

    struct LEDPatternNoiseInstance
        : public LEDPatternInstance
    {
		virtual int updateLEDs(int normalizedTime, int retIndices[], uint16_t retParameters[]);
    };

    // This implementation of the LEDPattern uses 8bit keyframes to store the time of leds switching on or off
    // The first keyframe stored is always a switch from OFF to ON. After that the keyframes alternate.
    struct LEDPatternKeyframes
        : public LEDPattern
    {
        struct LEDTrack
        {
            Profile::Array<uint8_t> keyframes; // first keyframe is always ON, then they alternate!
        };
        Profile::Array<LEDTrack> ledTracks;
    };

    struct LEDPatternKeyframesInstance
        : public LEDPatternInstance
    {
		virtual int updateLEDs(int normalizedTime, int retIndices[], uint16_t retParameters[]);
    };
}

#pragma pack(pop)


