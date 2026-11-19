#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <limits>

#include "math/geometric.hpp"
#include "math/interpolation.hpp"

namespace math::tests
{
    template <typename T>
    concept SupportsMoveTowards = requires(Vector2<T> value, T distance) {
        move_towards(value, value, distance);
    };

    static_assert(SupportsMoveTowards<float>);
    static_assert(!SupportsMoveTowards<int>);

    TEST(Interpolation, SlerpRotatesHalfwayAndClampsEndpoints)
    {
        const Quaternion<double> start;
        const Quaternion<double> target{0.0, 0.0, 1.0, 0.0};
        const auto halfway = slerp(start, target, 0.5);
        EXPECT_NEAR(halfway.length(), 1.0, 1e-12);
        EXPECT_NEAR(halfway.z(), std::sqrt(0.5), 1e-12);
        EXPECT_NEAR(halfway.w(), std::sqrt(0.5), 1e-12);
        const auto rotated = rotate(Vector3d{1.0, 0.0, 0.0}, halfway);
        EXPECT_NEAR(rotated.x(), 0.0, 1e-12);
        EXPECT_NEAR(rotated.y(), 1.0, 1e-12);
        EXPECT_NEAR(slerp(start, target, -1.0).w(), 1.0, 1e-12);
        EXPECT_NEAR(slerp(start, target, 2.0).z(), 1.0, 1e-12);
    }

    TEST(Interpolation, SlerpUsesShortestArcAndHandlesNearlyEqualInputs)
    {
        const Quaternion<double> start;
        const Quaternion<double> same_rotation{0.0, 0.0, 0.0, -1.0};
        EXPECT_NEAR(slerp(start, same_rotation, 0.5).w(), 1.0, 1e-12);
        const Quaternion<double> close{0.0, 0.0, std::sin(0.001), std::cos(0.001)};
        const auto halfway = slerp(start, close, 0.5);
        EXPECT_NEAR(halfway.z(), std::sin(0.0005), 1e-12);
        EXPECT_NEAR(halfway.w(), std::cos(0.0005), 1e-12);

        const Quaternion<double> target{0.0, 0.0, -std::sin(0.5), -std::cos(0.5)};
        EXPECT_NEAR(slerp(start, target, 0.5).z(), std::sin(0.25), 1e-12);
    }

    TEST(Interpolation, MovesUpToTheRequestedDistanceAndLerpClamps)
    {
        const Vector2d end{3.0, 4.0};
        EXPECT_EQ(move_towards(Vector2d::zero, end, 2.5), (Vector2d{1.5, 2.0}));
        EXPECT_EQ(move_towards(Vector2d::zero, end, 6.0), end);
        EXPECT_EQ(lerp(Vector2d::zero, end, -0.5), Vector2d::zero);
        EXPECT_EQ(lerp(Vector2d::zero, end, 1.5), end);
    }

    TEST(Interpolation, EasingClampsAndIsSymmetric)
    {
        static_assert(smoothstep(-1.0) == 0.0);
        static_assert(smoothstep(2.0) == 1.0);
        static_assert(smoothstep(0.25) == 0.15625);
        static_assert(smootherstep(-1.0) == 0.0);
        static_assert(smootherstep(2.0) == 1.0);
        static_assert(smootherstep(0.25) == 0.103515625);
        EXPECT_DOUBLE_EQ(smoothstep(0.5), 0.5);
        EXPECT_DOUBLE_EQ(smootherstep(0.5), 0.5);
        EXPECT_DOUBLE_EQ(smootherstep(0.2) + smootherstep(0.8), 1.0);
    }

    TEST(Interpolation, HermiteUsesExplicitEndpointDerivatives)
    {
        static_assert(cubic_hermite(2.0, 6.0, 4.0, 4.0, 0.25) == 3.0);
        constexpr Vector2d a{0.0, 0.0};
        constexpr Vector2d b{2.0, 2.0};
        constexpr Vector2d first{4.0, 0.0};
        constexpr Vector2d last{0.0, 4.0};
        EXPECT_EQ(cubic_hermite(a, b, first, last, -1.0), a);
        EXPECT_EQ(cubic_hermite(a, b, first, last, 2.0), b);
        EXPECT_EQ(cubic_hermite(a, b, first, last, 0.5), (Vector2d{1.5, 0.5}));
        constexpr double step = 1e-6;
        const auto derivative = (cubic_hermite(a, b, first, last, step) - a) / step;
        EXPECT_NEAR(derivative.x(), first.x(), 1e-5);
        EXPECT_NEAR(derivative.y(), first.y(), 1e-5);
    }

