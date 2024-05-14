#include "dice_variants.h"
#include "board_config.h"
#include "string.h"

using namespace Core;

#define MAX_COUNT 22		// Max LED count so far is 21 (on PD6)
                            // but we want room for one more 'fake' LED to test LED return
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
        3, 2, 1, 0,
        2, 3, 0, 1,
        1, 0, 3, 2,
        0, 1, 2, 3,
    };

    // Used for determining which face is up and/or for animations that color each face based on their normal
    // Note that the normal is also a good approximation of the position of the LED on the face. In most cases
    // the leds are equidistant from the center.
    // Note: these normals are defined in the canonical face order
    const Core::int3 D4Normals[] = {
        {-1000,  0000,  0000},
        { 0000,  0000,  1000},
        { 0000,  0000, -1000},
        { 1000,  0000,  0000},
    };

    // 0, 1, 2, 3, 4, 5 <-- daisy chain index
    // 1, 5, 2, 6, 4, 3 <-- face number (6-sided)
    // 1, -, -, 4, 2, 3 <-- face number (4-sided)
    // 0, -, -, 3, 1, 2 <-- face/led index (4-sided)

    // 0, 1, 2, 3 <-- face/led Index
    // 0, 3, 1, 2 <-- daisy chain index

    const uint8_t D4ElectricalIndices[] = {
        0, 3, 1, 2,   // Led Index
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

    // 0, 1, 2, 3, 4, 5 <-- led index
    // 0, 2, 5, 4, 1, 3 <-- DaisyChain Index

    const uint8_t D6ElectricalIndices[] = {
        0, 2, 5, 4, 1, 3,   // Led Index
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

    //  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 <-- daisy chain index
    //  4, 12, 10,  3, 17,  1, 11, 14,  6,  0, 18,  8,  5, 13, 19,  7,  9, 16,  2, 15 <-- face/led Index

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
        7, 8, 9, 4, 3, 6, 5, 8, 1, 2, 
        8, 7, 6, 5, 0, 9, 4, 3, 2, 1,
        9, 4, 3, 2, 1, 8, 7, 6, 5, 0,
    };

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

    // Animations are defined with the highest face up, so we may need to remap the Face (or LEDs) to
    // match the current face up. This table is used to do this.
    // If we have computed the color of a face in the canonical orientation (highest face up), then we
    // can use the following to figure out which face/led we should actually set to this color.
    // Actual face/led := DXRemap[currentFaceUp * face/led count + canonicalIndex]
    const uint8_t PD6FaceRemap[] = {
        5, 2, 1, 4, 3, 0,
        4, 0, 2, 3, 5, 1,
        3, 4, 5, 0, 1, 2,
        2, 5, 4, 1, 0, 3,
        1, 2, 0, 5, 3, 4, 
        0, 1, 2, 3, 4, 5, 
    };

    // Used for determining which face is up and/or for animations that color each face based on their normal
    // Note that the normal is also a good approximation of the position of the LED on the face. In most cases
    // the leds are equidistant from the center.
    // Note: these normals are defined in the canonical face order
    const Core::int3 PD6LEDNormals[] = {
        { 1000,     0,     0},
        {  442,  -442,  -780},
        { -442,  -442,  -780},
        { -442,   442,  -780},
        {  442,   780,  -442},
        { -442,   780,   442},
        {  442,   442,   780},
        {    0,     0,  1000},
        { -442,  -442,   780},
        { -442,  -780,   442},
        {    0, -1000,     0},
        {  442,  -780,   442},
        {  442,  -780,  -442},
        { -442,  -780,  -442},
        { -780,  -442,  -442},
        { -870,  -493,     0},
        { -780,  -442,   442},
        { -780,   442,   442},
        { -870,   493,     0},
        { -780,   442,  -442},
    };

    const uint8_t PD6LEDRemap[] = {
        // FIXME!!!
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // face 1
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // face 2
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // face 3
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // ...
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20,
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20,
    //  1   ==2==   ====3====   ======4======   ========5=========  ===========6==========
    };

    const uint8_t PD6ElectricalIndices[] = {
    //  0   --1--   ----2----   ------3------   ---------5--------  ----------6----------- // Face index
    //  0   0   1   0   1   2   0   1   2   3    0   1   2   3   4   0   1   2   3   4   5 // Led index in face
        0,	5,	6,	7,	8,	9,	1,	2,	3,	4,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20,
    };

    const Layout D20Layout = {
        .layoutType = DieLayoutType::DieLayoutType_D20,
        .faceCount = 20,
        .ledCount = 20,
        .adjacencyCount = 3,
        .faceNormals = D20Normals,
        .ledNormals = D20Normals,
        .canonicalIndexFaceToFaceRemapLookup = D20Remap,
        .daisyChainIndexFromLEDIndexLookup = D20ElectricalIndices,
        .adjacencyMap = D20Adjacency,
    };

    const Layout D12Layout = {
        .layoutType = DieLayoutType::DieLayoutType_D12,
        .faceCount = 12,
        .ledCount = 12,
        .adjacencyCount = 5,
        .faceNormals = D12Normals,
        .ledNormals = D12Normals,
        .canonicalIndexFaceToFaceRemapLookup = D12Remap,
        .daisyChainIndexFromLEDIndexLookup = D12ElectricalIndices,
        .adjacencyMap = D12Adjacency,
    };

    const Layout D10Layout = {
        .layoutType = DieLayoutType::DieLayoutType_D10_D00,
        .faceCount = 10,
        .ledCount = 10,
        .adjacencyCount = 4,
        .faceNormals = D10Normals,
        .ledNormals = D10Normals,
        .canonicalIndexFaceToFaceRemapLookup = D10Remap,
        .daisyChainIndexFromLEDIndexLookup = D10ElectricalIndices,
        .adjacencyMap = D10Adjacency,
    };

    const Layout D8Layout = {
        .layoutType = DieLayoutType::DieLayoutType_D8,
        .faceCount = 8,
        .ledCount = 8,
        .adjacencyCount = 3,
        .faceNormals = D8Normals,
        .ledNormals = D8Normals,
        .canonicalIndexFaceToFaceRemapLookup = D8Remap,
        .daisyChainIndexFromLEDIndexLookup = D8ElectricalIndices,
        .adjacencyMap = D8Adjacency,
    };

    const Layout D6Layout = {
        .layoutType = DieLayoutType::DieLayoutType_D6_FD6,
        .faceCount = 6,
        .ledCount = 6,
        .adjacencyCount = 4,
        .faceNormals = D6Normals,
        .ledNormals = D6Normals,
        .canonicalIndexFaceToFaceRemapLookup = D6Remap,
        .daisyChainIndexFromLEDIndexLookup = D6ElectricalIndices,
        .adjacencyMap = D6Adjacency,
    };

    const Layout D4Layout = {
        .layoutType = DieLayoutType::DieLayoutType_D4,
        .faceCount = 4,
        .ledCount = 4,
        .adjacencyCount = 2,
        .faceNormals = D4Normals,
        .ledNormals = D4Normals,
        .canonicalIndexFaceToFaceRemapLookup = D4Remap,
        .daisyChainIndexFromLEDIndexLookup = D4ElectricalIndices,
        .adjacencyMap = D4Adjacency,
    };

    // Die layout information
    const Layout PD6Layout = {
        .layoutType = DieLayoutType::DieLayoutType_PD6,
        .faceCount = 6,
        .ledCount = 21,
        .adjacencyCount = 4,
        .faceNormals = PD6FaceNormals,
        .ledNormals = PD6LEDNormals,
        .canonicalIndexFaceToFaceRemapLookup = PD6FaceRemap,
        .daisyChainIndexFromLEDIndexLookup = PD6ElectricalIndices,
        .adjacencyMap = D6Adjacency,
    };

    // Given a die type, return the matching layout (for normals, face ordering, remapping, etc...)
    DieLayoutType getLayoutType(DieType dieType) {
        switch (dieType) {
        case DieType::DieType_D4:
            return DieLayoutType::DieLayoutType_D4;
        case DieType::DieType_D6:
        case DieType::DieType_FD6:
            return DieLayoutType::DieLayoutType_D6_FD6;
        case DieType::DieType_D8:
            return DieLayoutType::DieLayoutType_D8;
        case DieType::DieType_D10:
        case DieType::DieType_D00:
            return DieLayoutType::DieLayoutType_D10_D00;
        case DieType::DieType_D12:
            return DieLayoutType::DieLayoutType_D12;
        case DieType::DieType_D20:
            return DieLayoutType::DieLayoutType_D20;
        case DieType::DieType_PD6:
            return DieLayoutType::DieLayoutType_PD6;
        default:
            return DieLayoutType::DieLayoutType_Unknown;
        }
    }

    const Layout* getLayout(DieLayoutType layoutType) {
        switch (layoutType) {
            case DieLayoutType::DieLayoutType_D4:
                return &D4Layout;
            case DieLayoutType::DieLayoutType_D6_FD6:
                return &D6Layout;
            case DieLayoutType::DieLayoutType_D8:
                return &D8Layout;
            case DieLayoutType::DieLayoutType_D10_D00:
                return &D10Layout;
            case DieLayoutType::DieLayoutType_D12:
                return &D12Layout;
            case DieLayoutType::DieLayoutType_D20:
                return &D20Layout;
            case DieLayoutType::DieLayoutType_PD6:
                return &PD6Layout;
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

    int Layout::faceIndexFromLEDIndex(int ledIndex) const {
        switch (layoutType) {
            case DieLayoutType::DieLayoutType_PD6:
                //  0   --1--   ----2----   ------3------   ---------4--------  ----------5----------- // Face index
                //  0   0   1   0   1   2   0   1   2   3    0   1   2   3   4   0   1   2   3   4   5 // Led index in face
                //  0,	5,	6,	7,	8,	9,	1,	2,	3,	4,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20,
               if (ledIndex == 0) {
                return 0;
               } else if (ledIndex <= 4) {
                return 3;
               } else if (ledIndex <= 6) {
                return 1;
               } else if (ledIndex <= 9) {
                return 2;
               } else if (ledIndex <= 14) {
                return 4;
               } else {
                return 5;
               }
            default:
                return ledIndex;
        }
    }

    // Mutates the passed in color array to remap face colors to led Colors
    void Layout::ledColorsFromFaceColors(uint32_t const faceColors[], uint32_t retLedColors[]) const {
        switch (layoutType)
        {
        case DieLayoutType::DieLayoutType_PD6:
            for (int i = 0; i < ledCount; i++) {
                retLedColors[i] = faceColors[faceIndexFromLEDIndex(i)];
            }
            break;
        default:
            // Nothing to do, led indices are the same as face indices
            break;
        }
    }


    // Compute the canonical led index to grab color from in an animation,
    // given the led we're trying to set and the remap face.
    int Layout::animIndexFromLEDIndex(int ledIndex, int remapFace) const {
        return canonicalIndexFaceToFaceRemapLookup[remapFace * ledCount + ledIndex];
    }

    // Compute the electrical led index (ie. index in daisy-chain) from the ledIndex,
    // given the led we're trying to set and the remap face.
    int Layout::daisyChainIndexFromLEDIndex(int LEDIndex) const {
        return daisyChainIndexFromLEDIndexLookup[LEDIndex];
    }

    uint32_t Layout::getTopFaceMask() const {
        switch (layoutType) {
            case DieLayoutType::DieLayoutType_D4:
            case DieLayoutType::DieLayoutType_D6_FD6:
            case DieLayoutType::DieLayoutType_D8:
            case DieLayoutType::DieLayoutType_D10_D00:
            case DieLayoutType::DieLayoutType_D12:
            case DieLayoutType::DieLayoutType_D20:
                return 1 << getTopFace();
            case DieLayoutType::DieLayoutType_PD6:
                return 0b111111 << 15;
            default:
                return 0xFFFFFFFF;
        }
   }

    uint8_t Layout::getTopFace() const {
        switch (layoutType) {
            case DieLayoutType::DieLayoutType_D4:
                return 3;
            case DieLayoutType::DieLayoutType_D6_FD6:
                return 5;
            case DieLayoutType::DieLayoutType_D8:
                return 7;
            case DieLayoutType::DieLayoutType_D10_D00:
                return 0;
            case DieLayoutType::DieLayoutType_D12:
                return 11;
            case DieLayoutType::DieLayoutType_D20:
                return 19;
            case DieLayoutType::DieLayoutType_PD6:
                return 5;
            default:
                return 0;
        }
    }

    uint8_t Layout::getAdjacentFaces(uint8_t face, uint8_t retFaces[]) const {
        uint32_t adj = adjacencyMap[face];
        uint8_t count = 0;
        for (int i = 0; i < faceCount; i++) {
            if (adj & (1 << i)) {
                retFaces[count++] = i;
            }
        }
        return count;
    }

}
}