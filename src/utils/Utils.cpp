#include "utils.h"
#include <nrf_delay.h>
#include <app_timer.h>
#include "config/settings.h"
#include "nrf_log.h"
#include "bluetooth/bluetooth_message_service.h"


using namespace Core;
using namespace Config;

static const int scaler = 64 * 1024;

namespace Utils
{
    uint32_t roundUpTo4(uint32_t value) {
        return (value + 3) & ~(uint32_t)3;
    }

    uint32_t addColors(uint32_t a, uint32_t b) {
        uint8_t red = MAX(getRed(a), getRed(b));
        uint8_t green = MAX(getGreen(a), getGreen(b));
        uint8_t blue = MAX(getBlue(a), getBlue(b));
        return toColor(red,green,blue);
    }

    uint32_t mulColors(uint32_t a, uint32_t b) {
        uint8_t red = getRed(a) * getRed(b) / 255;
        uint8_t green = getGreen(a) * getGreen(b) / 255;
        uint8_t blue = getBlue(a) * getBlue(b) / 255;
        return toColor(red,green,blue);
    }

    uint32_t scaleColor(uint32_t color, uint32_t scaleTimes1000) {
        uint8_t red = CLAMP(getRed(color) * scaleTimes1000 / 1000, 0, 255);
        uint8_t green = CLAMP(getGreen(color) * scaleTimes1000 / 1000, 0, 255);
        uint8_t blue = CLAMP(getBlue(color) * scaleTimes1000 / 1000, 0, 255);
        return toColor(red, green, blue);
    }

    uint32_t interpolateColors(uint32_t color1, uint32_t time1, uint32_t color2, uint32_t time2, uint32_t time) {
        // To stick to integer math, we'll scale the values
        int scaledPercent = (time - time1) * scaler / (time2 - time1);
        int scaledRed = getRed(color1)* (scaler - scaledPercent) + getRed(color2) * scaledPercent;
        int scaledGreen = getGreen(color1) * (scaler - scaledPercent) + getGreen(color2) * scaledPercent;
        int scaledBlue = getBlue(color1) * (scaler - scaledPercent) + getBlue(color2) * scaledPercent;
        return toColor(scaledRed / scaler, scaledGreen / scaler, scaledBlue / scaler);
    }

    uint16_t getEaseParam(uint16_t param, EasingType easing) {
        switch (easing) {
            case Utils::EasingType::EasingType_Unknown:
            default:
                NRF_LOG_ERROR("Unknown Easing Type");
                // Voluntary fall through
            case Utils::EasingType::EasingType_Step:
                return (param < 0x7FFF) ? 0 : 0xFFFF;
            case Utils::EasingType::EasingType_Linear:
                return param;
            case Utils::EasingType::EasingType_EaseIn:
                // We use x*x
                return (uint32_t)param * param / 0xFFFF;
            case Utils::EasingType::EasingType_EaseOut:
                // We use 1 - (1 - x) * (1 - x);
                return (uint32_t)0xFFFF - (uint32_t)(0xFFFF - param) * (0xFFFF - param) / 0xFFFF;
            case Utils::EasingType::EasingType_EaseInEaseOut:
                if (param <= 0x7FFF) {
                    param *= 2;
                    return (uint32_t)param * param / 0x1FFFE;
                } else {
                    param = (param - 0x8000);
                    param *= 2;
                    return (uint32_t)0xFFFF - (uint32_t)(0xFFFF - param) * (0xFFFF - param) / 0x1FFFE;
                }
        }
    }

    uint16_t interpolate(uint16_t start, uint16_t end, uint16_t param, EasingType easing) {
        uint16_t easeParam = getEaseParam(param, easing);
        return (uint16_t)(((uint32_t)start * (0xFFFF - easeParam) + (uint32_t)end * easeParam) / 0xFFFF);
    }

