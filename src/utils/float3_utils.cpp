#include "core/float3.h"
#include "core/float3.h"
#include "core/matrix3x3.h"
#include "float3_utils.h"
#include "config/board_config.h"
#include "config/dice_variants.h"

using namespace Core;
using namespace Config;

namespace Utils
{
	int findClosestNormal(const float3* normals, int count, const float3& n) {
		float bestDot = -1000.0f;
		int bestFace = -1;
		for (int i = 0; i < count; ++i) {
			float dot = float3::dot(n, normals[i]);
			if (dot > bestDot) {
				bestDot = dot;
				bestFace = i;
			}
		}
		return bestFace;
	}

    // Given 3 faces and normals, computes the rotation that makes the measures normals match the canonical normals
    // returns an amount of "confidence" as a float from 0 to 1
	float CalibrateNormals(
		int face1Index, const float3& face1Normal,
		int face2Index, const float3& face2Normal,
		int face3Index, const float3& face3Normal,
		float3* outNormals, int faceCount) {

		// Figure out the rotation that transforms canonical normals into accelerometer reference frame
		auto b = BoardManager::getBoard();
        auto& canonNormals = b->layout.baseNormals;

        // int closestCanonNormal1 = findClosestNormal(canonNormals, count, face1Normal);
        // int closestCanonNormal2 = findClosestNormal(canonNormals, count, face2Normal);

        // We need to build a rotation matrix that turns canonical face normals into the reference frame
        // of the accelerator, as defined by the measured coordinates of the 2 passed in face normals.
        float3 canonFace1Normal = canonNormals[face1Index];
        float3 canonFace2Normal = canonNormals[face2Index];

        // Create our intermediate reference frame in both spaces
        // Canonical space
        float3 intX_Canon = canonFace1Normal; intX_Canon.normalize();
        float3 intZ_Canon = float3::cross(intX_Canon, canonFace2Normal); intZ_Canon.normalize();
        float3 intY_Canon = float3::cross(intZ_Canon, intX_Canon);
        matrix3x3 int_Canon(intX_Canon, intY_Canon, intZ_Canon);

        // BLE_LOG_INFO("intX_Canon: %d, %d, %d", (int)(intX_Canon.x * 100), (int)(intX_Canon.y * 100), (int)(intX_Canon.z * 100));
        // BLE_LOG_INFO("intY_Canon: %d, %d, %d", (int)(intY_Canon.x * 100), (int)(intY_Canon.y * 100), (int)(intY_Canon.z * 100));
        // BLE_LOG_INFO("intZ_Canon: %d, %d, %d", (int)(intY_Canon.x * 100), (int)(intY_Canon.y * 100), (int)(intY_Canon.z * 100));

        // Accelerometer space
        float3 intX_Acc = face1Normal; intX_Acc.normalize();
        float3 intZ_Acc = float3::cross(intX_Acc, face2Normal); intZ_Acc.normalize();
        float3 intY_Acc = float3::cross(intZ_Acc, intX_Acc);
        matrix3x3 int_Acc(intX_Acc, intY_Acc, intZ_Acc);

        // This is the matrix that rotates canonical normals into accelerometer reference frame
        matrix3x3 rot = matrix3x3::mul(int_Acc, matrix3x3::transpose(int_Canon));

        // Compare the rotation of the third face with the measured one
        float3 canonFace3Normal = canonNormals[face3Index];
        //NRF_LOG_INFO("canon: %d, %d, %d", (int)(canonNormal.x * 100.0f), (int)(canonNormal.y * 100.0f), (int)(canonNormal.z * 100.0f));
        float3 rotatedFace3Normal = matrix3x3::mul(rot, canonFace3Normal);
        float dot = float3::dot(rotatedFace3Normal, face3Normal);
        float confidence = (dot + 1.0f) / 2.0f;

		// Now transform all the normals
		for (int i = 0; i < faceCount; ++i) {
			float3 canonNormal = canonNormals[i];
			//NRF_LOG_INFO("canon: %d, %d, %d", (int)(canonNormal.x * 100.0f), (int)(canonNormal.y * 100.0f), (int)(canonNormal.z * 100.0f));
			float3 newNormal = matrix3x3::mul(rot, canonNormal);
			//NRF_LOG_INFO("new: %d, %d, %d", (int)(newNormal.x * 100.0f), (int)(newNormal.y * 100.0f), (int)(newNormal.z * 100.0f));
			outNormals[i] = newNormal;
		}

		// Return the confidence
		return confidence;
	}

}