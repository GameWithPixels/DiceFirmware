#include "dice_variants.h"
#include "board_config.h"
#include "string.h"
#include "assert.h"

using namespace Core;

namespace Config
{
namespace DiceVariants
{
    // Animations are defined with the highest face up, so we may need to remap the Face (or LEDs) to
    // match the current face up. This table is used to do this.
    // If we have computed the color of a face in the canonical orientation (highest face up), then we
    // can use the following to figure out which face/led we should actually set to this color.
    // Actual face/led := DXRemap[currentFaceUp * face/led count + canonicalIndex]
    // Note: this remapping happens in the canonical face space, meaning in this case the face index
    // is one less than the number printed on the dice faces.
    const uint8_t D4Remap[] = {
        3, 2, 1, 0, // Face 1 up
        1, 3, 0, 2, // Face 3 up
        2, 0, 3, 1, // Face 2 up
        0, 1, 2, 3, // Face 4 up
    };

    // const uint8_t D4ReverseRemap[] = {
    //     3, 2, 1, 0,
    //     2, 3, 0, 1,
    //     1, 0, 3, 2,
    //     0, 1, 2, 3,
    // };

    // Used for determining which face is up and/or for animations that color each face based on their normal
    // Note that the normal is also a good approximation of the position of the LED on the face. In most cases
    // the leds are equidistant from the center.
    // Note: these normals are defined in the canonical face order
    const Core::int3 D4FaceNormals[] = {
        {-1000,  0000,  0000},
        { 0000,  0000, -1000},
        { 0000,  0000,  1000},
        { 1000,  0000,  0000},
    };

    // Used for LED-based animations
    const Core::int3 D4LEDNormals[] = {
        {-1000,  0000,  0000},
        { 0000,  0000, -1000},
        { 0000,  0000,  1000},
        { 1000,  0000,  0000},
        { 0000, -1000,  0000},
        { 0000,  1000,  0000},
    };

    // 0, 1, 2, 3, 4, 5 <-- daisy chain index
    // 1, 5, 2, 6, 4, 3 <-- face number (6-sided)
    // 1, -, -, 4, 2, 3 <-- face number (4-sided)
    // 0, 4, 5, 3, 1, 2 <-- led index (4-sided)
    const uint8_t D4LEDIndices[] = {
        0, 4, 5, 3, 1, 2,
    };

    // 0, 1, 2, 3, 4, 5 <-- led Index
    // 0, 4, 5, 3 <-- daisy chain index
    const uint8_t D4ElectricalIndices[] = {
        0, 4, 5, 3, 1, 2,   // daisy chain index
    };

    const uint32_t D4Adjacency[] = {
        // FIXME
        1 << 1 | 1 << 2, // 1
        1 << 0 | 1 << 3,
        1 << 0 | 1 << 3,
        1 << 1 | 1 << 2,
    };

    // Animations are defined with the highest face up, so we may need to remap the Face (or LEDs) to
    // match the current face up. This table is used to do this.
    // If we have computed the color of a face in the canonical orientation (highest face up), then we
    // can use the following to figure out which face/led we should actually set to this color.
    // Actual face/led := DXRemap[currentFaceUp * face/led count + canonicalIndex]
    const uint8_t D6Remap[] = {
        5, 2, 1, 4, 3, 0,
        4, 0, 2, 3, 5, 1,
        3, 4, 5, 0, 1, 2,
        2, 5, 4, 1, 0, 3,
        1, 2, 0, 5, 3, 4, 
        0, 1, 2, 3, 4, 5, 
    };

    // const uint8_t D6ReverseRemap[] = {
    //     5, 2, 1, 4, 3, 0,
    //     1, 5, 2, 3, 0, 4,
    //     3, 4, 5, 0, 1, 2,
    //     4, 3, 0, 5, 2, 1,
    //     2, 0, 1, 4, 5, 3,
    //     0, 1, 2, 3, 4, 5,
    // };

    // Used for determining which face is up and/or for animations that color each face based on their normal
    // Note that the normal is also a good approximation of the position of the LED on the face. In most cases
    // the leds are equidistant from the center.
    // Note: these normals are defined in the canonical face order
    const Core::int3 D6Normals[] = {
        {-1000,  0000,  0000},
        { 0000, -1000,  0000},
        { 0000,  0000, -1000},
        { 0000,  0000,  1000},
        { 0000,  1000,  0000},
        { 1000,  0000,  0000},
    };

    // 0, 1, 2, 3, 4, 5 <-- daisy chain index
    // 1, 5, 2, 6, 4, 3 <-- face number
    // 0, 4, 1, 5, 3, 2 <-- face/led index
    const uint8_t D6LEDIndices[] = {
        0, 4, 1, 5, 3, 2,
    };

    // 0, 1, 2, 3, 4, 5 <-- led index
    // 0, 2, 5, 4, 1, 3 <-- DaisyChain Index
    const uint8_t D6ElectricalIndices[] = {
        0, 2, 5, 4, 1, 3,
    };

    const uint32_t D6Adjacency[] = {
        1 << 1 | 1 << 2 | 1 << 3 | 1 << 4, // 1
        1 << 0 | 1 << 2 | 1 << 3 | 1 << 5,
        1 << 0 | 1 << 1 | 1 << 4 | 1 << 5,
        1 << 0 | 1 << 1 | 1 << 4 | 1 << 5,
        1 << 0 | 1 << 2 | 1 << 3 | 1 << 5,
        1 << 1 | 1 << 2 | 1 << 3 | 1 << 4,
    };