    uint32_t interpolateColors(uint32_t start, uint32_t end, uint16_t param, EasingType easing) {
        uint16_t easeParam = getEaseParam(param, easing);
        auto rs = getRed(start);	auto re = getRed(end);
        auto gs = getGreen(start);	auto ge = getGreen(end);
        auto bs = getBlue(start);	auto be = getBlue(end);
        uint8_t r = (uint8_t)(((uint32_t)rs * (0xFFFF - easeParam) + (uint32_t)re * easeParam) / 0xFFFF);
        uint8_t g = (uint8_t)(((uint32_t)gs * (0xFFFF - easeParam) + (uint32_t)ge * easeParam) / 0xFFFF);
        uint8_t b = (uint8_t)(((uint32_t)bs * (0xFFFF - easeParam) + (uint32_t)be * easeParam) / 0xFFFF);
        return toColor(r,g,b);
    }

    // Helper method to convert register readings to signed integers
    short twosComplement(uint8_t registerValue) {
        // If a positive value, return it
        if ((registerValue & 0x80) == 0) {
            return registerValue;
        } else {
            // Otherwise perform the 2's complement math on the value
            uint8_t comp = ~(registerValue - 0x01);
            return (short)comp * -1;
        }
    }

    // Helper method to convert register readings to signed integers
    int twosComplement12(uint16_t registerValue) {
        // If a positive value, return it
        if ((registerValue & 0x0800) == 0) {
            return registerValue;
        } else {
            // Otherwise perform the 2's complement math on the value
            uint16_t comp = ~((registerValue | 0xF000) - 0x01);
            return (int)comp * -1;
        }
    }

    // Helper method to convert register readings to signed integers
    int twosComplement16(uint16_t registerValue) {
        // If a positive value, return it
        if ((registerValue & 0x8000) == 0) {
            return registerValue;
        } else {
            // Otherwise perform the 2's complement math on the value
            uint16_t comp = ~(registerValue - 0x01);
            return (int)comp * -1;
        }
    }

    /// <summary>
    /// Parses the first word out of a string (typically a command or parameter)
    /// </summary>
    /// <param name="text">The string to parse the first word from</param>
    /// <param name="len">The length of the string</param>
    /// <param name="outWord">The return string buffer</param>
    /// <param name="outWordLen">The max length of the return string buffer</param>
    /// <returns>The length of the found word, otherwise 0</returns>
    int parseWord(char*& text, int& len, char* outWord, int outWordLen)
    {
        while (len > 0&& (*text == ' ' || *text == '\t'))
        {
            text++;
            len--;
        }

        int wordLen = 0;
        if (len > 0)
        {
            while (len > 0 && wordLen < outWordLen && *text != ' ' && *text != '\t' && *text != '\n' && *text != '\r' && *text != 0)
            {
                *outWord = *text;
                outWord++;
                text++;
                len--;
                wordLen++;
            }

            *outWord = 0;
            wordLen++;
        }

        return wordLen;
    }

    /* A PROGMEM (flash mem) table containing 8-bit unsigned sine wave (0-255).
    Copy & paste this snippet into a Python REPL to regenerate:
    import math
    for x in range(256):
        print("{:3},".format(int((math.sin(x/128.0*math.pi)+1.0)*127.5+0.5))),
        if x&15 == 15: print
    */
    static const uint8_t _sineTable[256] = {
    128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
    176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
    218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
    245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
    255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
    245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
    218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
    176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
    128,124,121,118,115,112,109,106,103,100, 97, 93, 90, 88, 85, 82,
    79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
    37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11,
    10,  9,  7,  6,  5,  5,  4,  3,  2,  2,  1,  1,  1,  0,  0,  0,
    0,  0,  0,  0,  1,  1,  1,  2,  2,  3,  4,  5,  5,  6,  7,  9,
    10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
    37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
    79, 82, 85, 88, 90, 93, 97,100,103,106,109,112,115,118,121,124 };

