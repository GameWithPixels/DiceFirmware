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
            const uint8_t* faceRemap;
            const uint8_t* faceToLedLookup;
            uint8_t faceCount;
            uint8_t ledCount;

            uint8_t animIndexToLEDIndex(int animFaceIndex, int remapFace);
        };
    }
}