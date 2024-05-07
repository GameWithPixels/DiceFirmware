#include "animation_context.h"
#include "malloc.h"
#include "settings.h"
#include "animation.h"
#include "config/dice_variants.h"
#include "utils/Rainbow.h"
#include "utils/Utils.h"
#include "nrf_log.h"

using namespace Config;

namespace Animations
{
    uint16_t operation(OperationOneOperand op, uint16_t value);
    uint16_t operation(OperationTwoOperands op, uint16_t left, uint16_t right);
    uint16_t trapeze(uint16_t param, uint16_t rampUp, uint16_t rampDown, Utils::EasingType rampUpEasing, Utils::EasingType rampDownEasing);


    AnimationContext::AnimationContext(
        const AnimationContextGlobals* theGlobals,
        Profile::BufferDescriptor theBuffer,
        Profile::BufferDescriptor theOverrideBuffer,
        Profile::Array<ParameterOverride> theOverrides)
        : globals(theGlobals)
        , buffer(theBuffer)
        , overrideBuffer(theOverrideBuffer)
        , overrides(theOverrides)
    {
    }

    uint16_t AnimationContext::evaluateScalar(DScalarPtr scalar) const {
        // Check if this is an overriden scalar
        auto copy = scalar;
        auto buf = getParameterBuffer(copy);
        auto s = copy.get(buf);

        uint16_t ret = 0;
        switch (s->type) {
            case ScalarType_UInt8: {
                    auto s8 = static_cast<const DScalarUInt8*>(s);
                    ret = s8->value;
                }
                break;
            case ScalarType_UInt16: {
                    auto s16 = static_cast<const DScalarUInt16*>(s);
                    ret = s16->value;
                }
                break;
            case ScalarType_Global: {
                    auto sg = static_cast<const DScalarGlobal*>(s);
                    ret = getGlobal(sg->name);
                }
                break;
            case ScalarType_Lookup: {
                    auto sl = static_cast<const DScalarLookup*>(s);
                    ret = evaluateCurve(sl->lookupCurve, evaluateScalar(sl->parameter));
                }
                break;
            case ScalarType_OperationScalar: {
                    auto op = static_cast<const DOperationScalar*>(s);
                    ret = operation(op->operation, evaluateScalar(op->parameter));
                }
                break;
            case ScalarType_OperationScalarAndUInt8: {
                    auto op = static_cast<const DOperationScalarAndUInt8*>(s);
                    ret = operation(op->operation, evaluateScalar(op->parameter1), op->parameter2);
                }
                break;
            case ScalarType_OperationScalarAndUInt16: {
                    auto op = static_cast<const DOperationScalarAndUInt16*>(s);
                    ret = operation(op->operation, evaluateScalar(op->parameter1), op->parameter2);
                }
                break;
            case ScalarType_OperationUInt8AndScalar: {
                    auto op = static_cast<const DOperationUInt8AndScalar*>(s);
                    ret = operation(op->operation, op->parameter1, evaluateScalar(op->parameter2));
                }
                break;
            case ScalarType_OperationUInt16AndScalar: {
                    auto op = static_cast<const DOperationUInt16AndScalar*>(s);
                    ret = operation(op->operation, op->parameter1, evaluateScalar(op->parameter2));
                }
                break;
            case ScalarType_OperationTwoScalars: {
                    auto op = static_cast<const DOperationTwoScalars*>(s);
                    ret = operation(op->operation, evaluateScalar(op->parameter1), evaluateScalar(op->parameter2));
                }
                break;
            default:
                NRF_LOG_ERROR("Bad scalar type %d", s->type);
                break;
        }
        return ret;
    }

    void AnimationContext::evaluateScalarVector(DScalarVectorPtr scalarVector, uint16_t retScalars[]) const {
        // Check if this is an overriden vector
        auto copy = scalarVector;
        auto buf = getParameterBuffer(copy);
        auto sv = copy.get(buf);

        switch (sv->type) {
            case ScalarVectorType_Repeated: {
                    auto svr = static_cast<const DScalarVectorRepeated*>(sv);
                    const auto value = evaluateScalar(svr->value);
                    for (int i = 0; i < MAX_COUNT; ++i) {
                        retScalars[i] = value;
                    }
                }
                break;
            default:
                for (int i = 0; i < MAX_COUNT; ++i) {
                    retScalars[i] = 0;
                }
                NRF_LOG_ERROR("Bad scalar vector type", sv->type);
                break;
        }
    }