    /* Similar to above, but for an 8-bit gamma-correction table.
    Copy & paste this snippet into a Python REPL to regenerate:
import math
gamma=5
for x in range(256):
    print("{:3},".format(int(math.pow((x)/255.0,gamma)*255.0+0.5))),
    if x&15 == 15: print
    */
    static const uint8_t _gammaTable[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
    3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  7,
    7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 11, 12, 12,
    13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
    20, 21, 21, 22, 22, 23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29,
    30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42,
    42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    58, 59, 60, 61, 62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 75,
    76, 77, 78, 80, 81, 82, 84, 85, 86, 88, 89, 90, 92, 93, 94, 96,
    97, 99,100,102,103,105,106,108,109,111,112,114,115,117,119,120,
    122,124,125,127,129,130,132,134,136,137,139,141,143,145,146,148,
    150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,
    182,184,186,188,191,193,195,197,199,202,204,206,209,211,213,215,
    218,220,223,225,227,230,232,235,237,240,242,245,247,250,252,255 };

    static const uint8_t _asinTable[256] = {
    63,  68,  70,  72,  73,  75,  76,  77,  78,  79,  79,  80,  81,  82,  82,  83, 
    84,  84,  85,  86,  86,  87,  87,  88,  89,  89,  90,  90,  91,  91,  92,  92,
    93,  93,  94,  94,  95,  95,  95,  96,  96,  97,  97,  98,  98,  98,  99,  99,
    100, 100, 100, 101, 101, 102, 102, 102, 103, 103, 104, 104, 104, 105, 105, 105,
    106, 106, 107, 107, 107, 108, 108, 108, 109, 109, 109, 110, 110, 110, 111, 111,
    112, 112, 112, 113, 113, 113, 114, 114, 114, 115, 115, 115, 116, 116, 116, 117,
    117, 117, 118, 118, 118, 119, 119, 119, 119, 120, 120, 120, 121, 121, 121, 122,
    122, 122, 123, 123, 123, 124, 124, 124, 125, 125, 125, 126, 126, 126, 127, 127,
    127, 127, 128, 128, 128, 129, 129, 129, 130, 130, 130, 131, 131, 131, 132, 132,
    132, 133, 133, 133, 134, 134, 134, 135, 135, 135, 135, 136, 136, 136, 137, 137,
    137, 138, 138, 138, 139, 139, 139, 140, 140, 140, 141, 141, 141, 142, 142, 142,
    143, 143, 144, 144, 144, 145, 145, 145, 146, 146, 146, 147, 147, 147, 148, 148,
    149, 149, 149, 150, 150, 150, 151, 151, 152, 152, 152, 153, 153, 154, 154, 154,
    155, 155, 156, 156, 156, 157, 157, 158, 158, 159, 159, 159, 160, 160, 161, 161,
    162, 162, 163, 163, 164, 164, 165, 165, 166, 167, 167, 168, 168, 169, 170, 170,
    171, 172, 172, 173, 174, 175, 175, 176, 177, 178, 179, 181, 182, 184, 186, 191 };


    uint8_t sine8(uint8_t x) {
        return _sineTable[x]; // 0-255 in, 0-255 out
    }

    uint8_t gamma8(uint8_t x) {
        return _gammaTable[x]; // 0-255 in, 0-255 out
    }

    // in: 0 => -1, 255 => +1
    // out: 0 => -pi, 255 => pi
    uint8_t asin8(uint8_t x) {
        return _asinTable[x]; // 0-255 in, 0-255 out
    }

    // in: 0 => -1, 255 => +1
    // out: 0 => -pi, 255 => pi
    uint8_t acos8(uint8_t x) {
        return 64 + _asinTable[x]; // 0-255 in, 0-255 out
    }

    uint32_t gamma(uint32_t color) {
        uint8_t r = gamma8(getRed(color));
        uint8_t g = gamma8(getGreen(color));
        uint8_t b = gamma8(getBlue(color));
        return toColor(r, g, b);
    }

    /* D. J. Bernstein hash function */
    uint32_t computeHash(const uint8_t* data, int size) {
        uint32_t hash = 5381;
        for (int i = 0; i < size; ++i) {
            hash = 33 * hash ^ data[i];
        }
        return hash;
    }

    // Originals: https://github.com/andyherbert/lz1
    
