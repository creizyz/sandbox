#include <gtest/gtest.h>

#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "math/vectors-soa.hpp"

namespace math::tests
{
    using Rows = VectorSoa<3, int>;
    static_assert(std::ranges::forward_range<Rows>);
    static_assert(std::ranges::forward_range<const Rows>);
    static_assert(std::is_same_v<std::ranges::range_reference_t<const Rows>, ConstVectorView<3, int>>);

    TEST(VectorSoa, StoresComponentsAndReturnsMutableAndConstViews)
    {
        Rows rows;
        const auto handle = rows.emplace(1, 2, 3);
        EXPECT_EQ(rows.size(), 1);
        EXPECT_TRUE(rows.handle_at(0).is_valid());
        EXPECT_FALSE(rows.handle_at(1).is_valid());
        EXPECT_EQ(rows[0].as_vector(), (Vector3<int>{1, 2, 3}));
        auto view = rows.get(handle);
        ASSERT_TRUE(view);
        view->y() = 4;

        auto read_only = std::as_const(rows).get(handle);
        ASSERT_TRUE(read_only);
        EXPECT_EQ(read_only->as_vector(), (Vector3<int>{1, 4, 3}));
        EXPECT_EQ(std::as_const(rows)[0].as_vector(), read_only->as_vector());
    }

    TEST(VectorSoa, HandlesSurviveGrowthAndSwapEraseWithoutResurrectingRemovedRows)
    {
        Rows rows{1};
        const auto first = rows.emplace(1, 2, 3);
        const auto middle = rows.emplace(4, 5, 6);
        const auto last = rows.emplace(7, 8, 9);
        rows.reserve(32);
        ASSERT_TRUE(rows.erase(middle));
        ASSERT_TRUE(rows.get(first));
        ASSERT_TRUE(rows.get(last));
        EXPECT_EQ(rows.get(first)->as_vector(), (Vector3<int>{1, 2, 3}));
        EXPECT_EQ(rows.get(last)->as_vector(), (Vector3<int>{7, 8, 9}));
        EXPECT_EQ(rows[1].as_vector(), rows.get(last)->as_vector());
        EXPECT_EQ(rows.handle_at(1).m_id, last.m_id);
        EXPECT_FALSE(rows.get(middle));
        EXPECT_FALSE(rows.erase(middle));

        const auto replacement = rows.emplace(10, 11, 12);
        EXPECT_TRUE(rows.get(replacement));
        EXPECT_FALSE(rows.get(middle));
    }

    TEST(VectorSoa, ResizesWithZerosAndInvalidatesOnlyRemovedHandles)
    {
        Rows rows{8};
        const auto kept = rows.emplace(1, 2, 3);
        rows.resize(4);
        ASSERT_EQ(rows.size(), 4);
        const auto removed = rows.handle_at(3);
        ASSERT_TRUE(removed.is_valid());
        EXPECT_EQ(rows[3].as_vector(), Vector3<int>::zero);
        rows.resize(2);
        rows.shrink_to_fit();
        EXPECT_EQ(rows.capacity(), 2);
        ASSERT_TRUE(rows.get(kept));
        EXPECT_EQ(rows.get(kept)->as_vector(), (Vector3<int>{1, 2, 3}));
        EXPECT_FALSE(rows.get(removed));

        rows.clear();
        EXPECT_TRUE(rows.empty());
        EXPECT_EQ(rows.capacity(), 2);
        EXPECT_FALSE(rows.get(kept));
        rows.resize(4);
        EXPECT_FALSE(rows.get(kept));
        EXPECT_FALSE(rows.get(removed));
    }

    TEST(VectorSoa, IteratesRowsAndMovesTheirHandlesWithStorage)
    {
        Rows rows;
        const auto first = rows.emplace(1, 2, 3);
        rows.emplace(4, 5, 6);
        for (auto row : rows)
        {
            row.x() += 10;
        }
        Rows moved{std::move(rows)};
        EXPECT_TRUE(rows.empty());
        ASSERT_TRUE(moved.get(first));
        EXPECT_EQ(moved.get(first)->x(), 11);
        int sum = 0;
        for (auto row : std::as_const(moved))
        {
            sum += row.x();
        }
        EXPECT_EQ(sum, 25);
    }

    TEST(VectorSoa, LeavesExistingRowsIntactWhenAnInputConversionThrows)
    {
        struct ThrowingInput
        {
            operator int() const
            {
                throw std::runtime_error("conversion failed");
            }
        };

        Rows rows{1};
        const auto original = rows.emplace(1, 2, 3);
        EXPECT_THROW(rows.emplace(4, ThrowingInput{}, 6), std::runtime_error);
        EXPECT_EQ(rows.size(), 1);
        EXPECT_EQ(rows.capacity(), 1);
        ASSERT_TRUE(rows.get(original));
        EXPECT_EQ(rows.get(original)->as_vector(), (Vector3<int>{1, 2, 3}));
    }
}
