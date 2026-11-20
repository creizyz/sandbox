#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>

#include "math/numerics/discrete-time.hpp"
#include "math/numerics/fixed-point.hpp"
#include "math/numerics/interpolation.hpp"

namespace math::tests
{
    TEST(FixedPoint, StoresThousandthsAndDefinesRoundingForEachOperation)
    {
        using Amount = FixedPoint<>;
        static_assert((Amount::from_raw(1200) + Amount::from_raw(300)).raw() == 1500);
        static_assert((Amount::from_raw(1250) * Amount::from_raw(2000)).raw() == 2500);
        static_assert((Amount::from_raw(-1000) / Amount::from_integer(3)).raw() == -333);
        static_assert((Amount::from_raw(-5) * Amount::from_raw(500)).raw() == -2);
        EXPECT_EQ(Amount::from_float(0.0625).raw(), 63);
        EXPECT_EQ(Amount::from_float(-0.0625).raw(), -63);
        EXPECT_DOUBLE_EQ(Amount::from_raw(4750).to_float(), 4.75);
        EXPECT_LT(Amount::from_raw(-2), Amount{});
        EXPECT_THROW((void)Amount::from_float(std::numeric_limits<double>::infinity()), std::invalid_argument);
        EXPECT_THROW((void)Amount::from_float(0x1p63), std::overflow_error);
    }

    TEST(FixedPoint, HandlesWideIntermediatesAndTheSignedMinimum)
    {
        using Amount = FixedPoint<>;
        constexpr auto min = std::numeric_limits<std::int64_t>::min();
        constexpr auto max = std::numeric_limits<std::int64_t>::max();
        constexpr auto one = Amount::from_integer(1);
        static_assert((Amount::from_raw(min) * one).raw() == min);
        static_assert((Amount::from_raw(max) / one).raw() == max);
        static_assert((Amount::from_raw(100000000500) * Amount::from_raw(50000000300)).raw() == 5000000055000000150);
        static_assert((Amount::from_raw(9223372036854775707) / Amount::from_raw(3001)).raw() == 3073432867995593371);
        static_assert((Amount::from_raw(min) / Amount::from_raw(min)).raw() == 1000);
        EXPECT_THROW((void)(Amount::from_raw(min) / Amount::from_integer(-1)), std::overflow_error);
        EXPECT_THROW((void)(Amount::from_raw(max) + Amount::from_raw(1)), std::overflow_error);
        EXPECT_THROW((void)(Amount::from_raw(min) - Amount::from_raw(1)), std::overflow_error);
        EXPECT_THROW((void)(Amount::from_raw(max) * Amount::from_integer(2)), std::overflow_error);
        EXPECT_THROW((void)(one / Amount{}), std::domain_error);
    }

    TEST(DiscreteTime, IntegratesRatesAndNetsFlowsBeforeAdjustingTheBalance)
    {
        using Amount = FixedPoint<>;
        using Integer = std::int64_t;
        constexpr auto max = std::numeric_limits<Integer>::max();
        static_assert(advance_balance(max, Integer{1}, Integer{1}) == max);
        static_assert(advance_balance(Amount::from_raw(max), Amount::from_raw(1), Amount::from_raw(1)).raw() == max);
        EXPECT_EQ(integrate_rate(Integer{-4}, Integer{3}), -12);
        EXPECT_DOUBLE_EQ(integrate_rate(2.5, 0.5), 1.25);
        EXPECT_EQ(integrate_rate(Amount::from_raw(1250), Amount::from_raw(250)).raw(), 312);
        EXPECT_DOUBLE_EQ(advance_balance(10.0, 2.5, 1.0), 11.5);
        EXPECT_THROW((void)integrate_rate(max, Integer{2}), std::overflow_error);
        EXPECT_THROW((void)integrate_rate(1.0, -1.0), std::invalid_argument);
        EXPECT_THROW((void)advance_balance(max, Integer{1}, Integer{0}), std::overflow_error);
        EXPECT_THROW((void)advance_balance(1.0, 1.0, -1.0), std::invalid_argument);
    }

    TEST(DiscreteTime, DeliversEqualTicksInInsertionOrderAndSupportsMoveOnlyPayloads)
    {
        TickQueue<std::unique_ptr<int>> queue;
        EXPECT_FALSE(queue.next_tick());
        queue.schedule(10, std::make_unique<int>(1));
        queue.schedule_after(3, 2, std::make_unique<int>(2));
        queue.schedule(10, std::make_unique<int>(3));
        EXPECT_EQ(queue.next_tick(), 5u);
        EXPECT_FALSE(queue.pop_ready(4));
        const auto earlier = queue.pop_ready(6);
        ASSERT_TRUE(earlier);
        EXPECT_EQ(earlier->tick, 5u);
        EXPECT_EQ(*earlier->value, 2);
        EXPECT_EQ(*queue.pop_ready(10)->value, 1);
        EXPECT_EQ(*queue.pop_ready(10)->value, 3);
        EXPECT_EQ(queue.size(), 0u);
        EXPECT_THROW(queue.schedule_after(std::numeric_limits<Tick>::max(), 1, std::make_unique<int>(4)), std::overflow_error);
        EXPECT_FALSE(queue.next_tick());
    }

    TEST(Response, ClampsEndpointsAndHandlesARepresentableMapAcrossAnOverflowingInterval)
    {
        constexpr auto max = std::numeric_limits<double>::max();
        EXPECT_DOUBLE_EQ(clamped_linear_map(-2.0, -1.0, 1.0, 3.0, 5.0), 3.0);
        EXPECT_DOUBLE_EQ(clamped_linear_map(2.0, -1.0, 1.0, 3.0, 5.0), 5.0);
        EXPECT_DOUBLE_EQ(clamped_linear_map(0.0, -max, max, 5.0, 3.0), 4.0);
        EXPECT_THROW((void)clamped_linear_map(0.0, 1.0, 1.0, 3.0, 5.0), std::invalid_argument);
    }

    TEST(Response, ExponentialSmoothingIsIndependentOfStepSubdivisionForAFixedTarget)
    {
        const auto part = smooth_towards(0.0, 10.0, 0.25, 1.0);
        EXPECT_NEAR(smooth_towards(part, 10.0, 0.75, 1.0), 10.0 * (1.0 - std::exp(-1.0)), 1e-14);
        EXPECT_DOUBLE_EQ(smooth_towards(5.0, 10.0, 0.0, 1.0), 5.0);
        EXPECT_GT(smooth_towards(0.0, 10.0, 1e-20, 1.0), 0.0);
        EXPECT_DOUBLE_EQ(smooth_towards(0.0, 10.0, 1e20, 1.0), 10.0);
        EXPECT_THROW((void)smooth_towards(0.0, 10.0, 1.0, 0.0), std::invalid_argument);
    }
}
