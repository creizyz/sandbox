#pragma once

#include <array>
#include <utility>
#include <type_traits>

#include "vectors.hpp"

namespace math
{
    template <std::size_t Rows, std::size_t Cols, typename T>
        requires (Rows > 0 && Cols > 0 && std::is_arithmetic_v<T>)
    class Matrix
    {
        std::array<Vector<Rows, T>, Cols> m_cols;

        static constexpr std::size_t K_UNROLL_THRESHOLD = 4;
        static constexpr auto min_dimension = std::min(Rows, Cols);

    public:
        constexpr Matrix() = default;

        template <typename... Args>
            requires (sizeof...(Args) == Cols && (std::is_same_v<Args, Vector<Rows, T>> && ...))
        explicit constexpr Matrix(Args... args)
            : m_cols{ args... }
        { }

        static constexpr Matrix<Rows, Cols, T> identity()
        {

            Matrix<Rows, Cols, T> result{};
            for (std::size_t i = 0; i < min_dimension; ++i)
            {
                result[i][i] = static_cast<T>(1);
            }
            return result;
        }

        // --- Accessors ---

        [[nodiscard]] constexpr Vector<Rows, T>& operator[](std::size_t col) noexcept
        {
            return m_cols[col];
        }

        [[nodiscard]] constexpr const Vector<Rows, T>& operator[](std::size_t col) const noexcept
        {
            return m_cols[col];
        }

        // --- Basic Arithmetic ---

        [[nodiscard]] constexpr Vector<Rows, T> operator*(const Vector<Cols, T>& vec) const noexcept
        {
            if constexpr (Cols <= K_UNROLL_THRESHOLD)
            {
                return multiply_vec_impl(vec, std::make_index_sequence<Cols>{});
            }
            else
            {
                Vector<Rows, T> result = m_cols[0] * vec[0];
                for (std::size_t i = 1; i < Cols; ++i)
                {
                    result += m_cols[i] * vec[i];
                }
                return result;
            }
        }

        template <std::size_t OtherCols>
        [[nodiscard]] constexpr Matrix<Rows, OtherCols, T> operator*(const Matrix<Cols, OtherCols, T> & other) const noexcept
        {
            Matrix<Rows, OtherCols, T> result;
            for (std::size_t i = 0; i < OtherCols; ++i)
            {
                result[i] = (*this) * other[i];
            }
            return result;
        }

        [[nodiscard]] constexpr Matrix<Rows, Cols, T> operator+(const Matrix<Rows, Cols, T> & other)
        {
            if constexpr (Cols <= K_UNROLL_THRESHOLD)
            {
                return add_matrix_impl(other, std::make_index_sequence<Cols>{});
            }
            else
            {
                Matrix<Rows, Cols, T> result{ *this };
                for (std::size_t i = 1; i < Cols; ++i)
                {
                    result[i] += other[i];
                }
                return result;
            }
        }

        [[nodiscard]] constexpr Matrix<Rows, Cols, T> operator-(const Matrix<Rows, Cols, T> & other)
        {
            if constexpr (Cols <= K_UNROLL_THRESHOLD)
            {
                return subtract_matrix_impl(other, std::make_index_sequence<Cols>{});
            }
            else
            {
                Matrix<Rows, Cols, T> result{ *this };
                for (std::size_t i = 1; i < Cols; ++i)
                {
                    result[i] -= other[i];
                }
                return result;
            }
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] constexpr Matrix<Rows, Cols, T> operator*(U scalar)
        {
            if constexpr (Cols <= K_UNROLL_THRESHOLD)
            {
                return multiply_scalar_impl(static_cast<T>(scalar), std::make_index_sequence<Cols>{});
            }
            else
            {
                Matrix<Rows, Cols, T> result{ *this };
                for (std::size_t i = 1; i < Cols; ++i)
                {
                    result[i] *= static_cast<T>(scalar);
                }
                return result;
            }
        }

        template <typename U>
            requires std::is_convertible_v<U, T>
        [[nodiscard]] constexpr Matrix<Rows, Cols, T> operator/(U scalar)
        {
            const auto inv = static_cast<T>(1.0) / static_cast<T>(scalar);
            if (inv > epsilon_v<T>)
            {
                return (*this) * inv;
            }

            return *this;
        }

        // --- Matrix Operations ---

        [[nodiscard]] constexpr Matrix<Cols, Rows, T> transposed() const noexcept
        {
            Matrix<Cols, Rows, T> out;
            for (std::size_t i = 0; i < Rows; ++i)
            {
                for (std::size_t j = 0; j < Cols; ++j)
                {
                    out[i][j] = m_cols[j][i];
                }
            }
            return out;
        }

    private:
        template <std::size_t... I>
        constexpr Vector<Rows, T> multiply_vec_impl(const Vector<Cols, T>& vec, std::index_sequence<I...>) const noexcept
        {
            return ((m_cols[I] * vec[I]) + ...);
        }

        template <std::size_t... I>
        constexpr Matrix<Rows, Cols, T> add_matrix_impl(const Matrix<Rows, Cols, T> & other, std::index_sequence<I...>) const noexcept
        {
            return Matrix<Rows, Cols, T>{ (m_cols[I] + other.m_cols[I])... };
        }

        template <std::size_t... I>
        constexpr Matrix<Rows, Cols, T> subtract_matrix_impl(const Matrix<Rows, Cols, T> & other, std::index_sequence<I...>) const noexcept
        {
            return Matrix<Rows, Cols, T>{ (m_cols[I] - other.m_cols[I])... };
        }

        template <std::size_t... I>
        constexpr Matrix<Rows, Cols, T> multiply_scalar_impl(T scalar, std::index_sequence<I...>) const noexcept
        {
            return Matrix<Rows, Cols, T>{ (m_cols[I] * scalar)... };
        }
    };

    // Common Aliases
    template <typename T> using Matrix4x4 = Matrix<4, 4, T>;
    using Matrix4f = Matrix4x4<float>;
    using Matrix4d = Matrix4x4<double>;
}
