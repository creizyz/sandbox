#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>

#include "math/random/sampling.hpp"
#include "math/statistics/summary.hpp"

namespace math::tests
{
    TEST(RandomStream, UsesTheStandardGeneratorAndReproducibleLogicalStreams)
    {
        RandomStream random(5489);
        std::mt19937_64 reference(5489);
        EXPECT_EQ(random.next(), reference());
        RandomStream first(42, 1);
        RandomStream second(42, 2);
        RandomStream replay_second(42, 2);
        RandomStream replay_first(42, 1);
        EXPECT_NE(first.next(), second.next());
        static_cast<void>(replay_second.next());
        static_cast<void>(replay_first.next());
        for (int i = 0; i < 20; ++i)
        {
            EXPECT_EQ(first.next(), replay_first.next());
            EXPECT_EQ(second.next(), replay_second.next());
        }
    }

    TEST(RandomStream, BoundedDrawsReproduciblyRejectSamplesOutsideTheUnbiasedRange)
    {
        constexpr std::uint64_t bound = (std::uint64_t{1} << 63) + 1;
        constexpr auto threshold = (std::uint64_t{0} - bound) % bound;
        RandomStream random(101);
        std::mt19937_64 reference(101);
        bool rejected = false;
        for (int i = 0; i < 20; ++i)
        {
            auto sample = reference();
            while (sample < threshold)
            {
                rejected = true;
                sample = reference();
            }
            EXPECT_EQ(random.below(bound), sample % bound);
            const auto unit = random.unit();
            EXPECT_EQ(unit, static_cast<double>(reference() >> 11) * 0x1p-53);
            EXPECT_GE(unit, 0.0);
            EXPECT_LT(unit, 1.0);
        }
        EXPECT_TRUE(rejected);
        EXPECT_EQ(random.below(1), 0u);
        EXPECT_THROW((void)random.below(0), std::invalid_argument);
    }

    TEST(RunningStatistics, ComputesPopulationAndSampleVarianceAndMergesPartialResults)
    {
        constexpr std::array samples{2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
        RunningStatistics<double> left;
        RunningStatistics<double> right;
        EXPECT_FALSE(left.mean());
        EXPECT_FALSE(left.population_variance());
        EXPECT_FALSE(left.sample_variance());

        for (std::size_t i = 0; i < samples.size(); ++i)
        {
            if (i == 1)
            {
                EXPECT_FALSE(left.sample_variance());
            }
            (i < 3 ? left : right).add(samples[i]);
        }
        left.merge(right);
        EXPECT_EQ(left.count(), 8u);
        EXPECT_NEAR(*left.mean(), 5.0, 1e-14);
        EXPECT_NEAR(*left.population_variance(), 4.0, 1e-14);
        EXPECT_NEAR(*left.sample_variance(), 32.0 / 7.0, 1e-14);
        EXPECT_THROW(left.add(std::numeric_limits<double>::infinity()), std::invalid_argument);
        EXPECT_EQ(left.count(), 8u);
    }

    TEST(RunningStatistics, LargeOffsetsAndOverflowPreserveUsefulState)
    {
        RunningStatistics<double> moments;
        moments.add(1e12 + 1.0);
        moments.add(1e12 + 2.0);
        moments.add(1e12 + 3.0);
        EXPECT_DOUBLE_EQ(*moments.mean(), 1e12 + 2.0);
        EXPECT_DOUBLE_EQ(*moments.sample_variance(), 1.0);

        RunningStatistics<float> extreme;
        extreme.add(std::numeric_limits<float>::max());
        EXPECT_THROW(extreme.add(-std::numeric_limits<float>::max()), std::overflow_error);
        EXPECT_EQ(extreme.count(), 1u);
    }

    TEST(RunningStatistics, PreservesDispersionSmallerThanAnUlpOfTheMean)
    {
        constexpr double large = 0x1p53;
        for (const double first : {large, large + 2.0})
        {
            const double second = first == large ? large + 2.0 : large;
            RunningStatistics<> moments;
            RunningCovariance<> pairs;
            moments.add(first);
            moments.add(second);
            pairs.add(first, first);
            pairs.add(second, second);
            EXPECT_EQ(moments.sample_variance(), 2.0);
            EXPECT_EQ(pairs.sample_covariance(), 2.0);
            EXPECT_DOUBLE_EQ(*pairs.correlation(), 1.0);
            ASSERT_TRUE(pairs.linear_fit());
            EXPECT_EQ(pairs.linear_fit()->slope, 1.0);
        }
        RunningStatistics<> left;
        RunningStatistics<> right;
        left.add(large);
        left.add(large + 2.0);
        right.add(large + 4.0);
        left.merge(right);
        EXPECT_EQ(left.sample_variance(), 4.0);
    }

    TEST(WeightedStatistics, PreservesTheSmallerWeightsContributionInEitherOrder)
    {
        WeightedStatistics<> light;
        WeightedStatistics<> heavy;
        light.add(1e16, 1.0);
        heavy.add(0.0, 1e16);
        auto forward = light;
        forward.merge(heavy);
        heavy.merge(light);
        EXPECT_DOUBLE_EQ(*forward.mean(), 1.0);
        EXPECT_DOUBLE_EQ(*heavy.mean(), 1.0);
    }

    TEST(Quantile, InterpolatesBetweenOrderStatisticsWithoutMutatingInput)
    {
        constexpr std::array samples{10.0, 2.0, 2.0, 6.0};
        EXPECT_DOUBLE_EQ(*quantile<double>(samples, 0.0), 2.0);
        EXPECT_DOUBLE_EQ(*quantile<double>(samples, 0.5), 4.0);
        EXPECT_DOUBLE_EQ(*quantile<double>(samples, 0.75), 7.0);
        EXPECT_DOUBLE_EQ(*quantile<double>(samples, 1.0), 10.0);
        EXPECT_FALSE(quantile<double>({}, 0.5));
        EXPECT_THROW((void)quantile<double>(samples, -0.1), std::invalid_argument);
    }
}
