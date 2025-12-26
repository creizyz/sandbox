#pragma once

namespace math
{
    template <typename T>
    struct epsilon { static constexpr T value = 0.00001; };

    template <typename T>
    constexpr auto epsilon_v = epsilon<T>::value;
}