    // Animations are defined with the highest face up, so we may need to remap the Face (or LEDs) to
    // match the current face up. This table is used to do this.
    // If we have computed the color of a face in the canonical orientation (highest face up), then we
    // can use the following to figure out which face/led we should actually set to this color.
    // Actual face/led := DXRemap[currentFaceUp * face/led count + canonicalIndex]
    const uint8_t D20Remap[] = {
        19, 12, 15, 14, 17, 16, 13, 18, 9, 8, 11, 10, 1, 6, 3, 2, 5, 4, 7, 0, // remap for face at index 0 (= face with number 1)
        18, 17, 16, 13, 10, 7, 0, 11, 15, 14, 5, 4, 8, 19, 12, 9, 6, 3, 2, 1, // remap for face at index 1 (= face with number 2)
        17, 15, 14, 8, 13, 0, 1, 16, 12, 9, 10, 7, 3, 18, 19, 6, 11, 5, 4, 2, // etc.
        16, 10, 11, 5, 18, 19, 6, 17, 7, 4, 15, 12, 2, 13, 0, 1, 14, 8, 9, 3,
        15, 14, 13, 0, 16, 10, 7, 17, 8, 1, 18, 11, 2, 12, 9, 3, 19, 6, 5, 4,
        14, 15, 12, 19, 9, 3, 6, 8, 17, 18, 1, 2, 11, 13, 16, 10, 0, 7, 4, 5,
        13, 16, 17, 18, 15, 12, 19, 14, 10, 11, 8, 9, 5, 0, 7, 4, 1, 2, 3, 6,
        12, 9, 8, 1, 14, 13, 0, 15, 3, 2, 17, 16, 4, 19, 6, 5, 18, 11, 10, 7,
        11, 10, 7, 0, 4, 2, 1, 5, 16, 13, 6, 3, 14, 18, 17, 15, 19, 12, 9, 8,
        10, 11, 18, 19, 17, 15, 12, 16, 5, 6, 13, 14, 3, 7, 4, 2, 0, 1, 8, 9,
        9, 3, 2, 4, 1, 0, 7, 8, 6, 5, 14, 13, 11, 12, 19, 18, 15, 17, 16, 10,
        8, 14, 15, 17, 12, 19, 18, 9, 13, 16, 3, 6, 10, 1, 0, 7, 2, 4, 5, 11,
        7, 4, 5, 6, 11, 18, 19, 10, 2, 3, 16, 17, 9, 0, 1, 8, 13, 14, 15, 12,
        6, 5, 4, 7, 2, 1, 0, 3, 11, 10, 9, 8, 16, 19, 18, 17, 12, 15, 14, 13,
        5, 11, 10, 16, 7, 0, 13, 4, 18, 17, 2, 1, 15, 6, 19, 12, 3, 9, 8, 14,
        4, 2, 3, 9, 6, 19, 12, 5, 1, 8, 11, 18, 14, 7, 0, 13, 10, 16, 17, 15,
        3, 2, 1, 0, 8, 14, 13, 9, 4, 7, 12, 15, 10, 6, 5, 11, 19, 18, 17, 16,
        2, 3, 6, 19, 5, 11, 18, 4, 9, 12, 7, 10, 15, 1, 8, 14, 0, 13, 16, 17,
        1, 8, 9, 12, 3, 6, 19, 2, 14, 15, 4, 5, 17, 0, 13, 16, 7, 10, 11, 18,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, // No remapping as animations are designed for face at index 20
    };

    // const uint8_t D20ReverseRemap[] = {
    //     19, 12, 15, 14, 17, 16, 13, 18, 9, 8, 11, 10, 1, 6, 3, 2, 5, 4, 7, 0,
    //     6, 19, 18, 17, 11, 10, 16, 5, 12, 15, 4, 7, 14, 3, 9, 8, 2, 1, 0, 13,
    //     5, 6, 19, 12, 18, 17, 15, 11, 3, 9, 10, 16, 8, 4, 2, 1, 7, 0, 13, 14,
    //     14, 15, 12, 19, 9, 3, 6, 8, 17, 18, 1, 2, 11, 13, 16, 10, 0, 7, 4, 5,
    //     3, 9, 12, 15, 19, 18, 17, 6, 8, 14, 5, 11, 13, 2, 1, 0, 4, 7, 10, 16,
    //     16, 10, 11, 5, 18, 19, 6, 17, 7, 4, 15, 12, 2, 13, 0, 1, 14, 8, 9, 3,
    //     13, 16, 17, 18, 15, 12, 19, 14, 10, 11, 8, 9, 5, 0, 7, 4, 1, 2, 3, 6,
    //     6, 3, 9, 8, 12, 15, 14, 19, 2, 1, 18, 17, 0, 5, 4, 7, 11, 10, 16, 13,
    //     3, 6, 5, 11, 4, 7, 10, 2, 19, 18, 1, 0, 17, 9, 12, 15, 8, 14, 13, 16,
    //     16, 17, 15, 12, 14, 8, 9, 13, 18, 19, 0, 1, 6, 10, 11, 5, 7, 4, 2, 3,
    //     5, 4, 2, 1, 3, 9, 8, 6, 7, 0, 19, 12, 13, 11, 10, 16, 18, 17, 15, 14,
    //     14, 13, 16, 10, 17, 18, 11, 15, 0, 7, 12, 19, 4, 8, 1, 2, 9, 3, 6, 5,
    //     13, 14, 8, 9, 1, 2, 3, 0, 15, 12, 7, 4, 19, 16, 17, 18, 10, 11, 5, 6,
    //     6, 5, 4, 7, 2, 1, 0, 3, 11, 10, 9, 8, 16, 19, 18, 17, 12, 15, 14, 13,
    //     5, 11, 10, 16, 7, 0, 13, 4, 18, 17, 2, 1, 15, 6, 19, 12, 3, 9, 8, 14,
    //     14, 8, 1, 2, 0, 7, 4, 13, 9, 3, 16, 10, 6, 15, 12, 19, 17, 18, 11, 5,
    //     3, 2, 1, 0, 8, 14, 13, 9, 4, 7, 12, 15, 10, 6, 5, 11, 19, 18, 17, 16,
    //     16, 13, 0, 1, 7, 4, 2, 10, 14, 8, 11, 5, 9, 17, 15, 12, 18, 19, 6, 3,
    //     13, 0, 7, 4, 10, 11, 5, 16, 1, 2, 17, 18, 3, 14, 8, 9, 15, 12, 19, 6,
    //     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    // };

