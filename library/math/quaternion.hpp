#pragma once

#include <cassert>
#include <cmath>
#include <concepts>
#include <cstddef>

#include "detail/norm.hpp"
#include "vectors.hpp"

namespace math
{
    template <typename T>
    struct Quaternion
    {
    protected:
        Vector4<T> m_data;

    public:
        constexpr Quaternion()
            : m_data(0, 0, 0, 1) {}
        constexpr Quaternion(T x, T y, T z, T w)
            : m_data(x, y, z, w) {}
        explicit constexpr Quaternion(const Vector4<T> &vec)
            : m_data(vec) {}

        // --- Access ---

        constexpr T x() const
        {
            return m_data.x();
        }
        constexpr T y() const
        {
            return m_data.y();
        }
        constexpr T z() const
        {
            return m_data.z();
        }
        constexpr T w() const
        {
            return m_data.w();
        }

        // --- Shared Features ---

        constexpr T length() const
        {
            return m_data.length();
        }

        constexpr T squared_length() const
        {
            return m_data.squared_length();
        }

        Quaternion &normalize()
        {
            m_data.normalize();
            return *this;
        }

        constexpr Quaternion normalized() const
        {
            return Quaternion(m_data.normalized());
        }

        // --- Quaternion Specific Math ---

        [[nodiscard]] static Quaternion from_axis_angle(const Vector3<T> &axis, T angle)
            requires std::floating_point<T>
        {
            const auto norm = detail::scaled_norm<T>(3, [&axis](std::size_t i)
                                                     { return axis[i]; });
            assert(norm.m_scale > T{0} && std::isfinite(norm.m_scale) && std::isfinite(angle));
            const T half_angle = angle / T{2};
            const T sine = std::sin(half_angle);
            return Quaternion{norm.normalize(axis.x()) * sine, norm.normalize(axis.y()) * sine,
                              norm.normalize(axis.z()) * sine, std::cos(half_angle)};
        }

        constexpr Quaternion operator*(const Quaternion &other) const
        {
            return Quaternion(
                m_data.w() * other.m_data.x() + m_data.x() * other.m_data.w() + m_data.y() * other.m_data.z() - m_data.z() * other.m_data.y(),
                m_data.w() * other.m_data.y() - m_data.x() * other.m_data.z() + m_data.y() * other.m_data.w() + m_data.z() * other.m_data.x(),
                m_data.w() * other.m_data.z() + m_data.x() * other.m_data.y() - m_data.y() * other.m_data.x() + m_data.z() * other.m_data.w(),
                m_data.w() * other.m_data.w() - m_data.x() * other.m_data.x() - m_data.y() * other.m_data.y() - m_data.z() * other.m_data.z());
        }

        constexpr Quaternion conjugate() const
        {
            return Quaternion(-m_data.x(), -m_data.y(), -m_data.z(), m_data.w());
        }
    };
}