    TEST(Interpolation, BezierUsesControlPointsAndClampsToExactEndpoints)
    {
        static_assert(cubic_bezier(0.0, 0.0, 0.0, 8.0, 0.5) == 1.0);
        constexpr Vector2d a{0.0, 0.0};
        constexpr Vector2d control_a{0.0, 2.0};
        constexpr Vector2d control_b{2.0, 2.0};
        constexpr Vector2d b{2.0, 0.0};
        EXPECT_EQ(cubic_bezier(a, control_a, control_b, b, 0.5), (Vector2d{1.0, 1.5}));
        EXPECT_EQ(cubic_bezier(a, control_a, control_b, b, -1.0), a);
        EXPECT_EQ(cubic_bezier(a, control_a, control_b, b, 2.0), b);
        constexpr double maximum = std::numeric_limits<double>::max();
        constexpr auto midpoint = lerp(Vector2d{-maximum, maximum}, Vector2d{maximum, 1.0}, 0.5);
        EXPECT_DOUBLE_EQ(midpoint.x(), 0.0);
        EXPECT_DOUBLE_EQ(lerp(Vector2d{maximum, 0.0}, Vector2d{1.0, 0.0}, 1.0).x(), 1.0);
    }

    TEST(Interpolation, CentripetalCatmullRomHandlesUnevenAndRepeatedPoints)
    {
        constexpr Vector2d p0{0.0, 0.0};
        constexpr Vector2d p1{1.0, 0.0};
        constexpr Vector2d p2{5.0, 0.0};
        constexpr Vector2d p3{14.0, 0.0};
        EXPECT_EQ(centripetal_catmull_rom(p0, p1, p2, p3, 0.0), p1);
        EXPECT_EQ(centripetal_catmull_rom(p0, p1, p2, p3, 1.0), p2);
        EXPECT_NEAR(centripetal_catmull_rom(p0, p1, p2, p3, 0.5).x(), 41.0 / 15.0, 1e-12);
        EXPECT_EQ(centripetal_catmull_rom(p0, p1, p1, p3, 0.5), p1);
        EXPECT_DOUBLE_EQ(centripetal_catmull_rom(p1, p1, p2, p2, 0.25).x(), 1.625);
    }

    TEST(Interpolation, GridInterpolationUsesXFastestAndClampsEachCoordinate)
    {
        static_assert(bilinear(1.0, 3.0, 4.0, 6.0, 0.25, 0.5) == 3.0);
        static_assert(bilinear(1.0, 3.0, 4.0, 6.0, -1.0, 2.0) == 4.0);
        constexpr std::array corners{1.0, 3.0, 4.0, 6.0, 5.0, 7.0, 8.0, 10.0};
        static_assert(trilinear(corners, 0.25, 0.5, 0.75) == 6.0);
        static_assert(trilinear(corners, 2.0, -1.0, 2.0) == 7.0);
        constexpr std::array vector_corners{
            Vector2f{0.0f, 0.0f},
            Vector2f{2.0f, 0.0f},
            Vector2f{0.0f, 4.0f},
            Vector2f{2.0f, 4.0f},
            Vector2f{8.0f, 0.0f},
            Vector2f{10.0f, 0.0f},
            Vector2f{8.0f, 4.0f},
            Vector2f{10.0f, 4.0f},
        };
        EXPECT_EQ(trilinear(vector_corners, 0.5f, 0.25f, 0.75f), (Vector2f{7.0f, 1.0f}));
    }

    TEST(Interpolation, NlerpNormalizesAndChoosesTheShortestSign)
    {
        const Quaternion<double> a;
        const Quaternion<double> b{0.0, 0.0, -std::sqrt(0.5), -std::sqrt(0.5)};
        const auto halfway = nlerp(a, b, 0.5);
        EXPECT_NEAR(halfway.length(), 1.0, 1e-12);
        EXPECT_NEAR(halfway.z(), slerp(a, b, 0.5).z(), 1e-12);
        EXPECT_GT(halfway.z(), 0.0);
        EXPECT_NEAR(nlerp(a, b, -1.0).w(), 1.0, 1e-12);
        EXPECT_NEAR(nlerp(a, b, 2.0).z(), -b.z(), 1e-12);
    }
}
