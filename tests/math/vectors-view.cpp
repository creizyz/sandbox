#include <gtest/gtest.h>

#include <array>
#include <type_traits>
#include <cmath>

#include "math/vectors-view.hpp"
#include "math/vectors.hpp"

namespace math::tests
{
    namespace
    {
        template <typename T>
        void expect_near(T a, T b, T eps)
        {
            ASSERT_LE(std::abs(a - b), eps);
        }

        template <std::size_t N, typename T>
        std::array<T*, N> ptrs_from_values(std::array<T, N>& values)
        {
            std::array<T*, N> out{};
            for (std::size_t i = 0; i < N; ++i)
            {
                out[i] = &values[i];
            }
            return out;
        }
    } // namespace

    // --- Compile-time sanity checks (API shape / constraints) ---

    static_assert(VectorView<3, int>::size == 3);
    static_assert(std::is_copy_constructible_v<VectorView<3, int>>);
    static_assert(std::is_move_constructible_v<VectorView<3, int>>);

    static_assert(std::is_constructible_v<VectorView<2, int>, int*, int*>);
    static_assert(!std::is_default_constructible_v<VectorView<2, int>>);

    static_assert(std::is_constructible_v<ConstVectorView<3, float>, VectorView<3, float>>);
    static_assert(!std::is_constructible_v<VectorView<3, float>, ConstVectorView<3, float>>);

    // --- Construction & basic access ---

    TEST(VectorView, ConstructsFromArrayOfPointersAndReflectsUnderlyingData)
    {
        std::array<int, 3> values{1, 2, 3};
        auto ptrs = ptrs_from_values<3>(values);

        VectorView<3, int> v(ptrs);

        EXPECT_EQ(v[0], 1);
        EXPECT_EQ(v[1], 2);
        EXPECT_EQ(v[2], 3);

        v[1] = 42;
        EXPECT_EQ(values[1], 42);
    }

    TEST(VectorView, ConstructsFromPointerPackAndMutatesUnderlying)
    {
        int a = 10, b = 20, c = 30;
        VectorView<3, int> v(&a, &b, &c);

        v[0] += 1;
        v[1] += 2;
        v[2] += 3;

        EXPECT_EQ(a, 11);
        EXPECT_EQ(b, 22);
        EXPECT_EQ(c, 33);
    }

    TEST(VectorView, ConstViewConstructsFromNonConstView)
    {
        float a = 1.0f, b = 2.0f, c = 3.0f;
        VectorView<3, float> v(&a, &b, &c);

        ConstVectorView<3, float> cv(v);
        EXPECT_FLOAT_EQ(cv[0], 1.0f);
        EXPECT_FLOAT_EQ(cv[1], 2.0f);
        EXPECT_FLOAT_EQ(cv[2], 3.0f);
    }

#ifndef NDEBUG
    TEST(VectorView, ConstructorRejectsNullPointer_DebugDeathTest)
    {
        int a = 1, b = 2;

        // Pack constructor uses assert_non_null_pack_
        EXPECT_DEATH(
            {
                VectorView<3, int> v(&a, &b, nullptr);
                (void)v;
            },
            "null pointer"
        );

        // Array constructor uses assert_non_null_
        std::array<int*, 3> ptrs{&a, &b, nullptr};
        EXPECT_DEATH(
            {
                VectorView<3, int> v(ptrs);
                (void)v;
            },
            "null pointer"
        );
    }
#endif

    // --- Named access (x/y/z/w) ---

    TEST(VectorView, NamedAccessorsWorkFor2D3D4D)
    {
        int x = 1, y = 2;
        VectorView2<int> v2(&x, &y);
        EXPECT_EQ(v2.x(), 1);
        EXPECT_EQ(v2.y(), 2);
        v2.x() = 10;
        v2.y() = 20;
        EXPECT_EQ(x, 10);
        EXPECT_EQ(y, 20);

        int a = 3, b = 4, c = 5;
        VectorView3<int> v3(&a, &b, &c);
        EXPECT_EQ(v3.x(), 3);
        EXPECT_EQ(v3.y(), 4);
        EXPECT_EQ(v3.z(), 5);

        int p = 6, q = 7, r = 8, s = 9;
        VectorView4<int> v4(&p, &q, &r, &s);
        EXPECT_EQ(v4.w(), 9);
    }

    // --- at()/get<>() ---

