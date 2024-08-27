#include "dice_variants.h"
#include "board_config.h"

using namespace Core;

namespace Config
{
namespace DiceVariants
{
    const uint8_t sixSidedRemap[] = {
        5, 2, 1, 4, 3, 0,
        4, 0, 2, 3, 5, 1,
        3, 4, 5, 0, 1, 2,
        2, 5, 4, 1, 0, 3,
        1, 2, 0, 5, 3, 4, 
        0, 1, 2, 3, 4, 5, 
    };

    const Core::int3 sixSidedNormals[] = {
        {-1000,  0000,  0000},
        { 0000, -1000,  0000},
        { 0000,  0000, -1000},
        { 0000,  0000,  1000},
        { 0000,  1000,  0000},
        { 1000,  0000,  0000},
    };

    // 0, 1, 2, 3, 4, 5 <-- Led index
    // 1, 5, 2, 6, 4, 3 <-- face number
    // 0, 4, 1, 5, 3, 2 <-- face index
    const uint8_t sixSidedFaceToLedLookup[] = {
        0, 2, 5, 4, 1, 3,   // Led Index
    };

    const uint32_t sixSidedAdjacency[] = {
        1 << 1 | 1 << 2 | 1 << 3 | 1 << 4, // 1
        1 << 0 | 1 << 2 | 1 << 3 | 1 << 5,
        1 << 0 | 1 << 1 | 1 << 4 | 1 << 5,
        1 << 0 | 1 << 1 | 1 << 4 | 1 << 5,
        1 << 0 | 1 << 2 | 1 << 3 | 1 << 5,
        1 << 1 | 1 << 2 | 1 << 3 | 1 << 4,
    };

