#include "handle.hpp"

#include <stdexcept>

namespace
{
    inline void ensure_size(std::vector<std::uint32_t> & container, std::size_t n, std::uint32_t fill)
    {
        if (container.size() < n)
        {
            container.resize(n, fill);
        }
    }
}

void HandleRegister::reserve(std::size_t handle_capacity, std::size_t index_capacity)
{
    if (handle_capacity > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        throw std::invalid_argument("handle_capacity exceeds uint32_t range");
    }

    if (index_capacity > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        throw std::invalid_argument("index_capacity exceeds uint32_t range");
    }

    m_id_to_index.reserve(handle_capacity);
    m_generations.reserve(handle_capacity);
    m_free_ids.reserve(handle_capacity);
    m_index_to_id.reserve(index_capacity);
}

void HandleRegister::resize(std::size_t handle_capacity, std::size_t index_capacity)
{
    if (handle_capacity > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        throw std::invalid_argument("handle_capacity exceeds uint32_t range");
    }

    if (index_capacity > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        throw std::invalid_argument("index_capacity exceeds uint32_t range");
    }

    if (handle_capacity < m_free_ids.size())
    {
        std::erase_if(m_free_ids, [=](std::uint32_t id) {
            return id >= handle_capacity;
        });
    }

    m_id_to_index.resize(handle_capacity, invalid_index);
    m_generations.resize(handle_capacity, 0);
    m_index_to_id.resize(index_capacity, invalid_id);
    m_free_ids.resize(handle_capacity);
}

Handle HandleRegister::insert(uint32_t index)
{
    ensure_size(m_index_to_id, static_cast<std::size_t>(index) + 1u, invalid_id);

    // refuse to create new handles for already existing indices
    if (m_index_to_id[index] != invalid_id)
    {
        return { };
    }

    // if at least one valid id is available
    uint32_t id = invalid_id;
    if (!m_free_ids.empty())
    {
        while (id >= m_generations.size() || id >= m_id_to_index.size())
        {
            id = m_free_ids.back();
            m_free_ids.pop_back();
        }
    }

    // otherwise, we need to use a new id
    if (id == invalid_id)
    {
        id = static_cast<uint32_t>(m_generations.size());
        m_id_to_index.push_back(invalid_index);
        m_generations.push_back(0);
    }

    m_index_to_id[index] = id;
    m_id_to_index[id] = index;

    return Handle{ id, m_generations[id] };
}

bool HandleRegister::update(Handle handle, uint32_t index)
{
    if (!is_valid(handle))
    {
        return false;
    }

    const auto id = handle.m_id;
    const auto old_index = m_id_to_index[id];

    ensure_size(m_index_to_id, static_cast<std::size_t>(index) + 1u, invalid_id);

    // refuse to update the handle to an already existing index
    if (m_index_to_id[index] != invalid_id)
    {
        return false;
    }

    // avoid clearing the old reverse mapping if we don't own it
    if (old_index != invalid_index && old_index < m_index_to_id.size() && m_index_to_id[old_index] == id)
    {
        m_index_to_id[old_index] = invalid_id;
    }

    m_id_to_index[id] = index;
    m_index_to_id[index] = id;

    return true;
}

void HandleRegister::erase(Handle handle)
{
    if (!is_valid(handle))
    {
        // do nothing
        return;
    }

    const auto id = handle.m_id;
    const auto index = m_id_to_index[id];

    // avoid clearing reverse mapping if we don't own it
    if (index != invalid_index && index < m_index_to_id.size() && m_index_to_id[index] == id)
    {
        m_index_to_id[index] = invalid_id;
    }

    m_id_to_index[id] = invalid_index;
    m_free_ids.push_back(id);
    m_generations[id]++;
}

bool HandleRegister::is_valid(Handle handle) const noexcept
{
    const auto id = handle.m_id;
    if (id == invalid_id
     || id >= m_id_to_index.size()
     || id >= m_generations.size()
     || m_generations[id] != handle.m_generation)
    {
        return false;
    }

    const auto index = m_id_to_index[id];
    if (index == invalid_index
     || index >= m_index_to_id.size()
     || m_index_to_id[index] != id)
    {
        return false;
    }

    return true;
}

uint32_t HandleRegister::get_index(Handle handle) const noexcept
{
    if (!is_valid(handle))
    {
        return invalid_index;
    }

    return m_id_to_index[handle.m_id];
}
