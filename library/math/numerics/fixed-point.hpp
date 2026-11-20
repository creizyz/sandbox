#pragma once

#include <cmath>
#include <compare>
#include <concepts>
#include <cstdint>
#include <stdexcept>

#include "math/detail/checked-integer.hpp"

namespace math
{
    // Exact addition and subtraction at a fixed scale. Multiplication and division truncate toward zero.
    template <std::int64_t Scale = 1000>
        requires(Scale > 0)
    class FixedPoint
    {
        std::int64_t m_raw{0};

    public:
        static constexpr auto scale = Scale;

        [[nodiscard]] static constexpr FixedPoint from_raw(std::int64_t value) noexcept
        {
            FixedPoint result;
            result.m_raw = value;
            return result;
        }

        [[nodiscard]] static constexpr FixedPoint from_integer(std::int64_t value)
        {
            return from_raw(detail::checked_scaled_product(value, Scale, 1));
        }

        // Rounds to the nearest representable step, with halfway values rounded away from zero.
        template <std::floating_point Float>
        [[nodiscard]] static FixedPoint from_float(Float value)
        {
            const auto rounded = std::round(static_cast<long double>(value) * Scale);
            constexpr auto exclusive_upper = 0x1p63L;
            if (!std::isfinite(value))
            {
                throw std::invalid_argument("Fixed-point conversion requires a finite value");
            }
            if (rounded < -exclusive_upper || rounded >= exclusive_upper)
            {
                throw std::overflow_error("Floating-point value exceeds the fixed-point range");
            }
            return from_raw(static_cast<std::int64_t>(rounded));
        }

        [[nodiscard]] constexpr std::int64_t raw() const noexcept
        {
            return m_raw;
        }

        template <std::floating_point Float = double>
        [[nodiscard]] constexpr Float to_float() const noexcept
        {
            return static_cast<Float>(m_raw) / static_cast<Float>(Scale);
        }

        [[nodiscard]] constexpr auto operator<=>(const FixedPoint &) const = default;

        [[nodiscard]] constexpr FixedPoint operator+(FixedPoint other) const
        {
            return from_raw(detail::checked_add(m_raw, other.m_raw));
        }

        [[nodiscard]] constexpr FixedPoint operator-(FixedPoint other) const
        {
            return from_raw(detail::checked_subtract(m_raw, other.m_raw));
        }

        [[nodiscard]] constexpr FixedPoint operator*(FixedPoint other) const
        {
            return from_raw(detail::checked_scaled_product(m_raw, other.m_raw, Scale));
        }

        [[nodiscard]] constexpr FixedPoint operator/(FixedPoint other) const
        {
            if (other.m_raw == 0)
            {
                throw std::domain_error("Fixed-point division by zero");
            }
            const auto magnitude = detail::multiply_divide(detail::unsigned_magnitude(m_raw), Scale, detail::unsigned_magnitude(other.m_raw));
            return from_raw(detail::signed_magnitude(magnitude, (m_raw < 0) != (other.m_raw < 0)));
        }
    };
}
