#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <string>
#include <type_traits>

#include "math/vectors.hpp"

using namespace math;

template <std::size_t N, typename T>
void ExpectVecEqExact(const Vector<N, T>& a, const Vector<N, T>& b)
{
  for (std::size_t i = 0; i < N; ++i)
    {
    EXPECT_EQ(a[static_cast<int>(i)], b[static_cast<int>(i)]) << "i=" << i;
  }
}

template <std::size_t N, typename T>
void ExpectVecNear(const Vector<N, T>& a, const Vector<N, T>& b, T tol)
{
  for (std::size_t i = 0; i < N; ++i)
    {
    EXPECT_NEAR(a[static_cast<int>(i)], b[static_cast<int>(i)], tol) << "i=" << i;
  }
}

template <typename T>
struct VecTypedTest : ::testing::Test {};

using FloatTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(VecTypedTest, FloatTypes);

// --- Compile-time / type-trait sanity checks (fast, production-friendly) ---

static_assert(Vector2<float>::size == 2);
static_assert(Vector3<float>::size == 3);
static_assert(Vector4<float>::size == 4);

// Constructor constraints: only arithmetic Args..., and count must be N.
static_assert(std::is_constructible_v<Vector2<float>, int, int>);
static_assert(!std::is_constructible_v<Vector2<float>, int>);
static_assert(!std::is_constructible_v<Vector2<float>, int, int, int>);
static_assert(!std::is_constructible_v<Vector2<float>, std::string, int>);

// --- Storage / aliasing (x,y,z,w and r,g,b,a) ---

TEST(VectorStorageAliasing, Vector2_XYAliasesData)
{
  Vector2f v;
  v.x() = 1.25f;
  v.y() = -2.0f;

  EXPECT_FLOAT_EQ(v[0], 1.25f);
  EXPECT_FLOAT_EQ(v[1], -2.0f);

  v[0] = 3.5f;
  v[1] = 4.5f;

  EXPECT_FLOAT_EQ(v.x(), 3.5f);
  EXPECT_FLOAT_EQ(v.y(), 4.5f);
}

TEST(VectorStorageAliasing, Vector3_XYZAndRGBAliasesData)
{
  Vector3f v;
  v.x() = 1.0f; v.y() = 2.0f; v.z() = 3.0f;

  EXPECT_FLOAT_EQ(v[0], 1.0f);
  EXPECT_FLOAT_EQ(v[1], 2.0f);
  EXPECT_FLOAT_EQ(v[2], 3.0f);
}

TEST(VectorStorageAliasing, Vector4_XYZWAndRGBAAliasesData)
{
  Vector4f v;
  v.x() = 1.0f; v.y() = 2.0f; v.z() = 3.0f; v.w() = 4.0f;

  EXPECT_FLOAT_EQ(v[0], 1.0f);
  EXPECT_FLOAT_EQ(v[1], 2.0f);
  EXPECT_FLOAT_EQ(v[2], 3.0f);
  EXPECT_FLOAT_EQ(v[3], 4.0f);
}

// --- Construction / cast / fill / constants ---

TYPED_TEST(VecTypedTest, FillAndConstants_WorkForAllComponents)
{
  using T = TypeParam;

  const auto z2 = Vector2<T>::zero;
  const auto o2 = Vector2<T>::one;
  ExpectVecEqExact(z2, Vector2<T>(T{0}, T{0}));
  ExpectVecEqExact(o2, Vector2<T>(T{1}, T{1}));

  const auto f3 = Vector3<T>::fill(T{5});
  ExpectVecEqExact(f3, Vector3<T>(T{5}, T{5}, T{5}));
}

TYPED_TEST(VecTypedTest, Cast_ConvertsEachComponent)
{
  using T = TypeParam;

  const Vector3<int> vi(1, -2, 3);
  const auto vt = static_cast<Vector3<T>>( vi);
  ExpectVecEqExact(vt, Vector3<T>(T{1}, T{-2}, T{3}));

  // Explicit converting ctor should behave like cast()
  const Vector3<T> vt2(vi);
  ExpectVecEqExact(vt2, vt);
}

// --- apply / transform / all / any / clamp ---

TYPED_TEST(VecTypedTest, Apply_MapsEachComponent)
{
  using T = TypeParam;

  const Vector3<T> v(T{1}, T{-2}, T{3});
  const auto absV = v.apply([](T x)
    { return std::abs(x); });

  ExpectVecEqExact(absV, Vector3<T>(T{1}, T{2}, T{3}));
}

TYPED_TEST(VecTypedTest, Transform_CombinesPairwise)
{
  using T = TypeParam;

  const Vector4<T> a(T{1}, T{2}, T{3}, T{4});
  const Vector4<T> b(T{10}, T{20}, T{30}, T{40});

  const auto c = a.transform(b, [](T x, T y)
    { return x + T{2} * y; });
  ExpectVecEqExact(c, Vector4<T>(T{21}, T{42}, T{63}, T{84}));
}

TYPED_TEST(VecTypedTest, AllAny_UnaryPredicate)
{
  using T = TypeParam;

  const Vector3<T> v(T{1}, T{2}, T{3});
  EXPECT_TRUE(v.all([](T x)
    { return x > T{0}; }));
  EXPECT_TRUE(v.any([](T x)
    { return x == T{2}; }));
  EXPECT_FALSE(v.all([](T x)
    { return x == T{2}; }));
  EXPECT_FALSE(v.any([](T x)
    { return x < T{0}; }));
}

TYPED_TEST(VecTypedTest, AllAny_BinaryPredicateAgainstOther)
{
  using T = TypeParam;

  const Vector3<T> a(T{1}, T{2}, T{3});
  const Vector3<T> b(T{1}, T{0}, T{3});

  EXPECT_TRUE(a.any(b, [](T x, T y)
    { return x == y; }));
  EXPECT_FALSE(a.all(b, [](T x, T y)
    { return x == y; }));
}

