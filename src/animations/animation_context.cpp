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
        DScalarPtr scopy = scalar;
        Profile::BufferDescriptor buf = getParameterBuffer(scopy);
        auto s = scopy.get(buf);

        uint16_t ret = 0;
        switch (s->type) {
            case ScalarType::ScalarType_Unknown:
                NRF_LOG_ERROR("Unknown Scalar type");
                break;
            case ScalarType::ScalarType_UInt8_t:
                {
                    auto s8 = static_cast<const DScalarUInt8*>(s);
                    ret = s8->value;
                }
                break;
            case ScalarType::ScalarType_UInt16_t:
                {
                    auto s16 = static_cast<const DScalarUInt16*>(s);
                    ret = s16->value;
                }
                break;
            case ScalarType::ScalarType_Global:
                {
                    auto sg = static_cast<const DScalarGlobal*>(s);
                    switch (sg->globalType) {
                        case GlobalType_Unknown:
                            NRF_LOG_ERROR("Unknown Global Scalar type");
                            break;
                        case GlobalType_NormalizedCurrentFace:
                            ret = globals->normalizedFace;
                            break;
                        default:
                            NRF_LOG_ERROR("Invalid Scalar Type");
                            break;
                    }
                }
                break;
            case ScalarType::ScalarType_Lookup:
                {
                    auto sl = static_cast<const DScalarLookup*>(s);
                    ret = evaluateCurve(sl->lookupCurve, evaluateScalar(sl->parameter));
                }
                break;
            case ScalarType::CurveType_TwoUInt8:
            case ScalarType::CurveType_TwoUInt16:
            case ScalarType::CurveType_TrapezeUInt8:
            case ScalarType::CurveType_TrapezeUInt16:
            case ScalarType::CurveType_UInt16Keyframes:
                NRF_LOG_ERROR("Unsupported curve type used in getScalarValue");
                break;
            default:
                NRF_LOG_ERROR("Invalid Scalar Type");
                break;
        }
        return ret;
    }

    uint32_t AnimationContext::evaluateColor(DColorPtr color) const {
        // Check if this is an overriden scalar
        DColorPtr ccopy = color;
        Profile::BufferDescriptor buf = getParameterBuffer(ccopy);
        auto c = ccopy.get(buf);
        uint32_t ret = 0;
        switch (c->type) {
        case ColorType::ColorType_Unknown:
            NRF_LOG_ERROR("Unknown color type");
            break;
        case ColorType::ColorType_Palette:
            {
                auto cp = static_cast<const DColorPalette*>(c);
                ret = Rainbow::palette(cp->index);
            }
            break;
        case ColorType::ColorType_RGB:
            {
                auto crgb = static_cast<const DColorRGB*>(c);
                ret = Utils::toColor(crgb->rValue, crgb->gValue, crgb->bValue);
            }
            break;
        case ColorType::ColorType_Lookup:
            {
                auto cl = static_cast<const DColorLookup*>(c);
                ret = evaluateGradient(cl->lookupGradient, evaluateScalar(cl->parameter));
            }
            break;
        case ColorType::GradientType_Rainbow:
        case ColorType::GradientType_TwoColors:
        case ColorType::GradientType_Keyframes:
            NRF_LOG_ERROR("Unsupported color type");
            break;
        default:
            NRF_LOG_ERROR("Invalid color type");
            break;
        }
        return ret;
    }

    uint16_t AnimationContext::evaluateCurve(DScalarPtr curve, uint16_t param) const {
        // Check if this is an overriden scalar
        DScalarPtr curveCopy = curve;
        Profile::BufferDescriptor buf = getParameterBuffer(curveCopy);
        auto c = curveCopy.get(buf);
        uint16_t ret = 0;
        switch (c->type) {
            case ScalarType::ScalarType_Unknown:
                NRF_LOG_ERROR("Unknown Scalar type");
                break;
            case ScalarType::ScalarType_UInt8_t:
            case ScalarType::ScalarType_UInt16_t:
            case ScalarType::ScalarType_Global:
            case ScalarType::ScalarType_Lookup:
                ret = evaluateScalar(curve);
                break;
            case ScalarType::CurveType_TwoUInt8:
                {
                    auto ct8 = static_cast<const DCurveTwoUInt8*>(c);
                    return Utils::interpolate(ct8->start, ct8->end, param, ct8->easing);
                }
                break;
            case ScalarType::CurveType_TwoUInt16:
                {
                    auto ct16 = static_cast<const DCurveTwoUInt16*>(c);
                    return Utils::interpolate(ct16->start, ct16->end, param, ct16->easing);
                }
                break;
            default:
                NRF_LOG_ERROR("Not implemented Scalar Type");
        }
        return ret;
    }

    uint32_t AnimationContext::evaluateGradient(DColorPtr color, uint16_t param) const {
        // Check if this is an overriden scalar
        DColorPtr colorCopy = color;
        Profile::BufferDescriptor buf = getParameterBuffer(colorCopy);
        auto c = colorCopy.get(buf);
        uint32_t ret = 0;
        switch (c->type) {
        case ColorType::ColorType_Unknown:
            NRF_LOG_ERROR("Unknown color type");
            break;
        case ColorType::ColorType_Palette:
        case ColorType::ColorType_RGB:
        case ColorType::ColorType_Lookup:
            ret = evaluateColor(color);
            break;
        case ColorType::GradientType_Rainbow:
            ret = Rainbow::wheel(param / 256);
            break;
        case ColorType::GradientType_TwoColors:
            {
                auto ctc = static_cast<const DGradientTwoColors*>(c);
                uint32_t startColor = evaluateColor(ctc->start);
                uint32_t endColor = evaluateColor(ctc->end);
                return Utils::interpolateColors(startColor, endColor, param, ctc->easing);
            }
            break;
        default:
            NRF_LOG_ERROR("Not implemented color type");
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

