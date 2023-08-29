#pragma once

#include "stdint.h"
#include "profile/profile_buffer.h"
#include "utils/Utils.h"

#pragma pack(push, 1)

namespace Animations
{
    struct DGradient;
    struct DCurve;

    // Scalar types
    enum ScalarType : uint8_t
    {
        ScalarType_Unknown = 0,
        ScalarType_UInt8_t,     // 8-bit value
        ScalarType_UInt16_t,    // 16-bit value
        ScalarType_Global,
        ScalarType_Lookup,

        // After this are Curve types
        CurveType_TwoUInt8,     // simple interpolation between two 8 bit values
        CurveType_TwoUInt16,    // simple interpolation between two 16 bit values
        CurveType_TrapezeUInt8, // trapeze shaped interpolation from 0 to a given value and back to 0
        CurveType_TrapezeUInt16, // trapeze shaped interpolation from 0 to a given value and back to 0
        CurveType_UInt16Keyframes,
    };

    // Color types
    enum ColorType : uint8_t
    {
        ColorType_Unknown = 0,
        ColorType_Palette,      // uses the global palette
        ColorType_RGB,          // stores actual rgb values
        ColorType_Lookup,       // uses a scalar to lookup the color in a gradient
        // After this are gradient types
        GradientType_Rainbow,   // basic programmatic rainbow gradient
        GradientType_TwoColors, // simple two-color gradient
        GradientType_Keyframes, // gradient with a few keyframes

    };

    // The most basic animation elements are scalars and colors
    // Again, to avoid storing pointers, we can't allow virtual functions. However, we do want the behavior to
    // be similar to a hierarchy of classes. To work around this, we will use a 'type' identifier and manually
    // handle the polymorphism. That is, a base class method will switch on the stored type to call the proper
    // derived struct method.

    // Note also that we are combining scalar an curve types to save memory. This only works because curve and scalar types
    // can be used in place of one-another. A scalar can be used where a curve is required (always returns the same value)
    // and a curve can be used where a scalar is required (although wasteful) by evaluating the curve at t=0.
    // One way to think about it is that we're union-ing scalar and curve so that we don't need to redefine a bunch of
    // constant curve types based on all the possible scalar types.

    // Base scalar class
    struct DScalar
    {
        ScalarType type;
    };

    // This implementation stores the value it will return
    struct DScalarUInt8 : public DScalar
    {
        uint8_t value;
    };
    // size: 2 bytes

    // This implementation stores the value it will return
    struct DScalarUInt16 : public DScalar
    {
        uint16_t value;
    };
    // size: 3 bytes

    enum GlobalType : uint8_t
    {
        GlobalType_Unknown = 0,
        GlobalType_NormalizedCurrentFace,
    };

    struct DScalarGlobal : public DScalar
    {
        GlobalType globalType;
    };

    struct DScalarLookup : public DScalar
    {
        Profile::Pointer<DCurve> lookupCurve;
        Profile::Pointer<DScalar> parameter;
    };

    // Base curve struct
    struct DCurve
    : public DScalar
    {
        // Base class for curves doesn't have any additional data
        // because we re-use the type identifier from DScalar
    };

    struct DCurveTwoUInt8 : public DCurve
    {
        uint8_t start;
        uint8_t end;
        Utils::EasingType easing;
    };
    // size: 4 bytes

    struct DCurveTwoUInt16 : public DCurve
    {
        uint16_t start;
        uint16_t end;
        Utils::EasingType easing;
    };
    // size: 6 bytes

    struct DCurveTrapezeUInt8 : public DCurve
    {
        uint8_t value;
        uint8_t rampUpScale;
        uint8_t rampDownScale;
        Utils::EasingType rampUpEasing;
        Utils::EasingType rampDownEasing;
    };
    // size: 6 bytes

    struct DCurveTrapezeUInt16 : public DCurve
    {
        uint16_t value;
        uint8_t rampUpScale;
        uint8_t rampDownScale;
        Utils::EasingType rampUpEasing;
        Utils::EasingType rampDownEasing;
    };
    // size: 7 bytes

    struct DCurveUInt16Keyframes : public DCurve
    {
        struct Keyframe
        {
            uint16_t time;
            uint16_t value;
        };
        Profile::Array<Keyframe> keyframes;
    };
    // Etc...


    // Base color struct
    struct DColor
    {
        ColorType type;
    };

    struct DColorPalette : public DColor
    {
        uint8_t index;
    };
    // size: 2 bytes

    struct DColorRGB : public DColor
    {
        uint8_t rValue;
        uint8_t gValue;
        uint8_t bValue;
    };
    // size: 4 bytes

    struct DColorLookup : public DColor
    {
        Profile::Pointer<DGradient> lookupGradient;
        Profile::Pointer<DScalar> parameter;
    };

    // Etc...
    struct DGradient
    : public DColor
    {
        // Base class for gradients doesn't have any additional data
        // because we re-use the type identifier from DGradient
    };

    struct DGradientRainbow : public DGradient
    {
        // No data for now
    };
    // size: 1 bytes

    struct DGradientTwoColors : public DGradient
    {
        Profile::Pointer<DColor> start; // 2 bytes
        Profile::Pointer<DColor> end; // 2 bytes
        Utils::EasingType easing;
    };
    // size: 6 bytes

    struct DGradientKeyframes : public DGradient
    {
        struct Keyframe
        {
            uint16_t time;
            uint8_t index; // palette index
            Utils::EasingType easing;
        };
        Profile::Array<Keyframe> keyframes;
    };
    // size: 4 + N * 4

    typedef Profile::Pointer<DScalar> DScalarPtr;
    typedef Profile::Pointer<DCurve> DCurvePtr;
    typedef Profile::Pointer<DColor> DColorPtr;
    typedef Profile::Pointer<DGradient> DGradientPtr;

    uint16_t getScalarSize(const DScalar* scalar, Profile::BufferDescriptor buf);
    uint16_t getColorSize(const DColor* color, Profile::BufferDescriptor buf);
}

#pragma pack(pop)