TYPED_TEST(VecTypedTest, Clamp_ClampsEachComponent)
{
  using T = TypeParam;

  const Vector4<T> v(T{-10}, T{-1}, T{3}, T{100});
  const auto c = v.clamp(T{-2}, T{5});

  ExpectVecEqExact(c, Vector4<T>(T{-2}, T{-1}, T{3}, T{5}));
}

// --- arithmetic operators (vector/vector and vector/scalar) ---

TYPED_TEST(VecTypedTest, VectorAddSub_WorksComponentwise)
{
  using T = TypeParam;

  const Vector3<T> a(T{1}, T{2}, T{3});
  const Vector3<T> b(T{10}, T{20}, T{30});

  ExpectVecEqExact(a + b, Vector3<T>(T{11}, T{22}, T{33}));
  ExpectVecEqExact(b - a, Vector3<T>(T{9}, T{18}, T{27}));
}

TYPED_TEST(VecTypedTest, ScalarMulDiv_WorksAndLeftScalarMulWorks)
{
  using T = TypeParam;

  const Vector2<T> v(T{3}, T{-4});

  ExpectVecEqExact(v * T{2}, Vector2<T>(T{6}, T{-8}));
  ExpectVecEqExact(T{2} * v, Vector2<T>(T{6}, T{-8}));

  const auto half = v / T{2};
  ExpectVecNear(half, Vector2<T>(T{1.5}, T{-2.0}), epsilon_v<T> * T{8});
}

TYPED_TEST(VecTypedTest, CompoundAssignment_MatchesNonCompound)
{
  using T = TypeParam;

  Vector3<T> a(T{1}, T{2}, T{3});
  const Vector3<T> b(T{5}, T{6}, T{7});

  auto expected = a + b;
  a += b;
  ExpectVecEqExact(a, expected);

  a = Vector3<T>(T{1}, T{2}, T{3});
  expected = a - b;
  a -= b;
  ExpectVecEqExact(a, expected);

  a = Vector3<T>(T{1}, T{2}, T{3});
  expected = a * T{3};
  a *= T{3};
  ExpectVecEqExact(a, expected);
}

TYPED_TEST(VecTypedTest, UnaryMinus_NegatesEachComponent)
{
  using T = TypeParam;

  const Vector4<T> v(T{1}, T{-2}, T{3}, T{-4});
  ExpectVecEqExact(-v, Vector4<T>(T{-1}, T{2}, T{-3}, T{4}));
}

// --- dot / length / normalize ---

TYPED_TEST(VecTypedTest, DotProduct_Correct)
{
  using T = TypeParam;

  const Vector3<T> a(T{1}, T{2}, T{3});
  const Vector3<T> b(T{4}, T{5}, T{6});

  EXPECT_NEAR(a.dot(b), T{32}, epsilon_v<T> * T{16}); // 1*4 + 2*5 + 3*6 = 32
}

TYPED_TEST(VecTypedTest, SquaredLengthAndLength_Correct)
{
  using T = TypeParam;

  const Vector2<T> v(T{3}, T{4});
  EXPECT_NEAR(v.squaredLength(), T{25}, epsilon_v<T> * T{16});
  EXPECT_NEAR(v.length(), T{5}, epsilon_v<T> * T{32});
}

TYPED_TEST(VecTypedTest, Normalized_HasUnitLengthAndSameDirection)
{
  using T = TypeParam;

  const Vector3<T> v(T{2}, T{0}, T{0});
  const auto n = v.normalized();

  ExpectVecNear(n, Vector3<T>(T{1}, T{0}, T{0}), epsilon_v<T> * T{32});
  EXPECT_NEAR(n.length(), T{1}, epsilon_v<T> * T{64});
}

TYPED_TEST(VecTypedTest, Normalize_InPlace_ScalesIfNonZero)
{
  using T = TypeParam;

  Vector2<T> v(T{3}, T{4});
  v.normalize();

  EXPECT_NEAR(v.length(), T{1}, epsilon_v<T> * T{64});
  // Direction checks (ratio preserved): v should be (0.6, 0.8)
  ExpectVecNear(v, Vector2<T>(T{0.6}, T{0.8}), epsilon_v<T> * T{256});
}

TYPED_TEST(VecTypedTest, Normalize_DoesNothingForNearZeroVector)
{
  using T = TypeParam;

  // Intentionally tiny values: length <= epsilon => should not modify.
  const T tiny = epsilon_v<T> * T{0.25};
  Vector3<T> v(tiny, -tiny, tiny);
  const auto before = v;

  v.normalize();
  ExpectVecEqExact(v, before);
}

// --- equality (epsilon-based) ---

TYPED_TEST(VecTypedTest, Equality_UsesEpsilonPerComponent)
{
  using T = TypeParam;

  const Vector3<T> a(T{1}, T{2}, T{3});
  const Vector3<T> b(T{1} + epsilon_v<T> * T{0.5}, T{2}, T{3});
  const Vector3<T> c(T{1} + epsilon_v<T> * T{2.0}, T{2}, T{3});

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
}

// --- operator[] ---

TYPED_TEST(VecTypedTest, IndexOperator_ReadWrite)
{
  using T = TypeParam;

  Vector4<T> v(T{0}, T{0}, T{0}, T{0});
  v[0] = T{1};
  v[1] = T{2};
  v[2] = T{3};
  v[3] = T{4};

  EXPECT_EQ(v[0], T{1});
  EXPECT_EQ(v[1], T{2});
  EXPECT_EQ(v[2], T{3});
  EXPECT_EQ(v[3], T{4});
}
