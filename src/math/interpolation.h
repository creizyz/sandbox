#pragma once

#include <algorithm>

#include "vectors.h"
#include "quaternion.h"

namespace math
{
    template <typename T, size_t N>
    constexpr Vector<N, T> lerp(const Vector<N, T>& a, const Vector<N, T>& b, T t)
    {
        return a + (b - a) * std::clamp(t, static_cast<T>(0), static_cast<T>(1));
    }

    template <typename T>
    constexpr Quaternion<T> slerp(const Quaternion<T>& a, const Quaternion<T>& b, T t)
    {
        T dot = a.v.dot(b.v);

        Quaternion<T> target = b;
        if (dot < static_cast<T>(0)) {
            dot = -dot;
            target.v = -b.v;
        }

        if (dot > static_cast<T>(0.9995)) {
            return Quaternion<T>(lerp(a.v, target.v, t)).normalized();
        }

        T theta_0 = std::acos(dot);
        T theta = theta_0 * t;
        T sin_theta = std::sin(theta);
        T sin_theta_0 = std::sin(theta_0);

        T s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
        T s1 = sin_theta / sin_theta_0;

        return Quaternion<T>(a.v * s0 + target.v * s1).normalized();
    }

    template <typename T, size_t N>
    constexpr Vector<N, T> moveTowards(const Vector<N, T>& current, const Vector<N, T>& target, T maxDistanceDelta)
    {
        Vector<N, T> toTarget = target - current;
        T dist = toTarget.length();
        if (dist <= maxDistanceDelta || dist < epsilon_v<T>) {
            return target;
        }
        return current + (toTarget / dist) * maxDistanceDelta;
    }
}
