#pragma once

#include <stdint.h>

namespace Core
{
    struct int3;
}

namespace Utils
{
    const Core::int3* getDefaultNormals(int faceCount);

    int CalibrateNormals(
        int face1Index, const Core::int3& face1Normal,
        int face2Index, const Core::int3& face2Normal,
        int face3Index, const Core::int3& face3Normal,
        Core::int3* outNormals, int faceCount);

}