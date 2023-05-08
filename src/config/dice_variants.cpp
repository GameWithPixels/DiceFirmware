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

    const Core::float3 sixSidedNormals[] = {
        { 0, -1,  0},
        { 0,  0,  1},
        { 1,  0,  0},
        {-1,  0,  0},
        { 0,  0, -1},
        { 0,  1,  0}
    };

    const uint8_t sixSidedFaceToLedLookup[] = {
    //  1, 2, 3, 4, 5, 6    // Face Number
    //  0, 1, 2, 3, 4, 5    // Face Index
        0, 4, 2, 1, 5, 3,   // Led Index
    };


    const uint8_t sixSidedLedToFaceLookup[] = {
        // Smooth LED order is 0, 5, 1, 4, 2, 3
        0, 4, 3, 1, 2, 5
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

    const uint8_t twentySidedLedToFaceLookup[] = {
            4, 12, 10,  3, 17,  1, 11, 14,  6,  0, 18,  8,  5, 13, 19,  7,  9, 16,  2, 15,
    };


    // LED number
    //  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
    //  4, 12, 10,  3, 17,  1, 11, 14,  6,  0, 18,  8,  5, 13, 19,  7,  9, 16,  2, 15

    const Core::float3 twentySidedNormals[] = {
        {-0.3351f,  0.9377f, -0.0921f},
        {-0.3525f, -0.9306f, -0.0989f},
        { 0.7162f,  0.5722f, -0.3996f},
        {-0.2534f, -0.3578f,  0.8988f},
        {-0.9956f,  0.0089f,  0.0937f},
        { 0.8045f, -0.0096f,  0.5939f},
        {-0.3963f,  0.5837f, -0.7087f},
        { 0.7055f, -0.5825f, -0.4038f},
        { 0.4070f,  0.5710f,  0.7129f},
        { 0.2468f, -0.3558f, -0.9014f},
        {-0.2468f,  0.3558f,  0.9014f},
        {-0.4070f, -0.5710f, -0.7129f},
        {-0.7055f,  0.5825f,  0.4038f},
        { 0.3963f, -0.5837f,  0.7087f},
        {-0.8045f,  0.0096f, -0.5939f},
        { 0.9956f, -0.0089f, -0.0937f},
        { 0.2534f,  0.3578f, -0.8988f},
        {-0.7162f, -0.5722f,  0.3996f},
        { 0.3525f,  0.9306f,  0.0989f},
        { 0.3351f, -0.9377f,  0.0921f},
    };

    const uint8_t twelveSidedRemap[] = {
        11, 9, 10, 7, 8, 6, 5, 3, 4, 1, 2, 0,
        10, 8, 11, 4, 9, 5, 6, 2, 7, 0, 3, 1,
        9, 11, 7, 10, 3, 5, 6, 1, 8, 0, 4, 2,
        8, 10, 4, 11, 2, 6, 5, 9, 0, 7, 1, 3,
        7, 3, 9, 1, 11, 6, 5, 0, 10, 2, 8, 4,
        6, 2, 1, 8, 7, 11, 0, 4, 3, 10, 9, 5,
        5, 10, 9, 4, 3, 0, 11, 7, 8, 1, 2, 6,
        4, 2, 8, 0, 10, 5, 6, 1, 11, 3, 9, 7,
        3, 7, 1, 9, 0, 5, 6, 11, 2, 10, 4, 8,
        2, 4, 0, 8, 1, 6, 5, 10, 3, 11, 7, 9,
        1, 0, 3, 2, 7, 6, 5, 4, 9, 8, 11, 10,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    };

    const Core::float3 twelveSidedNormals[] = {
        { 0.8509f, -0.2764f, -0.4468f},     // 1
        { 0.0004f, -0.0003f, -1.0000f},     // 2
        {-0.5258f, -0.7235f,  0.4472f},     // 3
        { 0.0003f, -0.8945f, -0.4470f},     // ...
        { 0.8504f,  0.2766f,  0.4475f},    
        { 0.5256f, -0.7234f,  0.4476f},    
        {-0.5256f,  0.7234f, -0.4476f},    
        {-0.8504f, -0.2766f, -0.4475f},    
        {-0.0003f,  0.8945f,  0.4470f},    
        { 0.5258f,  0.7235f, -0.4472f},       
        {-0.0004f,  0.0003f,  1.0000f},     
        {-0.8509f,  0.2764f,  0.4468f},     
    };

    const uint8_t twelveSidedFaceToLedLookup[] = {
    //   1,  2,  3,  4,  5,  6,  7.  8,  9, 10, 11, 12  // Face Number
    //   0,  1,  2,  3,  4,  5,  6,  7.  8,  9, 10, 11  // Face Index
            3,  0, 10,  4,  8,  9,  1,  5,  7,  2, 11,  6 // Led Index
    };

    const uint8_t twelveSidedLedToFaceLookup[] = {
    //   0,  1,  2,  3,  4,  5,  6,  7.  8,  9, 10, 11  // Led Index
    //   2,  7, 10,  1,  4,  8, 12,  9,  5,  6,  3, 11  // Face Number
            1,  6,  9,  0,  3,  7, 11,  8,  4,  5,  2, 10, // Face Index
    };


    const uint8_t tenSidedRemap[] = {
        9, 4, 3, 2, 1, 8, 7, 6, 5, 0,
        8, 7, 6, 5, 0, 9, 4, 3, 2, 1,
        7, 8, 9, 4, 3, 6, 5, 8, 1, 2, 
        6, 5, 0, 1, 2, 7, 8, 9, 4, 3, 
        5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 
        4, 9, 8, 7, 6, 3, 2, 1, 0, 5, 
        3, 2, 1, 0, 5, 4, 9, 8, 7, 6, 
        2, 3, 4, 9, 8, 1, 0, 5, 6, 7, 
        1, 0, 5, 6, 7, 2, 3, 4, 9, 8, 
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 
    };

    const Core::float3 tenSidedNormals[] = {
        { 0.9963f,  0.0551f, -0.0658f}, // 00
        {-0.6179f,  0.7687f,  0.1650f}, // 10
        {-0.0085f, -0.8718f,  0.4897f}, // 20
        { 0.0174f,  0.1117f, -0.9936f}, // ...
        { 0.6037f,  0.4612f,  0.6503f}, 
        {-0.6037f, -0.4612f, -0.6503f}, 
        {-0.0174f, -0.1117f,  0.9936f}, 
        { 0.0085f,  0.8718f, -0.4897f}, 
        { 0.6179f, -0.7687f, -0.1650f}, 
        {-0.9963f, -0.0551f,  0.0658f}, 
    };

    const uint8_t tenSidedFaceToLedLookup[] = {
    //   1,  2,  3,  4,  5,  6,  7,  8,  9,  0  // Face Number
    //   0,  1,  2,  3,  4,  5,  6,  7,  8,  9  // Face Index
            5, 0, 7, 3, 8, 4, 6, 1, 9, 2
    };

    const uint8_t tenSidedLedToFaceLookup[] = {
    //   0,  1,  2,  3,  4,  5,  6,  7,  8,  9  // Led Index
    //   2,  8,  0,  4,  6,  1,  7,  3,  5,  9  // Face Number
            1,  7,  9,  3,  5,  0,  6,  2,  4,  8  // Face Index (Face Number 0 is in fact 10)
    };


    const uint8_t eightSidedRemap[] = {
        7, 2, 1, 4, 3, 6, 5, 0,
        6, 3, 0, 5, 2, 7, 4, 1, 
        5, 4, 7, 6, 1, 0, 3, 2, 
        4, 5, 6, 7, 0, 1, 2, 3, 
        3, 6, 5, 0, 7, 2, 1, 4, 
        2, 7, 4, 1, 6, 3, 0, 5, 
        1, 0, 3, 2, 5, 4, 7, 6, 
        0, 1, 2, 3, 4, 5, 6, 7, 
    };

    const Core::float3 eightSidedNormals[] = {
        {-0.1988f, -0.3333f, -0.9216f}, // 1
        { 0.8976f, -0.3332f,  0.2885f}, // 2
        { 0.0002f, -1.0000f, -0.0001f}, // 3
        { 0.6986f,  0.3335f, -0.6330f}, // ...
        {-0.8976f,  0.3332f, -0.2885f}, 
        {-0.0002f,  1.0000f,  0.0001f}, 
        {-0.6986f, -0.3335f,  0.6330f}, 
        { 0.1988f,  0.3333f,  0.9216f}, 

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

    const uint8_t pippedD6LedToFaceLookup[] = {
        0,	6,	7,	8,	9,	1,	2,	3,	4,	5,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19, 20,
    };

    const Layout D20Layout = {
        .baseNormals = twentySidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = twentySidedRemap,
        .canonicalIndexToElectricalIndexLookup = twentySidedFaceToLedLookup,
        .electricalIndexToCanonicalIndexLookup = twentySidedLedToFaceLookup,
        .faceCount = 20,
        .ledCount = 20,
    };

    const Layout D12Layout = {
        .baseNormals = twelveSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = twelveSidedRemap,
        .canonicalIndexToElectricalIndexLookup = twelveSidedFaceToLedLookup,
        .electricalIndexToCanonicalIndexLookup = twelveSidedLedToFaceLookup,
        .faceCount = 12,
        .ledCount = 12,
    };

    const Layout D10Layout = {
        .baseNormals = tenSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = tenSidedRemap,
        .canonicalIndexToElectricalIndexLookup = tenSidedFaceToLedLookup,
        .electricalIndexToCanonicalIndexLookup = tenSidedLedToFaceLookup,
        .faceCount = 10,
        .ledCount = 10,
    };

    const Layout D8Layout = {
        .baseNormals = eightSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = eightSidedRemap,
        .canonicalIndexToElectricalIndexLookup = eightSidedFaceToLedLookup,
        .electricalIndexToCanonicalIndexLookup = eightSidedLedToFaceLookup,
        .faceCount = 8,
        .ledCount = 8,
    };

    const Layout D6Layout = {
        .baseNormals = sixSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = sixSidedRemap,
        .canonicalIndexToElectricalIndexLookup = sixSidedFaceToLedLookup,
        .electricalIndexToCanonicalIndexLookup = sixSidedLedToFaceLookup,
        .faceCount = 6,
        .ledCount = 6,
    };

    const Layout D4Layout = {
        //FIXME!!!
        .baseNormals = sixSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = sixSidedRemap,
        .canonicalIndexToElectricalIndexLookup = sixSidedFaceToLedLookup,
        .electricalIndexToCanonicalIndexLookup = sixSidedLedToFaceLookup,
        .faceCount = 4,
        .ledCount = 4,
    };

    // Die layout information
    const Layout PD6Layout = {
        .baseNormals = sixSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = pippedD6Remap,
        .canonicalIndexToElectricalIndexLookup = pippedD6FaceToLedLookup,
        .electricalIndexToCanonicalIndexLookup = pippedD6LedToFaceLookup,
        .faceCount = 6,
        .ledCount = 21,
    };

    const Layout FateLayout = {
        .baseNormals = sixSidedNormals,
        .canonicalIndexFaceToFaceRemapLookup = sixSidedRemap,
        .canonicalIndexToElectricalIndexLookup = sixSidedFaceToLedLookup,
        .electricalIndexToCanonicalIndexLookup = sixSidedLedToFaceLookup,
        .faceCount = 6,
        .ledCount = 6,
    };

    const Layout* getLayout() {
        switch (BoardManager::getBoard()->model) {
        case BoardModel::D20BoardV15:
            return &D20Layout;
        case BoardModel::D6BoardV4:
            return &D6Layout;
        case BoardModel::D12BoardV2:
            return &D12Layout;
        case BoardModel::PD6BoardV3:
            return &PD6Layout;
        case BoardModel::D10BoardV2:
            return &D10Layout;
        case BoardModel::D8BoardV2:
            return &D8Layout;
        default:
            return nullptr;
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

    uint8_t electricalToCanonicalIndex(int index) {
        return getLayout()->electricalIndexToCanonicalIndexLookup[index];
    }

}
}