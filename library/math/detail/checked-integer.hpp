#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace math::detail
{
    constexpr std::int64_t checked_add(std::int64_t a, std::int64_t b)
    {
        constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
        constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
        if ((b > 0 && a > maximum - b) || (b < 0 && a < minimum - b))
        {
            throw std::overflow_error("Integer addition overflow");
        }
        return a + b;
    }

    constexpr std::int64_t checked_subtract(std::int64_t a, std::int64_t b)
    {
        constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
        constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
        if ((b > 0 && a < minimum + b) || (b < 0 && a > maximum + b))
        {
            throw std::overflow_error("Integer subtraction overflow");
        }
        return a - b;
    }

    constexpr std::uint64_t unsigned_magnitude(std::int64_t value) noexcept
    {
        const auto bits = static_cast<std::uint64_t>(value);
        return value < 0 ? std::uint64_t{0} - bits : bits;
    }

    constexpr std::int64_t signed_magnitude(std::uint64_t magnitude, bool negative)
    {
        constexpr auto limit = std::uint64_t{1} << 63;
        if (magnitude >= limit)
        {
            if (negative && magnitude == limit)
            {
                return std::numeric_limits<std::int64_t>::min();
            }
            throw std::overflow_error("Integer result exceeds the signed range");
        }
        const auto result = static_cast<std::int64_t>(magnitude);
        return negative ? -result : result;
    }

    // Divisor is in [1, 2^63]. Only use the two-word product if one word is insufficient.
    constexpr std::uint64_t multiply_divide(std::uint64_t a, std::uint64_t b, std::uint64_t divisor)
    {
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        if (b == 0 || a <= maximum / b)
        {
            return a * b / divisor;
        }

        constexpr auto mask = std::uint64_t{0xffffffff};
        const auto low_product = (a & mask) * (b & mask);
        const auto cross_a = (a >> 32) * (b & mask) + (low_product >> 32);
        const auto cross_b = (a & mask) * (b >> 32) + (cross_a & mask);
        const auto low = (cross_b << 32) | (low_product & mask);
        auto remainder = (a >> 32) * (b >> 32) + (cross_a >> 32) + (cross_b >> 32);
        if (remainder >= divisor)
        {
            throw std::overflow_error("Integer quotient exceeds the unsigned range");
        }

        std::uint64_t quotient = 0;
        for (int bit = 63; bit >= 0; --bit)
        {
            remainder = (remainder << 1) | ((low >> bit) & 1);
            if (remainder >= divisor)
            {
                remainder -= divisor;
                quotient |= std::uint64_t{1} << bit;
            }
        }
        return quotient;
    }

    constexpr std::int64_t checked_scaled_product(std::int64_t a, std::int64_t b, std::uint64_t divisor)
    {
        return signed_magnitude(multiply_divide(unsigned_magnitude(a), unsigned_magnitude(b), divisor), (a < 0) != (b < 0));
    }
}