    uint16_t AnimationContext::evaluateCurve(CurvePtr curve, uint16_t param) const {
        // Check if this is an overriden scalar
        auto copy = curve;
        auto buf = getParameterBuffer(copy);
        auto c = copy.get(buf);

        uint16_t ret = 0;
        switch (c->type) {
            case CurveType_TwoUInt8: {
                    auto ct8 = static_cast<const CurveTwoUInt8*>(c);
                    ret = Utils::interpolate(ct8->start, ct8->end, param, ct8->easing);
                }
                break;
            case CurveType_TwoUInt16: {
                    auto ct16 = static_cast<const CurveTwoUInt16*>(c);
                    ret = Utils::interpolate(ct16->start, ct16->end, param, ct16->easing);
                }
                break;
            case CurveType_TrapezeUInt8: {
                    auto ct8 = static_cast<const CurveTrapezeUInt8*>(c);
                    ret = trapeze(param, ct8->rampUpScale * 0x100, ct8->rampDownScale * 0x100, ct8->rampUpEasing, ct8->rampDownEasing);
                }
                break;
            case CurveType_TrapezeUInt16: {
                    auto ct16 = static_cast<const CurveTrapezeUInt16*>(c);
                    ret = trapeze(param, ct16->rampUpScale * 0x100, ct16->rampDownScale * 0x100, ct16->rampUpEasing, ct16->rampDownEasing);
                }
                break;
            case CurveType_BitmaskUInt32: {
                    auto cb32 = static_cast<const CurveBitmaskUInt32*>(c);
                    const auto bit = param;
                    ret = (cb32->mask & (1 << bit)) != 0 ? 0xFFFFF : 0;
                }
                break;
            default:
                NRF_LOG_ERROR("Bad curve type", c->type);
        }
        return ret;
    }

    uint32_t AnimationContext::evaluateColor(DColorPtr color) const {
        // Check if this is an overriden scalar
        auto copy = color;
        auto buf = getParameterBuffer(copy);
        auto c = copy.get(buf);

        uint32_t ret = 0;
        switch (c->type) {
            case ColorType_Palette: {
                    auto cp = static_cast<const DColorPalette*>(c);
                    ret = Rainbow::palette(cp->index);
                }
                break;
            case ColorType_RGB: {
                    auto crgb = static_cast<const DColorRGB*>(c);
                    ret = Utils::toColor(crgb->rValue, crgb->gValue, crgb->bValue);
                }
                break;
            case ColorType_Lookup: {
                    auto cl = static_cast<const DColorLookup*>(c);
                    ret = evaluateColorCurve(cl->lookupCurve, evaluateScalar(cl->parameter));
                }
                break;
            default:
                NRF_LOG_ERROR("Bad color type %d", c->type);
                break;
        }
        return ret;
    }

    void AnimationContext::evaluateColorVector(DColorVectorPtr colorVector, uint32_t retColors[]) const {
        // Check if this is an overriden vector
        auto copy = colorVector;
        auto buf = getParameterBuffer(copy);
        auto cv = copy.get(buf);

        switch (cv->type) {
            case ColorVectorType_Repeated: {
                    auto cvr = static_cast<const DColorVectorRepeated*>(cv);
                    const auto color = evaluateColor(cvr->color);
                    for (int i = 0; i < MAX_COUNT; ++i) {
                        retColors[i] = color;
                    }
                }
                break;
            case ColorVectorType_Mixer: {
                    auto cvm = static_cast<const DColorVectorMixer*>(cv);
                    uint32_t colors[MAX_COUNT];
                    evaluateColorVector(cvm->colors, colors);
                    uint16_t intensities[MAX_COUNT];
                    evaluateScalarVector(cvm->intensities, intensities);
                    for (int i = 0; i < MAX_COUNT; ++i) {
                        retColors[i] = Utils::modulateColor(colors[i], intensities[i] / 256);
                    }
                }
                break;
            default:
                for (int i = 0; i < MAX_COUNT; ++i) {
                    retColors[i] = 0;
                }
                NRF_LOG_ERROR("Bad color vector type", cv->type);
                break;
        }
    }

    uint32_t AnimationContext::evaluateColorCurve(ColorCurvePtr colorCurve, uint16_t param) const {
        // Check if this is an overriden scalar
        auto copy = colorCurve;
        auto buf = getParameterBuffer(copy);
        auto c = copy.get(buf);

        uint32_t ret = 0;
        switch (c->type) {
            case ColorCurveType_Rainbow:
                ret = Rainbow::wheel(param / 256);
                break;
            case ColorCurveType_TwoColors: {
                    auto ctc = static_cast<const ColorCurveTwoColors*>(c);
                    uint32_t startColor = evaluateColor(ctc->start);
                    uint32_t endColor = evaluateColor(ctc->end);
                    ret = Utils::interpolateColors(startColor, endColor, param, ctc->easing);
                }
                break;
            default:
                NRF_LOG_ERROR("Bad color type", c->type);
                break;
        }
        return ret;
    }

