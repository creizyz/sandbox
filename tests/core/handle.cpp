#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <unordered_map>
#include <vector>

#include "core/handle.hpp"

namespace
{

    struct AliveInfo
    {
        uint32_t m_index;
        uint32_t m_generation;
    };

    void expect_invalid(const HandleRegister &reg, Handle h)
    {
        EXPECT_FALSE(h.is_valid());    // Handle struct-level validity (id != invalid_id)
        EXPECT_FALSE(reg.is_valid(h)); // Register-level validity
        EXPECT_EQ(reg.get_index(h), HandleRegister::invalid_index);
    }

    void expect_valid(const HandleRegister &reg, Handle h, uint32_t expected_index)
    {
        EXPECT_TRUE(h.is_valid());
        EXPECT_TRUE(reg.is_valid(h));
        EXPECT_EQ(reg.get_index(h), expected_index);
    }

    class HandleRegisterTest : public ::testing::Test
    {
    protected:
        HandleRegister reg;

        void SetUp() override
        {
            reg.reserve(1024, 1024);
        }
    };

} // namespace

TEST(HandleSmoke, DefaultHandleIsInvalid)
{
    Handle h{};
    EXPECT_EQ(h.m_id, Handle::invalid_id);
    EXPECT_FALSE(h.is_valid());
}

TEST_F(HandleRegisterTest, InsertReturnsValidHandleAndMapsToIndex)
{
    Handle h = reg.insert(42);
    expect_valid(reg, h, 42);
}

TEST_F(HandleRegisterTest, MultipleInsertsAreIndependent)
{
    Handle a = reg.insert(1);
    Handle b = reg.insert(2);
    Handle c = reg.insert(3);

    expect_valid(reg, a, 1u);
    expect_valid(reg, b, 2u);
    expect_valid(reg, c, 3u);

    EXPECT_NE(a.m_id, b.m_id);
    EXPECT_NE(a.m_id, c.m_id);
    EXPECT_NE(b.m_id, c.m_id);
}

TEST_F(HandleRegisterTest, UpdateChangesIndexForValidHandle)
{
    Handle h = reg.insert(10);
    expect_valid(reg, h, 10u);

    EXPECT_TRUE(reg.update(h, 99));
    expect_valid(reg, h, 99u);
}

TEST_F(HandleRegisterTest, UpdateFailsForInvalidHandleId)
{
    Handle invalid{};
    EXPECT_FALSE(reg.update(invalid, 123));
    expect_invalid(reg, invalid);
}

TEST_F(HandleRegisterTest, EraseInvalidatesHandle)
{
    Handle h = reg.insert(5);
    expect_valid(reg, h, 5u);

    reg.erase(h);

    EXPECT_FALSE(reg.is_valid(h));
    EXPECT_EQ(reg.get_index(h), HandleRegister::invalid_index);
}

TEST_F(HandleRegisterTest, ReinsertAfterEraseKeepsOldHandleInvalid)
{
    Handle h1 = reg.insert(111);
    ASSERT_TRUE(reg.is_valid(h1));

    const uint32_t old_id = h1.m_id;
    const uint32_t old_generation = h1.m_generation;

    reg.erase(h1);
    EXPECT_FALSE(reg.is_valid(h1));

    Handle h2 = reg.insert(222);
    expect_valid(reg, h2, 222u);

    // Accept both common implementations:
    // 1) Reuse the same id and bump generation
    // 2) Allocate a fresh id (no reuse)
    if (h2.m_id == old_id)
    {
        EXPECT_GT(h2.m_generation, old_generation);
    }
    else
    {
        EXPECT_NE(h2.m_id, old_id);
    }

    // Old handle must remain invalid regardless.
    EXPECT_FALSE(reg.is_valid(h1));
    EXPECT_EQ(reg.get_index(h1), HandleRegister::invalid_index);
}

TEST_F(HandleRegisterTest, EraseInvalidHandleIsNoopAndDoesNotCrash)
{
    Handle invalid{};
    reg.erase(invalid);
    expect_invalid(reg, invalid);
}

TEST_F(HandleRegisterTest, StaleGenerationIsInvalid)
{
    Handle h = reg.insert(7);
    expect_valid(reg, h, 7u);

    Handle stale = h;
    stale.m_generation += 1;

    EXPECT_FALSE(reg.is_valid(stale));
    EXPECT_EQ(reg.get_index(stale), HandleRegister::invalid_index);

    // Original still valid.
    expect_valid(reg, h, 7u);
}

