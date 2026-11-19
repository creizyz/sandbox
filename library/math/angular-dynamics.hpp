#pragma once

#include <cassert>
#include <cmath>
#include <concepts>

#include "detail/angle.hpp"
#include "geometric.hpp"
#include "matrix.hpp"
#include "quaternion.hpp"
#include "vectors.hpp"

namespace math
{
    template <std::floating_point T>
    [[nodiscard]] T integrate_angle(T angle, T angular_speed, T timestep)
    {
        const T step = angular_speed * timestep;
        assert(std::isfinite(angle) && std::isfinite(step));
        return detail::wrap_radians(detail::wrap_radians(angle) + detail::wrap_radians(step));
    }

    // The orientation maps local to world. Angular velocity is constant during the step.
    template <std::floating_point T>
    [[nodiscard]] Quaternion<T> integrate_orientation(const Quaternion<T> &orientation, const Vector3<T> &world_angular_velocity, T timestep)
    {
        const auto half_step = (world_angular_velocity * timestep) / T{2};
        const T half_angle = half_step.length();
        assert(std::isfinite(half_angle));
        const T scale = half_angle == T{0} ? T{1} : std::sin(half_angle) / half_angle;
        const auto vector_part = half_step * scale;
        const Quaternion<T> delta{vector_part.x(), vector_part.y(), vector_part.z(), std::cos(half_angle)};
        return (delta * orientation).normalized();
    }

    template <std::floating_point T>
    [[nodiscard]] constexpr Vector3<T> apply_inverse_inertia(const Quaternion<T> &orientation,
                                                             const Matrix<3, 3, T> &body_inverse_inertia,
                                                             const Vector3<T> &world_vector)
    {
        const auto body_vector = rotate(world_vector, orientation.conjugate());
        return rotate(body_inverse_inertia * body_vector, orientation);
    }
}
