#pragma once

#include "core/float3.h"
#include "stdint.h"

namespace Config
{
    namespace DiceVariants
    {
        // This enum describes what the dice looks like, so the App can use the appropriate 3D model/color
        enum DesignAndColor : uint8_t
        {
            DesignAndColor_Unknown = 0,
            DesignAndColor_Generic,
            DesignAndColor_V3_Orange,
            DesignAndColor_V4_BlackClear,
            DesignAndColor_V4_WhiteClear,
            DesignAndColor_V5_Grey,
            DesignAndColor_V5_White,
            DesignAndColor_V5_Black,
            DesignAndColor_V5_Gold,
            DesignAndColor_Onyx_Back,
            DesignAndColor_Hematite_Grey,
            DesignAndColor_Midnight_Galaxy,
            DesignAndColor_Aurora_Sky
        };

        struct Layout
        {
            const Core::float3* baseNormals;
            const uint8_t* canonicalIndexFaceToFaceRemapLookup;     // This remaps led indices to remap faces when retargetting the "up" face
                                                                    // So if an animation pattern was authored with the 20 face up, we can
                                                                    // remap the leds to play the animation with any face being up.
            const uint8_t* canonicalIndexToElectricalIndexLookup;   // Because leds are organized in a daisy chain, but that daisy chain
                                                                    // doesn't necessarily follow the order of the faces, we need to be able
                                                                    // to remap from canonical (i.e. face) index to electrical (i.e. led) index.
            const uint8_t* electricalIndexToCanonicalIndexLookup;   // Reverse Lookup Table from previous, this is useful for 'traveling' animations
            uint8_t faceCount; // Face count isn't always equal to led count (i.e. PD6, D4)
        };
    }
}