    TEST(VectorView, AtAndGetProvideAccessAndAtIsBoundsCheckedInDebug)
    {
        int a = 1, b = 2, c = 3;
        VectorView<3, int> v(&a, &b, &c);

        EXPECT_EQ(v.at(0), 1);
        EXPECT_EQ(v.at(2), 3);

        v.get<1>() = 99;
        EXPECT_EQ(b, 99);

#ifndef NDEBUG
        EXPECT_DEATH(
            {
                (void)v.at(3);
            },
            ""
        );
#endif
    }

    // --- Helpers (as_vector / to_vector) ---

    TEST(VectorView, AsVectorCopiesValues)
    {
        int a = 1, b = 2, c = 3;
        VectorView<3, int> v(&a, &b, &c);

        auto out = v.as_vector();
        EXPECT_EQ(out[0], 1);
        EXPECT_EQ(out[1], 2);
        EXPECT_EQ(out[2], 3);

        // Ensure it's a copy (mutating underlying doesn't mutate out)
        b = 99;
        EXPECT_EQ(out[1], 2);
    }

    TEST(VectorView, ToVectorConvertsType)
    {
        int a = 1, b = 2, c = 3;
        VectorView<3, int> v(&a, &b, &c);

        auto out = v.to_vector<double>();
        EXPECT_DOUBLE_EQ(out[0], 1.0);
        EXPECT_DOUBLE_EQ(out[1], 2.0);
        EXPECT_DOUBLE_EQ(out[2], 3.0);
    }

    // --- apply/fill/clamp ---

    TEST(VectorView, FillOverwritesAllComponents)
    {
        int a = 1, b = 2, c = 3;
        VectorView<3, int> v(&a, &b, &c);

        v.fill(7);

        EXPECT_EQ(a, 7);
        EXPECT_EQ(b, 7);
        EXPECT_EQ(c, 7);
    }

    TEST(VectorView, ClampLimitsValues)
    {
        int a = -10, b = 5, c = 99;
        VectorView<3, int> v(&a, &b, &c);

        v.clamp(0, 10);

        EXPECT_EQ(a, 0);
        EXPECT_EQ(b, 5);
        EXPECT_EQ(c, 10);
    }

    TEST(VectorView, ApplyUnaryMutatesEachComponent)
    {
        int a = 1, b = 2, c = 3;
        VectorView<3, int> v(&a, &b, &c);

        v.apply([](int x) { return x * 2; });

        EXPECT_EQ(a, 2);
        EXPECT_EQ(b, 4);
        EXPECT_EQ(c, 6);
    }

    TEST(VectorView, ApplyBinaryCombinesWithOtherView)
    {
        int a = 1, b = 2, c = 3;
        int d = 10, e = 20, f = 30;

        VectorView<3, int> v(&a, &b, &c);
        VectorView<3, int> other(&d, &e, &f);

        v.apply(other, [](int a, int b) { return a + b; });

        EXPECT_EQ(a, 11);
        EXPECT_EQ(b, 22);
        EXPECT_EQ(c, 33);
    }

    // --- all/any ---

    TEST(VectorView, AllAndAnyWithUnaryPredicate)
    {
        int a = 2, b = 4, c = 6;
        VectorView<3, int> v(&a, &b, &c);

        EXPECT_TRUE(v.all([](int x) { return x % 2 == 0; }));
        EXPECT_TRUE(v.any([](int x) { return x == 4; }));
        EXPECT_FALSE(v.any([](int x) { return x == 5; }));
    }

    TEST(VectorView, AllAndAnyAgainstVectorWithBinaryPredicate)
    {
        int a = 1, b = 2, c = 3;
        VectorView<3, int> v(&a, &b, &c);

        Vector<3, int> other{};
        other[0] = 1;
        other[1] = 0;
        other[2] = 3;

        EXPECT_FALSE(v.all(other, [](int lhs, int rhs) { return lhs == rhs; }));
        EXPECT_TRUE(v.any(other, [](int lhs, int rhs) { return lhs == rhs; }));
    }

    // --- Arithmetic ops (mutating) ---

