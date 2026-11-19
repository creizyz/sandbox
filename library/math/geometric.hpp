#pragma once

#include <cmath>
#include <concepts>

#include "matrix.hpp"
#include "vectors.hpp"
#include "quaternion.hpp"

namespace math
{
    template <typename T>
    constexpr T cross(const Vector2<T> &a, const Vector2<T> &b)
    {
        return a.x() * b.y() - a.y() * b.x();
    }

    template <typename T>
    [[nodiscard]] constexpr Vector2<T> cross(T z, const Vector2<T> &v)
    {
        return Vector2<T>{-z * v.y(), z * v.x()};
    }

    template <typename T>
    [[nodiscard]] constexpr Vector2<T> cross(const Vector2<T> &v, T z)
    {
        return -cross(z, v);
    }

    template <typename T>
    constexpr Vector3<T> cross(const Vector3<T> &a, const Vector3<T> &b)
    {
        return Vector3<T>{
            a.y() * b.z() - a.z() * b.y(),
            a.z() * b.x() - a.x() * b.z(),
            a.x() * b.y() - a.y() * b.x()};
    }

    template <std::floating_point T>
    [[nodiscard]] Vector2<T> rotate(const Vector2<T> &v, T angle)
    {
        const T cosine = std::cos(angle);
        const T sine = std::sin(angle);
        return Vector2<T>{cosine * v.x() - sine * v.y(), sine * v.x() + cosine * v.y()};
    }

    template <typename T>
    constexpr Vector3<T> rotate(const Vector3<T> &v, const Quaternion<T> &q)
    {
        // Standard formula: v' = v + 2 * cross(q.x()yz, cross(q.x()yz, v) + q.w * v)
        Vector3<T> qv{q.x(), q.y(), q.z()};
        Vector3<T> t = static_cast<T>(2) * cross(qv, v);
        return v + q.w() * t + cross(qv, t);
    }

    template <std::floating_point T>
    [[nodiscard]] constexpr Matrix<3, 3, T> rotation_matrix(const Quaternion<T> &q)
    {
        return Matrix<3, 3, T>{rotate(Vector3<T>{1, 0, 0}, q),
                               rotate(Vector3<T>{0, 1, 0}, q),
                               rotate(Vector3<T>{0, 0, 1}, q)};
    }

    template <typename T, size_t N>
    constexpr Vector<N, T> reflect(const Vector<N, T> &vector, const Vector<N, T> &normal)
    {
        return vector - normal * (static_cast<T>(2) * vector.dot(normal));
    }

    template <typename T, size_t N>
    constexpr Vector<N, T> refract(const Vector<N, T> &incident, const Vector<N, T> &normal, T eta)
    {
        T dot_ni = normal.dot(incident);
        T k = static_cast<T>(1) - eta * eta * (static_cast<T>(1) - dot_ni * dot_ni);
        if (k < static_cast<T>(0))
            return Vector<N, T>::zero;
        return incident * eta - normal * (eta * dot_ni + std::sqrt(k));
    }
}
