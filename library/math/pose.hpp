#pragma once

#include <cassert>
#include <cmath>
#include <concepts>
#include <limits>

#include "detail/angle.hpp"
#include "geometric.hpp"
#include "quaternion.hpp"
#include "vectors.hpp"

namespace math
{
    template <std::floating_point T>
    class Pose2
    {
        Vector2<T> m_translation{};
        T m_angle{0};

    public:
        constexpr Pose2() = default;

        Pose2(const Vector2<T> &translation, T angle)
            : m_translation(translation)
        {
            assert(std::isfinite(angle));
            m_angle = detail::wrap_radians(angle);
        }

        [[nodiscard]] constexpr Vector2<T> translation() const noexcept
        {
            return m_translation;
        }

        [[nodiscard]] constexpr T angle() const noexcept
        {
            return m_angle;
        }

        [[nodiscard]] Vector2<T> transform_direction(const Vector2<T> &direction) const
        {
            return rotate(direction, m_angle);
        }

        [[nodiscard]] Vector2<T> transform_point(const Vector2<T> &point) const
        {
            return transform_direction(point) + m_translation;
        }

        [[nodiscard]] Pose2 inverse() const
        {
            return Pose2{rotate(-m_translation, -m_angle), -m_angle};
        }

        // Apply the right-hand pose first.
        [[nodiscard]] Pose2 operator*(const Pose2 &other) const
        {
            return Pose2{transform_point(other.m_translation), m_angle + other.m_angle};
        }
    };

    template <std::floating_point T>
    class Pose3
    {
        Vector3<T> m_translation{};
        Quaternion<T> m_rotation{};

    public:
        constexpr Pose3() = default;

        constexpr Pose3(const Vector3<T> &translation, const Quaternion<T> &rotation)
            : m_translation(translation), m_rotation(rotation)
        {
            assert(std::abs(rotation.squared_length() - T{1}) <= T{8} * std::numeric_limits<T>::epsilon());
        }

        [[nodiscard]] constexpr Vector3<T> translation() const noexcept
        {
            return m_translation;
        }

        [[nodiscard]] constexpr Quaternion<T> rotation() const noexcept
        {
            return m_rotation;
        }

        [[nodiscard]] constexpr Vector3<T> transform_direction(const Vector3<T> &direction) const
        {
            return rotate(direction, m_rotation);
        }

        [[nodiscard]] constexpr Vector3<T> transform_point(const Vector3<T> &point) const
        {
            return transform_direction(point) + m_translation;
        }

        [[nodiscard]] constexpr Pose3 inverse() const
        {
            const auto inverse_rotation = m_rotation.conjugate();
            return Pose3{rotate(-m_translation, inverse_rotation), inverse_rotation};
        }

        // Apply the right-hand pose first and correct rounding drift in the rotation.
        [[nodiscard]] constexpr Pose3 operator*(const Pose3 &other) const
        {
            return Pose3{transform_point(other.m_translation), (m_rotation * other.m_rotation).normalized()};
        }
    };
}