    //  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 <-- daisy chain index
    //  4, 12, 10,  3, 17,  1, 11, 14,  6,  0, 18,  8,  5, 13, 19,  7,  9, 16,  2, 15 <-- face/led Index
    const uint8_t D20LEDIndices[] = {
        4, 12, 10,  3, 17,  1, 11, 14,  6,  0, 18,  8,  5, 13, 19,  7,  9, 16,  2, 15
    };


    //  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19 <-- Face/led index
    //  9,  5, 18,  3,  0, 12,  8, 15, 11, 16,  2,  6,  1, 13,  7, 19, 17,  4, 10, 14 <-- daisy chain index
    //  9, 13,  7, 19, 11, 16,  1,  5, 17,  4, 18,  3, 10, 15,  2,  6,  0, 12,  8, 14 <-- old molds daisy chain index
    const uint8_t D20ElectricalIndices[] = {
        9,  5, 18,  3,  0, 12,  8, 15, 11, 16,  2,  6,  1, 13,  7, 19, 17,  4, 10, 14
    };

    // Used for determining which face is up and/or for animations that color each face based on their normal
    // Note that the normal is also a good approximation of the position of the LED on the face. In most cases
    // the leds are equidistant from the center.
    // Note: these normals are defined in the canonical face order
    const Core::int3 D20Normals[] = {
        {-335,  937, - 92},
        {-352, -930, - 98},
        { 716,  572, -399},
        {-253, -357,  898},
        {-995,    8,   93},
        { 804, -  9,  593},
        {-396,  583, -708},
        { 705, -582, -403},
        { 407,  571,  712},
        { 246, -355, -901},
        {-246,  355,  901},
        {-407, -571, -712},
        {-705,  582,  403},
        { 396, -583,  708},
        {-804,    9, -593},
        { 995, -  8, - 93},
        { 253,  357, -898},
        {-716, -572,  399},
        { 352,  930,   98},
        { 335, -937,   92},
    };

    const uint32_t D20Adjacency[] = {
        1 <<  6 | 1 << 18 | 1 << 13, // 1
        1 << 11 | 1 << 17 | 1 << 19,
        1 << 15 | 1 << 16 | 1 << 18,
        1 << 10 | 1 << 13 | 1 << 17,
        1 << 12 | 1 << 14 | 1 << 17,
        1 <<  8 | 1 << 13 | 1 << 15,
        1 <<  0 | 1 << 14 | 1 << 16,
        1 <<  9 | 1 << 15 | 1 << 19,
        1 <<  5 | 1 << 10 | 1 << 18,
        1 <<  7 | 1 << 11 | 1 << 16,
        1 <<  3 | 1 <<  8 | 1 << 12,
        1 <<  1 | 1 <<  9 | 1 << 14,
        1 <<  0 | 1 <<  4 | 1 << 10,
        1 <<  3 | 1 <<  5 | 1 << 19,
        1 <<  4 | 1 <<  6 | 1 << 11,
        1 <<  2 | 1 <<  5 | 1 <<  7,
        1 <<  2 | 1 <<  6 | 1 <<  9,
        1 <<  1 | 1 <<  3 | 1 <<  4,
        1 <<  0 | 1 <<  2 | 1 <<  8,
        1 <<  1 | 1 <<  7 | 1 << 13,
    };

    // Animations are defined with the highest face up, so we may need to remap the Face (or LEDs) to
    // match the current face up. This table is used to do this.
    // If we have computed the color of a face in the canonical orientation (highest face up), then we
    // can use the following to figure out which face/led we should actually set to this color.
    // Actual face/led := DXRemap[currentFaceUp * face/led count + canonicalIndex]
    const uint8_t D12Remap[] = {
        11, 7, 9, 6, 10, 8, 3, 1, 5, 2, 4, 0,
        10, 2, 6, 11, 4, 8, 3, 7, 0, 5, 9, 1,
        9, 1, 5, 0, 8, 4, 7, 3, 11, 6, 10, 2,
        8, 10, 7, 11, 9, 6, 5, 2, 0, 4, 1, 3,
        7, 6, 10, 11, 3, 2, 9, 8, 0, 1, 5, 4,
        6, 8, 2, 11, 1, 7, 4, 10, 0, 9, 3, 5,
        5, 4, 1, 0, 2, 3, 8, 9, 11, 10, 7, 6,
        4, 9, 3, 0, 10, 5, 6, 1, 11, 8, 2, 7,
        3, 5, 9, 0, 7, 1, 10, 4, 11, 2, 6, 8,
        2, 7, 8, 11, 5, 10, 1, 6, 0, 3, 4, 9,
        1, 3, 4, 0, 6, 9, 2, 5, 11, 7, 8, 10,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    };

