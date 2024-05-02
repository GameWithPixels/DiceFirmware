#include "animation_parameters.h"
#include "utils/Rainbow.h"
#include "utils/Utils.h"
#include "nrf_log.h"

namespace Animations
{
    uint16_t getScalarSize(const DScalar* scalar, Profile::BufferDescriptor buf) {
        switch (scalar->type) {
            case ScalarType::ScalarType_Unknown:
                return sizeof(DScalar);
            case ScalarType::ScalarType_UInt8_t:
                return sizeof(DScalarUInt8);
            case ScalarType::ScalarType_UInt16_t:
                return sizeof(DScalarUInt16);
            case ScalarType::ScalarType_Global:
                return sizeof(DScalarGlobal);
            case ScalarType::ScalarType_Lookup:
                {
                auto s = static_cast<const DScalarLookup*>(scalar);
                auto c = s->lookupCurve.get(buf);
                auto d = s->parameter.get(buf);
                return sizeof(DScalarLookup) + getScalarSize(d, buf) + getScalarSize(c, buf);
                }
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
            case ColorType::ColorType_Lookup:
                {
                auto s = static_cast<const DColorLookup*>(color);
                auto g = s->lookupGradient.get(buf);
                auto d = s->parameter.get(buf);
                return sizeof(DColorLookup) + getScalarSize(d, buf) + getColorSize(g, buf);
                }
            case ColorType::GradientType_Rainbow:
                return sizeof(DGradientRainbow);
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
