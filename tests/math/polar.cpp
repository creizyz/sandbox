#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <numbers>

#include "math/interpolation.hpp"
#include "math/polar.hpp"

namespace math::tests
{
    namespace
    {
        constexpr double pi = std::numbers::pi_v<double>;

        constexpr double radians(double degrees)
        {
            return degrees * pi / 180.0;
        }

        template <typename V>
        concept SupportsCubicHermite = requires(V point) {
            cubic_hermite(point, point, point, point, 0.5);
        };

        static_assert(!SupportsCubicHermite<Polar2d>);
    }

    TEST(Polar, CanonicalizesRadiusAngleAndOrigin)
    {
        const Polar2d wrapped{2.0, 5.0 * pi / 2.0};
        EXPECT_DOUBLE_EQ(wrapped.radius(), 2.0);
        EXPECT_NEAR(wrapped.angle(), pi / 2.0, 1e-14);
        const Polar2d negative{-2.0, 0.0};
        EXPECT_DOUBLE_EQ(negative.radius(), 2.0);
        EXPECT_DOUBLE_EQ(negative.angle(), -pi);
        const Polar2d origin{-0.0, pi / 2.0};
        EXPECT_DOUBLE_EQ(origin.radius(), 0.0);
        EXPECT_FALSE(std::signbit(origin.radius()));
        EXPECT_DOUBLE_EQ(origin.angle(), 0.0);
        EXPECT_DOUBLE_EQ((Polar2d{2.0, pi}).angle(), -pi);
    }

    TEST(Polar, ConvertsCartesianPointsInEveryQuadrant)
    {
        const std::array points{Vector2d{3.0, 4.0}, Vector2d{-3.0, 4.0}, Vector2d{-3.0, -4.0}, Vector2d{3.0, -4.0}};
        for (const auto &point : points)
        {
            const auto polar = Polar2d::from_cartesian(point);
            EXPECT_DOUBLE_EQ(polar.radius(), 5.0);
            EXPECT_NEAR(polar.angle(), std::atan2(point.y(), point.x()), 1e-14);
            const auto round_trip = polar.to_cartesian();
            EXPECT_NEAR(round_trip.x(), point.x(), 1e-14);
            EXPECT_NEAR(round_trip.y(), point.y(), 1e-14);
        }
        const auto axis = Polar2d::from_cartesian(Vector2d{-2.0, 0.0});
        EXPECT_DOUBLE_EQ(axis.angle(), -pi);
        const auto origin = Polar2f::from_cartesian(Vector2f{-0.0f, -0.0f});
        EXPECT_FLOAT_EQ(origin.radius(), 0.0f);
        EXPECT_FLOAT_EQ(origin.angle(), 0.0f);
        EXPECT_TRUE(std::isfinite(Polar2d::from_cartesian(Vector2d{1e200, 1e200}).radius()));
    }

    TEST(Polar, RotatesInRadiansAndPreservesRadius)
    {
        const Polar2d start{3.0, pi / 2.0};
        const auto rotated = start.rotated(pi / 2.0);
        EXPECT_DOUBLE_EQ(rotated.radius(), 3.0);
        EXPECT_DOUBLE_EQ(rotated.angle(), -pi);
        const auto cartesian = rotated.to_cartesian();
        EXPECT_NEAR(cartesian.x(), -3.0, 1e-14);
        EXPECT_NEAR(cartesian.y(), 0.0, 1e-14);
        EXPECT_DOUBLE_EQ(Polar2d{}.rotated(pi).angle(), 0.0);
    }

    TEST(PolarInterpolation, ShortAndLongSweepsCrossDifferentSidesOfWrapBoundary)
    {
        const Polar2d a{1.0, radians(170.0)};
        const Polar2d b{3.0, radians(-170.0)};
        const auto shortest = lerp(a, b, 0.25);
        const auto longest = lerp(a, b, 0.25, PolarArc::Longest);
        EXPECT_DOUBLE_EQ(shortest.radius(), 1.5);
        EXPECT_DOUBLE_EQ(longest.radius(), 1.5);
        EXPECT_NEAR(shortest.angle(), radians(175.0), 1e-14);
        EXPECT_NEAR(longest.angle(), radians(85.0), 1e-14);
        EXPECT_NEAR(lerp(b, a, 0.25).angle(), radians(-175.0), 1e-14);
        EXPECT_NEAR(lerp(b, a, 0.25, PolarArc::Longest).angle(), radians(-85.0), 1e-14);
    }

    TEST(PolarInterpolation, UsesDeterministicOppositeAndIdenticalDirectionRules)
    {
        const Polar2d a{1.0, 0.0};
        const Polar2d opposite{1.0, pi};
        EXPECT_DOUBLE_EQ(lerp(a, opposite, 0.5).angle(), pi / 2.0);
        EXPECT_DOUBLE_EQ(lerp(a, opposite, 0.5, PolarArc::Longest).angle(), -pi / 2.0);

        for (const auto arc : {PolarArc::Shortest, PolarArc::Longest})
        {
            const auto same_direction = lerp(Polar2d{1.0, pi / 3.0}, Polar2d{3.0, pi / 3.0}, 0.5, arc);
            EXPECT_DOUBLE_EQ(same_direction.radius(), 2.0);
            EXPECT_DOUBLE_EQ(same_direction.angle(), pi / 3.0);
        }
    }

    TEST(PolarInterpolation, ClampsEndpointsAndBorrowsDirectionAtOrigin)
    {
        const Polar2f origin;
        const Polar2f target{4.0f, -std::numbers::pi_v<float> / 2.0f};
        for (const auto arc : {PolarArc::Shortest, PolarArc::Longest})
        {
            const auto from_origin = lerp(origin, target, 0.25f, arc);
            const auto to_origin = lerp(target, origin, 0.75f, arc);
            EXPECT_FLOAT_EQ(from_origin.radius(), 1.0f);
            EXPECT_FLOAT_EQ(to_origin.radius(), 1.0f);
            EXPECT_FLOAT_EQ(from_origin.angle(), target.angle());
            EXPECT_FLOAT_EQ(to_origin.angle(), target.angle());
            EXPECT_FLOAT_EQ(lerp(origin, target, -1.0f, arc).radius(), 0.0f);
            EXPECT_FLOAT_EQ(lerp(origin, target, 2.0f, arc).angle(), target.angle());
            EXPECT_FLOAT_EQ(lerp(target, origin, 1.0f, arc).angle(), 0.0f);
            EXPECT_FLOAT_EQ(lerp(origin, origin, 0.5f, arc).angle(), 0.0f);
        }
    }
}
