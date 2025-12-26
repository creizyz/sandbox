#include <benchmark/benchmark.h>
#include "math/geometric.hpp"

using namespace math;

static void BM_Vector2_cross(benchmark::State& state)
{
    static Vector2f inputsA[] = {
        Vector2f{1.2f, 3.4f},
        Vector2f{2.2f, 4.4f},
        Vector2f{3.2f, 5.4f},
        Vector2f{4.2f, 6.4f},
    };
    static Vector2f inputsB[] = {
        Vector2f{7.8f, 9.0f},
        Vector2f{8.8f, 1.0f},
        Vector2f{9.8f, 2.0f},
        Vector2f{1.8f, 3.0f},
    };

    std::uint32_t i = 0;
    for (auto _ : state)
    {
        const auto& a = inputsA[i & 3u];
        const auto& b = inputsB[i & 3u];
        benchmark::DoNotOptimize(a);
        benchmark::DoNotOptimize(b);

        auto res = cross(a, b);
        benchmark::DoNotOptimize(res);

        ++i;
    }
}
BENCHMARK(BM_Vector2_cross);

static void BM_Vector3_cross(benchmark::State& state)
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

        auto res = cross(a, b);
        benchmark::DoNotOptimize(res);

        ++i;
    }
}
BENCHMARK(BM_Vector3_cross);

static void BM_Vector3_rotate(benchmark::State& state)
{
    static Vector3f inputsA[] = {
        Vector3f{1.2f, 3.4f, 5.6f},
        Vector3f{2.2f, 4.4f, 6.6f},
        Vector3f{3.2f, 5.4f, 7.6f},
        Vector3f{4.2f, 6.4f, 8.6f},
    };
    static Quaternion<float> inputsB[] = {
        Quaternion<float>{ 1.0f, 0.0f, 0.0f, 0.0f },
        Quaternion<float>{ 0.70710678f, 0.70710678f, 0.0f, 0.0f },
        Quaternion<float>{ 0.86602540f, 0.0f, 0.5f, 0.0f },
        Quaternion<float>{ 0.5f, 0.5f, 0.5f, 0.5f },
    };

    std::uint32_t i = 0;
    for (auto _ : state)
    {
        const auto& a = inputsA[i & 3u];
        const auto& b = inputsB[i & 3u];
        benchmark::DoNotOptimize(a);
        benchmark::DoNotOptimize(b);

        auto res = rotate(a, b);
        benchmark::DoNotOptimize(res);

        ++i;
    }
}
BENCHMARK(BM_Vector3_rotate);
