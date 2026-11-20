#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "core/handle.hpp"
#include "vectors-view.hpp"

namespace math
{
    template <std::size_t N, typename T>
        requires(N > 0 && std::is_arithmetic_v<T> && std::is_same_v<T, std::remove_cv_t<T>>)
    class VectorSoa
    {
        std::array<std::unique_ptr<T[]>, N> m_data;
        std::size_t m_capacity{0};
        std::size_t m_size{0};
        HandleRegister m_handles;

        static constexpr std::size_t max_capacity = HandleRegister::invalid_index;

    public:
        VectorSoa() = default;

        explicit VectorSoa(std::size_t capacity)
        {
            reserve(capacity);
        }

        VectorSoa(const VectorSoa &) = delete;
        VectorSoa &operator=(const VectorSoa &) = delete;

        VectorSoa(VectorSoa &&other) noexcept
            : m_data(std::move(other.m_data)), m_capacity(std::exchange(other.m_capacity, 0)), m_size(std::exchange(other.m_size, 0)), m_handles(std::move(other.m_handles))
        {
        }

        void reserve(std::size_t capacity)
        {
            if (capacity > m_capacity)
            {
                if (capacity > max_capacity)
                {
                    throw std::length_error("VectorSoa capacity exceeds the handle index range");
                }
                m_handles.reserve(capacity, capacity);
                reallocate(capacity);
            }
        }

        void resize(std::size_t size)
        {
            reserve(size);
            const std::array<T, N> zero{};
            while (m_size < size)
            {
                append(zero);
            }
            while (m_size > size)
            {
                m_handles.erase(handle_at(m_size - 1));
                --m_size;
            }
        }

        void shrink_to_fit()
        {
            if (m_size < m_capacity)
            {
                reallocate(m_size);
            }
        }

        void clear() noexcept
        {
            while (m_size > 0)
            {
                m_handles.erase(handle_at(m_size - 1));
                --m_size;
            }
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

        [[nodiscard]] VectorView<N, T> operator[](std::size_t index) noexcept
        {
            assert(index < m_size);
            return [this, index]<std::size_t... I>(std::index_sequence<I...>)
            {
                return VectorView<N, T>{&m_data[I][index]...};
            }(std::make_index_sequence<N>{});
        }

        [[nodiscard]] ConstVectorView<N, T> operator[](std::size_t index) const noexcept
        {
            assert(index < m_size);
            return [this, index]<std::size_t... I>(std::index_sequence<I...>)
            {
                return ConstVectorView<N, T>{static_cast<const T *>(&m_data[I][index])...};
            }(std::make_index_sequence<N>{});
        }

        [[nodiscard]] Handle handle_at(std::size_t index) const noexcept
        {
            return index < m_size ? m_handles.get_handle(static_cast<std::uint32_t>(index)) : Handle{};
        }

        [[nodiscard]] std::optional<VectorView<N, T>> get(Handle handle) noexcept
        {
            const auto index = m_handles.get_index(handle);
            if (index < m_size)
            {
                return (*this)[index];
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<ConstVectorView<N, T>> get(Handle handle) const noexcept
        {
            const auto index = m_handles.get_index(handle);
            if (index < m_size)
            {
                return (*this)[index];
            }
            return std::nullopt;
        }

        template <typename... Args>
            requires(sizeof...(Args) == N && (std::is_convertible_v<Args, T> && ...))
        Handle emplace(Args &&...args)
        {
            const std::array<T, N> values{static_cast<T>(std::forward<Args>(args))...};
            if (m_size == m_capacity)
            {
                if (m_size == max_capacity)
                {
                    throw std::length_error("VectorSoa has no remaining handle indices");
                }
                reserve(m_capacity == 0 ? 1 : m_capacity + std::min(m_capacity, max_capacity - m_capacity));
            }
            return append(values);
        }

        bool erase(Handle handle) noexcept
        {
            const auto index = m_handles.get_index(handle);
            if (index >= m_size)
            {
                return false;
            }

            const auto last = m_size - 1;
            m_handles.erase(handle);
            if (index != last)
            {
                const auto moved = handle_at(last);
                for (auto &component : m_data)
                {
                    component[index] = component[last];
                }
                [[maybe_unused]] const auto updated = m_handles.update(moved, index);
                assert(updated);
            }
            --m_size;
            return true;
        }

        template <bool IsConst>
        class Iterator
        {
            using Container = std::conditional_t<IsConst, const VectorSoa, VectorSoa>;
            Container *m_container{nullptr};
            std::size_t m_index{0};

            Iterator(Container &container, std::size_t index) noexcept
                : m_container(&container), m_index(index)
            {
            }

            friend class VectorSoa;

        public:
            using iterator_concept = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = std::conditional_t<IsConst, ConstVectorView<N, T>, VectorView<N, T>>;

            Iterator() = default;

            [[nodiscard]] value_type operator*() const noexcept
            {
                return (*m_container)[m_index];
            }

            Iterator &operator++() noexcept
            {
                ++m_index;
                return *this;
            }

            Iterator operator++(int) noexcept
            {
                Iterator previous = *this;
                ++*this;
                return previous;
            }

            bool operator==(const Iterator &) const = default;
        };

        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;

        [[nodiscard]] iterator begin() noexcept
        {
            return {*this, 0};
        }
        [[nodiscard]] iterator end() noexcept
        {
            return {*this, m_size};
        }
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return {*this, 0};
        }
        [[nodiscard]] const_iterator end() const noexcept
        {
            return {*this, m_size};
        }
        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return begin();
        }
        [[nodiscard]] const_iterator cend() const noexcept
        {
            return end();
        }

    private:
        Handle append(const std::array<T, N> &values)
        {
            const auto handle = m_handles.insert(static_cast<std::uint32_t>(m_size));
            assert(handle.is_valid());
            for (std::size_t i = 0; i < N; ++i)
            {
                m_data[i][m_size] = values[i];
            }
            ++m_size;
            return handle;
        }

        void reallocate(std::size_t capacity)
        {
            std::array<std::unique_ptr<T[]>, N> data;
            if (capacity > 0)
            {
                for (std::size_t i = 0; i < N; ++i)
                {
                    data[i] = std::make_unique_for_overwrite<T[]>(capacity);
                    if (m_size > 0)
                    {
                        std::copy_n(m_data[i].get(), m_size, data[i].get());
                    }
                }
            }
            m_data = std::move(data);
            m_capacity = capacity;
        }
    };
}
