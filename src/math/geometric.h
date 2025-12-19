#pragma once

#include "vectors.h"
#include "quaternion.h"

namespace math
{
    template <typename T>
    constexpr T cross(const Vector2<T>& a, const Vector2<T>& b)
    {
        return a.x * b.y - a.y * b.x;
    }

    template <typename T>
    constexpr Vector3<T> cross(const Vector3<T>& a, const Vector3<T>& b)
    {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    template <typename T>
    constexpr Vector3<T> rotate(const Vector3<T>& v, const Quaternion<T>& q)
    {
        // Standard formula: v' = v + 2 * cross(q.xyz, cross(q.xyz, v) + q.w * v)
        Vector3<T> qv{ q.x(), q.y(), q.z() };
        Vector3<T> t = static_cast<T>(2) * cross(qv, v);
        return v + q.w() * t + cross(qv, t);
    }

    template <typename T, size_t N>
    constexpr Vector<N, T> reflect(const Vector<N, T> & vector, const Vector<N, T> & normal)
    {
        return vector - normal * (static_cast<T>(2) * vector.dot(normal));
    }

    template <typename T, size_t N>
    constexpr Vector<N, T> refract(const Vector<N, T>& incident, const Vector<N, T>& normal, T eta)
    {
        T dotNI = normal.dot(incident);
        T k = static_cast<T>(1) - eta * eta * (static_cast<T>(1) - dotNI * dotNI);
        if (k < static_cast<T>(0)) return Vector<N, T>::zero;
        return incident * eta - normal * (eta * dotNI + std::sqrt(k));
    }
}