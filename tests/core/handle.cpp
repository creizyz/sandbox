#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <random>
#include <unordered_map>
#include <vector>

#include "core/handle.hpp"

namespace {

struct AliveInfo {
    uint32_t index;
    uint32_t generation;
};

void ExpectInvalid(const HandleRegister& reg, Handle h) {
    EXPECT_FALSE(h.is_valid());    // Handle struct-level validity (id != invalid_id)
    EXPECT_FALSE(reg.is_valid(h)); // Register-level validity
    EXPECT_EQ(reg.get_index(h), HandleRegister::invalid_index);
}

void ExpectValid(const HandleRegister& reg, Handle h, uint32_t expectedIndex) {
    EXPECT_TRUE(h.is_valid());
    EXPECT_TRUE(reg.is_valid(h));
    EXPECT_EQ(reg.get_index(h), expectedIndex);
}

class HandleRegisterTest : public ::testing::Test {
protected:
    HandleRegister reg;

    void SetUp() override {
        reg.reserve(1024, 1024);
    }
};

} // namespace

TEST(HandleSmoke, DefaultHandleIsInvalid) {
    Handle h{};
    EXPECT_EQ(h.id, Handle::invalid_id);
    EXPECT_FALSE(h.is_valid());
}

TEST_F(HandleRegisterTest, InsertReturnsValidHandleAndMapsToIndex) {
    Handle h = reg.insert(42);
    ExpectValid(reg, h, 42);
}

TEST_F(HandleRegisterTest, MultipleInsertsAreIndependent) {
    Handle a = reg.insert(1);
    Handle b = reg.insert(2);
    Handle c = reg.insert(3);

    ExpectValid(reg, a, 1u);
    ExpectValid(reg, b, 2u);
    ExpectValid(reg, c, 3u);

    EXPECT_NE(a.id, b.id);
    EXPECT_NE(a.id, c.id);
    EXPECT_NE(b.id, c.id);
}

TEST_F(HandleRegisterTest, UpdateChangesIndexForValidHandle) {
    Handle h = reg.insert(10);
    ExpectValid(reg, h, 10u);

    EXPECT_TRUE(reg.update(h, 99));
    ExpectValid(reg, h, 99u);
}

TEST_F(HandleRegisterTest, UpdateFailsForInvalidHandleId) {
    Handle invalid{};
    EXPECT_FALSE(reg.update(invalid, 123));
    ExpectInvalid(reg, invalid);
}

TEST_F(HandleRegisterTest, EraseInvalidatesHandle) {
    Handle h = reg.insert(5);
    ExpectValid(reg, h, 5u);

    reg.erase(h);

    EXPECT_FALSE(reg.is_valid(h));
    EXPECT_EQ(reg.get_index(h), HandleRegister::invalid_index);
}

TEST_F(HandleRegisterTest, ReinsertAfterEraseKeepsOldHandleInvalid) {
    Handle h1 = reg.insert(111);
    ASSERT_TRUE(reg.is_valid(h1));

    const uint32_t oldId  = h1.id;
    const uint32_t oldGen = h1.generation;

    reg.erase(h1);
    EXPECT_FALSE(reg.is_valid(h1));

    Handle h2 = reg.insert(222);
    ExpectValid(reg, h2, 222u);

    // Accept both common implementations:
    // 1) Reuse the same id and bump generation
    // 2) Allocate a fresh id (no reuse)
    if (h2.id == oldId) {
        EXPECT_GT(h2.generation, oldGen);
    } else {
        EXPECT_NE(h2.id, oldId);
    }

    // Old handle must remain invalid regardless.
    EXPECT_FALSE(reg.is_valid(h1));
    EXPECT_EQ(reg.get_index(h1), HandleRegister::invalid_index);
}

TEST_F(HandleRegisterTest, EraseInvalidHandleIsNoopAndDoesNotCrash) {
    Handle invalid{};
    reg.erase(invalid);
    ExpectInvalid(reg, invalid);
}

TEST_F(HandleRegisterTest, StaleGenerationIsInvalid) {
    Handle h = reg.insert(7);
    ExpectValid(reg, h, 7u);

    Handle stale = h;
    stale.generation += 1;

    EXPECT_FALSE(reg.is_valid(stale));
    EXPECT_EQ(reg.get_index(stale), HandleRegister::invalid_index);

    // Original still valid.
    ExpectValid(reg, h, 7u);
}

TEST_F(HandleRegisterTest, RandomizedOperationsMaintainConsistency) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> opDist(0, 2); // 0 insert, 1 update, 2 erase
    std::uniform_int_distribution<uint32_t> idxDist(0, 100000);

    std::vector<Handle> handles;
    handles.reserve(2000);

    std::unordered_map<uint32_t, AliveInfo> aliveById; // id -> {index, generation}

    auto checkAllAlive = [&] {
        for (const Handle& h : handles) {
            if (!h.is_valid()) continue;

            auto it = aliveById.find(h.id);
            const bool shouldBeAlive = (it != aliveById.end() && it->second.generation == h.generation);

            EXPECT_EQ(reg.is_valid(h), shouldBeAlive);

            if (shouldBeAlive) {
                EXPECT_EQ(reg.get_index(h), it->second.index);
            } else {
                EXPECT_EQ(reg.get_index(h), HandleRegister::invalid_index);
            }
        }
    };

    for (int step = 0; step < 5000; ++step) {
        SCOPED_TRACE(step);

        const int op = opDist(rng);

        if (op == 0 || handles.empty()) {
            // INSERT
            const uint32_t idx = idxDist(rng);
            Handle h = reg.insert(idx);

            ExpectValid(reg, h, idx);

            aliveById[h.id] = AliveInfo{idx, h.generation};
            handles.push_back(h);
        } else {
            // Pick an existing handle (may be stale).
            std::uniform_int_distribution<size_t> pick(0, handles.size() - 1);
            Handle h = handles[pick(rng)];

            if (op == 1) {
                // UPDATE
                const uint32_t newIdx = idxDist(rng);
                const bool ok = reg.update(h, newIdx);

                const bool isAlive = reg.is_valid(h);
                EXPECT_EQ(ok, isAlive);

                if (isAlive) {
                    EXPECT_EQ(reg.get_index(h), newIdx);
                    aliveById[h.id] = AliveInfo{newIdx, h.generation};
                } else {
                    EXPECT_EQ(reg.get_index(h), HandleRegister::invalid_index);
                }
            } else {
                // ERASE
                reg.erase(h);
                EXPECT_FALSE(reg.is_valid(h));
                EXPECT_EQ(reg.get_index(h), HandleRegister::invalid_index);

                // If it was alive in the model, remove it.
                auto it = aliveById.find(h.id);
                if (it != aliveById.end() && it->second.generation == h.generation) {
                    aliveById.erase(it);
                }
            }
        }

        if ((step % 100) == 0) checkAllAlive();
    }

    checkAllAlive();
}