# Rigid motion

The rotation, pose, and angular dynamics APIs described here are **experimental**.
Angles are in radians. Positive rotation turns counterclockwise from +X toward +Y in 2D
and follows the right-hand rule in 3D.

## Rotations

[`rotate(vector, angle)`](../geometry/vector-geometry.hpp) rotates a Cartesian 2D vector about the origin.
[`Quaternion<T>::from_axis_angle(axis, angle)`](../geometry/quaternion.hpp) accepts any finite,
nonzero axis and internally normalizes it. `rotation_matrix(quaternion)` returns a column-indexed
3×3 matrix. Multiplying it by a vector gives the same rotation as `rotate(vector, quaternion)`
up to rounding. Both operations expect a unit quaternion.

## Poses

[`Pose2<T>` and `Pose3<T>`](../geometry/rigid-motion.hpp) represent rotation followed by translation and default to
identity. Construct a pose from a translation and a rotation. `Pose2` stores the angle in
`[-π, π)`. `Pose3` expects a unit quaternion and renormalizes the product when composing.

- `transform_point()` rotates and translates a position.
- `transform_direction()` rotates a direction and preserves its length up to rounding.
- `outer * inner` applies `inner` first and then `outer`.
- `inverse()` reverses the mapping. For a local-to-world pose, the inverse maps world to local.

## Angular dynamics

[`integrate_angle(angle, angular_speed, timestep)`](../geometry/rigid-motion.hpp) advances a 2D
angle at a constant angular speed and returns the wrapped result in `[-π, π)`.
The scalar–vector `cross(angular_speed, offset)` is the tangential velocity at a 2D point
relative to the rotation center. Reversing the argument order negates the result.

`integrate_orientation(orientation, world_angular_velocity, timestep)` expects a unit
local-to-world orientation. It constructs the incremental rotation for a constant world-space
angular velocity, multiplies it on the left, and normalizes the result to correct rounding.
Angular speeds are in radians per unit time and the timestep uses the same time unit.
Both integration functions accept signed timesteps. Inputs and intermediate arithmetic must be
finite and representable in the component type.

`apply_inverse_inertia(orientation, body_inverse_inertia, world_vector)` applies the body-space
inverse inertia tensor to a world-space torque or angular impulse. Given the unit quaternion's
rotation matrix `R`, this computes `R * I_body_inverse * Rᵀ * world_vector`. Applied to a torque,
the result is that torque's contribution to angular acceleration. Applied to an angular impulse,
it gives the corresponding change in angular velocity. In 2D, inverse inertia is a scalar
and can be multiplied directly by the torque or angular impulse.

The [rigid motion benchmarks](../../../benchmarks/math/docs/01-rigid-motion-performance.md)
measure these operations for `float` and `double`.
