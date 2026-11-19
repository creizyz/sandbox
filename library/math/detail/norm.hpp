#pragma once

#include <cmath>
#include <concepts>
#include <cstddef>
#include <limits>

#include "../constants.hpp"

namespace math::detail
{
    template <std::floating_point T>
    struct ScaledNorm
    {
        T m_scale;
        T m_length;

        [[nodiscard]] constexpr T length() const
        {
            return m_scale * m_length;
        }

        [[nodiscard]] constexpr bool can_normalize() const
        {
            return m_scale > T{0} && std::isfinite(m_scale) && m_scale > epsilon_v<T> / m_length;
        }

        [[nodiscard]] constexpr T normalize(T component) const
        {
            return (component / m_scale) / m_length;
        }
    };

    template <std::floating_point T, typename Read>
    constexpr ScaledNorm<T> scaled_norm(std::size_t count, Read component)
    {
        T scale{0};
        bool has_nan = false;
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto magnitude = std::abs(component(i));
            if (std::isinf(magnitude))
            {
                return {magnitude, T{1}};
            }
            has_nan |= std::isnan(magnitude);
            if (magnitude > scale)
            {
                scale = magnitude;
            }
        }
        if (has_nan)
        {
            return {std::numeric_limits<T>::quiet_NaN(), T{1}};
        }
        if (scale == T{0})
        {
            return {T{0}, T{0}};
        }

        T sum{0};
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto ratio = component(i) / scale;
            sum += ratio * ratio;
        }
        return {scale, std::sqrt(sum)};
    }
}