    TEST(VectorView, AddSubtractMultiplyDivide_Int)
    {
        int a = 10, b = 20, c = 30;
        int d = 1, e = 2, f = 3;

        VectorView<3, int> v(&a, &b, &c);
        VectorView<3, int> other(&d, &e, &f);

        v.add(other);
        EXPECT_EQ(a, 11);
        EXPECT_EQ(b, 22);
        EXPECT_EQ(c, 33);

        v.subtract(other);
        EXPECT_EQ(a, 10);
        EXPECT_EQ(b, 20);
        EXPECT_EQ(c, 30);

        v.multiply(2);
        EXPECT_EQ(a, 20);
        EXPECT_EQ(b, 40);
        EXPECT_EQ(c, 60);

        v.divide(4);
        EXPECT_EQ(a, 5);
        EXPECT_EQ(b, 10);
        EXPECT_EQ(c, 15);
    }

    TEST(VectorView, Divide_FloatUsesReciprocalPath)
    {
        float a = 10.0f, b = 20.0f, c = 30.0f;
        VectorView<3, float> v(&a, &b, &c);

        v.divide(4.0f);

        EXPECT_FLOAT_EQ(a, 2.5f);
        EXPECT_FLOAT_EQ(b, 5.0f);
        EXPECT_FLOAT_EQ(c, 7.5f);
    }

    // --- Dot / length / normalize / invert ---

    TEST(VectorView, DotComputesInnerProduct)
    {
        int a = 1, b = 2, c = 3;
        int d = 4, e = 5, f = 6;

        VectorView<3, int> v(&a, &b, &c);
        VectorView<3, int> w(&d, &e, &f);

        EXPECT_EQ(v.dot(w), 1 * 4 + 2 * 5 + 3 * 6);
    }

    TEST(VectorView, SquaredLengthMatchesDotWithSelf)
    {
        int a = 2, b = 3, c = 6;
        VectorView<3, int> v(&a, &b, &c);

        EXPECT_EQ(v.squared_length(), v.dot(v));
        EXPECT_EQ(v.squared_length(), 4 + 9 + 36);
    }

    TEST(VectorView, InvertNegatesEachComponent)
    {
        int a = 1, b = -2, c = 3;
        VectorView<3, int> v(&a, &b, &c);

        v.invert();

        EXPECT_EQ(a, -1);
        EXPECT_EQ(b, 2);
        EXPECT_EQ(c, -3);
    }

    TEST(VectorView, NormalizeProducesUnitLengthForNonZeroVector)
    {
        float a = 3.0f, b = 4.0f;
        VectorView<2, float> v(&a, &b);

        v.normalize();

        const float len = v.length();
        // Use a loose-ish tolerance since epsilon_v might be small and normalize uses sqrt/divide.
        expect_near(len, 1.0f, 1e-5f);
        expect_near(v.x(), 0.6f, 1e-5f);
        expect_near(v.y(), 0.8f, 1e-5f);
    }

    TEST(VectorView, NormalizeLeavesNearZeroVectorUnchanged)
    {
        // Relies on epsilon_v<T> in implementation: values below that should remain unchanged.
        float a = 0.0f, b = 0.0f, c = 0.0f;
        VectorView<3, float> v(&a, &b, &c);

        v.normalize();

        EXPECT_FLOAT_EQ(a, 0.0f);
        EXPECT_FLOAT_EQ(b, 0.0f);
        EXPECT_FLOAT_EQ(c, 0.0f);
    }

    // --- Comparison: equals/near_equals/operator== ---

    TEST(VectorView, EqualsForIntegers)
    {
        int a = 1, b = 2, c = 3;
        int d = 1, e = 2, f = 3;

        VectorView<3, int> v(&a, &b, &c);
        VectorView<3, int> w(&d, &e, &f);

        EXPECT_TRUE(v.equals(w));
        EXPECT_TRUE(v == w);

        f = 99;
        EXPECT_FALSE(v.equals(w));
        EXPECT_TRUE(v != w);
    }

    TEST(VectorView, NearEqualsForFloatsAndOperatorUsesNearEquals)
    {
        float a = 1.0f, b = 2.0f, c = 3.0f;
        float d = 1.0f, e = 2.0f, f = 3.0f + 1e-6f;

        VectorView<3, float> v(&a, &b, &c);
        VectorView<3, float> w(&d, &e, &f);

        EXPECT_TRUE(v.near_equals(w, 1e-5f));
        EXPECT_TRUE(v == w); // operator== uses near_equals for floating point T

        EXPECT_FALSE(v.near_equals(w, 1e-8f));
    }
} // namespace math::tests