#include <benchmark/benchmark.h>
#include "math/vectors.hpp"

using namespace math;

static void BM_Vector3_Addition(benchmark::State& state) {
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
    for (auto _ : state) {
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

static void BM_Vector3_DotProduct(benchmark::State& state) {
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
    for (auto _ : state) {
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
static void BM_Vector_SquaredLength(benchmark::State& state) {
    Vector<N, float> v = Vector<N, float>::one;

    std::uint32_t i = 0;
    for (auto _ : state) {
        // Make v change so squaredLength can't be hoisted to a constant.
        v[0] = 1.0f + static_cast<float>(i) * 1e-7f;
        benchmark::DoNotOptimize(v);

        float lenSq = v.squaredLength();
        benchmark::DoNotOptimize(lenSq);

        ++i;
    }
}
BENCHMARK(BM_Vector_SquaredLength<2>);
BENCHMARK(BM_Vector_SquaredLength<3>);
BENCHMARK(BM_Vector_SquaredLength<4>);
BENCHMARK(BM_Vector_SquaredLength<16>);