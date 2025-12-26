#include <gtest/gtest.h>

#include <cmath>

#include "math/vectors.hpp"
#include "math/quaternion.hpp"

template <typename T>
void ExpectQuatNear(const math::Quaternion<T>& a, const math::Quaternion<T>& b, T tol)
{
  EXPECT_NEAR(a.x(), b.x(), tol);
  EXPECT_NEAR(a.y(), b.y(), tol);
  EXPECT_NEAR(a.z(), b.z(), tol);
  EXPECT_NEAR(a.w(), b.w(), tol);
}

template <typename T>
void ExpectQuatEqExact(const math::Quaternion<T>& a, const math::Quaternion<T>& b)
{
  EXPECT_EQ(a.x(), b.x());
  EXPECT_EQ(a.y(), b.y());
  EXPECT_EQ(a.z(), b.z());
  EXPECT_EQ(a.w(), b.w());
}

template <typename T>
struct QuatTypedTest : ::testing::Test {};

using FloatTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(QuatTypedTest, FloatTypes);

// Small helper: for float/double, use a tolerance that’s stable under a few ops.
template <typename T>
constexpr T kTol()
{
  return math::epsilon_v<T> * T{256};
}

// --- Construction / accessors / basic invariants ---

TYPED_TEST(QuatTypedTest, DefaultConstructor_IsIdentityLike)
{
  using T = TypeParam;

  const math::Quaternion<T> q;
  ExpectQuatEqExact(q, math::Quaternion<T>(T{0}, T{0}, T{0}, T{1}));

  // Length of identity quaternion should be 1.
  EXPECT_NEAR(q.length(), T{1}, kTol<T>());
  EXPECT_NEAR(q.squaredLength(), T{1}, kTol<T>());
}

TYPED_TEST(QuatTypedTest, ComponentConstructor_AndAccessors)
{
  using T = TypeParam;

  const math::Quaternion<T> q(T{1}, T{2}, T{3}, T{4});
  EXPECT_EQ(q.x(), T{1});
  EXPECT_EQ(q.y(), T{2});
  EXPECT_EQ(q.z(), T{3});
  EXPECT_EQ(q.w(), T{4});
}

TYPED_TEST(QuatTypedTest, VectorConstructor_PreservesComponents)
{
  using T = TypeParam;

  const math::Vector4<T> v(T{1}, T{-2}, T{3}, T{-4});
  const math::Quaternion<T> q(v);

  EXPECT_EQ(q.x(), T{1});
  EXPECT_EQ(q.y(), T{-2});
  EXPECT_EQ(q.z(), T{3});
  EXPECT_EQ(q.w(), T{-4});
}

// --- length / normalization semantics ---

TYPED_TEST(QuatTypedTest, LengthAndSquaredLength_MatchExpectedForSimpleCase)
{
  using T = TypeParam;

  const math::Quaternion<T> q(T{3}, T{4}, T{0}, T{0});
  EXPECT_NEAR(q.squaredLength(), T{25}, kTol<T>());
  EXPECT_NEAR(q.length(), T{5}, kTol<T>());
}

TYPED_TEST(QuatTypedTest, Normalized_ReturnsUnitQuaternion_ForNonZero)
{
  using T = TypeParam;

  const math::Quaternion<T> q(T{0}, T{0}, T{0}, T{2});
  const auto n = q.normalized();

  ExpectQuatNear(n, math::Quaternion<T>(T{0}, T{0}, T{0}, T{1}), kTol<T>());
  EXPECT_NEAR(n.length(), T{1}, kTol<T>());
  EXPECT_NEAR(n.squaredLength(), T{1}, kTol<T>());
}

TYPED_TEST(QuatTypedTest, Normalize_InPlace_MakesUnitQuaternion_ForNonZero)
{
  using T = TypeParam;

  math::Quaternion<T> q(T{1}, T{2}, T{3}, T{4});
  const auto beforeLen = q.length();
  ASSERT_GT(beforeLen, T{0});

  q.normalize();

  EXPECT_NEAR(q.length(), T{1}, kTol<T>());
  EXPECT_NEAR(q.squaredLength(), T{1}, kTol<T>());
}

TYPED_TEST(QuatTypedTest, Normalize_DoesNotExplodeForNearZero)
{
  using T = TypeParam;

  // This relies on Vector::normalize() behavior: it should avoid division for tiny length.
  const T tiny = math::epsilon_v<T> * T{0.25};
  math::Quaternion<T> q(tiny, -tiny, tiny, -tiny);
  const auto before = q;

  q.normalize();

  // Should remain unchanged (or at least finite and very close).
  ExpectQuatNear(q, before, T{0});
  EXPECT_TRUE(std::isfinite(q.x()));
  EXPECT_TRUE(std::isfinite(q.y()));
  EXPECT_TRUE(std::isfinite(q.z()));
  EXPECT_TRUE(std::isfinite(q.w()));
}