TEST_F(HandleRegisterTest, RandomizedOperationsMaintainConsistency)
{
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> op_dist(0, 2); // 0 insert, 1 update, 2 erase
    std::uniform_int_distribution<uint32_t> idx_dist(0, 100000);

    std::vector<Handle> handles;
    handles.reserve(2000);

    std::unordered_map<uint32_t, AliveInfo> alive_by_id; // id -> {index, generation}

    auto index_is_occupied = [&](uint32_t index)
    {
        return std::ranges::any_of(alive_by_id, [index](const auto &entry)
                                   { return entry.second.m_index == index; });
    };

    auto check_all_alive = [&]
    {
        for (const Handle &h : handles)
        {
            if (!h.is_valid())
                continue;

            auto it = alive_by_id.find(h.m_id);
            const bool should_be_alive = (it != alive_by_id.end() && it->second.m_generation == h.m_generation);

            EXPECT_EQ(reg.is_valid(h), should_be_alive);

            if (should_be_alive)
            {
                EXPECT_EQ(reg.get_index(h), it->second.m_index);
            }
            else
            {
                EXPECT_EQ(reg.get_index(h), HandleRegister::invalid_index);
            }
        }
    };

    for (int step = 0; step < 5000; ++step)
    {
        SCOPED_TRACE(step);

        const int op = op_dist(rng);

        if (op == 0 || handles.empty())
        {
            // INSERT
            const uint32_t idx = idx_dist(rng);
            const bool should_insert = !index_is_occupied(idx);
            Handle h = reg.insert(idx);

            if (should_insert)
            {
                ASSERT_TRUE(h.is_valid());
                expect_valid(reg, h, idx);
                ASSERT_TRUE(alive_by_id.emplace(h.m_id, AliveInfo{idx, h.m_generation}).second);
                handles.push_back(h);
            }
            else
            {
                expect_invalid(reg, h);
            }
        }
        else
        {
            // Pick an existing handle (may be stale).
            std::uniform_int_distribution<size_t> pick(0, handles.size() - 1);
            Handle h = handles[pick(rng)];
            const auto it = alive_by_id.find(h.m_id);
            const bool is_alive = it != alive_by_id.end() && it->second.m_generation == h.m_generation;

            if (op == 1)
            {
                // UPDATE
                const uint32_t new_index = idx_dist(rng);
                const bool should_update = is_alive && !index_is_occupied(new_index);
                ASSERT_EQ(reg.update(h, new_index), should_update);

                if (is_alive)
                {
                    if (should_update)
                    {
                        it->second.m_index = new_index;
                    }
                    expect_valid(reg, h, it->second.m_index);
                }
                else
                {
                    EXPECT_FALSE(reg.is_valid(h));
                    EXPECT_EQ(reg.get_index(h), HandleRegister::invalid_index);
                }
            }
            else
            {
                // ERASE
                reg.erase(h);
                EXPECT_FALSE(reg.is_valid(h));
                EXPECT_EQ(reg.get_index(h), HandleRegister::invalid_index);

                if (is_alive)
                {
                    alive_by_id.erase(it);
                }
            }
        }

        if ((step % 100) == 0)
            check_all_alive();
    }

    check_all_alive();
}

TEST(HandleRegisterResize, PreservesSurvivorsAndNeverResurrectsRemovedHandles)
{
    HandleRegister reg;
    reg.resize(4, 8);
    const Handle kept = reg.insert(0);
    const Handle lost_index = reg.insert(7);
    const Handle lost_id = reg.insert(1);
    EXPECT_NE(kept.m_id, lost_index.m_id);
    reg.resize(2, 2);
    expect_valid(reg, kept, 0);
    EXPECT_FALSE(reg.is_valid(lost_index));
    EXPECT_FALSE(reg.is_valid(lost_id));
    EXPECT_FALSE(reg.get_handle(1).is_valid());
    reg.resize(4, 8);
    expect_valid(reg, reg.insert(1), 1);
    expect_valid(reg, reg.insert(7), 7);
    EXPECT_FALSE(reg.is_valid(lost_index));
    EXPECT_FALSE(reg.is_valid(lost_id));
    EXPECT_FALSE(reg.insert(HandleRegister::invalid_index).is_valid());
    EXPECT_FALSE(reg.update(kept, HandleRegister::invalid_index));
}

TEST(HandleRegisterOwnership, CopyIssuesIndependentHandlesAndMoveTransfersOwnership)
{
    HandleRegister first;
    const auto a = first.insert(0);
    HandleRegister copy = first;
    const auto b = copy.get_handle(0);
    EXPECT_FALSE(copy.is_valid(a));
    EXPECT_FALSE(first.is_valid(b));
    copy = std::move(first);
    EXPECT_FALSE(copy.is_valid(b));
    expect_valid(copy, a, 0);
    auto c = first.insert(0);
    EXPECT_FALSE(first.is_valid(a));
    EXPECT_FALSE(copy.is_valid(c));
}