    const uint8_t twentySidedRemap[] = {
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

    const uint8_t twentySidedFaceToLedLookup[] = {
    //   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  // Face number (face index+1)
    //   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  // Face index
            9,  5, 18,  3,  0, 12,  8, 15, 11, 16,  2,  6,  1, 13,  7, 19, 17,  4, 10, 14
    // Old Molds:
    //   9, 13,  7, 19, 11, 16,  1,  5, 17,  4, 18,  3, 10, 15,  2,  6,  0, 12,  8, 14
    };

    // LED number
    //  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
    //  4, 12, 10,  3, 17,  1, 11, 14,  6,  0, 18,  8,  5, 13, 19,  7,  9, 16,  2, 15

    const Core::int3 twentySidedNormals[] = {
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

    const Core::int3 M20Normals[] {
        {-796,  575,  188},
        { 879,   75, -472},
        {-337, -188,  922},
        { -80, -391, -917},
        { -45,  738, -674},
        {-260, -963,   70},
        {-204,  863,  463},
        { 698, -498,  515},
        {-831, -553,  -55},
        { 720,  200,  665},
        {-720, -200, -665},
        { 831,  553,   55},
        {-698,  498, -515},
        { 204, -863, -463},
        { 260,  963,  -70},
        {  45, -738,  674},
        {  80,  391,  917},
        { 337,  188, -922},
        {-879,  -75,  472},
        { 796, -575, -188},
    };

    const uint32_t twentySidedAdjacency[] = {
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

    const uint8_t twelveSidedRemap[] = {
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

    const Core::int3 twelveSidedNormals[] = {
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

    // 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11 <-- Led Index
    // 3,  7,  1,  0,  5,  2, 11,  6,  9,  4, 10,  8 <-- Face index
    const uint8_t twelveSidedFaceToLedLookup[] = {
        3, 2, 5, 0, 9, 4, 7, 1, 11, 8, 10, 6
    };

    const uint32_t twelveSidedAdjacency[] = {
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

    const uint8_t tenSidedRemap[] = {
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

    const Core::int3 tenSidedNormals[] = {
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

    // 0  1  2  3  4  5  6  7  8  9 <-- LED
    // 3  5  9  1  7  4  6  2  8  0 <-- Face / Index
    const uint8_t tenSidedFaceToLedLookup[] = {
         9, 3, 7, 0, 5, 1, 6, 4, 8, 2
    };

    const uint32_t tenSidedAdjacency[] = {
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

    const uint8_t eightSidedRemap[] = {
        7, 2, 1, 4, 3, 6, 5, 0,
        6, 3, 0, 5, 2, 7, 4, 1, 
        5, 4, 7, 6, 1, 0, 3, 2, 
        4, 5, 6, 7, 0, 1, 2, 3, 
        1, 0, 3, 2, 5, 4, 7, 6, 
        2, 7, 4, 1, 6, 3, 0, 5, 
        3, 6, 5, 0, 7, 2, 1, 4, 
        0, 1, 2, 3, 4, 5, 6, 7, 
    };

    const Core::int3 eightSidedNormals[] = {
        {-921, -198, -333}, // 1
        { 288,  897, -333}, // 2
        {-000,  000,-1000}, // 3
        {-633,  698,  333}, // ...
        { 633, -698, -333}, 
        { 000, -000, 1000}, 
        {-288, -897,  333}, 
        { 921,  198,  333}, 
    };

    const uint8_t eightSidedFaceToLedLookup[] = {
    //  1, 2, 3, 4, 5, 6, 7, 8  // Face Number
    //  0, 1, 2, 3, 4, 5, 6, 7  // Face Index
        1, 3, 0, 2, 6, 5, 7, 4  // Led Index
    };

    const uint8_t eightSidedLedToFaceLookup[] = {
    //  0, 1, 2, 3, 4, 5, 6, 7  // Led Index
    //  3, 1, 4, 2, 8, 6, 5, 7  // Face Number
        2, 0, 3, 1, 7, 5, 4, 6  // Face Index
    };

    const uint32_t eightSidedAdjacency[] = {
        1 << 2 | 1 << 3 | 1 << 6, // 1
        1 << 2 | 1 << 3 | 1 << 7,
        1 << 0 | 1 << 1 | 1 << 4,
        1 << 0 | 1 << 1 | 1 << 5,
        1 << 2 | 1 << 6 | 1 << 7,
        1 << 3 | 1 << 6 | 1 << 7,
        1 << 0 | 1 << 4 | 1 << 5,
        1 << 1 | 1 << 4 | 1 << 5,
    };

    const Core::int3 pippedD6Normals[] = {
        {-1000,  0000,  0000},
        { 0000,  1000,  0000},
        { 0000,  0000,  1000},
        { 0000,  0000, -1000},
        { 0000, -1000,  0000},
        { 1000,  0000,  0000},
    };

    const uint8_t pippedD6Remap[] = {
        // FIXME!!!
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // face 1
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // face 2
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // face 3
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20, // ...
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20,
        0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20,
    //  1   ==2==   ====3====   ======4======   ========5=========  ===========6==========
    };

    const uint8_t pippedD6FaceToLedLookup[] = {
    //  0   --1--   ----2----   ------3------   ---------5--------  ----------6----------- // Face index
    //  0   0   1   0   1   2   0   1   2   3    0   1   2   3   4   0   1   2   3   4   5 // Led index in face
        0,	5,	6,	7,	8,	9,	1,	2,	3,	4,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20,
    };

    const Layout D20Layout = {
        .baseNormals = twentySidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = twentySidedRemap,
        .canonicalIndexToElectricalIndexLookup = twentySidedFaceToLedLookup,
        .adjacencyMap = twentySidedAdjacency,
        .faceCount = 20,
        .ledCount = 20,
        .adjacencyCount = 3,
    };

    const Layout D12Layout = {
        .baseNormals = twelveSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = twelveSidedRemap,
        .canonicalIndexToElectricalIndexLookup = twelveSidedFaceToLedLookup,
        .adjacencyMap = twelveSidedAdjacency,
        .faceCount = 12,
        .ledCount = 12,
        .adjacencyCount = 5,
    };

    const Layout D10Layout = {
        .baseNormals = tenSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = tenSidedRemap,
        .canonicalIndexToElectricalIndexLookup = tenSidedFaceToLedLookup,
        .adjacencyMap = tenSidedAdjacency,
        .faceCount = 10,
        .ledCount = 10,
        .adjacencyCount = 4,
    };

    const Layout D8Layout = {
        .baseNormals = eightSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = eightSidedRemap,
        .canonicalIndexToElectricalIndexLookup = eightSidedFaceToLedLookup,
        .adjacencyMap = eightSidedAdjacency,
        .faceCount = 8,
        .ledCount = 8,
        .adjacencyCount = 3,
    };

    const Layout D6Layout = {
        .baseNormals = sixSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = sixSidedRemap,
        .canonicalIndexToElectricalIndexLookup = sixSidedFaceToLedLookup,
        .adjacencyMap = sixSidedAdjacency,
        .faceCount = 6,
        .ledCount = 6,
        .adjacencyCount = 4,
    };

    // D4 is using D6 layout
    // const Layout D4Layout = {
    //     .baseNormals = sixSidedNormals,
    //     .canonicalIndexFaceToFaceRemapLookup = sixSidedRemap,
    //     .canonicalIndexToElectricalIndexLookup = sixSidedFaceToLedLookup,
    //     .adjacencyMap = sixSidedAdjacency,
    //     .faceCount = 4,
    //     .ledCount = 4,
    //     .adjacencyCount = 4,
    // };

    // Die layout information
    const Layout PD6Layout = {
        .baseNormals = pippedD6Normals,
        .canonicalIndexFaceToFaceRemapLookup = pippedD6Remap,
        .canonicalIndexToElectricalIndexLookup = pippedD6FaceToLedLookup,
        .adjacencyMap = sixSidedAdjacency,
        .faceCount = 6,
        .ledCount = 21,
        .adjacencyCount = 4,
    };

    const Layout FateLayout = {
        .baseNormals = sixSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = sixSidedRemap,
        .canonicalIndexToElectricalIndexLookup = sixSidedFaceToLedLookup,
        .adjacencyMap = sixSidedAdjacency,
        .faceCount = 6,
        .ledCount = 6,
        .adjacencyCount = 4,
    };

    const Layout* getLayout() {
        switch (BoardManager::getBoard()->model) {
        case BoardModel::D20BoardV15:
            return &D20Layout;
        case BoardModel::D6BoardV4:
        case BoardModel::D6BoardV6:
            return &D6Layout;
        case BoardModel::D12BoardV2:
            return &D12Layout;
        case BoardModel::PD6BoardV3:
        case BoardModel::PD6BoardV5:
            return &PD6Layout;
        case BoardModel::D10BoardV2:
            return &D10Layout;
        case BoardModel::D8BoardV2:
            return &D8Layout;
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
                break;
        }
    }


    uint8_t animIndexToLEDIndex(int animLEDIndex, int remapFace) {
        // The transformation is:
        // animFaceIndex (what face the animation says it wants to light up)
        //	-> rotatedAnimFaceIndex (based on remapFace and remapping table, i.e. what actual
        //	   face should light up to "retarget" the animation around the current up face)
        //		-> ledIndex (based on pcb face to led mapping, i.e. to account for the internal rotation
        //		   of the PCB and the fact that the LEDs are not accessed in the same order as the number of the faces)
        int rotatedAnimFaceIndex = getLayout()->canonicalIndexFaceToFaceRemapLookup[remapFace * getLayout()->ledCount + animLEDIndex];
        return getLayout()->canonicalIndexToElectricalIndexLookup[rotatedAnimFaceIndex];
    }

    uint32_t getTopFaceMask() {
        switch (BoardManager::getBoard()->model) {
            case BoardModel::D20BoardV15:
                return 1 << 19;
            case BoardModel::D6BoardV4:
            case BoardModel::D6BoardV6:
                return 1 << 5;
            case BoardModel::D12BoardV2:
                return 1 << 11;
            case BoardModel::PD6BoardV3:
            case BoardModel::PD6BoardV5:
                return 0b111111 << 15;
            case BoardModel::D10BoardV2:
                return 1;
            case BoardModel::D8BoardV2:
                return 1 << 7;
            default:
                return 0xFFFFFFFF;
        }
   }

    uint8_t getTopFace() {
        switch (BoardManager::getBoard()->model) {
            case BoardModel::D20BoardV15:
                return 19;
            case BoardModel::D6BoardV4:
            case BoardModel::D6BoardV6:
                return 5;
            case BoardModel::D12BoardV2:
                return 11;
            case BoardModel::PD6BoardV3:
            case BoardModel::PD6BoardV5:
                return 5;
            case BoardModel::D10BoardV2:
                return 0;
            case BoardModel::D8BoardV2:
                return 7;
            default:
                return 0;
        }
    }

    uint8_t getFaceForLEDIndex(int ledIndex) {
        switch (BoardManager::getBoard()->model) {
            case BoardModel::PD6BoardV3:
            case BoardModel::PD6BoardV5:
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

    uint8_t getAdjacentFaces(uint8_t face, uint8_t retFaces[]) {
        uint32_t adj = getLayout()->adjacencyMap[face];
        uint8_t count = 0;
        for (int i = 0; i < getLayout()->faceCount; i++) {
            if (adj & (1 << i)) {
                retFaces[count++] = i;
            }
        }
        return count;
    }

}
}