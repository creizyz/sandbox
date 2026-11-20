#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <numbers>
#include <stdexcept>

#include "math/geometry/polar.hpp"
#include "math/linear-algebra/vectors.hpp"
#include "math/geometry/quaternion.hpp"
#include "math/detail/angle.hpp"

namespace math
{
    template <std::floating_point T>
    constexpr T lerp(T a, T b, T t)
    {
        return std::lerp(a, b, std::clamp(t, T{0}, T{1}));
    }

    template <typename T, std::size_t N>
    constexpr Vector<N, T> lerp(const Vector<N, T> &a, const Vector<N, T> &b, T t)
    {
        if constexpr (std::floating_point<T>)
        {
            t = std::clamp(t, T{0}, T{1});
            return a.transform(b, [t](T left, T right)
                               { return std::lerp(left, right, t); });
        }
        else
        {
            return t < T{1} ? a : b;
        }
    }

    namespace detail
    {
        template <typename V, typename T>
        concept LinearInterpolation = std::floating_point<T> && requires(const V &a, const V &b, T t) {
            { math::lerp(a, b, t) } -> std::same_as<V>;
        };

        template <std::floating_point T>
        constexpr Vector4<T> quaternion_components(const Quaternion<T> &q)
        {
            return Vector4<T>{q.x(), q.y(), q.z(), q.w()};
        }
    }

    template <std::floating_point T>
    constexpr T smoothstep(T t)
    {
        t = std::clamp(t, T{0}, T{1});
        return t * t * (T{3} - T{2} * t);
    }

    template <std::floating_point T>
    constexpr T smootherstep(T t)
    {
        t = std::clamp(t, T{0}, T{1});
        const auto ease = [](T x)
        { return x * x * x * (x * (T{6} * x - T{15}) + T{10}); };
        return t > T{0.5} ? T{1} - ease(T{1} - t) : ease(t);
    }

    // Tangents are derivatives with respect to the normalized segment parameter.
    template <typename V, std::floating_point T>
        requires detail::LinearInterpolation<V, T>
    constexpr V cubic_hermite(const V &a, const V &b, const V &tangent_a, const V &tangent_b, T t)
    {
        t = std::clamp(t, T{0}, T{1});
        const T t_squared = t * t;
        const T t_cubed = t_squared * t;
        const T weight_b = T{3} * t_squared - T{2} * t_cubed;
        const T weight_a = T{1} - weight_b;
        const T tangent_weight_a = t_cubed - T{2} * t_squared + t;
        const T tangent_weight_b = t_cubed - t_squared;
        return a * weight_a + tangent_a * tangent_weight_a + b * weight_b + tangent_b * tangent_weight_b;
    }

    template <typename V, std::floating_point T>
        requires detail::LinearInterpolation<V, T>
    constexpr V cubic_bezier(const V &a, const V &control_a, const V &control_b, const V &b, T t)
    {
        const V first = lerp(a, control_a, t);
        const V middle = lerp(control_a, control_b, t);
        const V last = lerp(control_b, b, t);
        return lerp(lerp(first, middle, t), lerp(middle, last, t), t);
    }

    // Evaluates the segment from p1 to p2. Coincident outer neighbors give zero tangents.
    template <std::size_t N, std::floating_point T>
    constexpr Vector<N, T> centripetal_catmull_rom(
        const Vector<N, T> &p0, const Vector<N, T> &p1, const Vector<N, T> &p2, const Vector<N, T> &p3, T t)
    {
        const auto segment = p2 - p1;
        const T segment_interval = std::sqrt(segment.length());
        if (segment_interval == T{0})
        {
            return p1;
        }

        const auto segment_velocity = segment / segment_interval;
        const auto tangent = [&](const Vector<N, T> &edge)
        {
            const T interval = std::sqrt(edge.length());
            if (interval == T{0})
            {
                return Vector<N, T>::zero;
            }
            const T sum = interval + segment_interval;
            const auto outer_velocity = edge / interval;
            const auto knot_velocity = outer_velocity * (segment_interval / sum) + segment_velocity * (interval / sum);
            return knot_velocity * segment_interval;
        };
        return cubic_hermite(p1, p2, tangent(p1 - p0), tangent(p3 - p2), t);
    }

    template <typename V, std::floating_point T>
        requires detail::LinearInterpolation<V, T>
    constexpr V bilinear(const V &v00, const V &v10, const V &v01, const V &v11, T x, T y)
    {
        return lerp(lerp(v00, v10, x), lerp(v01, v11, x), y);
    }

