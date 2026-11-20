#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <random>
#include <span>
#include <stdexcept>
#include <vector>

#include "math/linear-algebra/solver.hpp"
#include "math/numerics/numerical-analysis.hpp"
#include "math/random/sampling.hpp"
#include "math/statistics/summary.hpp"

namespace math::tests
{
    static_assert(std::uniform_random_bit_generator<RandomStream>);

    TEST(RandomDistributions, SupportsStandardDistributionsWithReproducibleStreams)
    {
        RandomStream stream(20);
        RandomStream replay(20);
        std::poisson_distribution<int> first(4.0);
        std::poisson_distribution<int> second(4.0);
        for (int i = 0; i < 100; ++i)
        {
            EXPECT_EQ(first(stream), second(replay));
        }
    }

    TEST(RandomDistributions, UsesClosedIntegersAndHalfOpenRealsAtTheLimits)
    {
        RandomStream random(42);
        RandomStream reference(42);
        constexpr auto min = std::numeric_limits<std::int64_t>::min();
        constexpr auto max = std::numeric_limits<std::int64_t>::max();
        EXPECT_EQ(random.uniform_integer(min, max), static_cast<std::int64_t>(static_cast<std::uint64_t>(min) + reference.next()));
        EXPECT_EQ(random.uniform_integer(max, max), max);
        EXPECT_EQ(random.uniform_real(2.0, 2.0), 2.0);
        EXPECT_FALSE(random.bernoulli(0.0));
        EXPECT_TRUE(random.bernoulli(1.0));
        EXPECT_EQ(random.next(), reference.next());
        for (int i = 0; i < 64; ++i)
        {
            const auto integer = random.uniform_integer(-7, 5);
            EXPECT_GE(integer, -7);
            EXPECT_LE(integer, 5);
            EXPECT_EQ(random.uniform_real(1.0, std::nextafter(1.0, 2.0)), 1.0);
            const auto full = random.uniform_real(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
            EXPECT_TRUE(std::isfinite(full));
        }
        EXPECT_THROW((void)random.uniform_integer(5, -5), std::invalid_argument);
        EXPECT_THROW((void)random.bernoulli(std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
    }

    TEST(RandomDistributions, NormalAndExponentialReplayAndHaveExpectedMoments)
    {
        RandomStream stream(909, 7);
        RandomStream replay(909, 7);
        RunningStatistics gaussian;
        RunningStatistics arrivals;
        for (int i = 0; i < 16000; ++i)
        {
            const auto normal = stream.normal(2.0, 3.0);
            const auto exponential = stream.exponential(2.0);
            EXPECT_EQ(normal, replay.normal(2.0, 3.0));
            EXPECT_EQ(exponential, replay.exponential(2.0));
            gaussian.add(normal);
            arrivals.add(exponential);
        }
        EXPECT_NEAR(*gaussian.mean(), 2.0, 0.10);
        EXPECT_NEAR(*gaussian.population_variance(), 9.0, 0.40);
        EXPECT_NEAR(*arrivals.mean(), 0.5, 0.025);
        EXPECT_NEAR(*arrivals.population_variance(), 0.25, 0.035);
        EXPECT_EQ(stream.normal(4.0, 0.0), 4.0);
        EXPECT_THROW((void)stream.normal(0.0, -1.0), std::invalid_argument);
        EXPECT_THROW((void)stream.exponential(0.0), std::invalid_argument);
    }

    TEST(RandomDistributions, WeightedDrawsIgnoreZeroWeightsAndHandleAScaledSum)
    {
        constexpr auto max = std::numeric_limits<double>::max();
        const WeightedSampler sampler(std::array{0.0, max, 0.0, max / 3.0, 0.0});
        RandomStream stream(21);
        std::array<int, 5> counts{};
        for (int i = 0; i < 4000; ++i)
        {
            ++counts.at(sampler.draw(stream));
        }
        EXPECT_EQ(counts[0] + counts[2] + counts[4], 0);
        EXPECT_NEAR(counts[1], 3000, 140);
        EXPECT_THROW((void)WeightedSampler(std::array{0.0, 0.0}), std::invalid_argument);
        EXPECT_THROW((void)WeightedSampler(std::array{1.0, -1.0}), std::invalid_argument);
    }

    TEST(RandomDistributions, PartialShuffleMatchesThePrefixOfAFullShuffle)
    {
        constexpr std::array initial{0, 1, 2, 3, 4, 5, 6, 7};
        auto partial = initial;
        auto shuffled = initial;
        RandomStream stream(11);
        RandomStream replay(11);
        const auto sample = stream.sample_prefix(std::span{partial}, 4);
        replay.shuffle(std::span{shuffled});
        EXPECT_TRUE(std::equal(sample.begin(), sample.end(), shuffled.begin()));
        EXPECT_TRUE(std::is_permutation(shuffled.begin(), shuffled.end(), initial.begin()));
        EXPECT_TRUE(std::is_permutation(partial.begin(), partial.end(), initial.begin()));
        EXPECT_THROW((void)stream.sample_prefix(std::span{partial}, initial.size() + 1), std::out_of_range);
    }

    TEST(StatisticalAnalysis, ComputesCovarianceCorrelationAndOLSWithMerge)
    {
        RunningCovariance left;
        RunningCovariance right;
        EXPECT_FALSE(left.linear_fit());
        left.add(1.0, 5.0);
        left.add(2.0, 3.0);
        right.add(3.0, 1.0);
        right.add(4.0, -1.0);
        left.merge(right);
        EXPECT_EQ(left.count(), 4u);
        EXPECT_NEAR(*left.population_covariance(), -2.5, 1e-14);
        EXPECT_NEAR(*left.sample_covariance(), -10.0 / 3.0, 1e-14);
        EXPECT_NEAR(*left.correlation(), -1.0, 1e-14);
        ASSERT_TRUE(left.linear_fit());
        EXPECT_NEAR(left.linear_fit()->slope, -2.0, 1e-14);
        EXPECT_NEAR(left.linear_fit()->intercept, 7.0, 1e-14);
        EXPECT_THROW(left.add(1.0, std::numeric_limits<double>::infinity()), std::invalid_argument);
        EXPECT_EQ(left.count(), 4u);

        RunningCovariance degenerate;
        degenerate.add(1.0, 0.0);
        degenerate.add(1.0, 3.0);
        EXPECT_FALSE(degenerate.correlation());
        EXPECT_FALSE(degenerate.linear_fit());
    }

    TEST(StatisticalAnalysis, WeightedMomentsMatchRepeatedSamplesAndMerge)
    {
        WeightedStatistics left;
        WeightedStatistics right;
        left.add(1.0, 1.0);
        right.add(5.0, 3.0);
        right.add(100.0, 0.0);
        left.merge(right);
        EXPECT_EQ(left.weight(), 4.0);
        EXPECT_EQ(left.mean(), 4.0);
        EXPECT_EQ(left.population_variance(), 3.0);
        EXPECT_THROW(left.add(2.0, -1.0), std::invalid_argument);
        EXPECT_EQ(left.weight(), 4.0);

        RunningStatistics ordinary;
        ordinary.add(0.0);
        ordinary.add(2.0);
        EXPECT_DOUBLE_EQ(*ordinary.sample_standard_deviation(), std::sqrt(2.0));
        EXPECT_DOUBLE_EQ(*ordinary.standard_error_of_mean(), 1.0);
    }

    TEST(StatisticalAnalysis, HistogramOwnsEndpointCountsAndQuantilesKeepRequestOrder)
    {
        constexpr std::array values{2.0, -1.0, 0.0, 1.0, 1.0, 3.0};
        constexpr std::array edges{0.0, 1.0, 2.0};
        const auto counts = histogram<double>(values, edges);
        EXPECT_EQ(counts.bins, (std::vector<std::size_t>{1, 3}));
        EXPECT_EQ(counts.underflow, 1u);
        EXPECT_EQ(counts.overflow, 1u);
        EXPECT_THROW((void)histogram<double>(values, std::array{0.0, 0.0}), std::invalid_argument);

        constexpr std::array probabilities{0.9, 0.0, 0.5, 1.0, 0.5};
        const auto batch = quantiles<double>(values, probabilities);
        ASSERT_TRUE(batch);
        for (std::size_t i = 0; i < probabilities.size(); ++i)
        {
            EXPECT_DOUBLE_EQ((*batch)[i], *quantile<double>(values, probabilities[i]));
        }
        EXPECT_FALSE(quantiles<double>({}, probabilities));
        EXPECT_THROW((void)quantiles<double>(values, std::array{1.1}), std::invalid_argument);
    }

    TEST(StatisticalAnalysis, LaggedCorrelationMeasuresPairedSubsequences)
    {
        constexpr std::array values{1.0, -1.0, 1.0, -1.0, 1.0};
        EXPECT_NEAR(*lagged_correlation<double>(values, 1), -1.0, 1e-14);
        EXPECT_NEAR(*lagged_correlation<double>(values, 2), 1.0, 1e-14);
        EXPECT_FALSE(lagged_correlation<double>(values, 4));
        EXPECT_FALSE(lagged_correlation<double>(values, 5));
    }

    TEST(NumericalAnalysis, CompensatedSumPreservesSmallTerms)
    {
        EXPECT_DOUBLE_EQ(compensated_sum<double>(std::array{1e16, 1.0, -1e16, 0.5}), 1.5);
        EXPECT_DOUBLE_EQ(compensated_sum<double>({}), 0.0);
        const auto maximum = std::numeric_limits<double>::max();
        EXPECT_THROW((void)compensated_sum<double>(std::array{maximum, maximum}), std::overflow_error);
    }

    TEST(NumericalAnalysis, FindsBracketedRootsAndConstrainedMinima)
    {
        const auto quadratic = [](double x)
        { return x * x - 2.0; };
        const auto root = bisect_root(quadratic, 0.0, 2.0, 1e-12);
        ASSERT_TRUE(root);
        EXPECT_NEAR(*root, std::sqrt(2.0), 1e-12);
        EXPECT_FALSE(bisect_root(quadratic, 2.0, 3.0, 1e-12));
        EXPECT_FALSE(bisect_root(quadratic, 0.0, 2.0, 1e-12, 1));
        EXPECT_TRUE(bisect_root(quadratic, 0.0, 2.0, 0.5, 1));
        EXPECT_EQ(bisect_root([](double x)
                              { return x; }, 0.0, 1.0, 1e-9),
                  0.0);

        const auto minimum = golden_section_minimum([](double x)
                                                    { return (x - 1.25) * (x - 1.25); }, -6.0, 8.0, 1e-10);
        ASSERT_TRUE(minimum);
        EXPECT_NEAR(*minimum, 1.25, 2e-10);
        EXPECT_EQ(golden_section_minimum([](double x)
                                         { return -x; }, 0.0, 1.0, 1e-9),
                  1.0);
        EXPECT_THROW((void)bisect_root(quadratic, 1.0, 0.0, 1e-9), std::invalid_argument);
    }

    TEST(NumericalAnalysis, DifferentiatesAndIntegratesWithExplicitErrorControls)
    {
        const auto sine = [](double x)
        { return std::sin(x); };
        EXPECT_NEAR(central_derivative(sine, 0.3, 1e-5), std::cos(0.3), 1e-10);
        EXPECT_THROW((void)central_derivative(sine, 1.0, 1e-30), std::invalid_argument);
        const auto integral = integrate_adaptive(sine, 0.0, std::numbers::pi, 1e-10);
        const auto reversed = integrate_adaptive(sine, std::numbers::pi, 0.0, 1e-10);
        ASSERT_TRUE(integral && reversed);
        EXPECT_NEAR(*integral, 2.0, 1e-10);
        EXPECT_DOUBLE_EQ(*reversed, -*integral);
        EXPECT_FALSE(integrate_adaptive(sine, 0.0, std::numbers::pi, 1e-10, 0));
        EXPECT_EQ(integrate_adaptive(sine, 1.0, 1.0, 1e-10), 0.0);
    }

    TEST(LinearSolve, HandlesPivotingAndReturnsFailureForSingularSystems)
    {
        const auto result = solve_linear_system<double>(std::array{0.0, 1.0, 2.0, 3.0}, std::array{4.0, 14.0});
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, (std::vector<double>{1.0, 4.0}));
        const auto scaled = solve_linear_system<double>(std::array{1e-100, 0.0, 0.0, 1e100}, std::array{2e-100, 3e100});
        ASSERT_TRUE(scaled);
        EXPECT_DOUBLE_EQ((*scaled)[0], 2.0);
        EXPECT_DOUBLE_EQ((*scaled)[1], 3.0);
        EXPECT_FALSE(solve_linear_system<double>(std::array{1.0, 2.0, 2.0, 4.0}, std::array{1.0, 2.0}));
        EXPECT_THROW((void)solve_linear_system<double>(std::array{1.0}, std::array{1.0, 2.0}), std::invalid_argument);
    }

    TEST(LinearSolve, RecoversKnownSolutionsFromPermutedDenseRows)
    {
        constexpr std::size_t size = 9;
        constexpr std::array known{1.0, 2.0, -3.0, 4.0, -5.0, 6.0, 7.0, 8.0, -9.0};
        std::array<double, size * size> matrix{};
        std::array<double, size> rhs{};
        RandomStream random(612);
        for (std::size_t row = 0; row < size; ++row)
        {
            const auto diagonal = (row + 4) % size;
            for (std::size_t col = 0; col < size; ++col)
            {
                const auto value = col == diagonal ? 10.0 : random.uniform_real(-1.0, 1.0);
                matrix[row * size + col] = value;
                rhs[row] += value * known[col];
            }
        }
        const auto result = solve_linear_system<double>(matrix, rhs);
        ASSERT_TRUE(result);
        for (std::size_t i = 0; i < size; ++i)
        {
            EXPECT_NEAR((*result)[i], known[i], 1e-12);
        }
    }

    TEST(LinearSolve, EliminatesRowsWhenTheMultiplierIsOutsideTheScalarRange)
    {
        for (const int exponent : {600, -600})
        {
            const double first_scale = std::ldexp(1.0, exponent);
            const double second_scale = std::ldexp(1.0, -exponent);
            const std::array matrix{first_scale, first_scale, second_scale, 2 * second_scale};
            const std::array rhs{2 * first_scale, 3 * second_scale};
            const auto solution = solve_linear_system<double>(matrix, rhs);
            ASSERT_TRUE(solution);
            EXPECT_EQ((*solution)[0], 1.0);
            EXPECT_EQ((*solution)[1], 1.0);
        }
    }
}
