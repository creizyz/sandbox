#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <numbers>

#include "math/geometry/rigid-motion.hpp"
#include "math/geometry/vector-geometry.hpp"
#include "math/linear-algebra/matrix.hpp"
#include "math/geometry/quaternion.hpp"
#include "math/linear-algebra/vectors.hpp"

namespace math::tests
{
    namespace
    {
        constexpr double pi = std::numbers::pi_v<double>;
        constexpr double tolerance = 1e-13;

        template <std::size_t N, typename T>
        void expect_vector_near(const Vector<N, T> &actual, const Vector<N, T> &expected)
        {
            constexpr auto error = std::numeric_limits<T>::epsilon() * T{64};
            for (std::size_t i = 0; i < N; ++i)
            {
                EXPECT_NEAR(actual[i], expected[i], error) << "component " << i;
            }
        }
    }

    TEST(Rotation, Cartesian2DUsesCounterclockwiseRadians)
    {
        const Vector2d forward{2.0, 0.0};
        expect_vector_near(rotate(forward, pi / 2.0), Vector2d{0.0, 2.0});
        expect_vector_near(rotate(forward, -pi / 2.0), Vector2d{0.0, -2.0});
    }

    TEST(Rotation, AxisAngleAndMatrixAgreeWithRightHandedRotation)
    {
        const auto rotation = Quaternion<double>::from_axis_angle(Vector3d{0.0, 0.0, 4.0}, pi / 2.0);
        const auto matrix = rotation_matrix(rotation);
        const Vector3d vector{1.0, 2.0, 3.0};
        const Vector3d expected{-2.0, 1.0, 3.0};
        EXPECT_NEAR(rotation.length(), 1.0, tolerance);
        expect_vector_near(rotate(vector, rotation), expected);
        expect_vector_near(matrix * vector, expected);
        expect_vector_near(matrix.transposed() * expected, vector);
        expect_vector_near(rotation_matrix(Quaternion<double>{-rotation.x(), -rotation.y(), -rotation.z(), -rotation.w()}) * vector, expected);
    }

    TEST(Rotation, AxisAngleHandlesExtremeNonzeroAxisMagnitudes)
    {
        const auto tiny_axis = Quaternion<float>::from_axis_angle(Vector3f{0.0f, std::numeric_limits<float>::min(), 0.0f}, std::numbers::pi_v<float>);
        const auto turned = rotate(Vector3f{1.0f, 0.0f, 0.0f}, tiny_axis);
        EXPECT_NEAR(turned.x(), -1.0f, 1e-6f);
        EXPECT_NEAR(turned.z(), 0.0f, 1e-6f);

        constexpr auto maximum = std::numeric_limits<double>::max();
        const auto huge_axis = Quaternion<double>::from_axis_angle(Vector3d{maximum, maximum, 0.0}, pi);
        expect_vector_near(rotate(Vector3d{1.0, 0.0, 0.0}, huge_axis), Vector3d{0.0, 1.0, 0.0});
    }

    TEST(Rotation, ThreeDimensionalRotationAroundAnObliqueAxis)
    {
        constexpr float angle = 2.0f * std::numbers::pi_v<float> / 3.0f;
        const Vector3f axis{1.0f, 1.0f, 1.0f};
        const Vector3f vector{1.0f, 2.0f, 3.0f};
        const Vector3f expected{3.0f, 1.0f, 2.0f};
        const auto orientation = Quaternion<float>::from_axis_angle(axis, angle);
        expect_vector_near(rotation_matrix(orientation) * vector, expected);
        expect_vector_near(rotate(vector, orientation), expected);
    }

