#pragma once

#include <bit>
#include <cmath>
#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
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

        template <std::floating_point Float>
        static std::uint64_t rounded_fraction(Float fraction)
        {
            if constexpr (std::numeric_limits<Float>::digits + std::bit_width(static_cast<std::uint64_t>(Scale)) <=
                          std::numeric_limits<long double>::digits)
            {
                return static_cast<std::uint64_t>(std::round(static_cast<long double>(fraction) * Scale));
            }
            int exponent;
            const auto significand = std::frexp(fraction, &exponent);
            if (significand == Float{0} || exponent < -63)
            {
                return 0;
            }
            // Fold the binary fraction from its least significant bit. Intermediate
            // halves can truncate: only the last discarded bit determines rounding.
            constexpr int digits = std::numeric_limits<Float>::digits;
            auto bits = std::ldexp(significand, digits);
            std::uint64_t steps = 0;
            for (int remaining = digits - exponent; remaining > 0; --remaining)
            {
                const auto half = std::floor(bits / Float{2});
                const bool set = bits != half + half;
                steps += set ? static_cast<std::uint64_t>(Scale) : 0;
                steps = (steps + (remaining == 1 ? 1u : 0u)) / 2;
                bits = half;
            }
            return steps;
        }

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
            if (!std::isfinite(value))
            {
                throw std::invalid_argument("Fixed-point conversion requires a finite value");
            }
            Float whole;
            const auto fraction = std::modf(std::abs(value), &whole);
            if (whole > 0x1p63L)
            {
                throw std::overflow_error("Floating-point value exceeds the fixed-point range");
            }
            const auto units = static_cast<std::uint64_t>(whole);
            const auto partial = rounded_fraction(fraction);
            constexpr auto limit = std::uint64_t{1} << 63;
            if (units > (limit - partial) / Scale)
            {
                throw std::overflow_error("Floating-point value exceeds the fixed-point range");
            }
            return from_raw(detail::signed_magnitude(units * Scale + partial, value < Float{0}));
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
