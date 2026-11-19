# Rigid motion benchmarks

[The benchmark](../rigid-motion.cpp) measures rotation, pose, and angular dynamics operations
with both `float` and `double`. Pose cases cover identity and explicit construction, reading the
translation and rotation, point and direction transforms, inversion, and composition.
The 2D cross product is measured in both argument orders.

Each timed iteration performs one operation. The fixture cycles through four inputs with mixed
angles, axes, translations, and anisotropic inverse inertia tensors. One angular step crosses
the 2D wrap boundary. Fixture construction happens before timing. Timings include input selection,
compiler barriers, and result handling. The `read_vector2` case provides a reference for the
overhead of the smallest cases. The suite uses one thread and a small, repeatedly accessed fixture.

## Run

Use a Release build for measurements. [Build and validation](../../../docs/development/02-build-and-validation.md)

```sh
cmake --build cmake-build-release --target benchmarks_run
./cmake-build-release/benchmarks/benchmarks_run \
  --benchmark_filter='^bm_rigid_motion/' --benchmark_min_time=0.05s --benchmark_repetitions=5 \
  --benchmark_enable_random_interleaving=true \
  --benchmark_out=cmake-build-release/rigid-motion-benchmarks.json --benchmark_out_format=json
```

## Initial measurements

Measured on 19 November 2026 at `07173e0` with the rigid motion changes in the working tree.
The host was an Intel Core i7-10870H running WSL2 Ubuntu. The Release build used C++23,
GCC 15.2, `-O3`, `-DNDEBUG`, and Google Benchmark 1.10.0.

Median CPU time in nanoseconds per operation, with the minimum and maximum of five
repetitions in parentheses.

| Case | float | double |
| --- | ---: | ---: |
| `read_vector2` | 0.52 (0.52–0.55) | 0.54 (0.52–0.63) |
| `rotate2` | 7.03 (6.88–7.29) | 11.28 (11.04–12.89) |
| `axis_angle` | 10.99 (10.65–12.01) | 17.13 (16.61–20.50) |
| `rotation_matrix` | 8.44 (8.29–8.50) | 10.76 (10.67–12.47) |
| `cross_angular_offset` | 0.95 (0.94–0.97) | 0.69 (0.69–0.72) |
| `cross_offset_angular` | 0.80 (0.77–0.91) | 0.71 (0.69–0.77) |
| `pose2_identity` | 0.53 (0.53–0.60) | 0.53 (0.53–0.59) |
| `pose2_construct` | 8.32 (7.68–8.49) | 8.00 (7.81–9.29) |
| `pose2_read` | 0.58 (0.53–0.60) | 0.59 (0.57–0.62) |
| `pose2_transform_point` | 8.34 (8.27–9.53) | 14.50 (14.21–15.90) |
| `pose2_transform_direction` | 8.05 (7.98–8.32) | 13.89 (13.15–14.22) |
| `pose2_inverse` | 15.28 (14.63–15.59) | 21.51 (20.69–23.29) |
| `pose2_compose` | 14.61 (14.53–16.12) | 18.76 (18.67–18.85) |
| `pose3_identity` | 0.69 (0.68–0.71) | 1.15 (1.14–1.18) |
| `pose3_construct` | 0.72 (0.70–0.74) | 0.93 (0.92–1.00) |
| `pose3_read` | 0.70 (0.69–0.72) | 0.92 (0.92–1.00) |
| `pose3_transform_point` | 4.88 (4.83–5.06) | 4.88 (4.85–4.95) |
| `pose3_transform_direction` | 4.46 (4.43–4.63) | 4.67 (4.43–4.82) |
| `pose3_inverse` | 4.81 (4.73–5.34) | 5.70 (5.63–5.79) |
| `pose3_compose` | 16.61 (16.12–19.07) | 17.85 (17.40–19.00) |
| `integrate_angle` | 29.29 (28.30–32.38) | 28.58 (27.70–29.15) |
| `integrate_orientation` | 38.83 (38.60–39.56) | 44.23 (43.73–45.74) |
| `apply_inverse_inertia` | 12.60 (12.03–14.18) | 12.25 (11.88–12.92) |

Identity construction and the smallest value reads are close to the `read_vector2` reference.
Loop and result handling account for a substantial part of those timings.