    uint32_t lz77_compress (uint8_t *uncompressed_text, uint32_t uncompressed_size, uint8_t *compressed_text)
    {
        uint8_t pointer_length, temp_pointer_length;
        uint16_t pointer_pos, temp_pointer_pos, output_pointer;
        uint32_t compressed_pointer, output_size, coding_pos, output_lookahead_ref, look_behind, look_ahead;
        
        *((uint32_t *) compressed_text) = uncompressed_size;
        compressed_pointer = output_size = 4;
        
        for(coding_pos = 0; coding_pos < uncompressed_size; ++coding_pos)
        {
            pointer_pos = 0;
            pointer_length = 0;
            for(temp_pointer_pos = 1; (temp_pointer_pos < 4096) && (temp_pointer_pos <= coding_pos); ++temp_pointer_pos)
            {
                look_behind = coding_pos - temp_pointer_pos;
                look_ahead = coding_pos;
                for(temp_pointer_length = 0; uncompressed_text[look_ahead++] == uncompressed_text[look_behind++]; ++temp_pointer_length)
                    if(temp_pointer_length == 15)
                        break;
                if(temp_pointer_length > pointer_length)
                {
                    pointer_pos = temp_pointer_pos;
                    pointer_length = temp_pointer_length;
                    if(pointer_length == 15)
                        break;
                }
            }
            coding_pos += pointer_length;
            if(pointer_length && (coding_pos == uncompressed_size))
            {
                output_pointer = (pointer_pos << 4) | (pointer_length - 1);
                output_lookahead_ref = coding_pos - 1;
            }
            else
            {
                output_pointer = (pointer_pos << 4) | pointer_length;
                output_lookahead_ref = coding_pos;
            }
            *((uint32_t *) (compressed_text + compressed_pointer)) = output_pointer;
            compressed_pointer += 2;
            *(compressed_text + compressed_pointer++) = *(uncompressed_text + output_lookahead_ref);
            output_size += 3;
        }
        
        return output_size;
    }

    uint32_t lz77_decompress (uint8_t *compressed_text, uint8_t *uncompressed_text)
    {
        uint8_t pointer_length;
        uint16_t input_pointer, pointer_pos;
        uint32_t compressed_pointer, coding_pos, pointer_offset, uncompressed_size;
        
        uncompressed_size = *((uint32_t *) compressed_text);
        compressed_pointer = 4;
        
        for(coding_pos = 0; coding_pos < uncompressed_size; ++coding_pos)
        {
            input_pointer = *((uint32_t *) (compressed_text + compressed_pointer));
            compressed_pointer += 2;
            pointer_pos = input_pointer >> 4;
            pointer_length = input_pointer & 15;
            if(pointer_pos)
                for(pointer_offset = coding_pos - pointer_pos; pointer_length > 0; --pointer_length)
                    uncompressed_text[coding_pos++] = uncompressed_text[pointer_offset++];
            *(uncompressed_text + coding_pos) = *(compressed_text + compressed_pointer++);
        }
        
        return coding_pos;
    }	

    uint8_t interpolateIntensity(uint8_t intensity1, int time1, uint8_t intensity2, int time2, int time) {
        int scaledPercent = (time - time1) * scaler / (time2 - time1);
        return (uint8_t)((intensity1 * (scaler - scaledPercent) + intensity2 * scaledPercent) / scaler);
    }

    uint32_t modulateColor(uint32_t color, uint8_t intensity) {
        int red = getRed(color) * intensity / 255;
        int green = getGreen(color) * intensity / 255;
        int blue = getBlue(color) * intensity / 255;
        return toColor((uint8_t)red, (uint8_t)green, (uint8_t)blue);
    }


    // sqrt_i32 computes the squrare root of a 32bit integer and returns
    // a 32bit integer value. It requires that v is positive.
    // Source: https://github.com/chmike/fpsqrt/tree/master
    int32_t sqrt_i32(int32_t v) {
        uint32_t b = 1<<30, q = 0, r = v;
        while (b > r)
            b >>= 2;
        while( b > 0 ) {
            uint32_t t = q + b;
            q >>= 1;           
            if( r >= t ) {     
                r -= t;        
                q += b;        
            }
            b >>= 2;
        }
        return q;
    }
}
