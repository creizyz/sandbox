#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "math/numerics/discrete-time.hpp"
#include "math/numerics/fixed-point.hpp"
#include "math/numerics/interpolation.hpp"

namespace
{
    using namespace math;
    using Thousandths = FixedPoint<>;

    enum class Operation
    {
        Balance,
        Rate,
        Divide
    };

    template <typename T>
    T from_thousandths(std::int64_t value)
    {
        if constexpr (std::is_same_v<T, Thousandths>)
        {
            return Thousandths::from_raw(value);
        }
        else
        {
            return static_cast<T>(value) / T{1000};
        }
    }

    template <Operation Work, typename T>
    void bench_discrete(benchmark::State &state)
    {
        // These operands and their results are exactly representable in all three types.
        std::array<std::array<T, 3>, 32> inputs;
        for (std::size_t i = 0; i < inputs.size(); ++i)
        {
            const auto index = static_cast<std::int64_t>(i);
            inputs[i] = {from_thousandths<T>(1000 + 250 * index),
                         from_thousandths<T>((i % 2 == 0) ? 2000 : 500),
                         from_thousandths<T>(125 * index)};
        }
        std::size_t index = 0;
        for (auto _ : state)
        {
            auto *sample = &inputs[index];
            benchmark::DoNotOptimize(sample);
            const auto &values = *sample;
            if constexpr (Work == Operation::Balance)
            {
                auto result = advance_balance(values[0], values[1], values[2]);
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Work == Operation::Rate)
            {
                auto result = integrate_rate(values[0], values[1]);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                auto result = values[0] / values[1];
                benchmark::DoNotOptimize(result);
            }
            index = (index + 1) % inputs.size();
        }
    }

    template <bool Smooth>
    void bench_response(benchmark::State &state)
    {
        std::array<double, 8> inputs{-0.25, 0.0, 0.125, 0.25, 0.5, 0.75, 1.0, 1.25};
        std::size_t index = 0;
        for (auto _ : state)
        {
            auto value = inputs[index];
            benchmark::DoNotOptimize(value);
            double result;
            if constexpr (Smooth)
            {
                double duration = 0.125 + static_cast<double>(index) / 32.0;
                benchmark::DoNotOptimize(duration);
                result = smooth_towards(value, 5.0, duration, 1.0);
            }
            else
            {
                result = clamped_linear_map(value, 0.0, 1.0, 2.0, 5.0);
            }
            benchmark::DoNotOptimize(result);
            index = (index + 1) % inputs.size();
        }
    }

    void bench_tick_queue(benchmark::State &state)
    {
        constexpr Tick count = 128;
        TickQueue<Tick> queue;
        for (auto _ : state)
        {
            for (Tick value = 0; value < count; ++value)
            {
                queue.schedule_after(0, (value * 13) % 32, value);
            }
            while (auto event = queue.pop_ready(31))
            {
                benchmark::DoNotOptimize(event);
            }
        }
        state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(count));
    }

    BENCHMARK(bench_discrete<Operation::Balance, float>)->Name("bm_discrete/balance/float");
    BENCHMARK(bench_discrete<Operation::Balance, double>)->Name("bm_discrete/balance/double");
    BENCHMARK(bench_discrete<Operation::Balance, Thousandths>)->Name("bm_discrete/balance/fixed");
    BENCHMARK(bench_discrete<Operation::Rate, float>)->Name("bm_discrete/rate/float");
    BENCHMARK(bench_discrete<Operation::Rate, double>)->Name("bm_discrete/rate/double");
    BENCHMARK(bench_discrete<Operation::Rate, Thousandths>)->Name("bm_discrete/rate/fixed");
    BENCHMARK(bench_discrete<Operation::Divide, float>)->Name("bm_discrete/divide/float");
    BENCHMARK(bench_discrete<Operation::Divide, double>)->Name("bm_discrete/divide/double");
    BENCHMARK(bench_discrete<Operation::Divide, Thousandths>)->Name("bm_discrete/divide/fixed");
    BENCHMARK(bench_response<false>)->Name("bm_discrete/clamped_map");
    BENCHMARK(bench_response<true>)->Name("bm_discrete/smoothing");
    BENCHMARK(bench_tick_queue)->Name("bm_discrete/tick_queue_128");
}
