#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <numbers>
#include <string>
#include <utility>

#include "math/angular-dynamics.hpp"
#include "math/geometric.hpp"
#include "math/matrix.hpp"
#include "math/pose.hpp"
#include "math/quaternion.hpp"
#include "math/vectors.hpp"

namespace
{
    using namespace math;

    template <typename T>
    struct Sample
    {
        Vector2<T> offset_2d;
        Vector3<T> offset_3d;
        Vector3<T> axis;
        T angle;
        T angular_speed;
        T timestep;
        Quaternion<T> rotation;
        Pose2<T> pose_2d;
        Pose2<T> child_2d;
        Pose3<T> pose_3d;
        Pose3<T> child_3d;
        Matrix<3, 3, T> inverse_inertia;
    };

    template <typename T>
    std::array<Sample<T>, 4> make_samples()
    {
        constexpr auto pi = std::numbers::pi_v<T>;
        constexpr std::array angles{T{1} / T{4}, pi - T{1} / T{64}, -T{1}, T{2}};
        std::array<Sample<T>, angles.size()> samples{};
        for (std::size_t i = 0; i < samples.size(); ++i)
        {
            const T value = static_cast<T>(i + 1);
            const T angle = angles[i];
            const Vector2<T> offset_2d{value, T{1} - value};
            const Vector3<T> offset_3d{T{3} - value, T{2} + value, -value};
            const Vector3<T> axis{value, T{2}, -T{1}};
            const auto rotation = Quaternion<T>::from_axis_angle(axis, angle);
            const auto child_rotation = Quaternion<T>::from_axis_angle(Vector3<T>{T{1}, value, T{2}}, -angle / T{2});
            const Matrix<3, 3, T> inverse_inertia{Vector3<T>{T{2}, T{1} / T{4}, 0},
                                                  Vector3<T>{T{1} / T{4}, T{3}, 0},
                                                  Vector3<T>{0, 0, T{1} + value}};
            samples[i] = {offset_2d, offset_3d, axis, angle, T{2} * value, T{1} / T{64}, rotation,
                          Pose2<T>{offset_2d, angle}, Pose2<T>{-offset_2d, -angle / T{2}},
                          Pose3<T>{offset_3d, rotation}, Pose3<T>{-offset_3d, child_rotation},
                          inverse_inertia};
        }
        return samples;
    }

    template <typename T, typename Operation>
    void measure(benchmark::State &state, Operation operation)
    {
        auto samples = make_samples<T>();
        std::size_t index = 0;
        for (auto _ : state)
        {
            auto *input = &samples[index];
            benchmark::DoNotOptimize(input);
            auto result = operation(*input);
            benchmark::DoNotOptimize(result);
            index = (index + 1) % samples.size();
        }
    }

    template <typename Operation>
    void register_rigid_motion(const std::string &operation_name, Operation operation)
    {
        const std::string name = "bm_rigid_motion/" + operation_name;
        benchmark::RegisterBenchmark(name + "/float", [operation](benchmark::State &state)
                                     { measure<float>(state, operation); });
        benchmark::RegisterBenchmark(name + "/double", [operation](benchmark::State &state)
                                     { measure<double>(state, operation); });
    }

    [[maybe_unused]] const bool registered = []
    {
        register_rigid_motion("read_vector2", [](const auto &s)
                              { return s.offset_2d; });
        register_rigid_motion("rotate2", [](const auto &s)
                              { return rotate(s.offset_2d, s.angle); });
        register_rigid_motion("axis_angle", [](const auto &s)
                              { return decltype(s.rotation)::from_axis_angle(s.axis, s.angle); });
        register_rigid_motion("rotation_matrix", [](const auto &s)
                              { return rotation_matrix(s.rotation); });
        register_rigid_motion("cross_angular_offset", [](const auto &s)
                              { return cross(s.angular_speed, s.offset_2d); });
        register_rigid_motion("cross_offset_angular", [](const auto &s)
                              { return cross(s.offset_2d, s.angular_speed); });

        register_rigid_motion("pose2_identity", [](const auto &s)
                              { return decltype(s.pose_2d){}; });
        register_rigid_motion("pose2_construct", [](const auto &s)
                              { return decltype(s.pose_2d){s.offset_2d, s.angle}; });
        register_rigid_motion("pose2_read", [](const auto &s)
                              { return std::pair{s.pose_2d.translation(), s.pose_2d.angle()}; });
        register_rigid_motion("pose2_transform_point", [](const auto &s)
                              { return s.pose_2d.transform_point(s.child_2d.translation()); });
        register_rigid_motion("pose2_transform_direction", [](const auto &s)
                              { return s.pose_2d.transform_direction(s.child_2d.translation()); });
        register_rigid_motion("pose2_inverse", [](const auto &s)
                              { return s.pose_2d.inverse(); });
        register_rigid_motion("pose2_compose", [](const auto &s)
                              { return s.pose_2d * s.child_2d; });

        register_rigid_motion("pose3_identity", [](const auto &s)
                              { return decltype(s.pose_3d){}; });
        register_rigid_motion("pose3_construct", [](const auto &s)
                              { return decltype(s.pose_3d){s.offset_3d, s.rotation}; });
        register_rigid_motion("pose3_read", [](const auto &s)
                              { return std::pair{s.pose_3d.translation(), s.pose_3d.rotation()}; });
        register_rigid_motion("pose3_transform_point", [](const auto &s)
                              { return s.pose_3d.transform_point(s.child_3d.translation()); });
        register_rigid_motion("pose3_transform_direction", [](const auto &s)
                              { return s.pose_3d.transform_direction(s.child_3d.translation()); });
        register_rigid_motion("pose3_inverse", [](const auto &s)
                              { return s.pose_3d.inverse(); });
        register_rigid_motion("pose3_compose", [](const auto &s)
                              { return s.pose_3d * s.child_3d; });

        register_rigid_motion("integrate_angle", [](const auto &s)
                              { return integrate_angle(s.angle, s.angular_speed, s.timestep); });
        register_rigid_motion("integrate_orientation", [](const auto &s)
                              { return integrate_orientation(s.rotation, s.axis, s.timestep); });
        register_rigid_motion("apply_inverse_inertia", [](const auto &s)
                              { return apply_inverse_inertia(s.rotation, s.inverse_inertia, s.offset_3d); });
        return true;
    }();
}