    // const uint8_t D12ReverseRemap[] = {
    //     11, 7, 9, 6, 10, 8, 3, 1, 5, 2, 4, 0,
    //     8, 11, 1, 6, 4, 9, 2, 7, 5, 10, 0, 3,
    //     3, 1, 11, 7, 5, 2, 9, 6, 4, 0, 10, 8,
    //     8, 10, 7, 11, 9, 6, 5, 2, 0, 4, 1, 3,
    //     8, 9, 5, 4, 11, 10, 1, 0, 7, 6, 2, 3,
    //     8, 4, 2, 10, 6, 11, 0, 5, 1, 9, 7, 3,
    //     3, 2, 4, 5, 1, 0, 11, 10, 6, 7, 9, 8,
    //     3, 7, 10, 2, 0, 5, 6, 11, 9, 1, 4, 8,
    //     3, 5, 9, 0, 7, 1, 10, 4, 11, 2, 6, 8,
    //     8, 6, 0, 9, 10, 4, 7, 1, 2, 11, 5, 3,
    //     3, 0, 6, 1, 2, 7, 4, 9, 10, 5, 11, 8,
    //     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    // };

    // Used for determining which face is up and/or for animations that color each face based on their normal
    // Note that the normal is also a good approximation of the position of the LED on the face. In most cases
    // the leds are equidistant from the center.
    // Note: these normals are defined in the canonical face order
    const Core::int3 D12Normals[] = {
        { -446,  850, -276},     // 1
        { -447,  525,  723},     // 2
        { -447, -850, -276},     // 3
        {-1000,  000, -000},     // 4
        {  447,  525, -723},     // 5
        { -447,  000, -894},     // 6
        {  447, -000,  894},     // 7
        { -447, -525,  723},     // 8
        { 1000, -000,  000},     // 9
        {  447,  850,  276},     // 10
        {  447, -525, -723},     // 11
        {  446, -850,  276},     // 12
    };

    // 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11 <-- daisy chain index
    // 3,  7,  1,  0,  5,  2, 11,  6,  9,  4, 10,  8 <-- Face/led index
    const uint8_t D12LEDIndices[] = {
        3,  7,  1,  0,  5,  2, 11,  6,  9,  4, 10,  8
    };


    // 0, 1, 2, 3, 4, 5, 6, 7,  8, 9, 10, 11 <-- face/Led Index
    // 3, 2, 5, 0, 9, 4, 7, 1, 11, 8, 10,  6 <-- DaisyChain Index
    const uint8_t D12ElectricalIndices[] = {
        3, 2, 5, 0, 9, 4, 7, 1, 11, 8, 10, 6
    };

    const uint32_t D12Adjacency[] = {
        1 <<  1 | 1 <<  3 | 1 <<  4 | 1 <<  5 | 1 <<  9, // 1
        1 <<  0 | 1 <<  3 | 1 <<  6 | 1 <<  7 | 1 <<  9,
        1 <<  3 | 1 <<  5 | 1 <<  7 | 1 << 10 | 1 << 11,
        1 <<  0 | 1 <<  1 | 1 <<  2 | 1 <<  5 | 1 <<  7,
        1 <<  0 | 1 <<  5 | 1 <<  8 | 1 <<  9 | 1 << 10,
        1 <<  0 | 1 <<  2 | 1 <<  3 | 1 <<  4 | 1 << 10,
        1 <<  1 | 1 <<  7 | 1 <<  8 | 1 <<  9 | 1 << 11,
        1 <<  1 | 1 <<  2 | 1 <<  3 | 1 <<  6 | 1 << 11,
        1 <<  4 | 1 <<  6 | 1 <<  9 | 1 << 10 | 1 << 11,
        1 <<  0 | 1 <<  1 | 1 <<  4 | 1 <<  6 | 1 <<  8,
        1 <<  2 | 1 <<  4 | 1 <<  5 | 1 <<  8 | 1 << 11,
        1 <<  2 | 1 <<  6 | 1 <<  7 | 1 <<  8 | 1 << 10,
    };

    // Animations are defined with the highest face up, so we may need to remap the Face (or LEDs) to
    // match the current face up. This table is used to do this.
    // If we have computed the color of a face in the canonical orientation (highest face up), then we
    // can use the following to figure out which face/led we should actually set to this color.
    // Actual face/led := DXRemap[currentFaceUp * face/led count + canonicalIndex]
    const uint8_t D10Remap[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 
        1, 0, 5, 6, 7, 2, 3, 4, 9, 8, 
        2, 3, 4, 9, 8, 1, 0, 5, 6, 7, 
        3, 2, 1, 0, 5, 4, 9, 8, 7, 6, 
        4, 9, 8, 7, 6, 3, 2, 1, 0, 5, 
        5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 
        6, 5, 0, 1, 2, 7, 8, 9, 4, 3, 
        7, 8, 9, 4, 3, 6, 5, 0, 1, 2, 
        8, 7, 6, 5, 0, 9, 4, 3, 2, 1,
        9, 4, 3, 2, 1, 8, 7, 6, 5, 0,
    };

