#pragma once

#include "vectors.h"

namespace math
{
    template <typename T>
    struct Quaternion
    {
    protected:
        Vector4<T> data;

    public:
        constexpr Quaternion() : data(0, 0, 0, 1) {}
        constexpr Quaternion(T x, T y, T z, T w) : data(x, y, z, w) {}
        explicit constexpr Quaternion(const Vector4<T>& vec) : data(vec) {}

        // --- Access ---

        constexpr T x() const
        {
            return data.x;
        }
        constexpr T y() const
        {
            return data.y;
        }
        constexpr T z() const
        {
            return data.z;
        }
        constexpr T w() const
        {
            return data.w;
        }

        // --- Shared Features ---

        constexpr T length() const
        {
            return data.length();
        }

        constexpr T squaredLength() const
        {
            return data.squaredLength();
        }

        Quaternion& normalize()
        {
            data.normalize();
            return *this;
        }

        constexpr Quaternion normalized() const
        {
            return Quaternion(data.normalized());
        }

        // --- Quaternion Specific Math ---

        constexpr Quaternion operator*(const Quaternion & other) const
        {
            return Quaternion(
                data.w * other.data.x + data.x * other.data.w + data.y * other.data.z - data.z * other.data.y,
                data.w * other.data.y - data.x * other.data.z + data.y * other.data.w + data.z * other.data.x,
                data.w * other.data.z + data.x * other.data.y - data.y * other.data.x + data.z * other.data.w,
                data.w * other.data.w - data.x * other.data.x - data.y * other.data.y - data.z * other.data.z
            );
        }

        constexpr Quaternion conjugate() const
        {
            return Quaternion(-data.x, -data.y, -data.z, data.w);
        }
    };
}