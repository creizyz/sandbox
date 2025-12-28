#pragma once

#include <array>
#include "vectors.hpp"

namespace math
{
    template <std::size_t N, typename T>
        requires (N > 0 && std::is_arithmetic_v<T>)
    class VectorView
    {
        std::array<T *, N> m_data;

        template <std::size_t M, typename U>
            requires (M > 0 && std::is_arithmetic_v<U>)
        friend class VectorView;

    public:
        static constexpr auto size = N;

        using value_type = T;
        using self_reference = VectorView<N, T> &;
        using const_self_reference = const VectorView<N, T> &;

        // --- Constructors (no null view accepted) ---

        VectorView() = delete;

        VectorView(const VectorView&) noexcept = default;
        VectorView& operator=(const VectorView&) noexcept = default;

        VectorView(VectorView&&) noexcept = default;
        VectorView& operator=(VectorView&&) noexcept = default;

        ~VectorView() = default;

        explicit constexpr VectorView(const std::array<T*, N>& ptrs) noexcept
            : m_data(ptrs)
        {
            assert_non_null_(m_data);
        }

        template <typename... Ptrs>
            requires (sizeof...(Ptrs) == N && (std::same_as<Ptrs, T*> && ...))
        explicit constexpr VectorView(Ptrs... ptrs) noexcept
            : m_data{ptrs...}
        {
            assert_non_null_pack_(ptrs...);
        }