    // const uint8_t D10ReverseRemap[] = {
    //     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    //     1, 0, 5, 6, 7, 2, 3, 4, 9, 8,
    //     6, 5, 0, 1, 2, 7, 8, 9, 4, 3,
    //     3, 2, 1, 0, 5, 4, 9, 8, 7, 6,
    //     8, 7, 6, 5, 0, 9, 4, 3, 2, 1,
    //     5, 6, 7, 8, 9, 0, 1, 2, 3, 4,
    //     2, 3, 4, 9, 8, 1, 0, 5, 6, 7,
    //     7, 8, 9, 4, 3, 6, 5, 0, 1, 2,
    //     4, 9, 8, 7, 6, 3, 2, 1, 0, 5,
    //     9, 4, 3, 2, 1, 8, 7, 6, 5, 0,
    // };

    // Used for determining which face is up and/or for animations that color each face based on their normal
    // Note that the normal is also a good approximation of the position of the LED on the face. In most cases
    // the leds are equidistant from the center.
    // Note: these normals are defined in the canonical face order
    const Core::int3 D10Normals[] = {
        {-065,  996,  055}, // 00
        { 165, -617,  768}, // 10
        { 489, -  8, -871}, // 20
        {-993,  017,  111}, // ...
        { 650,  603,  461}, 
        {-650, -603, -461}, 
        { 993, -017, -111}, 
        {-489,    8,  871}, 
        {-165,  617, -768}, 
        { 065, -996, -055}, 
    };

    // 0  1  2  3  4  5  6  7  8  9 <-- DaisyChain Index
    // 3  5  9  1  7  4  6  2  8  0 <-- Face / Led Index
    const uint8_t D10LEDIndices[] = {
         3,  5,  9,  1,  7,  4,  6,  2,  8,  0
    };

    // 0  1  2  3  4  5  6  7  8  9 <-- Face / Led Index
    // 9, 3, 7, 0, 5, 1, 6, 4, 8, 2 <-- DaisyChain Index
    const uint8_t D10ElectricalIndices[] = {
         9, 3, 7, 0, 5, 1, 6, 4, 8, 2
    };

    const uint32_t D10Adjacency[] = {
        1 << 3 | 1 << 4 | 1 << 7 | 1 << 8, // 0
        1 << 4 | 1 << 6 | 1 << 7 | 1 << 9,
        1 << 5 | 1 << 6 | 1 << 8 | 1 << 9,
        1 << 0 | 1 << 5 | 1 << 7 | 1 << 8,
        1 << 0 | 1 << 1 | 1 << 6 | 1 << 7,
        1 << 2 | 1 << 3 | 1 << 8 | 1 << 9,
        1 << 1 | 1 << 2 | 1 << 4 | 1 << 9,
        1 << 0 | 1 << 1 | 1 << 3 | 1 << 4,
        1 << 0 | 1 << 2 | 1 << 3 | 1 << 5,
        1 << 1 | 1 << 2 | 1 << 5 | 1 << 6,
    };

    // Animations are defined with the highest face up, so we may need to remap the Face (or LEDs) to
    // match the current face up. This table is used to do this.
    // If we have computed the color of a face in the canonical orientation (highest face up), then we
    // can use the following to figure out which face/led we should actually set to this color.
    // Actual face/led := DXRemap[currentFaceUp * face/led count + canonicalIndex]
    const uint8_t D8Remap[] = {
        7, 2, 1, 4, 3, 6, 5, 0,
        6, 3, 0, 5, 2, 7, 4, 1, 
        5, 4, 7, 6, 1, 0, 3, 2, 
        4, 5, 6, 7, 0, 1, 2, 3, 
        1, 0, 3, 2, 5, 4, 7, 6, 
        2, 7, 4, 1, 6, 3, 0, 5, 
        3, 6, 5, 0, 7, 2, 1, 4, 
        0, 1, 2, 3, 4, 5, 6, 7, 
    };

    // const uint8_t D8ReverseRemap[] = {
    //     7, 2, 1, 4, 3, 6, 5, 0,
    //     2, 7, 4, 1, 6, 3, 0, 5,
    //     5, 4, 7, 6, 1, 0, 3, 2,
    //     4, 5, 6, 7, 0, 1, 2, 3,
    //     1, 0, 3, 2, 5, 4, 7, 6,
    //     6, 3, 0, 5, 2, 7, 4, 1,
    //     3, 6, 5, 0, 7, 2, 1, 4,
    //     0, 1, 2, 3, 4, 5, 6, 7,
    // };

    // Used for determining which face is up and/or for animations that color each face based on their normal
    // Note that the normal is also a good approximation of the position of the LED on the face. In most cases
    // the leds are equidistant from the center.
    // Note: these normals are defined in the canonical face order
    const Core::int3 D8Normals[] = {
        {-921, -198, -333}, // 1
        { 288,  897, -333}, // 2
        {-000,  000,-1000}, // 3
        {-633,  698,  333}, // ...
        { 633, -698, -333}, 
        { 000, -000, 1000}, 
        {-288, -897,  333}, 
        { 921,  198,  333}, 
    };

