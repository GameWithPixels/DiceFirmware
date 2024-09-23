#pragma once

#include "core/int3.h"
#include "stdint.h"

namespace Config::DiceVariants
{
    enum DieType : uint8_t
    {
        DieType_Unknown = 0,
        DieType_D4,
        DieType_D6,
        DieType_D8,
        DieType_D10,
        DieType_D00,
        DieType_D12,
        DieType_D20,
        DieType_PD6,
        DieType_FD6,
        DieType_M20,
    };

    // This enum describes what the dice looks like, so the App can use the appropriate 3D model/color
    enum Colorway : uint8_t
    {
        Colorway_Unknown = 0,
        Colorway_Onyx_Black,
        Colorway_Hematite_Grey,
        Colorway_Midnight_Galaxy,
        Colorway_Aurora_Sky,
        Colorway_Clear,
        Colorway_White_Aurora,
        Colorway_Custom = 0xFF,
    };

    enum LEDLayoutType : uint8_t
    {
        DieLayoutType_Unknown = 0,
        DieLayoutType_D4,
        DieLayoutType_D6_FD6,
        DieLayoutType_D8,
        DieLayoutType_D10_D00,
        DieLayoutType_D12,
        DieLayoutType_D20,
        DieLayoutType_PD6,
        DieLayoutType_M20,
    };

    struct Layout
    {
        LEDLayoutType layoutType;
        uint8_t faceCount; // Face count isn't always equal to LED count (i.e. PD6, D4)
        uint8_t ledCount;
        uint8_t adjacencyCount; // How many faces each face is adjacent to

        const Core::int3* faceNormals;
        const Core::int3* ledNormals;
        const uint8_t* faceIndexFromAnimFaceIndexLookup;        // This remaps Animation indices (interpreted as faces) to "current" face indices for a given up-face.
                                                                // So if an animation pattern was authored with the 20 face up (the canonical way), we can remap the
                                                                // LEDs to play the animation with any face being up so it looks the same.
                                                                // This version maps face to face, not LED to LED.

        const uint8_t* daisyChainIndexFromLEDIndexLookup;       // Converts from "logical" led index to electrical led index for driving the Neopixels.

        const uint8_t* LEDIndexFromDaisyChainLookup;            // Reverse lookup for daisy chain index to LED index

        const uint32_t* faceAdjacencyMap;                       // Bitfield indicating which faces are adjacent to the current face

        int daisyChainIndexFromLEDIndex(int daisyChainIndex) const;
        int LEDIndexFromDaisyChainIndex(int daisyChainIndex) const;

        int remapFaceIndexBasedOnUpFace(int upFace, int faceIndex, int outFaces[]) const;
        int remapLEDIndexBasedOnUpFace(int upFace, int ledIndex, int outFaces[]) const;
        int faceIndicesFromLEDIndex(int ledIndex, int outFaces[]) const;

        uint32_t getTopFaceMask() const;
        uint8_t getTopFace() const;
        uint8_t getAdjacentFaces(uint8_t face, uint8_t retFaces[]) const;
    };

    LEDLayoutType getLayoutType(DieType dieType);
    const Layout* getLayout(LEDLayoutType layoutType);

    DieType estimateDieTypeFromBoard();
}
