#pragma once

#include "stdint.h"
#include "profile/profile_buffer.h"
#include "utils/Utils.h"

#pragma pack(push, 1)

namespace Animations
{
    struct DScalar;
    struct Curve;
    struct DColor;
    struct ColorCurve;

    typedef Profile::Pointer<DScalar> DScalarPtr;
    typedef Profile::Pointer<Curve> CurvePtr;
    typedef Profile::Pointer<DColor> DColorPtr;
    typedef Profile::Pointer<ColorCurve> ColorCurvePtr;

    // Scalar types
    enum ScalarType : uint8_t
    {
        ScalarType_Unknown = 0,

        ScalarType_UInt8,
        ScalarType_UInt16,
        ScalarType_Global,
        ScalarType_Lookup,

        ScalarType_OperationUInt8 = 0x10,
        ScalarType_OperationUInt16,
        ScalarType_OperationScalar,
        ScalarType_OperationScalarAndUInt8,
        ScalarType_OperationScalarAndUInt16,
        ScalarType_OperationUInt8AndScalar,
        ScalarType_OperationUInt16AndScalar,
        ScalarType_OperationTwoScalars,
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

    enum GlobalName : uint8_t
    {
        GlobalName_Unknown = 0,
        GlobalName_NormalizedCurrentFace,
        GlobalName_NormalizedAnimationTime,
        GlobalName_AnimatedLED,
    };

    struct DScalarGlobal : public DScalar
    {
        GlobalName name;
    };
    // size: 2 bytes

    struct DScalarLookup : public DScalar
    {
        DScalarPtr parameter;
        CurvePtr lookupCurve;
    };
    // size: 5 bytes

    enum OperationOneOperand : uint8_t
    {
        OperationOneOperand_Unknown = 0,
        OperationOneOperand_Abs,
        OperationOneOperand_Sin,
        OperationOneOperand_Cos,
        OperationOneOperand_Asin,
        OperationOneOperand_Acos,
        OperationOneOperand_Sqr,
        OperationOneOperand_Sqrt,
        OperationOneOperand_LoopTime,
        // Pow, Log, Floor, Ceil, Round, Trunc, Frac, Neg, Inv, Sign, SignNonZero,
    };

    struct DOperationUInt8 : public DScalar
    {
        OperationOneOperand operation;
        uint8_t value;
    };
    // size: 3 bytes

    struct DOperationUInt16 : public DScalar
    {
        OperationOneOperand operation;
        uint16_t value;
    };
    // size: 4 bytes

    struct DOperationScalar : public DScalar
    {
        OperationOneOperand operation;
        DScalarPtr value;
    };
    // size: 4 bytes

    enum OperationTwoOperands : uint8_t
    {
        OperationTwoOperands_Unknown = 0,
        OperationTwoOperands_Add,
        OperationTwoOperands_Sub,
        OperationTwoOperands_Mul,
        OperationTwoOperands_Div,
        OperationTwoOperands_FMul,
        OperationTwoOperands_FIMul,
        OperationTwoOperands_FDiv,
        OperationTwoOperands_Mod,
        OperationTwoOperands_Min,
        OperationTwoOperands_Max,
        OperationTwoOperands_Traveling,
    };

    // Base operation with 2 operands struct
    struct DOperationTwoOperands : public DScalar
    {
        OperationTwoOperands operation;
    };

    struct DOperationScalarAndUInt8 : public DOperationTwoOperands
    {
        DScalarPtr value1;
        uint8_t value2;
    };
    // size: 5 bytes

    struct DOperationScalarAndUInt16 : public DOperationTwoOperands
    {
        DScalarPtr value1;
        uint16_t value2;
    };
    // size: 6 bytes

    struct DOperationUInt8AndScalar : public DOperationTwoOperands
    {
        uint8_t value1;
        DScalarPtr value2;
    };
    // size: 5 bytes

    struct DOperationUInt16AndScalar : public DOperationTwoOperands
    {
        uint16_t value1;
        DScalarPtr value2;
    };

    struct DOperationTwoScalars : public DOperationTwoOperands
    {
        DScalarPtr value1;
        DScalarPtr value2;
    };
    // size: 8 bytes

    // Curve types
    enum CurveType : uint8_t
    {
        CurveType_Unknown = 0,
        CurveType_TwoUInt8,   // simple interpolation between two 8 bit values
        CurveType_TwoUInt16,        // simple interpolation between two 16 bit values
        CurveType_TrapezeUInt8,     // trapeze shaped interpolation from 0 to a 1 and back to 0
        CurveType_TrapezeUInt16,    // trapeze shaped interpolation from 0 to a 1 and back to 0
        CurveType_UInt16Keyframes,
    };

    // Base curve struct
    struct Curve
    {
        CurveType type;
    };

    struct CurveTwoUInt8 : public Curve
    {
        uint8_t start;
        uint8_t end;
        Utils::EasingType easing;
    };
    // size: 4 bytes

    struct CurveTwoUInt16 : public Curve
    {
        uint16_t start;
        uint16_t end;
        Utils::EasingType easing;
    };
    // size: 6 bytes

    struct CurveTrapezeUInt8 : public Curve
    {
        uint8_t rampUpScale;
        uint8_t rampDownScale;
        Utils::EasingType rampUpEasing;
        Utils::EasingType rampDownEasing;
    };
    // size: 6 bytes

    struct CurveTrapezeUInt16 : public Curve
    {
        uint16_t rampUpScale;
        uint16_t rampDownScale;
        Utils::EasingType rampUpEasing;
        Utils::EasingType rampDownEasing;
    };
    // size: 7 bytes

    struct CurveUInt16Keyframes : public Curve
    {
        struct Keyframe
        {
            uint16_t time;
            uint16_t value;
        };
        Profile::Array<Keyframe> keyframes;
    };

    // Color types
    enum ColorType : uint8_t
    {
        ColorType_Unknown = 0,
        ColorType_Palette,  // uses the global palette
        ColorType_RGB,      // stores actual rgb values
        ColorType_Lookup,   // uses a scalar to lookup the color in a curve
    };

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
        DScalarPtr parameter;
        ColorCurvePtr lookupCurve;
    };
    // size: 5 bytes

    enum ColorCurveType : uint8_t
    {
        ColorCurveType_Unknown = 0,
        ColorCurveType_Rainbow,
        ColorCurveType_TwoColors,
        ColorCurveType_Keyframes,
    };

    struct ColorCurve
    {
        ColorCurveType type;
    };

    struct ColorCurveRainbow : public ColorCurve
    {
        // No data for now
    };
    // size: 1 bytes

    struct ColorCurveTwoColors : public ColorCurve
    {
        DColorPtr start;
        DColorPtr end;
        Utils::EasingType easing;
    };
    // size: 6 bytes

    struct ColorCurveKeyframes : public ColorCurve
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
}

#pragma pack(pop)
