#pragma once

#include "vectors.hpp"

namespace math
{
    template <typename T>
    struct Quaternion
    {
    protected:
        Vector4<T> m_data;

    public:
        constexpr Quaternion() : m_data(0, 0, 0, 1) {}
        constexpr Quaternion(T x, T y, T z, T w) : m_data(x, y, z, w) {}
        explicit constexpr Quaternion(const Vector4<T>& vec) : m_data(vec) {}

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

        Quaternion& normalize()
        {
            m_data.normalize();
            return *this;
        }

        constexpr Quaternion normalized() const
        {
            return Quaternion(m_data.normalized());
        }

        // --- Quaternion Specific Math ---

        constexpr Quaternion operator*(const Quaternion & other) const
        {
            return Quaternion(
                m_data.w() * other.m_data.x() + m_data.x() * other.m_data.w() + m_data.y() * other.m_data.z() - m_data.z() * other.m_data.y(),
                m_data.w() * other.m_data.y() - m_data.x() * other.m_data.z() + m_data.y() * other.m_data.w() + m_data.z() * other.m_data.x(),
                m_data.w() * other.m_data.z() + m_data.x() * other.m_data.y() - m_data.y() * other.m_data.x() + m_data.z() * other.m_data.w(),
                m_data.w() * other.m_data.w() - m_data.x() * other.m_data.x() - m_data.y() * other.m_data.y() - m_data.z() * other.m_data.z()
            );
        }

        constexpr Quaternion conjugate() const
        {
            return Quaternion(-m_data.x(), -m_data.y(), -m_data.z(), m_data.w());
        }
    };
}