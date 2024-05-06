#include "animation_context.h"
#include "utils/Rainbow.h"
#include "utils/Utils.h"
#include "nrf_log.h"
#include "malloc.h"

#include "animation.h"
#include "config/dice_variants.h"

using namespace Config;

namespace Animations
{
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
            case ScalarType_UInt8:
                {
                    auto s8 = static_cast<const DScalarUInt8*>(s);
                    ret = s8->value;
                }
                break;
            case ScalarType_UInt16:
                {
                    auto s16 = static_cast<const DScalarUInt16*>(s);
                    ret = s16->value;
                }
                break;
            case ScalarType_Global:
                {
                    auto sg = static_cast<const DScalarGlobal*>(s);
                    switch (sg->name) {
                        case GlobalName_NormalizedCurrentFace:
                            ret = globals->normalizedCurrentFace;
                            break;
                        case GlobalName_NormalizedAnimationTime:
                            ret = globals->normalizedAnimationTime;
                            break;
                        case GlobalName_AnimatedLED:
                            ret = globals->animatedLED;
                            break;
                        default:
                            NRF_LOG_ERROR("Bad scalar name %d", sg->name);
                            break;
                    }
                }
                break;
            case ScalarType_Lookup:
                {
                    auto sl = static_cast<const DScalarLookup*>(s);
                    ret = evaluateCurve(sl->lookupCurve, evaluateScalar(sl->parameter));
                }
                break;
            case ScalarType_OperationScalar:
                {
                    auto op = static_cast<const DOperationScalar*>(s);
                    const auto i = evaluateScalar(op->operand);
                    switch (op->operation)
                    {
                        case OperationOneOperand_Abs:
                            ret = std::abs(i);
                            break;
                        case OperationOneOperand_Sin:
                            ret = Utils::sine8(i);
                            break;
                        case OperationOneOperand_Cos:
                            ret = Utils::cos8(i);
                            break;
                        case OperationOneOperand_Asin:
                            ret = Utils::asin8(i);
                            break;
                        case OperationOneOperand_Acos:
                            ret = Utils::acos8(i);
                            break;
                        case OperationOneOperand_Sqr:
                            ret = i * i;
                            break;
                        case OperationOneOperand_Sqrt:
                            ret = i > 0 ? Utils::sqrt_i32(i) : 0;
                            break;
                        default:
                            NRF_LOG_ERROR("Bad operation %d", op->type);
                            break;
                    }
                }
            case ScalarType_OperationTwoScalars:
                {
                    auto op = static_cast<const DOperationTwoScalars*>(s);
                    const auto i1 = evaluateScalar(op->operand1);
                    const auto i2 = evaluateScalar(op->operand2);
                    switch (op->operation)
                    {
                        case OperationTwoOperands_Add:
                            ret = i1 + i2;
                        break;
                        case OperationTwoOperands_Sub:
                            ret = i1 - i2;
                            break;
                        case OperationTwoOperands_Mul:
                            ret = i1 * i2;
                            break;
                        case OperationTwoOperands_Div:
                            ret = i1 / i2;
                            break;
                        case OperationTwoOperands_Mod:
                            ret = i1 % i2;
                            break;
                        case OperationTwoOperands_Min:
                            ret = std::min(i1, i2);
                            break;
                        case OperationTwoOperands_Max:
                            ret = std::max(i1, i2);
                            break;
                        case OperationTwoOperands_Traveling:
                            ret = i1 + (uint32_t)globals->animatedLED * 0xffff * i2 / ((uint32_t)globals->ledCount * 256);
                            break;
                        default:
                            NRF_LOG_ERROR("Bad operation2 %d", op->type);
                            break;
                    }
                }
                break;
            default:
                NRF_LOG_ERROR("Bad scalar type %d", s->type);
                break;
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
            case ColorType_Palette:
                {
                    auto cp = static_cast<const DColorPalette*>(c);
                    ret = Rainbow::palette(cp->index);
                }
                break;
            case ColorType_RGB:
                {
                    auto crgb = static_cast<const DColorRGB*>(c);
                    ret = Utils::toColor(crgb->rValue, crgb->gValue, crgb->bValue);
                }
                break;
            case ColorType_Lookup:
                {
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

    uint16_t AnimationContext::evaluateCurve(CurvePtr curve, uint16_t param) const {
        // Check if this is an overriden scalar
        auto copy = curve;
        auto buf = getParameterBuffer(copy);
        auto c = copy.get(buf);
        uint16_t ret = 0;
        switch (c->type) {
            case CurveType_TwoUInt8:
                {
                    auto ct8 = static_cast<const CurveTwoUInt8*>(c);
                    return Utils::interpolate(ct8->start, ct8->end, param, ct8->easing);
                }
                break;
            case CurveType_TwoUInt16:
                {
                    auto ct16 = static_cast<const CurveTwoUInt16*>(c);
                    return Utils::interpolate(ct16->start, ct16->end, param, ct16->easing);
                }
                break;
            default:
                NRF_LOG_ERROR("Bad curve type", c->type);
        }
        return ret;
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
            case ColorCurveType_TwoColors:
                {
                    auto ctc = static_cast<const ColorCurveTwoColors*>(c);
                    uint32_t startColor = evaluateColor(ctc->start);
                    uint32_t endColor = evaluateColor(ctc->end);
                    return Utils::interpolateColors(startColor, endColor, param, ctc->easing);
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
}