    TEST(Rotation, LargeCartesianVectorPreservesRepresentableComponents)
    {
        const auto maximum = std::numeric_limits<double>::max();
        const auto tiny = std::numeric_limits<double>::denorm_min();
        const Quaternion<double> half_turn{0, 0, 1, 0};
        const Vector3d vector{maximum, tiny, tiny};
        const auto result = rotate(vector, half_turn);
        const auto matrix_result = rotation_matrix(half_turn) * vector;
        EXPECT_EQ(result.x(), -maximum);
        EXPECT_EQ(result.y(), -tiny);
        EXPECT_EQ(result.z(), tiny);
        for (std::size_t i = 0; i < 3; ++i)
        {
            EXPECT_EQ(result[i], matrix_result[i]);
        }
    }

    TEST(RigidPose, ConstructorsExposeTheStoredTranslationAndRotation)
    {
        constexpr Pose2<float> identity_2d;
        static_assert(identity_2d.angle() == 0.0f && identity_2d.translation().x() == 0.0f && identity_2d.translation().y() == 0.0f);
        const Pose2<float> pose_2d{Vector2f{2.0f, -7.0f}, std::numbers::pi_v<float>};
        EXPECT_EQ(pose_2d.translation(), (Vector2f{2.0f, -7.0f}));
        EXPECT_FLOAT_EQ(pose_2d.angle(), -std::numbers::pi_v<float>);

        constexpr Pose3<float> identity_3d;
        static_assert(identity_3d.rotation().w() == 1.0f && identity_3d.rotation().squared_length() == 1.0f && identity_3d.translation().squared_length() == 0.0f);
        const Pose3<float> pose_3d{Vector3f{1.0f, -2.0f, 3.0f}, Quaternion<float>{1.0f, 0.0f, 0.0f, 0.0f}};
        EXPECT_EQ(pose_3d.translation(), (Vector3f{1.0f, -2.0f, 3.0f}));
        expect_vector_near(rotate(Vector3f{0.0f, 1.0f, 0.0f}, pose_3d.rotation()), Vector3f{0.0f, -1.0f, 0.0f});
    }

    TEST(RigidPose, TwoDimensionalPointDirectionCompositionAndInverse)
    {
        const Pose2<double> outer{Vector2d{2.0, 3.0}, pi / 2.0};
        const Pose2<double> inner{Vector2d{1.0, 0.0}, pi / 2.0};
        const Vector2d local{1.0, 0.0};
        expect_vector_near(Pose2<double>{}.transform_point(local), local);
        expect_vector_near(outer.transform_point(local), Vector2d{2.0, 4.0});
        expect_vector_near(outer.transform_direction(local), Vector2d{0.0, 1.0});
        expect_vector_near(outer.inverse().transform_point(outer.transform_point(local)), local);
        expect_vector_near(outer.inverse().transform_direction(outer.transform_direction(local)), local);
        EXPECT_DOUBLE_EQ((outer * inner).angle(), -pi);
        expect_vector_near((outer * inner).transform_point(local), outer.transform_point(inner.transform_point(local)));
    }

    TEST(RigidPose, ThreeDimensionalCompositionAppliesRightHandPoseFirst)
    {
        const Pose3<double> outer{Vector3d{2.0, 3.0, 4.0}, Quaternion<double>::from_axis_angle(Vector3d{0.0, 0.0, 1.0}, pi / 2.0)};
        const Pose3<double> inner{Vector3d{1.0, 0.0, 0.0}, Quaternion<double>::from_axis_angle(Vector3d{1.0, 0.0, 0.0}, pi / 2.0)};
        const Vector3d local{0.0, 1.0, 0.0};
        const auto composed = outer * inner;
        expect_vector_near(Pose3<double>{}.transform_point(local), local);
        expect_vector_near(composed.transform_point(local), Vector3d{2.0, 4.0, 5.0});
        expect_vector_near(composed.transform_direction(local), Vector3d{0.0, 0.0, 1.0});
        expect_vector_near(composed.inverse().transform_point(composed.transform_point(local)), local);
        expect_vector_near(composed.inverse().transform_direction(composed.transform_direction(local)), local);
        EXPECT_NEAR(composed.rotation().length(), 1.0, tolerance);
    }