        explicit constexpr VectorView(const VectorView<N, std::remove_const_t<T>> & other) noexcept
            requires (std::is_const_v<T>)
            : m_data{ }
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                m_data[i] = other.m_data[i];
            }
            assert_non_null_(m_data);
        }

        // --- Helpers ---

        [[nodiscard]] constexpr Vector<N, T> as_vector() const
        {
            Vector<N, T> out{};
            for (std::size_t i = 0; i < N; ++i)
            {
                out[i] = *(m_data[i]);
            }
            return out;
        }

        template <typename U>
            requires std::is_convertible_v<T, U>
        [[nodiscard]] constexpr Vector<N, U> to_vector() const
        {
            Vector<N, U> out{};
            for (std::size_t i = 0; i < N; ++i)
            {
                out[i] = static_cast<U>(*(m_data[i]));
            }
            return out;
        }

        template <typename Op>
            requires (!std::is_const_v<T> && std::invocable<Op&, T> && std::is_convertible_v<std::invoke_result_t<Op&, T>, T>)
        constexpr self_reference apply(Op && op)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                *(m_data[i]) = std::invoke(op, *(m_data[i]));
            }
            return *this;
        }

        template <typename Op>
            requires (!std::is_const_v<T> && std::invocable<Op&, T, T> && std::is_convertible_v<std::invoke_result_t<Op&, T, T>, T>)
        constexpr self_reference apply(const VectorView<N, T> & other, Op && op)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                *(m_data[i]) = std::invoke(op, *(m_data[i]), *(other.m_data[i]));
            }
            return *this;
        }

        template <typename U>
            requires (!std::is_const_v<T> && std::is_convertible_v<U, T>)
        constexpr self_reference fill(U value)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                *(m_data[i]) = static_cast<T>(value);
            }
            return *this;
        }

        template <typename U>
            requires (!std::is_const_v<T> && std::is_convertible_v<U, T>)
        constexpr self_reference clamp(U minValue, U maxValue)
        {
            const auto min = static_cast<T>(minValue);
            const auto max = static_cast<T>(maxValue);
            for (std::size_t i = 0; i < N; ++i)
            {
                auto & val = *(m_data[i]);
                val = val < min ? min : (val > max ? max : val);
            }
            return *this;
        }

        template <typename Pred>
            requires std::predicate<Pred&, T>
        [[nodiscard]] constexpr bool all(Pred && pred) const
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                if (!static_cast<bool>(std::invoke(pred, *(m_data[i]))))
                {
                    return false;
                }
            }
            return true;
        }

        template <typename Pred>
            requires std::predicate<Pred&, T, T>
        [[nodiscard]] constexpr bool all(const Vector<N, T> & other, Pred && pred) const
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                if (!static_cast<bool>(std::invoke(pred, *(m_data[i]), other[i])))
                {
                    return false;
                }
            }
            return true;
        }

        template <typename Pred>
            requires std::predicate<Pred&, T>
        [[nodiscard]] constexpr bool any(Pred && pred) const
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                if (static_cast<bool>(std::invoke(pred, *(m_data[i]))))
                {
                    return true;
                }
            }
            return false;
        }

        template <typename Pred>
            requires std::predicate<Pred&, T, T>
        [[nodiscard]] constexpr bool any(const Vector<N, T> & other, Pred && pred) const
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                if (static_cast<bool>(std::invoke(pred, *(m_data[i]), other[i])))
                {
                    return true;
                }
            }
            return false;
        }

        // --- Named access ---

        [[nodiscard]] constexpr T& x() requires (N >= 1 && N <= 4) { return *(m_data[0]); }
        [[nodiscard]] constexpr T& y() requires (N >= 2 && N <= 4) { return *(m_data[1]); }
        [[nodiscard]] constexpr T& z() requires (N >= 3 && N <= 4) { return *(m_data[2]); }
        [[nodiscard]] constexpr T& w() requires (N == 4)           { return *(m_data[3]); }

        [[nodiscard]] constexpr const T& x() const requires (N >= 1 && N <= 4) { return *(m_data[0]); }
        [[nodiscard]] constexpr const T& y() const requires (N >= 2 && N <= 4) { return *(m_data[1]); }
        [[nodiscard]] constexpr const T& z() const requires (N >= 3 && N <= 4) { return *(m_data[2]); }
        [[nodiscard]] constexpr const T& w() const requires (N == 4)           { return *(m_data[3]); }

        // --- Basic Arithmetic ---

        constexpr self_reference add(const VectorView<N, T> & other)
            requires (!std::is_const_v<T>)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                *(m_data[i]) += other[i];
            }
            return *this;
        }

        constexpr self_reference subtract(const VectorView<N, T> & other)
            requires (!std::is_const_v<T>)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                *(m_data[i]) -= other[i];
            }
            return *this;
        }

        template <typename U>
            requires (!std::is_const_v<T> && std::is_convertible_v<U, T>)
        constexpr self_reference multiply(U scalar)
        {
            const auto s = static_cast<T>(scalar);
            for (std::size_t i = 0; i < N; ++i)
            {
                *(m_data[i]) *= s;
            }
            return *this;
        }

        template <typename U>
            requires (!std::is_const_v<T> && std::is_convertible_v<U, T>)
        constexpr self_reference divide(U scalar)
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                const auto inv = static_cast<T>(1.0) / static_cast<T>(scalar);
                for (std::size_t i = 0; i < N; ++i)
                {
                    *(m_data[i]) *= inv;
                }
            }
            else
            {
                for (std::size_t i = 0; i < N; ++i)
                {
                    *(m_data[i]) /= static_cast<T>(scalar);
                }
            }
            return *this;
        }

        // --- Vector Arithmetic ---

        [[nodiscard]] constexpr T dot(const VectorView<N, T> & other) const
        {
            auto out = static_cast<T>(0);
            for (std::size_t i = 0; i < N; ++i)
            {
                out += *(m_data[i]) * other[i];
            }
            return out;
        }

        // --- Unary ---

        constexpr self_reference invert()
            requires (!std::is_const_v<T>)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                *(m_data[i]) = -(*(m_data[i]));
            }
            return *this;
        }

        [[nodiscard]] constexpr T squared_length() const
        {
            return dot(*this);
        }

        [[nodiscard]] constexpr T length() const
            requires std::floating_point<T>
        {
            return std::sqrt(squared_length());
        }

        constexpr self_reference normalize()
            requires (!std::is_const_v<T> && std::floating_point<T>)
        {
            const auto len = length();
            if (len > epsilon_v<T>)
            {
                const auto invLen = static_cast<T>(1.0) / len;
                for (std::size_t i = 0; i < N; ++i)
                {
                    *(m_data[i]) *= invLen;
                }
            }
            return *this;
        }

        // --- Comparison ---

        template <typename U>
            requires (std::is_convertible_v<U, T>)
        [[nodiscard]] constexpr bool equals(const VectorView<N, U>& other) const noexcept
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                if (!((*this)[i] == static_cast<T>(other[i])))
                {
                    return false;
                }
            }
            return true;
        }

        template <typename U>
            requires (std::is_convertible_v<U, T> && std::floating_point<std::remove_const_t<T>>)
        [[nodiscard]] constexpr bool near_equals(const VectorView<N, U>& other, std::remove_const_t<T> eps = epsilon_v<std::remove_const_t<T>>) const noexcept
        {
            using base_t = std::remove_const_t<T>;
            for (std::size_t i = 0; i < N; ++i)
            {
                const auto a = static_cast<base_t>((*this)[i]);
                const auto b = static_cast<base_t>(static_cast<T>(other[i]));
                if (std::abs(a - b) > eps)
                {
                    return false;
                }
            }
            return true;
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] bool operator==(const VectorView<N, U> & other) const
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                return this->near_equals(other);
            }
            else
            {
                return this->equals(other);
            }
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] bool operator!=(const VectorView<N, U> & other) const
        {
            return !(*this == other);
        }

        // --- Access ---

        [[nodiscard]] constexpr const T & operator[](std::size_t index) const noexcept
        {
            return *(m_data[index]);
        }

        [[nodiscard]] constexpr T & operator[](std::size_t index) noexcept
        {
            return *(m_data[index]);
        }

        [[nodiscard]] constexpr const T & at(std::size_t index) const
        {
            assert(index < N);
            return *(m_data[index]);
        }

        [[nodiscard]] constexpr T & at(std::size_t index)
        {
            assert(index < N);
            return *(m_data[index]);
        }

        template <size_t Index>
            requires (Index < N)
        [[nodiscard]] constexpr const T & get() const noexcept
        {
            return *(m_data[Index]);
        }

        template <size_t Index>
            requires (Index < N)
        [[nodiscard]] constexpr T & get() noexcept
        {
            return *(m_data[Index]);
        }

    private:

        static constexpr void assert_non_null_(const std::array<T*, N> & data) noexcept
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                assert(data[i] != nullptr && "VectorView: null pointer is not allowed");
            }
        }

        static constexpr void assert_ptr_non_null_(const T* p) noexcept
        {
            assert(p != nullptr && "VectorView: null pointer is not allowed");
        }

        template <typename... Args>
        static constexpr void assert_non_null_pack_(Args... args) noexcept
        {
            static_assert(sizeof...(Args) == N);
            (assert_ptr_non_null_(args), ...);
        }
    };

    template <typename T>
    using VectorView2 = VectorView<2, T>;

    template <typename T>
    using VectorView3 = VectorView<3, T>;

    template <typename T>
    using VectorView4 = VectorView<4, T>;

    template <size_t N, typename T>
    using ConstVectorView = VectorView<N, const T>;
}