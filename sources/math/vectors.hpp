#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <cmath>
#include <type_traits>
#include <utility>
#include <functional>

#include "constants.hpp"

namespace math
{
    template <std::size_t N, typename T>
    inline constexpr std::size_t vector_alignment_v = (sizeof(T) * N >= 16 ? 16 : alignof(std::array<T, N>));

    template <std::size_t N, typename T>
        requires (N > 0 && std::is_arithmetic_v<T>)
    class alignas(vector_alignment_v<N, T>) Vector
    {
        std::array<T, N> m_data;

        template <std::size_t I, typename U>
            requires (I > 0 && std::is_arithmetic_v<U>)
        friend class Vector;

        static constexpr std::size_t K_UNROLL_THRESHOLD = 4;

    public:
        constexpr Vector() = default;

        template <typename... Args>
            requires (sizeof...(Args) == N && (std::is_arithmetic_v<Args> && ...))
        explicit constexpr Vector(Args... args)
            : m_data{ { static_cast<T>(args)... } }
        { }

        template <typename U>
            requires std::is_convertible_v<U, T>
        explicit constexpr Vector(const Vector<N, U>& other)
            : m_data{ convert_from_impl(other, std::make_index_sequence<N>{}) }
        { }

        template <typename U>
            requires std::is_convertible_v<T, U>
        [[nodiscard]] explicit constexpr operator Vector<N, U>() const
        {
            return cast_impl<U>(std::make_index_sequence<N>{});
        }

        // --- Helpers ---

        template <typename U>
            requires std::is_convertible_v<T, U>
        [[nodiscard]] constexpr Vector<N, U> cast() const
        {
            if constexpr (N <= K_UNROLL_THRESHOLD)
            {
                return cast_impl<U>(std::make_index_sequence<N>{});
            }
            else
            {
                Vector<N, U> out{};
                for (std::size_t i = 0; i < N; ++i)
                {
                    out.m_data[i] = static_cast<U>(m_data[i]);
                }
                return out;
            }
        }

        template <typename Op>
            requires (std::invocable<Op&, T> && std::is_convertible_v<std::invoke_result_t<Op&, T>, T>)
        [[nodiscard]] constexpr Vector<N, T> transform(Op && op) const noexcept(noexcept(std::invoke(op, T{})))
        {
            if constexpr (N <= K_UNROLL_THRESHOLD)
            {
                return transform_impl(std::forward<Op>(op), std::make_index_sequence<N>{});
            }
            else
            {
                Vector<N, T> out{};
                auto fn = std::forward<Op>(op);
                for (std::size_t i = 0; i < N; ++i)
                {
                    out.m_data[i] = static_cast<T>(std::invoke(fn, m_data[i]));
                }
                return out;
            }
        }

        template <typename Op>
            requires (std::invocable<Op&, T, T> && std::is_convertible_v<std::invoke_result_t<Op&, T, T>, T>)
        [[nodiscard]] constexpr Vector<N, T> transform(const Vector<N, T> & other, Op && op) const noexcept(noexcept(std::invoke(op, T{}, T{})))
        {
            if constexpr (N <= K_UNROLL_THRESHOLD)
            {
                return transform_impl(other, std::forward<Op>(op), std::make_index_sequence<N>{});
            }
            else
            {
                Vector<N, T> out{};
                auto fn = std::forward<Op>(op);
                for (std::size_t i = 0; i < N; ++i)
                {
                    out.m_data[i] = static_cast<T>(std::invoke(fn, m_data[i], other.m_data[i]));
                }
                return out;
            }
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] static constexpr Vector<N, T> fill(U value)
        {
            if constexpr (N <= K_UNROLL_THRESHOLD)
            {
                return fill_impl(static_cast<T>(value), std::make_index_sequence<N>{});
            }
            else
            {
                Vector<N, T> out{};
                for (std::size_t i = 0; i < N; ++i)
                {
                    out.m_data[i] = static_cast<T>(value);
                }
                return out;
            }
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] constexpr Vector<N, T> clamp(U minValue, U maxValue) const
        {
            const auto min = static_cast<T>(minValue);
            const auto max = static_cast<T>(maxValue);
            return transform([min, max](T val) {
                return val < min ? min : (val > max ? max : val);
            });
        }

        template <typename Pred>
            requires std::predicate<Pred, T>
        [[nodiscard]] constexpr bool all(Pred pred) const noexcept(noexcept(std::invoke(pred, T{})))
        {
            if constexpr (N <= K_UNROLL_THRESHOLD)
            {
                return all_impl(pred, std::make_index_sequence<N>{});
            }
            else
            {
                auto out{ true };
                for (std::size_t i = 0; i < N; ++i)
                {
                    out &= pred(m_data[i]);
                }
                return out;
            }
        }