// --- conjugate properties ---

TYPED_TEST(QuatTypedTest, Conjugate_NegatesVectorPart_LeavesScalarPart)
{
  using T = TypeParam;

  const math::Quaternion<T> q(T{1}, T{-2}, T{3}, T{-4});
  const auto c = q.conjugate();

  ExpectQuatEqExact(c, math::Quaternion<T>(T{-1}, T{2}, T{-3}, T{-4}));
}

TYPED_TEST(QuatTypedTest, ConjugateOfConjugate_ReturnsOriginal)
{
  using T = TypeParam;

  const math::Quaternion<T> q(T{1}, T{2}, T{3}, T{4});
  const auto cc = q.conjugate().conjugate();

  ExpectQuatEqExact(cc, q);
}

TYPED_TEST(QuatTypedTest, Conjugate_PreservesLength)
{
  using T = TypeParam;

  const math::Quaternion<T> q(T{1}, T{2}, T{3}, T{4});
  const auto c = q.conjugate();

  EXPECT_NEAR(q.length(), c.length(), kTol<T>());
  EXPECT_NEAR(q.squaredLength(), c.squaredLength(), kTol<T>());
}

// --- multiplication: identity, associativity sanity, conjugation relation ---

TYPED_TEST(QuatTypedTest, Multiplication_ByIdentity_NoChange)
{
  using T = TypeParam;

  const math::Quaternion<T> identity;
  const math::Quaternion<T> q(T{1}, T{2}, T{3}, T{4});

  // q * I == q and I * q == q
  ExpectQuatNear(q * identity, q, kTol<T>());
  ExpectQuatNear(identity * q, q, kTol<T>());
}

TYPED_TEST(QuatTypedTest, Multiplication_IsNotCommutative_InGeneral)
{
  using T = TypeParam;

  const math::Quaternion<T> a(T{1}, T{2}, T{3}, T{4});
  const math::Quaternion<T> b(T{5}, T{6}, T{7}, T{8});

  const auto ab = a * b;
  const auto ba = b * a;

  // We don't have operator!=, so check at least one component differs by more than tolerance.
  const T tol = kTol<T>();
  const bool differs =
      (std::abs(ab.x() - ba.x()) > tol) ||
      (std::abs(ab.y() - ba.y()) > tol) ||
      (std::abs(ab.z() - ba.z()) > tol) ||
      (std::abs(ab.w() - ba.w()) > tol);

  EXPECT_TRUE(differs);
}

TYPED_TEST(QuatTypedTest, Multiplication_Associativity_SanityCheck)
{
  using T = TypeParam;

  // Quaternion multiplication is associative; numerics may introduce tiny error.
  const math::Quaternion<T> a(T{1}, T{2}, T{3}, T{4});
  const math::Quaternion<T> b(T{2}, T{-1}, T{0.5}, T{3});
  const math::Quaternion<T> c(T{-2}, T{1}, T{1}, T{0});

  const auto left = (a * b) * c;
  const auto right = a * (b * c);

  ExpectQuatNear(left, right, kTol<T>() * T{8});
}

TYPED_TEST(QuatTypedTest, ConjugateOfProduct_ReversesOrder)
{
  using T = TypeParam;

  // (ab)* = b* a*
  const math::Quaternion<T> a(T{1}, T{2}, T{3}, T{4});
  const math::Quaternion<T> b(T{5}, T{6}, T{7}, T{8});

  const auto lhs = (a * b).conjugate();
  const auto rhs = b.conjugate() * a.conjugate();

  ExpectQuatNear(lhs, rhs, kTol<T>() * T{8});
}

// --- norm multiplicativity (important for production correctness) ---

TYPED_TEST(QuatTypedTest, SquaredNorm_IsMultiplicative)
{
  using T = TypeParam;

  const math::Quaternion<T> a(T{1}, T{2}, T{3}, T{4});
  const math::Quaternion<T> b(T{2}, T{-1}, T{0.5}, T{3});

  const auto ab = a * b;

  const T na2 = a.squaredLength();
  const T nb2 = b.squaredLength();
  const T nab2 = ab.squaredLength();

  EXPECT_NEAR(nab2, na2 * nb2, kTol<T>() * T{64});
}

TYPED_TEST(QuatTypedTest, ProductWithConjugate_IsPureScalarWithSquaredNorm)
{
  using T = TypeParam;

  const math::Quaternion<T> q(T{1}, T{2}, T{3}, T{4});
  const auto qc = q.conjugate();
  const auto p1 = q * qc;
  const auto p2 = qc * q;

  // q * q* = (0,0,0, ||q||^2)
  const T n2 = q.squaredLength();

  ExpectQuatNear(p1, math::Quaternion<T>(T{0}, T{0}, T{0}, n2), kTol<T>() * T{64});
  ExpectQuatNear(p2, math::Quaternion<T>(T{0}, T{0}, T{0}, n2), kTol<T>() * T{64});
}
