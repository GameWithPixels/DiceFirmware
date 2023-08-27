#pragma once

#include <stdint.h>
#include <algorithm>

#define CLAMP(a, min, max) ((a) < (min) ? (min) : ((a) > (max) ? (max) : (a)))

namespace Utils
{
    uint32_t roundUpTo4(uint32_t address);

    int parseWord(char*& text, int& len, char* outWord, int outWordLen);

    constexpr uint32_t toColor(uint8_t red, uint8_t green, uint8_t blue) { return (uint32_t)(red << 16) | (uint32_t)(green << 8) | (uint32_t)blue; }
    constexpr uint8_t getRed(uint32_t color) { return (color >> 16) & 0xFF; }
    constexpr uint8_t getGreen(uint32_t color) { return (color >> 8) & 0xFF; }
    constexpr uint8_t getBlue(uint32_t color) { return (color) & 0xFF; }
    constexpr uint8_t getGreyscale(uint32_t color) {
        return std::max(getRed(color), std::max(getGreen(color), getBlue(color)));
    }

    uint32_t mulColors(uint32_t a, uint32_t b);
    uint32_t addColors(uint32_t a, uint32_t b);
    template<typename T> T clamp(T value, T min, T max) {
        return value < min ? min : (value > max ? max : value);
    }

    uint32_t scaleColor(uint32_t color, uint32_t scaleTimes1000);
    uint32_t interpolateColors(uint32_t color1, uint32_t time1, uint32_t color2, uint32_t time2, uint32_t time);

    enum EasingType : uint8_t
    {
        EasingType_Unknown = 0,
        EasingType_Step,
        EasingType_Linear,
        EasingType_EaseIn,
        EasingType_EaseOut,
        EasingType_EaseInEaseOut, // S curve
        // Etc...
    };

    uint16_t getEaseParam(uint16_t param, EasingType easing);
    uint16_t interpolate(uint16_t start, uint16_t end, uint16_t param, EasingType easing);
    uint32_t interpolateColors(uint16_t start, uint16_t end, uint16_t param, EasingType easing);

    uint8_t sine8(uint8_t x);
    uint8_t gamma8(uint8_t x);
    uint32_t gamma(uint32_t color);
    uint8_t asin8(uint8_t x);
    uint8_t acos8(uint8_t x);

    uint32_t lz77_compress (uint8_t *uncompressed_text, uint32_t uncompressed_size, uint8_t *compressed_text);
    uint32_t lz77_decompress (uint8_t *compressed_text, uint8_t *uncompressed_text);

    uint32_t computeHash(const uint8_t* data, int size);

    uint8_t interpolateIntensity(uint8_t intensity1, int time1, uint8_t intensity2, int time2, int time);
    uint32_t modulateColor(uint32_t color, uint8_t intensity);

    short twosComplement(uint8_t registerValue);
    int twosComplement12(uint16_t registerValue);
    int twosComplement16(uint16_t registerValue);

    int32_t sqrt_i32(int32_t v);
}
