#pragma once

#include <cmath>
#include <cstddef>
#include <functional>
#include <concepts>
#include <type_traits>
#include <utility>

#include "constants.hpp"

namespace math
{
    template <size_t N, typename T>
    struct VectorStorage
    {
        T data[N];
    };

    template <typename T>
    struct alignas(16) VectorStorage<2, T>
    {
        union { T data[2]; struct { T x, y; }; };
    };

    template <typename T>
    struct alignas(16) VectorStorage<3, T>
    {
        union { T data[3]; struct { T x, y, z; }; struct { T r, g, b; }; };
    };

    template <typename T>
    struct alignas(16) VectorStorage<4, T>
    {
        union { T data[4]; struct { T x, y, z, w; }; struct { T r, g, b, a; }; };
    };

    template <size_t N, typename T>
    struct Vector : public VectorStorage<N, T>
    {
        static constexpr auto size = N;

        constexpr Vector() = default;

        template <typename... Args>
            requires (sizeof...(Args) == N && N > 0 && (std::is_arithmetic_v<Args> && ...))
        explicit constexpr Vector(Args... args)
            : VectorStorage<N, T>{ { static_cast<T>(args)... } }
        { }

        template <typename U>
        explicit constexpr Vector(const Vector<N, U>& other)
            : VectorStorage<N, T>{ { static_cast<T>(other.data[0]) } }
        {
            *this = other.template cast<T>();
        }

        // --- Helpers ---

        template <typename To>
        constexpr Vector<N, To> cast() const
        {
            return cast_impl<To>(std::make_index_sequence<N>{});
        }

        template <typename Op>
            requires std::invocable<Op, T>
        constexpr Vector apply(Op op) const
        {
            return apply_impl(op, std::make_index_sequence<N>{});
        }

        template <typename Op>
            requires std::invocable<Op, T, T>
        constexpr Vector transform(const Vector& other, Op op) const
        {
            return transform_impl(other, op, std::make_index_sequence<N>{});
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        static constexpr Vector fill(U value) {
            return fill_impl(static_cast<T>(value), std::make_index_sequence<N>{});
        }

        template <typename Pred>
            requires std::predicate<Pred, T>
        constexpr bool all(Pred pred) const
        {
            return all_impl(pred, std::make_index_sequence<N>{});
        }

        template <typename Pred>
            requires std::predicate<Pred, T, T>
        constexpr bool all(const Vector& other, Pred pred) const
        {
            return all_impl(other, pred, std::make_index_sequence<N>{});
        }

        template <typename Pred>
            requires std::predicate<Pred, T>
        constexpr bool any(Pred pred) const
        {
            return any_impl(pred, std::make_index_sequence<N>{});
        }

        template <typename Pred>
            requires std::predicate<Pred, T, T>
        constexpr bool any(const Vector& other, Pred pred) const
        {
            return any_impl(other, pred, std::make_index_sequence<N>{});
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        constexpr Vector<N, T> clamp(U minValue, U maxValue) const
        {
            const auto min = static_cast<T>(minValue);
            const auto max = static_cast<T>(maxValue);
            return apply([min, max](T val) {
                return val < min ? min : (val > max ? max : val);
            });
        }

        // --- Constants ---

        static constexpr Vector zero = fill(static_cast<T>(0));
        static constexpr Vector one  = fill(static_cast<T>(1));

        // --- Basic Arithmetic ---

        constexpr Vector<N, T> operator+(const Vector<N, T> & other) const
        {
            return transform(other, std::plus<T>{});
        }

        constexpr Vector<N, T> operator-(const Vector<N, T> & other) const
        {
            return transform(other, std::minus<T>{});
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        constexpr Vector<N, T> operator*(U scalar) const
        {
            const auto s = static_cast<T>(scalar);
            return apply([s](T val) { return val * s; });
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        constexpr Vector<N, T> operator/(U scalar) const
        {
            const auto inv = static_cast<T>(1.0) / static_cast<T>(scalar);
            return apply([inv](T val) { return val * inv; });
        }

        // --- Vector Arithmetic ---

        constexpr T dot(const Vector& other) const
        {
            return dot_impl(other, std::make_index_sequence<N>{});
        }

        // --- Compound Assignment ---

        Vector<N, T> & operator+=(const Vector<N, T> & other)
        {
            *this = *this + other;
            return *this;
        }

        Vector<N, T> & operator-=(const Vector<N, T> & other)
        {
            *this = *this - other;
            return *this;
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        Vector<N, T> & operator*=(U scalar)
        {
            *this = *this * scalar;
            return *this;
        }

        // --- Unary ---

        constexpr Vector<N, T> operator-() const
        {
            return apply([](T val) { return -val; });
        }

        constexpr T squaredLength() const
        {
            return squaredLength_impl(std::make_index_sequence<N>{});
        }

        constexpr T length() const
        {
            return std::sqrt(squaredLength());
        }

        constexpr Vector<N, T> normalized() const
        {
            return *this / length();
        }

        constexpr Vector<N, T> & normalize()
        {
            const auto len = length();
            if (len > epsilon_v<T>)
            {
                const auto invLen = static_cast<T>(1.0) / len;
                *this *= invLen;
            }
            return *this;
        }

        // --- Comparison (with Epsilon) ---

        bool operator==(const Vector<N, T> & other) const
        {
            return all(other, [](T a, T b) { return std::abs(a - b) < epsilon_v<T>; });
        }

        // --- Access ---

        T operator[](int index) const
        {
            return this->data[index];
        }

        T & operator[](int index)
        {
            return this->data[index];
        }

    private:

        template <typename To, std::size_t... I>
        constexpr Vector<N, To> cast_impl(std::index_sequence<I...>) const
        {
            return Vector<N, To>{ static_cast<To>(this->data[I])... };
        }

        template <typename Op, std::size_t... I>
            requires std::convertible_to<std::invoke_result_t<Op, T>, T>
        constexpr Vector apply_impl(Op op, std::index_sequence<I...>) const
        {
            return Vector{ static_cast<T>(op(this->data[I]))... };
        }

        template <typename Op, std::size_t... I>
            requires std::convertible_to<std::invoke_result_t<Op, T, T>, T>
        constexpr Vector transform_impl(const Vector& other, Op op, std::index_sequence<I...>) const
        {
            return Vector{ static_cast<T>(op(this->data[I], other.data[I]))... };
        }

        template <std::size_t... I>
        static constexpr Vector fill_impl(T value, std::index_sequence<I...>) {
            return Vector{ (static_cast<void>(I), value)... };
        }

        template <typename Pred, std::size_t... I>
        constexpr bool all_impl(Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(this->data[I])) && ...);
        }

        template <typename Pred, std::size_t... I>
        constexpr bool all_impl(const Vector& other, Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(this->data[I], other.data[I])) && ...);
        }

        template <typename Pred, std::size_t... I>
        constexpr bool any_impl(Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(this->data[I])) || ...);
        }

        template <typename Pred, std::size_t... I>
        constexpr bool any_impl(const Vector& other, Pred pred, std::index_sequence<I...>) const
        {
            return (static_cast<bool>(pred(this->data[I], other.data[I])) || ...);
        }

        template <std::size_t... I>
        constexpr T dot_impl(const Vector& other, std::index_sequence<I...>) const
        {
            return ((this->data[I] * other.data[I]) + ...);
        }

        template <std::size_t... I>
        constexpr T squaredLength_impl(std::index_sequence<I...>) const
        {
            return ((this->data[I] * this->data[I]) + ...);
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
