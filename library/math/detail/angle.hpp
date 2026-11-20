#pragma once

#include <cmath>
#include <concepts>
#include <numbers>

namespace math::detail
{
    template <std::floating_point T>
    T wrap_radians(T angle)
    {
        constexpr T pi = std::numbers::pi_v<T>;
        // Beyond one turn, reduction by the rounded 2*pi loses phase as the turn count grows.
        // The trigonometric functions perform full argument reduction on the original angle.
        const T wrapped = std::abs(angle) <= T{2} * pi
                              ? std::remainder(angle, T{2} * pi)
                              : std::atan2(std::sin(angle), std::cos(angle));
        if (wrapped == pi)
        {
            return -pi;
        }
        return wrapped == T{0} ? T{0} : wrapped;
    }
}
