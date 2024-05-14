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
        Colorway_Custom = 0xFF,
    };

    enum DieLayoutType : uint8_t
    {
        DieLayoutType_Unknown = 0,
        DieLayoutType_D4,
        DieLayoutType_D6_FD6,
        DieLayoutType_D8,
        DieLayoutType_D10_D00,
        DieLayoutType_D12,
        DieLayoutType_D20,
        DieLayoutType_PD6,
    };

    struct Layout
    {
        DieLayoutType layoutType;
        uint8_t faceCount; // Face count isn't always equal to LED count (i.e. PD6, D4)
        uint8_t ledCount;
        uint8_t adjacencyCount; // How many faces each face is adjacent to

        const Core::int3* faceNormals;
        const Core::int3* ledNormals;
        const uint8_t* canonicalIndexFaceToFaceRemapLookup;     // This remaps LED indices to new LED Indices for a given up-face.
                                                                // So if an animation pattern was authored with the 20 face up (the canonical way),
                                                                // we can remap the LEDs to play the animation with any face being up so it looks the same.
                                                                // This allows re-using animations across different orientations.
        const uint8_t* daisyChainIndexFromLEDIndexLookup;             // Because LEDs are organized in a daisy chain (daisy chain), but that daisy chain
                                                                // doesn't necessarily follow the natural order of the LEDs (typically matching
                                                                // the face numbers), we need to be able to remap from LED Index (often matching
                                                                // the face index) to daisy chain (daisy chain) index.
        const uint32_t* adjacencyMap;                           // Bitfield indicating which faces are adjacent to the current face

        int faceIndexFromLEDIndex(int ledIndex) const;
        void ledColorsFromFaceColors(uint32_t const faceColors[], uint32_t retLedColors[]) const;
        int animIndexFromLEDIndex(int ledIndex, int remapFace) const;
        int daisyChainIndexFromLEDIndex(int daisyChainIndex) const;

        uint32_t getTopFaceMask() const;
        uint8_t getTopFace() const;
        uint8_t getAdjacentFaces(uint8_t face, uint8_t retFaces[]) const;
    };

    DieLayoutType getLayoutType(DieType dieType);
    const Layout* getLayout(DieLayoutType layoutType);

    DieType estimateDieTypeFromBoard();
}
