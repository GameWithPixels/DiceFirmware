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
        Colorway_Aurora_Clear,
        Colorway_Custom = 0xFF,
    };

    struct Layout
    {
        const Core::int3* baseNormals;
        const uint8_t* canonicalIndexFaceToFaceRemapLookup;     // This remaps LED indices to remap faces when re-targeting the "up" face
                                                                // So if an animation pattern was authored with the 20 face up, we can
                                                                // remap the LEDs to play the animation with any face being up.
        const uint8_t* canonicalIndexToElectricalIndexLookup;   // Because LEDs are organized in a daisy chain, but that daisy chain
                                                                // doesn't necessarily follow the order of the faces, we need to be able
                                                                // to remap from canonical (i.e. face) index to electrical (i.e. LED) index.
        uint8_t faceCount; // Face count isn't always equal to LED count (i.e. PD6, D4)
        uint8_t ledCount;
    };

    const Layout* getLayout();
    DieType estimateDieTypeFromBoard();
    uint8_t animIndexToLEDIndex(int animFaceIndex, int remapFace);
    uint32_t getTopFaceMask();
    uint8_t getTopFace();
}