    // Corners use index x + 2*y + 4*z, with integer coordinates 0 or 1.
    template <typename V, std::floating_point T>
        requires detail::LinearInterpolation<V, T>
    constexpr V trilinear(const std::array<V, 8> &corners, T x, T y, T z)
    {
        const V first = bilinear(corners[0], corners[1], corners[2], corners[3], x, y);
        const V second = bilinear(corners[4], corners[5], corners[6], corners[7], x, y);
        return lerp(first, second, z);
    }

    // Inputs must be unit quaternions. The shortest path may negate b.
    template <std::floating_point T>
    constexpr Quaternion<T> nlerp(const Quaternion<T> &a, const Quaternion<T> &b, T t)
    {
        const auto start = detail::quaternion_components(a);
        const auto target = detail::quaternion_components(b);
        const auto nearest = start.dot(target) < T{0} ? -target : target;
        return Quaternion<T>{lerp(start, nearest, t)}.normalized();
    }

    // Inputs must be unit quaternions. Following the shortest arc may negate b.
    template <std::floating_point T>
    constexpr Quaternion<T> slerp(const Quaternion<T> &a, const Quaternion<T> &b, T t)
    {
        const auto start = detail::quaternion_components(a);
        auto target = detail::quaternion_components(b);
        t = std::clamp(t, T{0}, T{1});
        T dot = start.dot(target);

        if (dot < T{0})
        {
            dot = -dot;
            target = -target;
        }

        if (dot > static_cast<T>(0.9995))
        {
            return Quaternion<T>(lerp(start, target, t)).normalized();
        }

        const T angle = std::acos(dot);
        const T denominator = std::sin(angle);
        const T start_weight = std::sin((T{1} - t) * angle) / denominator;
        const T target_weight = std::sin(t * angle) / denominator;
        return Quaternion<T>(start * start_weight + target * target_weight).normalized();
    }

    template <std::floating_point T, std::size_t N>
    constexpr Vector<N, T> move_towards(const Vector<N, T> &current, const Vector<N, T> &target, T max_distance_delta)
    {
        Vector<N, T> to_target = target - current;
        T dist = to_target.length();
        if (dist <= max_distance_delta || dist < epsilon_v<T>)
        {
            return target;
        }
        return current + (to_target / dist) * max_distance_delta;
    }

    enum class PolarArc
    {
        Shortest,
        Longest
    };

    template <std::floating_point T>
    [[nodiscard]] Polar2<T> lerp(const Polar2<T> &a, const Polar2<T> &b, T t, PolarArc arc = PolarArc::Shortest)
    {
        assert(std::isfinite(t));
        if (t <= T{0})
        {
            return a;
        }
        if (t >= T{1})
        {
            return b;
        }

        const T radius = math::lerp(a.radius(), b.radius(), t);
        if (a.radius() == T{0} || b.radius() == T{0})
        {
            return Polar2<T>{radius, a.radius() == T{0} ? b.angle() : a.angle()};
        }

        constexpr T pi = std::numbers::pi_v<T>;
        T sweep = detail::wrap_radians(b.angle() - a.angle());
        if (sweep == -pi)
        {
            sweep = pi;
        }
        if (arc == PolarArc::Longest && sweep != T{0})
        {
            sweep -= std::copysign(T{2} * pi, sweep);
        }
        return Polar2<T>{radius, a.angle() + t * sweep};
    }

    template <std::floating_point T>
    [[nodiscard]] T clamped_linear_map(T value, T input_min, T input_max, T output_start, T output_end)
    {
        if (!std::isfinite(value) || !std::isfinite(input_min) || !std::isfinite(input_max) ||
            !std::isfinite(output_start) || !std::isfinite(output_end) || !(input_min < input_max))
        {
            throw std::invalid_argument("Linear mapping requires finite values and an increasing input interval");
        }
        const T clamped = std::clamp(value, input_min, input_max);
        const T range = input_max - input_min;
        const T t = std::isfinite(range)
                        ? (clamped - input_min) / range
                        : (clamped / T{2} - input_min / T{2}) / (input_max / T{2} - input_min / T{2});
        return std::lerp(output_start, output_end, t);
    }

    // After one time constant, the remaining distance to a fixed target is 1/e of its initial value.
    template <std::floating_point T>
    [[nodiscard]] T smooth_towards(T current, T target, T duration, T time_constant)
    {
        if (!std::isfinite(current) || !std::isfinite(target) || !std::isfinite(duration) || duration < T{0} ||
            !std::isfinite(time_constant) || !(time_constant > T{0}))
        {
            throw std::invalid_argument("Exponential smoothing requires finite values, a nonnegative duration, and a positive time constant");
        }
        return std::lerp(current, target, -std::expm1(-duration / time_constant));
    }
}
