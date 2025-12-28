#include <benchmark/benchmark.h>
#include "math/vectors.hpp"

using namespace math;

static void BM_Vector3_Creation(benchmark::State& state)
{
    std::uint32_t i = 0;
    for (auto _ : state)
    {
        // Vary inputs to avoid constant folding / hoisting.
        float x = 1.2f + static_cast<float>(i) * 1e-7f;
        float y = 3.4f + static_cast<float>(i) * 2e-7f;
        float z = 5.6f + static_cast<float>(i) * 3e-7f;

        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
        benchmark::DoNotOptimize(z);

        Vector3f v{x, y, z};
        benchmark::DoNotOptimize(v);

        ++i;
    }
}
BENCHMARK(BM_Vector3_Creation);

template <size_t N>
static void BM_Vector_Creation(benchmark::State& state)
{
    std::uint32_t i = 0;
    for (auto _ : state)
    {
        // Start from something simple; then mutate one lane so it's not a constant.
        Vector<N, float> v = Vector<N, float>::one;
        v[0] = 1.0f + static_cast<float>(i) * 1e-7f;

        benchmark::DoNotOptimize(v);

        ++i;
    }
}
BENCHMARK(BM_Vector_Creation<2>);
BENCHMARK(BM_Vector_Creation<3>);
BENCHMARK(BM_Vector_Creation<4>);
BENCHMARK(BM_Vector_Creation<16>);

static void BM_Vector3_Addition(benchmark::State& state)
{
    static Vector3f inputsA[] = {
        Vector3f{1.2f, 3.4f, 5.6f},
        Vector3f{2.2f, 4.4f, 6.6f},
        Vector3f{3.2f, 5.4f, 7.6f},
        Vector3f{4.2f, 6.4f, 8.6f},
    };
    static Vector3f inputsB[] = {
        Vector3f{7.8f, 9.0f, 1.2f},
        Vector3f{8.8f, 1.0f, 2.2f},
        Vector3f{9.8f, 2.0f, 3.2f},
        Vector3f{1.8f, 3.0f, 4.2f},
    };

    std::uint32_t i = 0;
    for (auto _ : state)
    {
        const auto& a = inputsA[i & 3u];
        const auto& b = inputsB[i & 3u];
        benchmark::DoNotOptimize(a);
        benchmark::DoNotOptimize(b);

        auto res = a + b;
        benchmark::DoNotOptimize(res);

        ++i;
    }
}
BENCHMARK(BM_Vector3_Addition);

static void BM_Vector3_DotProduct(benchmark::State& state)
{
    static Vector3f inputsA[] = {
        Vector3f{1.2f, 3.4f, 5.6f},
        Vector3f{2.2f, 4.4f, 6.6f},
        Vector3f{3.2f, 5.4f, 7.6f},
        Vector3f{4.2f, 6.4f, 8.6f}
    };
    static Vector3f inputsB[] = {
        Vector3f{7.8f, 9.0f, 1.2f},
        Vector3f{8.8f, 1.0f, 2.2f},
        Vector3f{9.8f, 2.0f, 3.2f},
        Vector3f{1.8f, 3.0f, 4.2f}
    };

    std::uint32_t i = 0;
    for (auto _ : state)
    {
        const auto& a = inputsA[i & 3u];
        const auto& b = inputsB[i & 3u];
        benchmark::DoNotOptimize(a);
        benchmark::DoNotOptimize(b);

        float res = a.dot(b);
        benchmark::DoNotOptimize(res);

        ++i;
    }
}
BENCHMARK(BM_Vector3_DotProduct);

template <size_t N>
static void BM_Vector_SquaredLength(benchmark::State& state)
{
    Vector<N, float> v = Vector<N, float>::one;

    std::uint32_t i = 0;
    for (auto _ : state)
    {
        // Make v change so squared_length can't be hoisted to a constant.
        v[0] = 1.0f + static_cast<float>(i) * 1e-7f;
        benchmark::DoNotOptimize(v);

        float lenSq = v.squared_length();
        benchmark::DoNotOptimize(lenSq);

        ++i;
    }
}
BENCHMARK(BM_Vector_SquaredLength<2>);
BENCHMARK(BM_Vector_SquaredLength<3>);
BENCHMARK(BM_Vector_SquaredLength<4>);
BENCHMARK(BM_Vector_SquaredLength<16>);