    Profile::BufferDescriptor AnimationContext::getParameterBuffer(uint16_t& inOutOffset) const {
        for (int i = 0; i < overrides.length; ++i) {
            auto ov = overrides.getAt(overrideBuffer, i);
            if (ov->index == inOutOffset) {
                // Modify output value
                inOutOffset = ov->overrideIndex;
                // Return override buffer
                return overrideBuffer;
            }
        }
        return buffer;
    }

    uint16_t AnimationContext::getGlobal(GlobalName name) const {
        uint16_t ret = 0;
        switch (name) {
        case GlobalName_NormalizedCurrentFace:
            ret = globals->normalizedCurrentFace;
            break;
        case GlobalName_NormalizedAnimationTime:
            ret = globals->normalizedAnimationTime;
            break;
        default:
            NRF_LOG_ERROR("Bad global name %d", name);
            break;
        }
        return ret;
    }

    uint16_t operation(OperationOneOperand op, uint16_t value) {
        uint16_t ret = 0;
        switch (op)
        {
        case OperationOneOperand_Abs:
            ret = std::abs(value);
            break;
        case OperationOneOperand_Sin:
            ret = Utils::sine8(value);
            break;
        case OperationOneOperand_Cos:
            ret = Utils::cos8(value);
            break;
        case OperationOneOperand_Asin:
            ret = Utils::asin8(value);
            break;
        case OperationOneOperand_Acos:
            ret = Utils::acos8(value);
            break;
        case OperationOneOperand_Sqr:
            ret = value * value;
            break;
        case OperationOneOperand_Sqrt:
            ret = value > 0 ? Utils::sqrt_i32(value) : 0;
            break;
        case OperationOneOperand_TwoPow:
            ret = 1 << value;
            break;
        case OperationOneOperand_ToMask:
            if (value > 0) {
                ret = 1 << (value / 0xFFF);
            }
            break;
        case OperationOneOperand_FlipBits:
            ret = value ^ 0xFFFF;
            break;
        default:
            NRF_LOG_ERROR("Bad operation %d", op);
            break;
        }
        return ret;
    }

    uint16_t operation(OperationTwoOperands op, uint16_t value1, uint16_t value2) {
        uint16_t ret = 0;
        switch (op)
        {
        case OperationTwoOperands_Add:
            ret = value1 + value2;
            break;
        case OperationTwoOperands_Sub:
            ret = value1 - value2;
            break;
        case OperationTwoOperands_Mul:
            ret = value1 * value2;
            break;
        case OperationTwoOperands_Div:
            ret = value1 / value2;
            break;
        case OperationTwoOperands_FMul:
            ret = (uint32_t)value1 * value2 / 0x10000;
            break;
        case OperationTwoOperands_FIMul:
            ret = (uint32_t)value1 * value2 / 0x100;
            break;
        case OperationTwoOperands_FDiv:
            ret = (uint32_t)value1 * 256 / value2;
            break;
        case OperationTwoOperands_Mod:
            ret = value1 % value2;
            break;
        case OperationTwoOperands_Min:
            ret = std::min(value1, value2);
            break;
        case OperationTwoOperands_Max:
            ret = std::max(value1, value2);
            break;
        case OperationTwoOperands_Mask:
            ret = (value1 & value2) != 0 ? 0xFFFF : 0;
            break;
        default:
            NRF_LOG_ERROR("Bad operation2 %d", op);
            break;
        }
        return ret;
    }

    uint16_t trapeze(uint16_t param, uint16_t rampUp, uint16_t rampDown, Utils::EasingType rampUpEasing, Utils::EasingType rampDownEasing) {
        uint16_t ret = 0x100;
        if (param <= rampUp) {
            // Ramp up
            const uint16_t t = param * 0xFFFF / rampUp;
            ret = Utils::interpolate(0, 0x100, t, rampUpEasing);
        } else if (param >= rampDown) {
            // Ramp down
            const uint16_t t = (param - rampDown) * 0xFFFF / (0xFFFF - rampDown);
            ret = Utils::interpolate(0x100, 0, t, rampDownEasing);
        }
        return ret;
    }
}
