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

void HandleRegister::reserve(std::size_t handleCapacity, std::size_t indexCapacity)
{
    if (handleCapacity > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        throw std::invalid_argument("handleCapacity exceeds uint32_t range");
    }

    if (indexCapacity > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        throw std::invalid_argument("indexCapacity exceeds uint32_t range");
    }

    m_idToIndex.reserve(handleCapacity);
    m_generations.reserve(handleCapacity);
    m_freeIds.reserve(handleCapacity);
    m_indexToId.reserve(indexCapacity);
}

void HandleRegister::resize(std::size_t handleCapacity, std::size_t indexCapacity)
{
    if (handleCapacity > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        throw std::invalid_argument("handleCapacity exceeds uint32_t range");
    }

    if (indexCapacity > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        throw std::invalid_argument("indexCapacity exceeds uint32_t range");
    }

    if (handleCapacity < m_freeIds.size())
    {
        std::erase_if(m_freeIds, [=](std::uint32_t id) {
            return id >= handleCapacity;
        });
    }

    m_idToIndex.resize(handleCapacity, invalid_index);
    m_generations.resize(handleCapacity, 0);
    m_indexToId.resize(indexCapacity, invalid_id);
    m_freeIds.resize(handleCapacity);
}

Handle HandleRegister::insert(uint32_t index)
{
    ensure_size(m_indexToId, static_cast<std::size_t>(index) + 1u, invalid_id);

    // refuse to create new handles for already existing indices
    if (m_indexToId[index] != invalid_id)
    {
        return { };
    }

    // if at least one valid id is available
    uint32_t id = invalid_id;
    if (!m_freeIds.empty())
    {
        while (id >= m_generations.size() || id >= m_idToIndex.size())
        {
            id = m_freeIds.back();
            m_freeIds.pop_back();
        }
    }

    // otherwise, we need to use a new id
    if (id == invalid_id)
    {
        id = static_cast<uint32_t>(m_generations.size());
        m_idToIndex.push_back(invalid_index);
        m_generations.push_back(0);
    }

    m_indexToId[index] = id;
    m_idToIndex[id] = index;

    return Handle{ id, m_generations[id] };
}

bool HandleRegister::update(Handle handle, uint32_t index)
{
    if (!is_valid(handle))
    {
        return false;
    }

    const auto id = handle.id;
    const auto oldIndex = m_idToIndex[id];

    ensure_size(m_indexToId, static_cast<std::size_t>(index) + 1u, invalid_id);

    // refuse to update the handle to an already existing index
    if (m_indexToId[index] != invalid_id)
    {
        return false;
    }

    // avoid clearing the old reverse mapping if we don't own it
    if (oldIndex != invalid_index && oldIndex < m_indexToId.size() && m_indexToId[oldIndex] == id)
    {
        m_indexToId[oldIndex] = invalid_id;
    }

    m_idToIndex[id] = index;
    m_indexToId[index] = id;

    return true;
}

void HandleRegister::erase(Handle handle)
{
    if (!is_valid(handle))
    {
        // do nothing
        return;
    }

    const auto id = handle.id;
    const auto index = m_idToIndex[id];

    // avoid clearing reverse mapping if we don't own it
    if (index != invalid_index && index < m_indexToId.size() && m_indexToId[index] == id)
    {
        m_indexToId[index] = invalid_id;
    }

    m_idToIndex[id] = invalid_index;
    m_freeIds.push_back(id);
    m_generations[id]++;
}

bool HandleRegister::is_valid(Handle handle) const noexcept
{
    const auto id = handle.id;
    if (id == invalid_id
     || id >= m_idToIndex.size()
     || id >= m_generations.size()
     || m_generations[id] != handle.generation)
    {
        return false;
    }

    const auto index = m_idToIndex[id];
    if (index == invalid_index
     || index >= m_indexToId.size()
     || m_indexToId[index] != id)
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

    return m_idToIndex[handle.id];
}
