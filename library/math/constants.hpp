#pragma once

namespace math
{
    template <typename T>
    struct Epsilon { static constexpr T value = 0.00001; };

    template <typename T>
    constexpr auto epsilon_v = Epsilon<T>::value;
}
