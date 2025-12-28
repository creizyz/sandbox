#pragma once

#include <array>

#include "core/handle.hpp"
#include "vectors-view.hpp"

namespace math
{
    template <size_t N, typename T>
    class VectorSOA
    {
    public:
        // template <bool IsConst>
        // struct IteratorImpl
        // {
        //     using iterator_category = std::random_access_iterator_tag;
        //     using value_type        = VectorView<N, T>;
        //     using difference_type   = std::ptrdiff_t;
        //     using pointer           = void;
        //     using reference         = std::conditional_t<IsConst, ConstVectorView<N, T>, VectorView<N, T>>;
        //     using ContainerPtr      = std::conditional_t<IsConst, const VectorSOA*, VectorSOA*>;
        //
        //     size_t index;
        //     ContainerPtr container;
        //
        //     reference operator*() const { return container->operator[](index); }
        //
        //     IteratorImpl& operator++() { ++index; return *this; }
        //     IteratorImpl operator++(int) { IteratorImpl tmp = *this; ++index; return tmp; }
        //     IteratorImpl& operator--() { --index; return *this; }
        //
        //     bool operator==(const IteratorImpl& other) const { return index == other.index; }
        //     bool operator!=(const IteratorImpl& other) const { return index != other.index; }
        //
        //     IteratorImpl operator+(difference_type n) const { return {container, index + n}; }
        //     difference_type operator-(const IteratorImpl& other) const { return index - other.index; }
        // };
        //
        // using iterator = IteratorImpl<false>;
        // using const_iterator = IteratorImpl<true>;
        //
        // iterator begin() { return {0, this}; }
        // iterator end() { return {m_size, this}; }
        //
        // const_iterator begin() const { return {0, this}; }
        // const_iterator end() const { return {m_size, this}; }
        // const_iterator cbegin() const { return {0, this}; }
        // const_iterator cend() const { return {m_size, this}; }

    private:
        std::array<T*, N> m_data{ nullptr };
        size_t            m_capacity{ 0 };
        size_t            m_size{ 0 };
        HandleRegister    m_handles;

    public:
        VectorSOA() = default;

        explicit VectorSOA(size_t capacity)
            : m_capacity(capacity)
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_data[i] = new T[m_capacity];
            }
            m_handles.reserve(capacity, capacity);
        }

        ~VectorSOA()
        {
            for (size_t i = 0; i < N; ++i)
            {
                delete[] m_data[i];
            }
        }

        VectorSOA(const VectorSOA &) = delete;
        VectorSOA& operator=(const VectorSOA &) = delete;

        VectorSOA(VectorSOA && other) noexcept
            : m_data(std::move(other.m_data))
            , m_capacity(other.m_capacity)
            , m_size(other.m_size)
            , m_handles(std::move(other.m_handles))
        {
            other.m_data.fill(nullptr);
            other.m_capacity = 0;
            other.m_size = 0;
        }

        void reserve(size_t capacity)
        {
            if (capacity <= m_capacity)
            {
                return;
            }

            reallocate(capacity);
            m_handles.reserve(capacity, capacity);
        }

        void resize(size_t size)
        {
            if (size > m_capacity)
            {
                reallocate(size);
            }

            m_handles.resize(size, size);
            m_size = size;
        }

        void shrink_to_fit()
        {
            if (m_size < m_capacity)
            {
                reallocate(m_size);
                m_handles.resize(m_size, m_size);
            }
        }

        void clear() noexcept
        {
            m_size = 0;
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return m_size;
        }

        [[nodiscard]] std::size_t capacity() const noexcept
        {
            return m_capacity;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return m_size == 0;
        }

        VectorView<N, T> operator[](const size_t i)
        {
            return get_view_internal(i, std::make_index_sequence<N>{});
        }

        ConstVectorView<N, T> operator[](const size_t i) const
        {
            return get_view_internal(i, std::make_index_sequence<N>{});
        }

        template <class... Args>
            requires (sizeof...(Args) == N && (std::is_convertible_v<Args, T> && ...))
        Handle emplace(Args&& ... xs)
        {
            if (m_size == m_capacity)
            {
                reserve(m_capacity > 0 ? m_capacity * 2 : 1);
            }

            const auto index = static_cast<uint32_t>(m_size);
            const auto handle = m_handles.insert(index);

            // TODO

            ++m_size;
            return handle;
        }

        bool erase(Handle handle) noexcept
        {
            if (!m_handles.is_valid(h))
            {
                return false;
            }

            const uint32_t index_u32 = m_handles.get_index(h);
            const size_t index = static_cast<size_t>(index_u32);
            const size_t last  = m_size - 1;

            if (index != last)
            {
                // Move last element into the hole.
                for (size_t c = 0; c < N; ++c)
                {
                    m_data[c][index] = m_data[c][last];
                }

                // Retarget moved element's handle from `last` to `index`.
                Handle moved = m_handles.get_handle(static_cast<uint32_t>(last));
                m_handles.update(moved, index_u32);
            }

            m_handles.erase(h);
            --m_size;
            return true;
        }

    private:
        void reallocate(size_t capacity)
        {
            for (size_t i = 0; i < N; ++i)
            {
                T* new_buffer = (capacity > 0) ? new T[capacity] : nullptr;
                if (m_size > 0 && m_data[i])
                {
                    std::copy_n(m_data[i], std::min(m_size, capacity), new_buffer);
                    delete[] m_data[i];
                }
                m_data[i] = new_buffer;
            }
            m_capacity = capacity;
            if (m_size > m_capacity) m_size = m_capacity;
        }

        template <size_t... Is>
        VectorView<N, T> get_view_internal(size_t i, std::index_sequence<Is...>)
        {
            return VectorView<N, T>(m_data[Is][i]...);
        }
    };
}
