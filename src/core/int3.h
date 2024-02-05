#pragma once

#include "stdint.h"

namespace Utils
{
    int32_t sqrt_i32(int32_t v);
}

namespace Core
{
    #pragma pack(push, 2)
    struct int3
    {
        int16_t xTimes1000;
        int16_t yTimes1000;
        int16_t zTimes1000;

        int3() {}
        constexpr int3(int axTimes1000, int ayTimes1000, int azTimes1000) : xTimes1000(axTimes1000), yTimes1000(ayTimes1000), zTimes1000(azTimes1000) {}
        int3(const int3& model) : xTimes1000(model.xTimes1000), yTimes1000(model.yTimes1000), zTimes1000(model.zTimes1000) {}
        int3& operator=(const int3& model)
        {
            xTimes1000 = model.xTimes1000;
            yTimes1000 = model.yTimes1000;
            zTimes1000 = model.zTimes1000;
            return *this;
        }
        int3& operator+=(const int3& right)
        {
            xTimes1000 += right.xTimes1000;
            yTimes1000 += right.yTimes1000;
            zTimes1000 += right.zTimes1000;
            return *this;
        }
        int3& operator-=(const int3& right)
        {
            xTimes1000 -= right.xTimes1000;
            yTimes1000 -= right.yTimes1000;
            zTimes1000 -= right.zTimes1000;
            return *this;
        }
        int3& operator*=(int rightTimes1000)
        {
            xTimes1000 = (int16_t)((int32_t)xTimes1000 * rightTimes1000 / 1000);
            yTimes1000 = (int16_t)((int32_t)yTimes1000 * rightTimes1000 / 1000);
            zTimes1000 = (int16_t)((int32_t)zTimes1000 * rightTimes1000 / 1000);
            return *this;
        }
        int3& operator/=(int rightTimes1000)
        {
            xTimes1000 = (int16_t)((int32_t)xTimes1000 * 1000 / rightTimes1000);
            yTimes1000 = (int16_t)((int32_t)yTimes1000 * 1000 / rightTimes1000);
            zTimes1000 = (int16_t)((int32_t)zTimes1000 * 1000 / rightTimes1000);
            return *this;
        }
        int32_t sqrMagnitudeTimes1000() const
        {
            return ((int32_t)xTimes1000 * xTimes1000 + (int32_t)yTimes1000 * yTimes1000 + (int32_t)zTimes1000 * zTimes1000) / 1000;
        }
        int32_t magnitudeTimes1000() const
        {
            return Utils::sqrt_i32(sqrMagnitudeTimes1000() * 1000);
        }
        int3& normalize()
        {
            int magTimes1000 = magnitudeTimes1000();
            xTimes1000 = (int16_t)((int32_t)xTimes1000 * 1000 / magTimes1000);
            yTimes1000 = (int16_t)((int32_t)yTimes1000 * 1000 / magTimes1000);
            zTimes1000 = (int16_t)((int32_t)zTimes1000 * 1000 / magTimes1000);
            return *this;
        }
        int3 normalized() const
        {
            int3 ret = *this;
            ret.normalize();
            return ret;
        }
        static int32_t dotTimes1000(const int3& left, const int3& right)
        {
            return ((int32_t)left.xTimes1000 * right.xTimes1000 + (int32_t)left.yTimes1000 * right.yTimes1000 + (int32_t)left.zTimes1000 * right.zTimes1000) / 1000;
        }
        static int3 cross(const int3& left, const int3& right)
        {
            return int3(
                (int16_t)(((int32_t)left.yTimes1000 * right.zTimes1000 - (int32_t)left.zTimes1000 * right.yTimes1000) / 1000),
                (int16_t)(((int32_t)left.zTimes1000 * right.xTimes1000 - (int32_t)left.xTimes1000 * right.zTimes1000) / 1000),
                (int16_t)(((int32_t)left.xTimes1000 * right.yTimes1000 - (int32_t)left.yTimes1000 * right.xTimes1000) / 1000)
            );
        }

        static int3 zero() { return int3(0, 0, 0); }
    };
#pragma pack(pop)

    inline int3 operator+(const int3& left, const int3& right)
    {
        return int3(left.xTimes1000 + right.xTimes1000, left.yTimes1000 + right.yTimes1000, left.zTimes1000 + right.zTimes1000);
    }
    inline int3 operator-(const int3& left, const int3& right)
    {
        return int3(left.xTimes1000 - right.xTimes1000, left.yTimes1000 - right.yTimes1000, left.zTimes1000 - right.zTimes1000);
    }
    inline int3 operator*(const int3& left, int rightTimes1000)
    {
        return int3(
            (int16_t)((int32_t)left.xTimes1000 * rightTimes1000 / 1000),
            (int16_t)((int32_t)left.yTimes1000 * rightTimes1000 / 1000),
            (int16_t)((int32_t)left.zTimes1000 * rightTimes1000 / 1000));
    }
    inline int3 operator*(int32_t leftTimes1000, const int3& right)
    {
        return int3(
            (int16_t)(leftTimes1000 * right.xTimes1000 / 1000),
            (int16_t)(leftTimes1000 * right.yTimes1000 / 1000),
            (int16_t)(leftTimes1000 * right.zTimes1000 / 1000));
    }
    inline int3 operator/(const int3& left, int rightTimes1000)
    {
        return int3(
            (int16_t)((int32_t)left.xTimes1000 * 1000 / rightTimes1000),
            (int16_t)((int32_t)left.yTimes1000 * 1000 / rightTimes1000),
            (int16_t)((int32_t)left.zTimes1000 * 1000 / rightTimes1000));
    }
}