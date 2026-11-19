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
        const T wrapped = std::remainder(angle, T{2} * pi);
        if (wrapped == pi)
        {
            return -pi;
        }
        return wrapped == T{0} ? T{0} : wrapped;
    }
}
