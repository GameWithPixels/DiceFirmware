#include "animation_parameters.h"
#include "utils/Rainbow.h"
#include "utils/Utils.h"
#include "nrf_log.h"

namespace Animations
{
    // Evaluate component values
    uint16_t getScalarValue(const DScalar* scalar, Profile::BufferDescriptor buf) {
        uint16_t ret = 0;
        switch (scalar->type) {
            case ScalarType::ScalarType_Unknown:
                NRF_LOG_ERROR("Unknown Scalar type");
                break;
            case ScalarType::ScalarType_UInt8_t:
                {
                    auto s = static_cast<const DScalarUInt8*>(scalar);
                    ret = s->value;
                }
                break;
            case ScalarType::ScalarType_UInt16_t:
                {
                    auto s = static_cast<const DScalarUInt16*>(scalar);
                    ret = s->value;
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

    uint32_t getColorValue(const DColor* color, Profile::BufferDescriptor buf) {
        uint32_t ret = 0;
        switch (color->type) {
        case ColorType::ColorType_Unknown:
            NRF_LOG_ERROR("Unknown color type");
            break;
        case ColorType::ColorType_Palette:
            {
                auto c = static_cast<const DColorPalette*>(color);
                ret = Rainbow::palette(c->index);
            }
            break;
        case ColorType::ColorType_RGB:
            {
                auto c = static_cast<const DColorRGB*>(color);
                ret = Utils::toColor(c->rValue, c->gValue, c->bValue);
            }
            break;
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

    uint16_t getCurveValue(const DScalar* curve, uint16_t param, Profile::BufferDescriptor buf) {
        uint16_t ret = 0;
        switch (curve->type) {
            case ScalarType::ScalarType_Unknown:
                NRF_LOG_ERROR("Unknown Scalar type");
                break;
            case ScalarType::ScalarType_UInt8_t:
                {
                    auto s = static_cast<const DScalarUInt8*>(curve);
                    ret = s->value;
                }
                break;
            case ScalarType::ScalarType_UInt16_t:
                {
                    auto s = static_cast<const DScalarUInt16*>(curve);
                    ret = s->value;
                }
                break;
            case ScalarType::CurveType_TwoUInt8:
                {
                    auto s = static_cast<const DCurveTwoUInt8*>(curve);
                    return Utils::interpolate(s->start, s->end, param, s->easing);
                }
                break;
            case ScalarType::CurveType_TwoUInt16:
                {
                    auto s = static_cast<const DCurveTwoUInt16*>(curve);
                    return Utils::interpolate(s->start, s->end, param, s->easing);
                }
                break;
            default:
                NRF_LOG_ERROR("Not implemented Scalar Type");
        }
        return ret;
    }

    uint32_t getGradientValue(const DColor* color, uint16_t param, Profile::BufferDescriptor buf) {
        uint32_t ret = 0;
        switch (color->type) {
        case ColorType::ColorType_Unknown:
            NRF_LOG_ERROR("Unknown color type");
            break;
        case ColorType::ColorType_Palette:
            {
                auto c = static_cast<const DColorPalette*>(color);
                ret = Rainbow::palette(c->index);
            }
            break;
        case ColorType::ColorType_RGB:
            {
                auto c = static_cast<const DColorRGB*>(color);
                ret = Utils::toColor(c->rValue, c->gValue, c->bValue);
            }
            break;
        case ColorType::GradientType_TwoColors:
            {
                auto c = static_cast<const DGradientTwoColors*>(color);
                uint32_t startColor = getColorValue(c->start, buf);
                uint32_t endColor = getColorValue(c->end, buf);
                return Utils::interpolateColors(startColor, endColor, param, c->easing);
            }
            break;
        default:
            NRF_LOG_ERROR("Not implemented color type");
            break;
        }
        return ret;
    }

    uint16_t getScalarSize(const DScalar* scalar, Profile::BufferDescriptor buf) {
        switch (scalar->type) {
            case ScalarType::ScalarType_Unknown:
                return sizeof(DScalar);
            case ScalarType::ScalarType_UInt8_t:
                return sizeof(DScalarUInt8);
            case ScalarType::ScalarType_UInt16_t:
                return sizeof(DScalarUInt16);
            case ScalarType::CurveType_TwoUInt8:
                return sizeof(DCurveTwoUInt8);
            case ScalarType::CurveType_TwoUInt16:
                return sizeof(DCurveTwoUInt16);
            case ScalarType::CurveType_TrapezeUInt8:
                return sizeof(DCurveTrapezeUInt8);
            case ScalarType::CurveType_TrapezeUInt16:
                return sizeof(DCurveTrapezeUInt16);
            case ScalarType::CurveType_UInt16Keyframes:
                {
                // This one is a little more complicated
                auto s = static_cast<const DCurveUInt16Keyframes*>(scalar);
                return sizeof(DCurveUInt16Keyframes) + s->keyframes.length * sizeof(DCurveUInt16Keyframes::Keyframe);
                }
            default:
                NRF_LOG_ERROR("Invalid Scalar Type");
                return sizeof(DScalar);
        }
    }

    uint16_t getColorSize(const DColor* color, Profile::BufferDescriptor buf) {
        switch (color->type) {
            case ColorType::ColorType_Unknown:
                NRF_LOG_ERROR("Unknown color type");
                return sizeof(DColor);
            case ColorType::ColorType_Palette:
                return sizeof(DColorPalette);
            case ColorType::ColorType_RGB:
                return sizeof(DColorRGB);
            case ColorType::GradientType_TwoColors:
                {
                    // This one is a little more complicated
                    auto c = static_cast<const DGradientTwoColors*>(color);
                    auto startColor = c->start.get(buf);
                    auto endColor = c->end.get(buf);
                    return sizeof(DGradientTwoColors) + getColorSize(startColor, buf) + getColorSize(endColor, buf);
                }
            case ColorType::GradientType_Keyframes:
                {
                    // This one is a little more complicated
                    auto c = static_cast<const DGradientKeyframes*>(color);
                    return sizeof(DGradientKeyframes) + c->keyframes.length * sizeof(DGradientKeyframes::Keyframe);
                }
            default:
                NRF_LOG_ERROR("Invalid color type");
                return sizeof(DColor);
        }
    }

}