    //  0, 1, 2, 3, 4, 5, 6, 7  // Daisy Chain Index
    //  2, 0, 3, 1, 7, 5, 4, 6  // LED Index
    const uint8_t D8LEDIndices[] = {
        2, 0, 3, 1, 7, 5, 4, 6
    };

    //  1, 2, 3, 4, 5, 6, 7, 8  // Face Number
    //  0, 1, 2, 3, 4, 5, 6, 7  // Face / LED Index
    //  1, 3, 0, 2, 6, 5, 7, 4  // DaisyChain Index
    const uint8_t D8ElectricalIndices[] = {
        1, 3, 0, 2, 6, 5, 7, 4  // DaisyChain Index
    };

    const uint32_t D8Adjacency[] = {
        1 << 2 | 1 << 3 | 1 << 6, // 1
        1 << 2 | 1 << 3 | 1 << 7,
        1 << 0 | 1 << 1 | 1 << 4,
        1 << 0 | 1 << 1 | 1 << 5,
        1 << 2 | 1 << 6 | 1 << 7,
        1 << 3 | 1 << 6 | 1 << 7,
        1 << 0 | 1 << 4 | 1 << 5,
        1 << 1 | 1 << 4 | 1 << 5,
    };

    const Core::int3 PD6FaceNormals[] = {
        {-1000,  0000,  0000},
        { 0000,  1000,  0000},
        { 0000,  0000,  1000},
        { 0000,  0000, -1000},
        { 0000, -1000,  0000},
        { 1000,  0000,  0000},
    };

    // Used for determining which face is up and/or for animations that color each face based on their normal
    // Note that the normal is also a good approximation of the position of the LED on the face. In most cases
    // the leds are equidistant from the center.
    // Note: these normals are defined in the canonical face order
    const Core::int3 PD6LEDNormals[] = {
        {-1000,     0,     0},

        { -442,  780,   -442},
        {  442,  780,    442},

        { -442,   442,   780},
        {    0,     0,  1000},
        {  442,  -442,   780},

        { -442,  -442,  -780},
        {  442,  -442,  -780},
        {  442,   442,  -780},
        { -442,   442,  -780},

        {  442,  -780,   442},
        {    0, -1000,     0},
        { -442,  -780,   442},
        { -442,  -780,  -442},
        {  442,  -780,  -442},

        {  780,  -442,  -442},
        {  870,  -493,     0},
        {  780,  -442,   442},
        {  780,   442,   442},
        {  870,   493,     0},
        {  780,   442,  -442},
    };

    // 0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // Daisy Chain Index
    // 0,	6,	7,	8,	9,	1,	2,	3,	4,	5, 10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // LED Index
    const uint8_t PD6LEDIndices[] = {
        0,	6,	7,	8,	9,	1,	2,	3,	4,	5, 10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20,
    };

    // 0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // LED Index
    // 0,	5,	6,	7,	8,	9,	1,	2,	3,	4,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // Daisy Chain Index
    const uint8_t PD6ElectricalIndices[] = {
        0,	5,	6,	7,	8,	9,	1,	2,	3,	4,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20,
    };

    // Indicates what face each LED (from its "logical" index) is on
    // 0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // LED Index
    // 0    --1--   ----2----   ------3------   ---------4--------  ----------5-----------  // Face index
    const uint8_t PD6FaceIndices[] = {
        0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5
    };

    const Core::int3 M20FaceNormals[] = {
        // These are incorrect, fix when we have proper M20 design
        {-335,  937, - 92}, // FIXME
        {-352, -930, - 98}, // FIXME
        { 716,  572, -399}, // FIXME
        {-253, -357,  898}, // FIXME
        {-995,    8,   93}, // FIXME
        { 804, -  9,  593}, // FIXME
        {-396,  583, -708}, // FIXME
        { 705, -582, -403}, // FIXME
        { 407,  571,  712}, // FIXME
        { 246, -355, -901}, // FIXME
        {-246,  355,  901}, // FIXME
        {-407, -571, -712}, // FIXME
        {-705,  582,  403}, // FIXME
        { 396, -583,  708}, // FIXME
        {-804,    9, -593}, // FIXME
        { 995, -  8, - 93}, // FIXME
        { 253,  357, -898}, // FIXME
        {-716, -572,  399}, // FIXME
        { 352,  930,   98}, // FIXME
        { 335, -937,   92}, // FIXME
    };
    
    const Core::int3 M20LEDNormals[] = {
        // These are incorrect, fix when we have proper M20 design
        { -446,  850, -276}, // FIXME
        { -447,  525,  723}, // FIXME
        { -447, -850, -276}, // FIXME
        {-1000,  000, -000}, // FIXME
        {  447,  525, -723}, // FIXME
        { -447,  000, -894}, // FIXME
        {  447, -000,  894}, // FIXME
        { -447, -525,  723}, // FIXME
        { 1000, -000,  000}, // FIXME
        {  447,  850,  276}, // FIXME
        {  447, -525, -723}, // FIXME
        {  446, -850,  276}, // FIXME
    };


