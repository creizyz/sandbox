#include "handle.hpp"

#include <algorithm>
#include <atomic>
#include <exception>
#include <stdexcept>
#include <utility>

namespace
{
    std::uint64_t next_owner() noexcept
    {
        static std::atomic<std::uint64_t> sequence{1};
        const auto owner = sequence.fetch_add(1, std::memory_order_relaxed);
        if (owner == 0)
        {
            std::terminate();
        }
        return owner;
    }
}

HandleRegister::HandleRegister() noexcept
    : m_owner(next_owner()) {}

HandleRegister::HandleRegister(const HandleRegister &other)
    : m_owner(next_owner()), m_slots(other.m_slots), m_index_to_id(other.m_index_to_id), m_free_ids(other.m_free_ids)
{
}

HandleRegister::HandleRegister(HandleRegister &&other) noexcept
    : HandleRegister()
{
    swap(other);
}

HandleRegister &HandleRegister::operator=(const HandleRegister &other)
{
    if (this != &other)
    {
        HandleRegister copy(other);
        swap(copy);
    }
    return *this;
}

HandleRegister &HandleRegister::operator=(HandleRegister &&other) noexcept
{
    if (this != &other)
    {
        HandleRegister moved(std::move(other));
        swap(moved);
    }
    return *this;
}

void HandleRegister::swap(HandleRegister &other) noexcept
{
    std::swap(m_owner, other.m_owner);
    m_slots.swap(other.m_slots);
    m_index_to_id.swap(other.m_index_to_id);
    m_free_ids.swap(other.m_free_ids);
}

void HandleRegister::reserve(std::size_t handle_capacity, std::size_t index_capacity)
{
    if (handle_capacity > invalid_id || index_capacity > invalid_index)
    {
        throw std::invalid_argument("Handle register capacity exceeds uint32_t range");
    }
    m_slots.reserve(handle_capacity);
    m_free_ids.reserve(handle_capacity);
    m_index_to_id.reserve(index_capacity);
}

void HandleRegister::resize(std::size_t handle_capacity, std::size_t index_capacity)
{
    // Retain generation history when a shrink invalidates a slot.
    reserve(std::max(handle_capacity, m_slots.size()), index_capacity);
    if (handle_capacity > m_slots.size())
    {
        m_slots.resize(handle_capacity);
    }
    m_index_to_id.resize(index_capacity, invalid_id);
    m_free_ids.clear();
    for (auto n = m_slots.size(); n > 0; --n)
    {
        const auto id = static_cast<uint32_t>(n - 1);
        auto &slot = m_slots[id];
        if (slot.index != invalid_index && (id >= handle_capacity || slot.index >= index_capacity))
        {
            if (slot.index < index_capacity)
            {
                m_index_to_id[slot.index] = invalid_id;
            }
            slot.index = invalid_index;
            ++slot.generation;
        }
        if (slot.index == invalid_index && slot.generation != UINT32_MAX)
        {
            m_free_ids.push_back(id);
        }
    }
}

Handle HandleRegister::insert(uint32_t index)
{
    if (index == invalid_index)
    {
        return {};
    }
    if (index >= m_index_to_id.size())
    {
        m_index_to_id.resize(static_cast<std::size_t>(index) + 1, invalid_id);
    }
    if (m_index_to_id[index] != invalid_id)
    {
        return {};
    }

    uint32_t id;
    if (m_free_ids.empty())
    {
        if (m_slots.size() == invalid_id)
        {
            throw std::length_error("Handle register exhausted its IDs");
        }
        id = static_cast<uint32_t>(m_slots.size());
        m_slots.emplace_back();
    }
    else
    {
        id = m_free_ids.back();
        m_free_ids.pop_back();
    }
    m_index_to_id[index] = id;
    m_slots[id].index = index;
    return {id, m_slots[id].generation, m_owner};
}

bool HandleRegister::update(Handle handle, uint32_t index)
{
    if (!is_valid(handle) || index == invalid_index)
    {
        return false;
    }
    if (index >= m_index_to_id.size())
    {
        m_index_to_id.resize(static_cast<std::size_t>(index) + 1, invalid_id);
    }
    if (m_index_to_id[index] != invalid_id)
    {
        return false;
    }
    auto &slot = m_slots[handle.m_id];
    m_index_to_id[slot.index] = invalid_id;
    slot.index = index;
    m_index_to_id[index] = handle.m_id;
    return true;
}

void HandleRegister::erase(Handle handle)
{
    if (is_valid(handle))
    {
        auto &slot = m_slots[handle.m_id];
        // Retire IDs whose generation would wrap.
        if (slot.generation < UINT32_MAX - 1)
        {
            m_free_ids.push_back(handle.m_id);
        }
        m_index_to_id[slot.index] = invalid_id;
        slot.index = invalid_index;
        ++slot.generation;
    }
}

bool HandleRegister::is_valid(Handle handle) const noexcept
{
    return handle.m_owner == m_owner && handle.m_id < m_slots.size() && m_slots[handle.m_id].generation == handle.m_generation && m_slots[handle.m_id].index != invalid_index;
}

uint32_t HandleRegister::get_index(Handle handle) const noexcept
{
    return is_valid(handle) ? m_slots[handle.m_id].index : invalid_index;
}

Handle HandleRegister::get_handle(uint32_t index) const noexcept
{
    if (index < m_index_to_id.size())
    {
        const auto id = m_index_to_id[index];
        if (id != invalid_id)
        {
            return {id, m_slots[id].generation, m_owner};
        }
    }
    return {};
}
