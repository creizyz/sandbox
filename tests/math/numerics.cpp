#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <type_traits>

#include "math/geometry/vector-geometry.hpp"
#include "math/linear-algebra/vectors-view.hpp"

namespace math::tests
{
    TEST(VectorNumerics, TransformBorrowsNoncopyableCallablesAtEveryDimension)
    {
        struct CountingOperation
        {
            int calls = 0;
            CountingOperation() = default;
            CountingOperation(const CountingOperation &) = delete;
            int operator()(int a, int b = 0)
            {
                return a + b + ++calls;
            }
        };
        const auto verify = []<std::size_t N>()
        {
            CountingOperation operation;
            const auto ones = Vector<N, int>::one;
            const auto unary = ones.transform(operation);
            EXPECT_EQ(unary[N - 1], 1 + static_cast<int>(N));
            const auto binary = ones.transform(ones, operation);
            EXPECT_EQ(binary[N - 1], 2 + static_cast<int>(2 * N));
        };
        verify.template operator()<4>();
        verify.template operator()<5>();
    }

    TEST(VectorNumerics, DividesBySubnormalScalarsWithoutOverflowing)
    {
        const auto tiny = std::numeric_limits<double>::denorm_min();
        const auto quotient = Vector2d{tiny, 0.0} / tiny;
        EXPECT_EQ(quotient.x(), 1.0);
        EXPECT_EQ(quotient.y(), 0.0);

        double x = tiny, y = 0.0;
        VectorView2<double>{&x, &y}.divide(tiny);
        EXPECT_EQ(x, 1.0);
        EXPECT_EQ(y, 0.0);
    }

    TEST(VectorNumerics, MixedViewComparisonsAreSymmetric)
    {
        int one = 1;
        double fraction = 1.5;
        VectorView<1, int> integral{&one};
        VectorView<1, double> floating{&fraction};
        EXPECT_FALSE(integral == floating);
        EXPECT_FALSE(floating == integral);
        fraction = 1.0 + epsilon_v<double> / 2;
        EXPECT_TRUE(integral == floating);
        EXPECT_TRUE(floating == integral);
        EXPECT_FALSE(integral.equals(floating));
        EXPECT_FALSE(floating.equals(integral));

        int negative = -1;
        unsigned maximum = std::numeric_limits<unsigned>::max();
        VectorView<1, int> signed_view{&negative};
        VectorView<1, unsigned> unsigned_view{&maximum};
        EXPECT_FALSE(signed_view == unsigned_view);
        EXPECT_FALSE(unsigned_view == signed_view);
    }

    TEST(VectorNumerics, PredicatesShortCircuitOnEitherSideOfUnrollThreshold)
    {
        const auto verify = []<std::size_t N>()
        {
            const auto values = Vector<N, int>::one;
            int calls = 0;
            EXPECT_TRUE(values.any([&](int)
                                   { return ++calls == 1; }));
            EXPECT_EQ(calls, 1);
            calls = 0;
            EXPECT_FALSE(values.all([&](int)
                                    { return ++calls != 1; }));
            EXPECT_EQ(calls, 1);
            calls = 0;
            EXPECT_TRUE(values.any(values, [&](int, int)
                                   { return ++calls == 1; }));
            EXPECT_EQ(calls, 1);
            calls = 0;
            EXPECT_FALSE(values.all(values, [&](int, int)
                                    { return ++calls != 1; }));
            EXPECT_EQ(calls, 1);
        };
        verify.template operator()<4>();
        verify.template operator()<5>();
    }

    TEST(VectorNumerics, ComputesRepresentableLengthsAndNormalizesHugeVectors)
    {
        const Vector2f finite{1e20f, 0.0f};
        EXPECT_FLOAT_EQ(finite.length(), 1e20f);
        EXPECT_EQ(finite.normalized(), (Vector2f{1.0f, 0.0f}));

        const auto maximum = std::numeric_limits<float>::max();
        Vector2f huge{maximum, maximum};
        EXPECT_TRUE(std::isinf(huge.length()));
        huge.normalize();
        EXPECT_NEAR(huge.x(), std::sqrt(0.5f), 1e-6f);
        EXPECT_NEAR(huge.length(), 1.0f, 1e-6f);

        const Vector2f tiny{std::numeric_limits<float>::min(), 0.0f};
        EXPECT_GT(tiny.length(), 0.0f);
        EXPECT_EQ(tiny.normalized().x(), 0.0f);
    }

    TEST(VectorNumerics, ConstViewCopiesAndFloatingViewsRejectNan)
    {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float infinity = std::numeric_limits<float>::infinity();
        const float one = 1.0f;
        const float two = 2.0f;
        const ConstVectorView<2, float> source{&one, &two};
        auto copy = source.as_vector();
        static_assert(std::is_same_v<decltype(copy), Vector2f>);
        copy.x() = 3.0f;
        EXPECT_FLOAT_EQ(source.x(), 1.0f);
        EXPECT_TRUE(source.all(Vector2f{one, two}, [](float a, float b)
                               { return a == b; }));

        const ConstVectorView<1, float> nan_view{&nan};
        const ConstVectorView<1, float> finite_view{&one};
        const ConstVectorView<1, float> infinity_view{&infinity};
        EXPECT_FALSE(nan_view.near_equals(finite_view));
        EXPECT_FALSE(finite_view.near_equals(nan_view));
        EXPECT_FALSE(nan_view == nan_view);
        EXPECT_TRUE(infinity_view == infinity_view);
        EXPECT_FALSE(finite_view.near_equals(infinity_view, infinity));

        float x = std::numeric_limits<float>::max();
        float y = x;
        VectorView2<float> mutable_view{&x, &y};
        mutable_view.normalize();
        EXPECT_NEAR(x, std::sqrt(0.5f), 1e-6f);
        EXPECT_NEAR(y, std::sqrt(0.5f), 1e-6f);
    }

    TEST(Geometry, CrossReflectAndRefractHaveKnownResults)
    {
        EXPECT_EQ(cross(Vector2<int>{1, 0}, Vector2<int>{0, 1}), 1);
        EXPECT_EQ(cross(Vector3<int>{1, 0, 0}, Vector3<int>{0, 1, 0}), (Vector3<int>{0, 0, 1}));
        const Vector2d normal{0.0, 1.0};
        EXPECT_EQ(reflect(Vector2d{1.0, -1.0}, normal), (Vector2d{1.0, 1.0}));
        EXPECT_EQ(refract(Vector2d{0.0, -1.0}, normal, 0.75), (Vector2d{0.0, -1.0}));
        EXPECT_EQ(refract(Vector2d{1.0, 0.0}, normal, 1.5), Vector2d::zero);
    }
}
