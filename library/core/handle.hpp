#pragma once

#include <cstdint>
#include <limits>
#include <vector>

class HandleRegister;

struct Handle
{
    static constexpr auto invalid_id = std::numeric_limits<std::uint32_t>::max();

    uint32_t m_id{invalid_id};
    uint32_t m_generation{0};

    [[nodiscard]] constexpr bool is_valid() const noexcept
    {
        return m_id != invalid_id;
    }
};

class HandleRegister
{
public:
    static constexpr auto invalid_id = std::numeric_limits<std::uint32_t>::max();
    static constexpr auto invalid_index = std::numeric_limits<std::uint32_t>::max();

    void reserve(std::size_t handle_capacity, std::size_t index_capacity);
    void resize(std::size_t handle_capacity, std::size_t index_capacity);

    Handle insert(uint32_t index);
    bool update(Handle handle, uint32_t index);
    void erase(Handle handle);

    [[nodiscard]] bool is_valid(Handle handle) const noexcept;
    [[nodiscard]] uint32_t get_index(Handle handle) const noexcept;
    [[nodiscard]] Handle get_handle(uint32_t index) const noexcept;

private:
    std::vector<uint32_t> m_id_to_index;
    std::vector<uint32_t> m_index_to_id;
    std::vector<uint32_t> m_generations;
    std::vector<uint32_t> m_free_ids;
};
