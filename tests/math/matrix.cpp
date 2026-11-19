#include <gtest/gtest.h>

#include "math/matrix.hpp"

namespace math::tests
{
    namespace
    {
        constexpr Matrix<2, 5, int> large{
            Vector2<int>{2, 4}, Vector2<int>{6, 8}, Vector2<int>{10, 12}, Vector2<int>{14, 16}, Vector2<int>{18, 20}};
        constexpr auto doubled = large + large;
        static_assert(doubled[0][0] == 4 && doubled[4][1] == 40);
        static_assert((large - large)[0][0] == 0);
        static_assert((large * 3)[0][1] == 12);
        static_assert((large * Vector<5, int>::one)[0] == 50);
        static_assert((large / 2)[0][1] == 2);
    }

    TEST(Matrix, AppliesArithmeticToEveryColumnInBothPaths)
    {
        const Matrix<2, 2, int> small{Vector2<int>{2, -3}, Vector2<int>{4, -5}};
        EXPECT_EQ((small + small)[0], (Vector2<int>{4, -6}));
        EXPECT_EQ((small * 3)[1], (Vector2<int>{12, -15}));
        EXPECT_EQ((small / 2)[0], (Vector2<int>{1, -1}));
        for (std::size_t column = 0; column < 5; ++column)
        {
            EXPECT_EQ((large + large)[column], large[column] * 2);
            EXPECT_EQ((large - large)[column], Vector2<int>::zero);
            EXPECT_EQ((large * -3)[column], large[column] * -3);
            EXPECT_EQ((large / 2)[column], large[column] / 2);
        }
    }

    TEST(Matrix, DividesByNegativeAndLargeFloatingPointScalars)
    {
        constexpr auto identity = Matrix<2, 2, double>::identity();
        EXPECT_DOUBLE_EQ((identity / -2.0)[0][0], -0.5);
        EXPECT_DOUBLE_EQ((identity / 1e7)[1][1], 1e-7);
        EXPECT_DOUBLE_EQ((identity / 1e7)[0][1], 0.0);
    }

    TEST(Matrix, MultipliesAndTransposesRectangularMatrices)
    {
        constexpr Matrix<2, 3, int> left{Vector2<int>{1, 2}, Vector2<int>{3, 4}, Vector2<int>{5, 6}};
        constexpr Matrix<3, 2, int> right{Vector3<int>{2, 0, -1}, Vector3<int>{0, 1, 0}};
        constexpr auto product = left * right;
        EXPECT_EQ(product[0], (Vector2<int>{-3, -2}));
        EXPECT_EQ(product[1], (Vector2<int>{3, 4}));
        constexpr auto transposed = left.transposed();
        EXPECT_EQ(transposed[0], (Vector3<int>{1, 3, 5}));
        EXPECT_EQ(transposed[1], (Vector3<int>{2, 4, 6}));
    }
}
