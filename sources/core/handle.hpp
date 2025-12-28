#pragma once

#include <cstdint>
#include <vector>
#include <numeric>

class HandleRegister;

struct Handle
{
    static constexpr auto invalid_id = std::numeric_limits<std::uint32_t>::max();

    uint32_t id{ invalid_id };
    uint32_t generation{ 0 };

    [[nodiscard]] constexpr  bool is_valid() const noexcept
    {
        return id != invalid_id;
    }
};

class HandleRegister
{
public:
    static constexpr auto invalid_id = std::numeric_limits<std::uint32_t>::max();
    static constexpr auto invalid_index = std::numeric_limits<std::uint32_t>::max();

    void reserve(std::size_t handleCapacity, std::size_t indexCapacity);
    void resize(std::size_t handleCapacity, std::size_t indexCapacity);

    Handle insert(uint32_t index);
    bool update(Handle handle, uint32_t index);
    void erase(Handle handle);

    [[nodiscard]] bool is_valid(Handle handle) const noexcept;
    [[nodiscard]] uint32_t get_index(Handle handle) const noexcept;

private:
    std::vector<uint32_t> m_idToIndex;
    std::vector<uint32_t> m_indexToId;
    std::vector<uint32_t> m_generations;
    std::vector<uint32_t> m_freeIds;
};
