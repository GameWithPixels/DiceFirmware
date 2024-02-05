#pragma once

#include "int3.h"

namespace Core
{
    #pragma pack(push, 2)
    struct matrixInt3x3
    {
        int16_t m11Times1000; int16_t m12Times1000; int16_t m13Times1000;
        int16_t m21Times1000; int16_t m22Times1000; int16_t m23Times1000;
        int16_t m31Times1000; int16_t m32Times1000; int16_t m33Times1000;

        matrixInt3x3() {}
        matrixInt3x3(const int3& col1, const int3& col2, const int3& col3)
            : m11Times1000(col1.xTimes1000), m12Times1000(col2.xTimes1000), m13Times1000(col3.xTimes1000)
            , m21Times1000(col1.yTimes1000), m22Times1000(col2.yTimes1000), m23Times1000(col3.yTimes1000)
            , m31Times1000(col1.zTimes1000), m32Times1000(col2.zTimes1000), m33Times1000(col3.zTimes1000)
        {}
        int3 col1() const { return int3(m11Times1000, m21Times1000, m31Times1000); }
        int3 col2() const { return int3(m12Times1000, m22Times1000, m32Times1000); }
        int3 col3() const { return int3(m13Times1000, m23Times1000, m33Times1000); }
        int3 row1() const { return int3(m11Times1000, m12Times1000, m13Times1000); }
        int3 row2() const { return int3(m21Times1000, m22Times1000, m23Times1000); }
        int3 row3() const { return int3(m31Times1000, m32Times1000, m33Times1000); }

        inline static matrixInt3x3 transpose(const matrixInt3x3& m) {
            matrixInt3x3 ret;
            ret.m11Times1000 = m.m11Times1000; ret.m12Times1000 = m.m21Times1000; ret.m13Times1000 = m.m31Times1000;
            ret.m21Times1000 = m.m12Times1000; ret.m22Times1000 = m.m22Times1000; ret.m23Times1000 = m.m32Times1000;
            ret.m31Times1000 = m.m13Times1000; ret.m32Times1000 = m.m23Times1000; ret.m33Times1000 = m.m33Times1000;
            return ret;
        }

        inline static int3 mul(const matrixInt3x3& left, const int3& right) {
            return int3(
                int3::dotTimes1000(left.row1(), right),
                int3::dotTimes1000(left.row2(), right),
                int3::dotTimes1000(left.row3(), right));
        }

        inline static matrixInt3x3 mul(const matrixInt3x3& left, const matrixInt3x3& right) {
            matrixInt3x3 ret;
            ret.m11Times1000 = int3::dotTimes1000(left.row1(), right.col1()); ret.m12Times1000 = int3::dotTimes1000(left.row1(), right.col2()); ret.m13Times1000 = int3::dotTimes1000(left.row1(), right.col3());
            ret.m21Times1000 = int3::dotTimes1000(left.row2(), right.col1()); ret.m22Times1000 = int3::dotTimes1000(left.row2(), right.col2()); ret.m23Times1000 = int3::dotTimes1000(left.row2(), right.col3());
            ret.m31Times1000 = int3::dotTimes1000(left.row3(), right.col1()); ret.m32Times1000 = int3::dotTimes1000(left.row3(), right.col2()); ret.m33Times1000 = int3::dotTimes1000(left.row3(), right.col3());
            return ret;
        }
    };
    #pragma pack(pop)
}