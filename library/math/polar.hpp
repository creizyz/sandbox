#pragma once

#include <cassert>
#include <cmath>
#include <concepts>
#include <numbers>

#include "vectors.hpp"

namespace math
{
    namespace detail
    {
        template <std::floating_point T>
        T wrap_polar_angle(T angle)
        {
            constexpr T pi = std::numbers::pi_v<T>;
            const T wrapped = std::remainder(angle, T{2} * pi);
            if (wrapped == pi)
            {
                return -pi;
            }
            return wrapped == T{0} ? T{0} : wrapped;
        }
    }

    template <std::floating_point T>
    class Polar2
    {
    public:
        constexpr Polar2() = default;

        // Angles are counterclockwise radians from positive X.
        Polar2(T radius, T angle)
            : m_radius(std::abs(radius))
        {
            assert(std::isfinite(radius) && std::isfinite(angle));
            if (m_radius != T{0})
            {
                m_angle = detail::wrap_polar_angle(angle);
                if (radius < T{0})
                {
                    m_angle = detail::wrap_polar_angle(m_angle + std::numbers::pi_v<T>);
                }
            }
        }

        [[nodiscard]] constexpr T radius() const noexcept
        {
            return m_radius;
        }

        [[nodiscard]] constexpr T angle() const noexcept
        {
            return m_angle;
        }

        [[nodiscard]] static Polar2 from_cartesian(const Vector2<T> &point)
        {
            const T radius = std::hypot(point.x(), point.y());
            if (radius == T{0})
            {
                return {};
            }
            return Polar2{radius, std::atan2(point.y(), point.x())};
        }

        [[nodiscard]] Vector2<T> to_cartesian() const
        {
            return Vector2<T>{m_radius * std::cos(m_angle), m_radius * std::sin(m_angle)};
        }

        [[nodiscard]] Polar2 rotated(T angle) const
        {
            assert(std::isfinite(angle));
            return Polar2{m_radius, m_angle + detail::wrap_polar_angle(angle)};
        }

    private:
        T m_radius{0};
        T m_angle{0};
    };

    using Polar2f = Polar2<float>;
    using Polar2d = Polar2<double>;
}