    const Layout D20Layout = {
        .layoutType = LEDLayoutType::DieLayoutType_D20,
        .faceCount = 20,
        .ledCount = 20,
        .adjacencyCount = 3,
        .faceNormals = D20Normals,
        .ledNormals = D20Normals,
        .faceIndexFromAnimFaceIndexLookup = D20Remap,
        .daisyChainIndexFromLEDIndexLookup = D20ElectricalIndices,
        .LEDIndexFromDaisyChainLookup = D20LEDIndices,
        .faceAdjacencyMap = D20Adjacency,
    };

    const Layout D12Layout = {
        .layoutType = LEDLayoutType::DieLayoutType_D12,
        .faceCount = 12,
        .ledCount = 12,
        .adjacencyCount = 5,
        .faceNormals = D12Normals,
        .ledNormals = D12Normals,
        .faceIndexFromAnimFaceIndexLookup = D12Remap,
        .daisyChainIndexFromLEDIndexLookup = D12ElectricalIndices,
        .LEDIndexFromDaisyChainLookup = D12LEDIndices,
        .faceAdjacencyMap = D12Adjacency,
    };

    const Layout D10Layout = {
        .layoutType = LEDLayoutType::DieLayoutType_D10_D00,
        .faceCount = 10,
        .ledCount = 10,
        .adjacencyCount = 4,
        .faceNormals = D10Normals,
        .ledNormals = D10Normals,
        .faceIndexFromAnimFaceIndexLookup = D10Remap,
        .daisyChainIndexFromLEDIndexLookup = D10ElectricalIndices,
        .LEDIndexFromDaisyChainLookup = D10LEDIndices,
        .faceAdjacencyMap = D10Adjacency,
    };

    const Layout D8Layout = {
        .layoutType = LEDLayoutType::DieLayoutType_D8,
        .faceCount = 8,
        .ledCount = 8,
        .adjacencyCount = 3,
        .faceNormals = D8Normals,
        .ledNormals = D8Normals,
        .faceIndexFromAnimFaceIndexLookup = D8Remap,
        .daisyChainIndexFromLEDIndexLookup = D8ElectricalIndices,
        .LEDIndexFromDaisyChainLookup = D8LEDIndices,
        .faceAdjacencyMap = D8Adjacency,
    };

    const Layout D6Layout = {
        .layoutType = LEDLayoutType::DieLayoutType_D6_FD6,
        .faceCount = 6,
        .ledCount = 6,
        .adjacencyCount = 4,
        .faceNormals = D6Normals,
        .ledNormals = D6Normals,
        .faceIndexFromAnimFaceIndexLookup = D6Remap,
        .daisyChainIndexFromLEDIndexLookup = D6ElectricalIndices,
        .LEDIndexFromDaisyChainLookup = D6LEDIndices,
        .faceAdjacencyMap = D6Adjacency,
    };

    const Layout D4Layout = {
        .layoutType = LEDLayoutType::DieLayoutType_D4,
        .faceCount = 4,
        .ledCount = 6,
        .adjacencyCount = 2,
        .faceNormals = D4FaceNormals,
        .ledNormals = D4LEDNormals,
        .faceIndexFromAnimFaceIndexLookup = D4Remap,
        .daisyChainIndexFromLEDIndexLookup = D4ElectricalIndices,
        .LEDIndexFromDaisyChainLookup = D4LEDIndices,
        .faceAdjacencyMap = D4Adjacency,
    };

    // Die layout information
    const Layout PD6Layout = {
        .layoutType = LEDLayoutType::DieLayoutType_PD6,
        .faceCount = 6,
        .ledCount = 21,
        .adjacencyCount = 4,
        .faceNormals = PD6FaceNormals,
        .ledNormals = PD6LEDNormals,
        .faceIndexFromAnimFaceIndexLookup = D6Remap,
        .daisyChainIndexFromLEDIndexLookup = PD6ElectricalIndices,
        .LEDIndexFromDaisyChainLookup = PD6LEDIndices,
        .faceAdjacencyMap = D6Adjacency,
    };

    const Layout M20Layout = {
        .layoutType = LEDLayoutType::DieLayoutType_M20,
        .faceCount = 20,
        .ledCount = 12,
        .adjacencyCount = 3,
        .faceNormals = M20FaceNormals,
        .ledNormals = M20LEDNormals,
        .faceIndexFromAnimFaceIndexLookup = D20Remap,
        .daisyChainIndexFromLEDIndexLookup = D12ElectricalIndices,
        .LEDIndexFromDaisyChainLookup = D12LEDIndices,
        .faceAdjacencyMap = D20Adjacency,
    };

    // Given a die type, return the matching layout (for normals, face ordering, remapping, etc...)
    LEDLayoutType getLayoutType(DieType dieType) {
        switch (dieType) {
        case DieType::DieType_D4:
            return LEDLayoutType::DieLayoutType_D4;
        case DieType::DieType_D6:
        case DieType::DieType_FD6:
            return LEDLayoutType::DieLayoutType_D6_FD6;
        case DieType::DieType_D8:
            return LEDLayoutType::DieLayoutType_D8;
        case DieType::DieType_D10:
        case DieType::DieType_D00:
            return LEDLayoutType::DieLayoutType_D10_D00;
        case DieType::DieType_D12:
            return LEDLayoutType::DieLayoutType_D12;
        case DieType::DieType_D20:
            return LEDLayoutType::DieLayoutType_D20;
        case DieType::DieType_PD6:
            return LEDLayoutType::DieLayoutType_PD6;
        case DieType::DieType_M20:
            return LEDLayoutType::DieLayoutType_M20;
        default:
            return LEDLayoutType::DieLayoutType_Unknown;
        }
    }

