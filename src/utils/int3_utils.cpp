#include "core/int3.h"
#include "core/matrixint3x3.h"
#include "int3_utils.h"
#include "config/dice_variants.h"
#include "config/settings.h"

using namespace Core;
using namespace Config;

namespace Utils
{
    int findClosestNormal(const int3* normals, int count, const int3& n) {
        int bestDot = -1000000;
        int bestFace = -1;
        for (int i = 0; i < count; ++i) {
            int dot = int3::dotTimes1000(n, normals[i]);
            if (dot > bestDot) {
                bestDot = dot;
                bestFace = i;
            }
        }
        return bestFace;
    }

    // Given 3 faces and normals, computes the rotation that makes the measures normals match the canonical normals
    // returns an amount of "confidence" as an int from 0 to 1000
    int CalibrateNormals(
        int face1Index, const int3& face1Normal,
        int face2Index, const int3& face2Normal,
        int face3Index, const int3& face3Normal,
        int3* outNormals, int faceCount) {

        // Figure out the rotation that transforms canonical normals into accelerometer reference frame
        auto l = SettingsManager::getLayout();
        auto& canonNormals = l->faceNormals;

        // int closestCanonNormal1 = findClosestNormal(canonNormals, count, face1Normal);
        // int closestCanonNormal2 = findClosestNormal(canonNormals, count, face2Normal);

        // We need to build a rotation matrix that turns canonical face normals into the reference frame
        // of the accelerator, as defined by the measured coordinates of the 2 passed in face normals.
        int3 canonFace1Normal = canonNormals[face1Index];
        int3 canonFace2Normal = canonNormals[face2Index];

        // Create our intermediate reference frame in both spaces
        // Canonical space
        int3 intX_Canon = canonFace1Normal; intX_Canon.normalize();
        int3 intZ_Canon = int3::cross(intX_Canon, canonFace2Normal); intZ_Canon.normalize();
        int3 intY_Canon = int3::cross(intZ_Canon, intX_Canon);
        matrixInt3x3 int_Canon(intX_Canon, intY_Canon, intZ_Canon);

        // BLE_LOG_INFO("intX_Canon: %d, %d, %d", (int)(intX_Canon.x * 100), (int)(intX_Canon.y * 100), (int)(intX_Canon.z * 100));
        // BLE_LOG_INFO("intY_Canon: %d, %d, %d", (int)(intY_Canon.x * 100), (int)(intY_Canon.y * 100), (int)(intY_Canon.z * 100));
        // BLE_LOG_INFO("intZ_Canon: %d, %d, %d", (int)(intY_Canon.x * 100), (int)(intY_Canon.y * 100), (int)(intY_Canon.z * 100));

        // Accelerometer space
        int3 intX_Acc = face1Normal; intX_Acc.normalize();
        int3 intZ_Acc = int3::cross(intX_Acc, face2Normal); intZ_Acc.normalize();
        int3 intY_Acc = int3::cross(intZ_Acc, intX_Acc);
        matrixInt3x3 int_Acc(intX_Acc, intY_Acc, intZ_Acc);

        // This is the matrix that rotates canonical normals into accelerometer reference frame
        matrixInt3x3 rot = matrixInt3x3::mul(int_Acc, matrixInt3x3::transpose(int_Canon));

        // Compare the rotation of the third face with the measured one
        int3 canonFace3Normal = canonNormals[face3Index];
        int3 rotatedFace3Normal = matrixInt3x3::mul(rot, canonFace3Normal);
        int dotTimes1000 = int3::dotTimes1000(rotatedFace3Normal, face3Normal);
        int confidence = (dotTimes1000 + 1000) / 2;

        // Now transform all the normals
        for (int i = 0; i < faceCount; ++i) {
            int3 canonNormal = canonNormals[i];
            int3 newNormal = matrixInt3x3::mul(rot, canonNormal);
            outNormals[i] = newNormal;
        }

        // Return the confidence
        return confidence;
    }

}