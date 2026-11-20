#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "math/random/sampling.hpp"
#include "math/statistics/summary.hpp"

namespace
{
    using namespace math;

    enum class Draw
    {
        Raw,
        Bounded,
        Unit
    };

    template <Draw Distribution>
    void bench_random(benchmark::State &state)
    {
        RandomStream random(101, 4);
        for (auto _ : state)
        {
            if constexpr (Distribution == Draw::Raw)
            {
                auto result = random.next();
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Distribution == Draw::Bounded)
            {
                auto result = random.below((std::uint64_t{1} << 63) + 1);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                auto result = random.unit();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    template <bool Quantile>
    void bench_statistics(benchmark::State &state)
    {
        const auto size = static_cast<std::size_t>(state.range(0));
        std::vector<double> values(size);
        RandomStream random(7);
        for (auto &value : values)
        {
            value = random.unit();
        }
        auto *data = values.data();
        benchmark::DoNotOptimize(data);
        for (auto _ : state)
        {
            if constexpr (Quantile)
            {
                auto result = quantile<double>(values, 0.95);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                RunningStatistics<double> first;
                RunningStatistics<double> second;
                for (std::size_t i = 0; i < size / 2; ++i)
                {
                    first.add(values[i]);
                    second.add(values[size / 2 + i]);
                }
                first.merge(second);
                auto result = first.sample_variance();
                benchmark::DoNotOptimize(result);
            }
        }
        state.SetItemsProcessed(state.iterations() * state.range(0));
    }

    BENCHMARK(bench_random<Draw::Raw>)->Name("bm_sampling/random/raw");
    BENCHMARK(bench_random<Draw::Bounded>)->Name("bm_sampling/random/bounded");
    BENCHMARK(bench_random<Draw::Unit>)->Name("bm_sampling/random/unit");
    BENCHMARK(bench_statistics<false>)->Name("bm_sampling/moments")->Arg(256)->Arg(4096);
    BENCHMARK(bench_statistics<true>)->Name("bm_sampling/p95")->Arg(256)->Arg(4096);
}