    TEST(AngularDynamics, PlanarAngularVelocityAdvancesAngleAndMovesAnOffsetPoint)
    {
        EXPECT_NEAR(integrate_angle(3.0 * pi / 4.0, pi, 0.5), -3.0 * pi / 4.0, tolerance);
        EXPECT_NEAR(integrate_angle(-3.0 * pi / 4.0, -pi, 0.5), 3.0 * pi / 4.0, tolerance);
        const Vector2d offset{2.0, 3.0};
        expect_vector_near(cross(4.0, offset), Vector2d{-12.0, 8.0});
        expect_vector_near(cross(offset, 4.0), Vector2d{12.0, -8.0});
    }

    TEST(AngularDynamics, ConstantWorldAngularVelocityLeftMultipliesTheOrientation)
    {
        const auto orientation = Quaternion<double>::from_axis_angle(Vector3d{0.0, 0.0, 1.0}, pi / 2.0);
        const Vector3d velocity{pi, 0.0, 0.0};
        const Vector3d local{1.0, 0.0, 0.0};
        const auto next = integrate_orientation(orientation, velocity, 0.5);
        expect_vector_near(rotate(local, next), Vector3d{0.0, 0.0, 1.0});
        expect_vector_near(rotate(local, integrate_orientation(next, velocity, -0.5)), rotate(local, orientation));
        expect_vector_near(rotate(local, integrate_orientation(orientation, velocity, 0.0)), rotate(local, orientation));
        EXPECT_NEAR(next.length(), 1.0, tolerance);

        const auto tiny = integrate_orientation(Quaternion<double>{}, Vector3d{0.0, 0.0, 1e-8}, 1.0);
        EXPECT_NEAR(tiny.z(), 5e-9, 1e-22);
    }

    TEST(AngularDynamics, AppliesAnisotropicBodyInverseInertiaInWorldCoordinates)
    {
        const double angle = pi / 4.0;
        const auto orientation = Quaternion<double>::from_axis_angle(Vector3d{0.0, 0.0, 1.0}, angle);
        const Matrix<3, 3, double> inverse_inertia{Vector3d{2.0, 0.0, 0.0}, Vector3d{0.0, 4.0, 0.0}, Vector3d{0.0, 0.0, 5.0}};
        const Vector3d torque{1.0, 0.0, 2.0};
        expect_vector_near(apply_inverse_inertia(orientation, inverse_inertia, torque), Vector3d{3.0, -1.0, 10.0});
        expect_vector_near(apply_inverse_inertia(orientation, Matrix<3, 3, double>{}, torque), Vector3d{});
    }

    TEST(AngularDynamics, SubdividingAConstantAngularStepPreservesTheRotation)
    {
        const Vector3f velocity{2.0f, -1.0f, 4.0f};
        const auto initial = Quaternion<float>::from_axis_angle(Vector3f{1.0f, 2.0f, 3.0f}, -0.75f);
        const auto whole = integrate_orientation(initial, velocity, 0.5f);
        const auto half = integrate_orientation(initial, velocity, 0.25f);
        const auto twice = integrate_orientation(half, velocity, 0.25f);
        expect_vector_near(rotate(Vector3f{1.0f, 0.0f, 0.0f}, whole), rotate(Vector3f{1.0f, 0.0f, 0.0f}, twice));
        expect_vector_near(rotate(Vector3f{0.0f, 1.0f, 0.0f}, whole), rotate(Vector3f{0.0f, 1.0f, 0.0f}, twice));
    }

#ifndef NDEBUG
    TEST(Rotation, AxisAngleRejectsAZeroAxisInDebug)
    {
        EXPECT_DEATH((void)Quaternion<double>::from_axis_angle(Vector3d{}, pi), "norm.m_scale");
    }

    TEST(RigidPose, ThreeDimensionalPoseRejectsNonUnitRotationInDebug)
    {
        EXPECT_DEATH((void)(Pose3<double>{Vector3d{}, Quaternion<double>{0.0, 0.0, 0.0, 2.0}}), "squared_length");
    }
#endif
}