    const Layout* getLayout(LEDLayoutType layoutType) {
        switch (layoutType) {
            case LEDLayoutType::DieLayoutType_D4:
                return &D4Layout;
            case LEDLayoutType::DieLayoutType_D6_FD6:
                return &D6Layout;
            case LEDLayoutType::DieLayoutType_D8:
                return &D8Layout;
            case LEDLayoutType::DieLayoutType_D10_D00:
                return &D10Layout;
            case LEDLayoutType::DieLayoutType_D12:
                return &D12Layout;
            case LEDLayoutType::DieLayoutType_D20:
                return &D20Layout;
            case LEDLayoutType::DieLayoutType_PD6:
                return &PD6Layout;
            case LEDLayoutType::DieLayoutType_M20:
                return &M20Layout;
            default:
                return nullptr;
        }
    }

    DieType estimateDieTypeFromBoard() {
        switch (BoardManager::getBoard()->model) {
            case BoardModel::D20BoardV15:
                return DieType_D20;
            case BoardModel::D6BoardV4:
            case BoardModel::D6BoardV6:
                return DieType_D6;
            case BoardModel::D12BoardV2:
                return DieType_D12;
            case BoardModel::PD6BoardV3:
            case BoardModel::PD6BoardV5:
                return DieType_PD6;
            case BoardModel::D10BoardV2:
                return DieType_D10;
            case BoardModel::D8BoardV2:
                return DieType_D8;
            case BoardModel::Unsupported:
            default:
                return DieType_Unknown;
        }
    }

    // Compute the electrical led index (ie. index in daisy-chain) from the ledIndex,
    // given the led we're trying to set and the remap face.
    int Layout::daisyChainIndexFromLEDIndex(int LEDIndex) const {
        return daisyChainIndexFromLEDIndexLookup[LEDIndex];
    }

    int Layout::LEDIndexFromDaisyChainIndex(int daisyChainIndex) const {
        return LEDIndexFromDaisyChainLookup[daisyChainIndex];
    }

    int Layout::remapFaceIndexBasedOnUpFace(int upFace, int faceIndex) const {
        return faceIndexFromAnimFaceIndexLookup[upFace * faceCount + faceIndex];
    }

    int Layout::faceIndicesFromLEDIndex(int ledIndex, int outFaces[]) const {
        switch (layoutType) {
            case LEDLayoutType::DieLayoutType_D6_FD6:
            case LEDLayoutType::DieLayoutType_D8:
            case LEDLayoutType::DieLayoutType_D10_D00:
            case LEDLayoutType::DieLayoutType_D12:
            case LEDLayoutType::DieLayoutType_D20:
                // For all these, led index == face index
                outFaces[0] = ledIndex;
                return 1;
            case LEDLayoutType::DieLayoutType_PD6:
                outFaces[0] = PD6FaceIndices[ledIndex];
                return 1;
            case LEDLayoutType::DieLayoutType_D4:
                if (ledIndex == 4 || ledIndex == 5) {
                    outFaces[0] = 0;
                    outFaces[1] = 1;
                    outFaces[2] = 2;
                    outFaces[3] = 3;
                    return 4;
                } else {
                    outFaces[0] = ledIndex;
                    return 1;
                }
            case LEDLayoutType::DieLayoutType_M20:
                // FIXME!!!
                outFaces[0] = 0;
                outFaces[1] = 1;
                outFaces[2] = 2;
                outFaces[3] = 3;
                outFaces[4] = 4;
                return 5;
            default:
                return 0;
        }
    }


    uint32_t Layout::getTopFaceMask() const {
        switch (layoutType) {
            case LEDLayoutType::DieLayoutType_D4:
            case LEDLayoutType::DieLayoutType_D6_FD6:
            case LEDLayoutType::DieLayoutType_D8:
            case LEDLayoutType::DieLayoutType_D10_D00:
            case LEDLayoutType::DieLayoutType_D12:
            case LEDLayoutType::DieLayoutType_D20:
                return 1 << getTopFace();
            case LEDLayoutType::DieLayoutType_PD6:
                return 0b111111 << 15;
            default:
                return 0xFFFFFFFF;
        }
   }

    uint8_t Layout::getTopFace() const {
        switch (layoutType) {
            case LEDLayoutType::DieLayoutType_D4:
                return 3;
            case LEDLayoutType::DieLayoutType_D6_FD6:
            case LEDLayoutType::DieLayoutType_PD6:
                return 5;
            case LEDLayoutType::DieLayoutType_D8:
                return 7;
            case LEDLayoutType::DieLayoutType_D10_D00:
                return 0;
            case LEDLayoutType::DieLayoutType_D12:
                return 11;
            case LEDLayoutType::DieLayoutType_D20:
                return 19;
            default:
                return 0;
        }
    }

    uint8_t Layout::getAdjacentFaces(uint8_t face, uint8_t retFaces[]) const {
        uint32_t adj = faceAdjacencyMap[face];
        uint8_t count = 0;
        for (int i = 0; i < faceCount; i++) {
            if (adj & (1 << i)) {
                retFaces[count++] = i;
            }
        }
        assert(count == adjacencyCount);
        return count;
    }
}
}