#pragma once

#include <cmath>
#include <concepts>
#include <cstddef>
#include <functional>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>

namespace math
{
    namespace detail
    {
        template <std::floating_point T>
        [[nodiscard]] T finite_analysis_result(T value)
        {
            if (!std::isfinite(value))
            {
                throw std::overflow_error("Numerical result exceeds the scalar range");
            }
            return value;
        }

        template <std::floating_point T>
        void validate_interval(T lower, T upper, T tolerance)
        {
            if (!std::isfinite(lower) || !std::isfinite(upper) || lower > upper ||
                !std::isfinite(tolerance) || !(tolerance > T{0}))
            {
                throw std::invalid_argument("Numerical search requires finite ordered bounds and positive tolerance");
            }
        }

        template <std::floating_point T, typename Function>
        [[nodiscard]] T evaluate(Function &function, T x)
        {
            const T value = static_cast<T>(std::invoke(function, x));
            if (!std::isfinite(value))
            {
                throw std::domain_error("Numerical function must return finite values");
            }
            return value;
        }
    }

    // Neumaier compensation preserves small contributions lost in the main sum.
    template <std::floating_point T>
    [[nodiscard]] T compensated_sum(std::span<const T> values)
    {
        T sum{0};
        T correction{0};
        for (const auto value : values)
        {
            if (!std::isfinite(value))
            {
                throw std::invalid_argument("Compensated sum requires finite inputs");
            }
            const T next = detail::finite_analysis_result(sum + value);
            const T lost = std::abs(sum) >= std::abs(value) ? (sum - next) + value : (value - next) + sum;
            correction = detail::finite_analysis_result(correction + lost);
            sum = next;
        }
        return detail::finite_analysis_result(sum + correction);
    }

    // The function must be continuous. Tolerance bounds the half-width of the final bracket.
    template <std::floating_point T, typename Function>
    [[nodiscard]] std::optional<T> bisect_root(Function &&function, T lower, T upper, T tolerance, std::size_t max_iterations = 256)
    {
        detail::validate_interval(lower, upper, tolerance);
        T left = detail::evaluate(function, lower);
        if (left == T{0})
        {
            return lower;
        }
        const T right = detail::evaluate(function, upper);
        if (right == T{0})
        {
            return upper;
        }
        if (std::signbit(left) == std::signbit(right))
        {
            return std::nullopt;
        }
        for (std::size_t i = 0;; ++i)
        {
            const T middle = std::midpoint(lower, upper);
            if (middle - lower <= tolerance && upper - middle <= tolerance)
            {
                return middle;
            }
            if (i == max_iterations || middle == lower || middle == upper)
            {
                return std::nullopt;
            }
            const T value = detail::evaluate(function, middle);
            if (value == T{0})
            {
                return middle;
            }
            if (std::signbit(value) == std::signbit(left))
            {
                lower = middle;
                left = value;
            }
            else
            {
                upper = middle;
            }
        }
    }

    // Search a closed interval with a strict decrease and increase on either side of the minimum.
    template <std::floating_point T, typename Function>
    [[nodiscard]] std::optional<T> golden_section_minimum(Function &&function, T lower, T upper, T tolerance, std::size_t max_iterations = 256)
    {
        detail::validate_interval(lower, upper, tolerance);
        constexpr T ratio = static_cast<T>(0.618033988749894848204586834365638L);
        const T original_lower = lower;
        const T original_upper = upper;
        T x1 = std::lerp(lower, upper, T{1} - ratio);
        T x2 = std::lerp(lower, upper, ratio);
        T f1 = detail::evaluate(function, x1);
        T f2 = detail::evaluate(function, x2);
        for (std::size_t i = 0;; ++i)
        {
            const T middle = std::midpoint(lower, upper);
            if (middle - lower <= tolerance && upper - middle <= tolerance)
            {
                const T candidate = f1 <= f2 ? x1 : x2;
                const T value = f1 <= f2 ? f1 : f2;
                const T left = detail::evaluate(function, original_lower);
                const T right = detail::evaluate(function, original_upper);
                if (left <= value && left <= right)
                {
                    return original_lower;
                }
                return right < value ? original_upper : candidate;
            }
            if (i == max_iterations || !(lower < x1 && x1 < x2 && x2 < upper))
            {
                return std::nullopt;
            }
            if (f1 <= f2)
            {
                upper = x2;
                x2 = x1;
                f2 = f1;
                x1 = std::lerp(lower, upper, T{1} - ratio);
                f1 = detail::evaluate(function, x1);
            }
            else
            {
                lower = x1;
                x1 = x2;
                f1 = f2;
                x2 = std::lerp(lower, upper, ratio);
                f2 = detail::evaluate(function, x2);
            }
        }
    }

