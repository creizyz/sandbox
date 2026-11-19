# Math contracts

## Vector storage and handles

[`VectorSoa<N, T>`](../vectors-soa.hpp) keeps each component in a separate contiguous array.
`emplace()` appends a row and returns its handle. `resize()` appends zero-filled rows or removes
rows from the end. `handle_at()` retrieves a row handle by index, and `get()` resolves a handle to
an optional view. Handles belong to the container that issued them and transfer with its storage
on move construction.

Handles to surviving rows remain valid through capacity changes, resizing, and swap-erase of
another row. Erasing a row or calling `clear()` invalidates the removed row handles. Iteration
visits rows in their current index order, which can change on erase. `shrink_to_fit()` shrinks the
component arrays. The handle register retains generation history for subsequent inserts.

Treat views and iterators as short-lived borrows. Fetch a fresh view after structural changes to
the container when a caller needs to keep referring to the same row.

## Arithmetic

[`Matrix`](../matrix.hpp) uses column indexing. Scalar division divides each element and requires
a nonzero divisor after conversion to the element type. Integer division truncates toward zero
as in C++.

[`Vector`](../vectors.hpp) and [`VectorView`](../vectors-view.hpp) scale components before taking
the Euclidean norm. In-place normalization leaves near-zero and non-finite inputs alone.
`Vector::normalized()` returns a zero vector for those inputs. `VectorView::near_equals()` rejects
NaN components and treats matching infinities as equal.

`rotate()` expects a unit quaternion. Reflection expects a unit normal, and refraction expects
unit incident and normal vectors.

## Polar coordinates

[`Polar2<T>`](../polar.hpp) stores a nonnegative radius and an angle in `[-π, π)`.
Angles are counterclockwise radians from positive X. A negative input radius is converted by
adding a half-turn. The origin has radius and angle zero. Use `from_cartesian()` and
`to_cartesian()` to convert between polar coordinates and `Vector2<T>`. `rotated()` adds
an angle and preserves the radius. Inputs must be finite, and a Cartesian point's radius must
be representable in the chosen scalar type.

## Interpolation

The functions in [interpolation.hpp](../interpolation.hpp) clamp finite interpolation parameters
to `[0, 1]`. Scalar and vector overloads expect matching floating-point types for new methods.

| Function | Contract |
| --- | --- |
| `smoothstep(t)`, `smootherstep(t)` | Return eased parameters. Both give zero first derivative at the endpoints. Smootherstep also gives zero second derivative. |
| `cubic_hermite(a, b, tangent_a, tangent_b, t)` | The tangents are derivatives with respect to the normalized segment parameter. Scale time-domain derivatives by the segment duration before passing them. |
| `cubic_bezier(a, control_a, control_b, b, t)` | The first and last arguments before `t` are the curve endpoints. The middle arguments shape the curve. |
| `centripetal_catmull_rom(p0, p1, p2, p3, t)` | Evaluates the segment from `p1` to `p2`. Knot intervals use the square root of chord length. A repeated outer neighbor gives zero tangent at that end. A zero-length central segment returns `p1`. [Centripetal parameterization](https://www.cemyuksel.com/research/catmullrom_param/) |
| `bilinear(v00, v10, v01, v11, x, y)` | Corners are ordered with X varying fastest. Each coordinate is clamped independently. |
| `trilinear(corners, x, y, z)` | Corners are indexed as `x + 2*y + 4*z`, using integer coordinates 0 or 1. Each interpolation coordinate is clamped independently. |
| `lerp(a, b, t, arc)` for `Polar2` | Interpolates radius linearly and sweeps the angle along `PolarArc::Shortest` by default or `PolarArc::Longest`. Opposite directions use a counterclockwise half-turn for the shortest mode and clockwise for the longest. Identical stored angles use zero sweep. At a zero-radius endpoint, interpolation borrows the other direction and follows a radial line. |
| `slerp(a, b, t)`, `nlerp(a, b, t)` | Inputs must be unit quaternions. Both take the shortest rotational arc and may negate the target. Slerp uses spherical interpolation with a normalized linear fallback for nearby orientations. Nlerp always uses normalized linear interpolation, so angular speed can vary. |

The spline parameter measures progress within a segment. Constant-speed traversal requires
mapping distance along the curve to that parameter.

For a straight line between two polar positions, convert them to Cartesian and use vector
`lerp()`. A polar sweep with changing radius can cover different distances at equal steps of `t`.