        template <typename Pred>
            requires std::predicate<Pred, T, T>
        [[nodiscard]] constexpr bool all(const Vector<N, T> & other, Pred pred) const noexcept(noexcept(std::invoke(pred, T{}, T{})))
        {
            if constexpr (N <= K_UNROLL_THRESHOLD)
            {
                return all_impl(other, pred, std::make_index_sequence<N>{});
            }
            else
            {
                auto out{ true };
                for (std::size_t i = 0; i < N; ++i)
                {
                    out &= pred(m_data[i], other.m_data[i]);
                }
                return out;
            }
        }

        template <typename Pred>
            requires std::predicate<Pred, T>
        [[nodiscard]] constexpr bool any(Pred pred) const noexcept(noexcept(std::invoke(pred, T{})))
        {
            if constexpr (N <= K_UNROLL_THRESHOLD)
            {
                return any_impl(pred, std::make_index_sequence<N>{});
            }
            else
            {
                auto out{ false };
                for (std::size_t i = 0; i < N; ++i)
                {
                    out |= pred(m_data[i]);
                }
                return out;
            }
        }

        template <typename Pred>
            requires std::predicate<Pred, T, T>
        [[nodiscard]] constexpr bool any(const Vector<N, T> & other, Pred pred) const noexcept(noexcept(std::invoke(pred, T{}, T{})))
        {
            if constexpr (N <= K_UNROLL_THRESHOLD)
            {
                return any_impl(other, pred, std::make_index_sequence<N>{});
            }
            else
            {
                auto out{ false };
                for (std::size_t i = 0; i < N; ++i)
                {
                    out |= pred(m_data[i], other.m_data[i]);
                }
                return out;
            }
        }

        // --- Constants ---

        static constexpr Vector<N, T> zero = fill(static_cast<T>(0));
        static constexpr Vector<N, T> one  = fill(static_cast<T>(1));

        static constexpr std::size_t size() noexcept
        {
            return N;
        }

        // --- Named access ---

        [[nodiscard]] constexpr T& x() noexcept requires (N >= 1 && N <= 4) { return m_data[0]; }
        [[nodiscard]] constexpr T& y() noexcept requires (N >= 2 && N <= 4) { return m_data[1]; }
        [[nodiscard]] constexpr T& z() noexcept requires (N >= 3 && N <= 4) { return m_data[2]; }
        [[nodiscard]] constexpr T& w() noexcept requires (N == 4)           { return m_data[3]; }

        [[nodiscard]] constexpr const T& x() const noexcept requires (N >= 1 && N <= 4) { return m_data[0]; }
        [[nodiscard]] constexpr const T& y() const noexcept requires (N >= 2 && N <= 4) { return m_data[1]; }
        [[nodiscard]] constexpr const T& z() const noexcept requires (N >= 3 && N <= 4) { return m_data[2]; }
        [[nodiscard]] constexpr const T& w() const noexcept requires (N == 4)           { return m_data[3]; }

        // --- Basic Arithmetic ---

        [[nodiscard]] constexpr Vector<N, T> operator+(const Vector<N, T> & other) const noexcept
        {
            return transform(other, std::plus<T>{});
        }

