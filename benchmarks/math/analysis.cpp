#include <benchmark/benchmark.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <random>
#include <span>
#include <vector>

#include "math/linear-algebra/solver.hpp"
#include "math/numerics/numerical-analysis.hpp"
#include "math/random/sampling.hpp"
#include "math/statistics/summary.hpp"

namespace
{
    using namespace math;

    enum class Distribution
    {
        Integer,
        Real,
        Bernoulli,
        Normal,
        Exponential
    };

    template <Distribution Type>
    void bench_draw(benchmark::State &state)
    {
        RandomStream stream(82, 1);
        for (auto _ : state)
        {
            if constexpr (Type == Distribution::Integer)
            {
                auto result = stream.uniform_integer(-170, 83);
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Type == Distribution::Real)
            {
                auto result = stream.uniform_real(-100.0, 100.0);
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Type == Distribution::Bernoulli)
            {
                auto result = stream.bernoulli(0.3);
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Type == Distribution::Normal)
            {
                auto result = stream.normal(2.0, 1.5);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                auto result = stream.exponential(2.0);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    void bench_weighted_draw(benchmark::State &state)
    {
        std::vector<double> weights(static_cast<std::size_t>(state.range(0)));
        for (std::size_t i = 0; i < weights.size(); ++i)
        {
            weights[i] = static_cast<double>(i % 17);
        }
        WeightedSampler sampler(weights);
        RandomStream stream(14);
        for (auto _ : state)
        {
            auto result = sampler.draw(stream);
            benchmark::DoNotOptimize(result);
        }
    }

    void bench_poisson(benchmark::State &state)
    {
        RandomStream stream(14);
        std::poisson_distribution<int> distribution(4.0);
        for (auto _ : state)
        {
            auto result = distribution(stream);
            benchmark::DoNotOptimize(result);
        }
    }

    template <bool Partial>
    void bench_shuffle(benchmark::State &state)
    {
        std::vector<std::size_t> values(static_cast<std::size_t>(state.range(0)));
        std::iota(values.begin(), values.end(), std::size_t{0});
        RandomStream stream(14);
        auto *data = values.data();
        benchmark::DoNotOptimize(data);
        for (auto _ : state)
        {
            if constexpr (Partial)
            {
                auto result = stream.sample_prefix(std::span{values}, 16);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                stream.shuffle(std::span{values});
            }
            benchmark::ClobberMemory();
        }
    }

    enum class Statistic
    {
        Weighted,
        Paired,
        Histogram,
        Quantiles,
        Dispersion,
        Lag,
        Sum
    };

    template <Statistic Type>
    void bench_statistic(benchmark::State &state)
    {
        RandomStream stream(10);
        std::vector<double> values(static_cast<std::size_t>(state.range(0)));
        for (auto &value : values)
        {
            value = stream.unit();
        }
        auto *data = values.data();
        benchmark::DoNotOptimize(data);
        constexpr std::array cuts{0.125, 0.25, 0.5, 0.75, 0.875};
        constexpr std::array probabilities{0.1, 0.25, 0.5, 0.75, 0.95};
        for (auto _ : state)
        {
            if constexpr (Type == Statistic::Weighted)
            {
                WeightedStatistics summary;
                double weight = 0.5;
                for (const auto value : values)
                {
                    summary.add(value, weight);
                    weight = 1.5 - weight;
                }
                auto result = summary.population_variance();
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Type == Statistic::Paired)
            {
                RunningCovariance summary;
                for (std::size_t i = 0; i < values.size() / 2; ++i)
                {
                    summary.add(values[i], values[i + values.size() / 2]);
                }
                auto correlation = summary.correlation();
                auto fit = summary.linear_fit();
                benchmark::DoNotOptimize(correlation);
                benchmark::DoNotOptimize(fit);
            }
            else if constexpr (Type == Statistic::Histogram)
            {
                auto result = histogram<double>(values, cuts);
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Type == Statistic::Quantiles)
            {
                auto result = quantiles<double>(values, probabilities);
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Type == Statistic::Lag)
            {
                auto result = lagged_correlation<double>(values, 7);
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Type == Statistic::Sum)
            {
                auto result = compensated_sum<double>(values);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                RunningStatistics summary;
                for (const auto value : values)
                {
                    summary.add(value);
                }
                auto deviation = summary.sample_standard_deviation();
                auto error = summary.standard_error_of_mean();
                benchmark::DoNotOptimize(deviation);
                benchmark::DoNotOptimize(error);
            }
        }
    }

    enum class Analysis
    {
        Root,
        Minimum,
        Derivative,
        Integral
    };

    template <Analysis Operation>
    void bench_scalar(benchmark::State &state)
    {
        double parameter = 1.2;
        for (auto _ : state)
        {
            benchmark::DoNotOptimize(parameter);
            if constexpr (Operation == Analysis::Root)
            {
                auto result = bisect_root([&](double x)
                                          { return x * x - parameter; },
                                          0.0, 3.0, 1e-10);
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Operation == Analysis::Minimum)
            {
                auto result = golden_section_minimum([&](double x)
                                                     { return (x - parameter) * (x - parameter); },
                                                     0.0, 3.0, 1e-10);
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Operation == Analysis::Derivative)
            {
                auto result = central_derivative([](double x)
                                                 { return std::sin(x); },
                                                 parameter, 1e-5);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                auto result = integrate_adaptive([](double x)
                                                 { return std::sin(x); },
                                                 0.0, parameter, 1e-8);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    void bench_linear_solve(benchmark::State &state)
    {
        const auto size = static_cast<std::size_t>(state.range(0));
        std::vector<double> matrix(size * size);
        std::vector<double> rhs(size);
        RandomStream random(520);
        for (std::size_t row = 0; row < size; ++row)
        {
            for (std::size_t col = 0; col < size; ++col)
            {
                matrix[row * size + col] = row == col ? static_cast<double>(size) : random.unit();
            }
            rhs[row] = random.unit();
        }
        auto *coefficients = matrix.data();
        benchmark::DoNotOptimize(coefficients);
        for (auto _ : state)
        {
            auto solution = solve_linear_system<double>(matrix, rhs);
            benchmark::DoNotOptimize(solution);
        }
    }

    BENCHMARK(bench_draw<Distribution::Integer>)->Name("bm_analysis/random/integer");
    BENCHMARK(bench_draw<Distribution::Real>)->Name("bm_analysis/random/real");
    BENCHMARK(bench_draw<Distribution::Bernoulli>)->Name("bm_analysis/random/bernoulli");
    BENCHMARK(bench_draw<Distribution::Normal>)->Name("bm_analysis/random/normal");
    BENCHMARK(bench_draw<Distribution::Exponential>)->Name("bm_analysis/random/exponential");
    BENCHMARK(bench_weighted_draw)->Name("bm_analysis/random/weighted")->Arg(16)->Arg(1024);
    BENCHMARK(bench_poisson)->Name("bm_analysis/random/poisson");
    BENCHMARK(bench_shuffle<false>)->Name("bm_analysis/random/shuffle")->Arg(128)->Arg(4096);
    BENCHMARK(bench_shuffle<true>)->Name("bm_analysis/random/sample16")->Arg(128)->Arg(4096);
    BENCHMARK(bench_statistic<Statistic::Weighted>)->Name("bm_analysis/stats/weighted")->Arg(256)->Arg(4096);
    BENCHMARK(bench_statistic<Statistic::Paired>)->Name("bm_analysis/stats/paired")->Arg(256)->Arg(4096);
    BENCHMARK(bench_statistic<Statistic::Histogram>)->Name("bm_analysis/stats/histogram")->Arg(256)->Arg(4096);
    BENCHMARK(bench_statistic<Statistic::Quantiles>)->Name("bm_analysis/stats/quantiles")->Arg(256)->Arg(4096);
    BENCHMARK(bench_statistic<Statistic::Dispersion>)->Name("bm_analysis/stats/dispersion")->Arg(256)->Arg(4096);
    BENCHMARK(bench_statistic<Statistic::Lag>)->Name("bm_analysis/stats/lag7")->Arg(256)->Arg(4096);
    BENCHMARK(bench_statistic<Statistic::Sum>)->Name("bm_analysis/scalar/compensated_sum")->Arg(256)->Arg(4096);
    BENCHMARK(bench_scalar<Analysis::Root>)->Name("bm_analysis/scalar/root");
    BENCHMARK(bench_scalar<Analysis::Minimum>)->Name("bm_analysis/scalar/minimum");
    BENCHMARK(bench_scalar<Analysis::Derivative>)->Name("bm_analysis/scalar/derivative");
    BENCHMARK(bench_scalar<Analysis::Integral>)->Name("bm_analysis/scalar/integral");
    BENCHMARK(bench_linear_solve)->Name("bm_analysis/linear_solve")->Arg(16)->Arg(64);
}
