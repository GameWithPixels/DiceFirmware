#pragma once

#include <stdint.h>

namespace Core
{
	struct float3;
}

namespace Utils
{
	const Core::float3* getDefaultNormals(int faceCount);

	float CalibrateNormals(
		int face1Index, const Core::float3& face1Normal,
		int face2Index, const Core::float3& face2Normal,
		int face3Index, const Core::float3& face3Normal,
		Core::float3* outNormals, int faceCount);

}