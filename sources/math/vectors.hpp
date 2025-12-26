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
        requires (N > 0 && std::is_arithmetic_v<T>)
    class alignas(16) Vector
    {
        std::array<T, N> m_data;

        template <std::size_t I, typename U>
            requires (I > 0 && std::is_arithmetic_v<U>)
        friend class Vector;

    public:
        static constexpr auto size = N;

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
            return cast_impl<U>(std::make_index_sequence<N>{});
        }

        template <typename Op>
            requires std::invocable<Op, T>
        [[nodiscard]] constexpr Vector apply(Op op) const
        {
            return apply_impl(op, std::make_index_sequence<N>{});
        }

        template <typename Op>
            requires std::invocable<Op, T, T>
        [[nodiscard]] constexpr Vector transform(const Vector& other, Op op) const
        {
            return transform_impl(other, op, std::make_index_sequence<N>{});
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] static constexpr Vector fill(U value) {
            return fill_impl(static_cast<T>(value), std::make_index_sequence<N>{});
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] constexpr Vector<N, T> clamp(U minValue, U maxValue) const
        {
            const auto min = static_cast<T>(minValue);
            const auto max = static_cast<T>(maxValue);
            return apply([min, max](T val) {
                return val < min ? min : (val > max ? max : val);
            });
        }

        template <typename Pred>
            requires std::predicate<Pred, T>
        [[nodiscard]] constexpr bool all(Pred pred) const
        {
            return all_impl(pred, std::make_index_sequence<N>{});
        }

        template <typename Pred>
            requires std::predicate<Pred, T, T>
        [[nodiscard]] constexpr bool all(const Vector& other, Pred pred) const
        {
            return all_impl(other, pred, std::make_index_sequence<N>{});
        }

        template <typename Pred>
            requires std::predicate<Pred, T>
        [[nodiscard]] constexpr bool any(Pred pred) const
        {
            return any_impl(pred, std::make_index_sequence<N>{});
        }

        template <typename Pred>
            requires std::predicate<Pred, T, T>
        [[nodiscard]] constexpr bool any(const Vector& other, Pred pred) const
        {
            return any_impl(other, pred, std::make_index_sequence<N>{});
        }

        // --- Constants ---

        static constexpr Vector zero = fill(static_cast<T>(0));
        static constexpr Vector one  = fill(static_cast<T>(1));

        // --- Named access ---

        [[nodiscard]] constexpr T& x() requires (N >= 1 && N <= 4) { return this->m_data[0]; }
        [[nodiscard]] constexpr T& y() requires (N >= 2 && N <= 4) { return this->m_data[1]; }
        [[nodiscard]] constexpr T& z() requires (N >= 3 && N <= 4) { return this->m_data[2]; }
        [[nodiscard]] constexpr T& w() requires (N >= 4 && N <= 4) { return this->m_data[3]; }

        [[nodiscard]] constexpr const T& x() const requires (N >= 1 && N <= 4) { return this->m_data[0]; }
        [[nodiscard]] constexpr const T& y() const requires (N >= 2 && N <= 4) { return this->m_data[1]; }
        [[nodiscard]] constexpr const T& z() const requires (N >= 3 && N <= 4) { return this->m_data[2]; }
        [[nodiscard]] constexpr const T& w() const requires (N >= 4 && N <= 4) { return this->m_data[3]; }

        // --- Basic Arithmetic ---

        [[nodiscard]] constexpr Vector<N, T> operator+(const Vector<N, T> & other) const
        {
            return transform(other, std::plus<T>{});
        }

        [[nodiscard]] constexpr Vector<N, T> operator-(const Vector<N, T> & other) const
        {
            return transform(other, std::minus<T>{});
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] constexpr Vector<N, T> operator*(U scalar) const
        {
            const auto s = static_cast<T>(scalar);
            return apply([s](T val) { return val * s; });
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] constexpr Vector<N, T> operator/(U scalar) const
        {
            const auto inv = static_cast<T>(1.0) / static_cast<T>(scalar);
            return apply([inv](T val) { return val * inv; });
        }

        // --- Vector Arithmetic ---

        [[nodiscard]] constexpr T dot(const Vector& other) const
        {
            return dot_impl(other, std::make_index_sequence<N>{});
        }

        // --- Compound Assignment ---

        Vector<N, T> & operator+=(const Vector<N, T> & other)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                m_data[i] += other.m_data[i];
            }
            return *this;
        }

        Vector<N, T> & operator-=(const Vector<N, T> & other)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                m_data[i] -= other.m_data[i];
            }
            return *this;
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        Vector<N, T> & operator*=(U scalar)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                m_data[i] *= static_cast<T>(scalar);
            }
            return *this;
        }

        // --- Unary ---

        [[nodiscard]] constexpr Vector<N, T> operator-() const
        {
            return apply([](T val) { return -val; });
        }

        [[nodiscard]] constexpr T squaredLength() const
        {
            return squaredLength_impl(std::make_index_sequence<N>{});
        }

        [[nodiscard]] constexpr T length() const
            requires std::floating_point<T>
        {
            return std::sqrt(squaredLength());
        }

        [[nodiscard]] constexpr Vector<N, T> normalized() const
            requires std::floating_point<T>
        {
            return *this / length();
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

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] bool operator==(const Vector<N, U> & other) const
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                return all(other, [](const T & a, const U & b) { return std::abs(a - static_cast<T>(b)) < epsilon_v<T>; });
            }
            else
            {
                return all(other, [](const T & a, const U & b) { return a == static_cast<T>(b); });
            }
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] bool operator!=(const Vector<N, U> & other) const
        {
            return !(*this == other);
        }

        // --- Access ---

        [[nodiscard]] constexpr const T & operator[](std::size_t index) const noexcept
        {
            assert(index < N);
            return this->m_data[index];
        }

        [[nodiscard]] constexpr T & operator[](std::size_t index) noexcept
        {
            assert(index < N);
            return this->m_data[index];
        }

        [[nodiscard]] constexpr const T* data() const noexcept
        {
            return this->m_data.data();
        }

        [[nodiscard]] constexpr T* data() noexcept
        {
            return this->m_data.data();
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
            return Vector<N, U>{ static_cast<U>(this->m_data[I])... };
        }

        template <typename Op, std::size_t... I>
            requires std::convertible_to<std::invoke_result_t<Op, T>, T>
        constexpr Vector apply_impl(Op op, std::index_sequence<I...>) const
        {
            return Vector{ static_cast<T>(op(this->m_data[I]))... };
        }

        template <typename Op, std::size_t... I>
            requires std::convertible_to<std::invoke_result_t<Op, T, T>, T>
        constexpr Vector transform_impl(const Vector& other, Op op, std::index_sequence<I...>) const
        {
            return Vector{ static_cast<T>(op(this->m_data[I], other.m_data[I]))... };
        }

        template <std::size_t... I>
        static constexpr Vector fill_impl(T value, std::index_sequence<I...>) {
            return Vector{ (static_cast<void>(I), value)... };
        }

        template <typename Pred, std::size_t... I>
        constexpr bool all_impl(Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(this->m_data[I])) && ...);
        }

        template <typename Pred, std::size_t... I>
        constexpr bool all_impl(const Vector& other, Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(this->m_data[I], other.m_data[I])) && ...);
        }

        template <typename Pred, std::size_t... I>
        constexpr bool any_impl(Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(this->m_data[I])) || ...);
        }

        template <typename Pred, std::size_t... I>
        constexpr bool any_impl(const Vector& other, Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(this->m_data[I], other.m_data[I])) || ...);
        }

        template <std::size_t... I>
        constexpr T dot_impl(const Vector& other, std::index_sequence<I...>) const
        {
            return ((this->m_data[I] * other.m_data[I]) + ...);
        }

        template <std::size_t... I>
        constexpr T squaredLength_impl(std::index_sequence<I...>) const
        {
            return ((this->m_data[I] * this->m_data[I]) + ...);
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