    template <std::floating_point T, typename Function>
    [[nodiscard]] T central_derivative(Function &&function, T point, T step)
    {
        const T lower = point - step;
        const T upper = point + step;
        if (!std::isfinite(lower) || !std::isfinite(upper) || !(lower < point && point < upper))
        {
            throw std::invalid_argument("Derivative requires a positive step that forms two distinct finite endpoints");
        }
        const T left = detail::evaluate(function, lower);
        const T right = detail::evaluate(function, upper);
        const T width = upper - lower;
        const T difference = right - left;
        if (std::isfinite(width) && std::isfinite(difference))
        {
            return detail::finite_analysis_result(difference / width);
        }
        return detail::finite_analysis_result((right / T{2} - left / T{2}) / (upper / T{2} - lower / T{2}));
    }

    // Adaptive Simpson quadrature for smooth functions, with an absolute error estimate and subdivision limit.
    template <std::floating_point T, typename Function>
    [[nodiscard]] std::optional<T> integrate_adaptive(Function &&function, T from, T to, T tolerance, std::size_t max_depth = 20)
    {
        const bool reversed = from > to;
        if (reversed)
        {
            std::swap(from, to);
        }
        detail::validate_interval(from, to, tolerance);
        if (from == to)
        {
            return T{0};
        }

        const auto simpson = [](T a, T b, T fa, T fm, T fb)
        {
            const T average = fa / T{6} + fm * (T{2} / T{3}) + fb / T{6};
            const T width = b - a;
            return detail::finite_analysis_result(std::isfinite(width)
                                                      ? width * average
                                                      : ((b / T{2} - a / T{2}) * average) * T{2});
        };

        const auto refine = [&](auto &&self, T a, T b, T fa, T fm, T fb, T estimate, T error_limit, std::size_t depth) -> std::optional<T>
        {
            const T middle = std::midpoint(a, b);
            const T left_middle = std::midpoint(a, middle);
            const T right_middle = std::midpoint(middle, b);
            if (!(a < left_middle && left_middle < middle && middle < right_middle && right_middle < b))
            {
                return std::nullopt;
            }
            const T fl = detail::evaluate(function, left_middle);
            const T fr = detail::evaluate(function, right_middle);
            const T left = simpson(a, middle, fa, fl, fm);
            const T right = simpson(middle, b, fm, fr, fb);
            const T refined = detail::finite_analysis_result(left + right);
            const T difference = refined - estimate;
            const T correction = std::isfinite(difference) ? difference / T{15} : refined / T{15} - estimate / T{15};
            if (std::abs(correction) <= error_limit)
            {
                return detail::finite_analysis_result(refined + correction);
            }
            if (depth == 0)
            {
                return std::nullopt;
            }
            const auto first = self(self, a, middle, fa, fl, fm, left, error_limit / T{2}, depth - 1);
            if (!first)
            {
                return std::nullopt;
            }
            const auto second = self(self, middle, b, fm, fr, fb, right, error_limit / T{2}, depth - 1);
            return second ? std::optional<T>{detail::finite_analysis_result(*first + *second)} : std::nullopt;
        };

        const T fa = detail::evaluate(function, from);
        const T fm = detail::evaluate(function, std::midpoint(from, to));
        const T fb = detail::evaluate(function, to);
        auto result = refine(refine, from, to, fa, fm, fb, simpson(from, to, fa, fm, fb), tolerance, max_depth);
        if (result && reversed)
        {
            *result = -*result;
        }
        return result;
    }
}