        [[nodiscard]] constexpr Vector<N, T> operator-(const Vector<N, T> & other) const noexcept
        {
            return transform(other, std::minus<T>{});
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] constexpr Vector<N, T> operator*(U scalar) const noexcept
        {
            const auto s = static_cast<T>(scalar);
            return transform([s](T val) { return val * s; });
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] constexpr Vector<N, T> operator/(U scalar) const noexcept
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                const auto inv = static_cast<T>(1.0) / static_cast<T>(scalar);
                return transform([inv](T val) { return val * inv; });
            }
            else
            {
                const auto s = static_cast<T>(scalar);
                return transform([s](T val) { return val / s; });
            }
        }

        // --- Vector Arithmetic ---

        [[nodiscard]] constexpr T dot(const Vector<N, T> & other) const noexcept
        {
            if constexpr (N <= K_UNROLL_THRESHOLD)
            {
                return dot_impl(other, std::make_index_sequence<N>{});
            }
            else
            {
                T out{ 0 };
                for (std::size_t i = 0; i < N; ++i)
                {
                    out += m_data[i] * other.m_data[i];
                }
                return out;
            }
        }

        // --- Compound Assignment ---

        Vector<N, T> & operator+=(const Vector<N, T> & other) noexcept
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                m_data[i] += other.m_data[i];
            }
            return *this;
        }

        Vector<N, T> & operator-=(const Vector<N, T> & other) noexcept
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                m_data[i] -= other.m_data[i];
            }
            return *this;
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        Vector<N, T> & operator*=(U scalar) noexcept
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                m_data[i] *= static_cast<T>(scalar);
            }
            return *this;
        }

        // --- Unary ---

        [[nodiscard]] constexpr Vector<N, T> operator-() const noexcept
        {
            return transform([](T val) { return -val; });
        }

        [[nodiscard]] constexpr T squared_length() const noexcept
        {
            return dot(*this);
        }

        [[nodiscard]] constexpr T length() const
            requires std::floating_point<T>
        {
            return std::sqrt(squared_length());
        }

        [[nodiscard]] constexpr Vector<N, T> normalized() const
            requires std::floating_point<T>
        {
            const auto len = length();
            if (len > epsilon_v<T>)
            {
                const auto invLen = static_cast<T>(1.0) / len;
                return *this * invLen;
            }

            return zero;
        }

        constexpr Vector<N, T> & normalize()
            requires std::floating_point<T>
        {
            const auto len = length();
            if (len > epsilon_v<T>)
            {
                const auto invLen = static_cast<T>(1.0) / len;
                *this *= invLen;
            }
            return *this;
        }

        // --- Comparison ---

        [[nodiscard]] constexpr bool operator==(const Vector<N, T> & other) const
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                return all(other, [](const T & a, const T & b) { return std::abs(a - static_cast<T>(b)) < epsilon_v<T>; });
            }
            else
            {
                return all(other, [](const T & a, const T & b) { return a == static_cast<T>(b); });
            }
        }

        [[nodiscard]] constexpr bool operator!=(const Vector<N, T> & other) const
        {
            return !(*this == other);
        }

        // --- Access ---

        [[nodiscard]] constexpr const T & operator[](std::size_t index) const noexcept
        {
            return m_data[index];
        }

        [[nodiscard]] constexpr T & operator[](std::size_t index) noexcept
        {
            return m_data[index];
        }

        [[nodiscard]] constexpr const T & at(std::size_t index) const
        {
            assert(index < N);
            return m_data[index];
        }

        [[nodiscard]] constexpr T & at(std::size_t index)
        {
            assert(index < N);
            return m_data[index];
        }

        template <std::size_t Index>
            requires (Index < N)
        [[nodiscard]] constexpr const T & get() const noexcept
        {
            return m_data[Index];
        }

        template <std::size_t Index>
            requires (Index < N)
        [[nodiscard]] constexpr T & get() noexcept
        {
            return m_data[Index];
        }

        [[nodiscard]] constexpr const T* data() const noexcept
        {
            return m_data.data();
        }

        [[nodiscard]] constexpr T* data() noexcept
        {
            return m_data.data();
        }

    private:

        template <typename U, std::size_t... I>
        static constexpr std::array<T, N> convert_from_impl(const Vector<N, U>& other, std::index_sequence<I...>)
        {
            return { { static_cast<T>(other.m_data[I])... } };
        }

        template <typename U, std::size_t... I>
        constexpr Vector<N, U> cast_impl(std::index_sequence<I...>) const
        {
            return Vector<N, U>{ static_cast<U>(m_data[I])... };
        }

        template <typename Op, std::size_t... I>
            requires std::convertible_to<std::invoke_result_t<Op&, T>, T>
        constexpr Vector<N, T> transform_impl(Op && op, std::index_sequence<I...>) const
        {
            auto fn = std::forward<Op>(op);
            return Vector{ static_cast<T>(std::invoke(fn, m_data[I]))... };
        }

        template <typename Op, std::size_t... I>
            requires std::convertible_to<std::invoke_result_t<Op&, T, T>, T>
        constexpr Vector<N, T> transform_impl(const Vector<N, T> & other, Op && op, std::index_sequence<I...>) const
        {
            auto fn = std::forward<Op>(op);
            return Vector{ static_cast<T>(std::invoke(fn, m_data[I], other.m_data[I]))... };
        }

        template <std::size_t... I>
        static constexpr Vector<N, T> fill_impl(T value, std::index_sequence<I...>)
        {
            return Vector{ (static_cast<void>(I), value)... };
        }

        template <typename Pred, std::size_t... I>
        constexpr bool all_impl(Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(m_data[I])) && ...);
        }

        template <typename Pred, std::size_t... I>
        constexpr bool all_impl(const Vector<N, T> & other, Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(m_data[I], other.m_data[I])) && ...);
        }

        template <typename Pred, std::size_t... I>
        constexpr bool any_impl(Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(m_data[I])) || ...);
        }

        template <typename Pred, std::size_t... I>
        constexpr bool any_impl(const Vector<N, T> & other, Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(m_data[I], other.m_data[I])) || ...);
        }

        template <std::size_t... I>
        constexpr T dot_impl(const Vector<N, T> & other, std::index_sequence<I...>) const
        {
            return ((m_data[I] * other.m_data[I]) + ...);
        }
    };

    template <typename T>
    using Vector2 = Vector<2, T>;

    using Vector2f = Vector2<float>;
    using Vector2d = Vector2<double>;

    template <typename T>
    using Vector3 = Vector<3, T>;

    using Vector3f = Vector3<float>;
    using Vector3d = Vector3<double>;

    template <typename T>
    using Vector4 = Vector<4, T>;

    using Vector4f = Vector4<float>;
    using Vector4d = Vector4<double>;
}

template <typename U, size_t N, typename T>
    requires std::is_convertible_v<U, T>
constexpr math::Vector<N, T> operator*(U scalar, const math::Vector<N, T> & vec)
{
    return vec * scalar;
}
