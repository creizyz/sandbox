#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace math
{
    // Solve Ax=b with scaled partial pivoting. A is provided in row-major order.
    template <std::floating_point T>
    [[nodiscard]] std::optional<std::vector<T>> solve_linear_system(
        std::span<const T> matrix, std::span<const T> right_hand_side,
        T relative_pivot_tolerance = T{8} * std::numeric_limits<T>::epsilon())
    {
        const auto size = right_hand_side.size();
        if ((size == 0 && !matrix.empty()) ||
            (size != 0 && (matrix.size() / size != size || matrix.size() % size != 0)) ||
            !(relative_pivot_tolerance >= T{0} && relative_pivot_tolerance < T{1}))
        {
            throw std::invalid_argument("Linear solve requires matching square dimensions and a pivot tolerance in [0, 1)");
        }
        std::vector<T> coefficients(matrix.begin(), matrix.end());
        std::vector<T> result(right_hand_side.begin(), right_hand_side.end());
        std::vector<T> scales(size, T{0});
        for (std::size_t row = 0; row < size; ++row)
        {
            if (!std::isfinite(result[row]))
            {
                throw std::invalid_argument("Linear solve requires finite inputs");
            }
            for (std::size_t column = 0; column < size; ++column)
            {
                const T value = coefficients[row * size + column];
                if (!std::isfinite(value))
                {
                    throw std::invalid_argument("Linear solve requires finite inputs");
                }
                scales[row] = std::max(scales[row], std::abs(value));
            }
        }
        for (std::size_t column = 0; column < size; ++column)
        {
            auto pivot = column;
            T best_ratio{0};
            for (std::size_t row = column; row < size; ++row)
            {
                if (scales[row] > T{0})
                {
                    const T ratio = std::abs(coefficients[row * size + column]) / scales[row];
                    if (ratio > best_ratio)
                    {
                        pivot = row;
                        best_ratio = ratio;
                    }
                }
            }
            if (!(best_ratio > relative_pivot_tolerance))
            {
                return std::nullopt;
            }
            auto *pivot_row = coefficients.data() + column * size;
            if (pivot != column)
            {
                std::swap_ranges(pivot_row, pivot_row + size, coefficients.data() + pivot * size);
                std::swap(result[column], result[pivot]);
                std::swap(scales[column], scales[pivot]);
            }
            for (std::size_t row = column + 1; row < size; ++row)
            {
                auto *target = coefficients.data() + row * size;
                if (target[column] == T{0})
                {
                    continue;
                }
                const T multiplier = target[column] / pivot_row[column];
                if (!std::isfinite(multiplier))
                {
                    throw std::overflow_error("Linear solve multiplier exceeds the scalar range");
                }
                for (std::size_t k = column + 1; k < size; ++k)
                {
                    target[k] -= multiplier * pivot_row[k];
                    if (!std::isfinite(target[k]))
                    {
                        throw std::overflow_error("Linear solve coefficient exceeds the scalar range");
                    }
                }
                result[row] -= multiplier * result[column];
                if (!std::isfinite(result[row]))
                {
                    throw std::overflow_error("Linear solve right-hand side exceeds the scalar range");
                }
            }
        }
        for (std::size_t i = size; i > 0; --i)
        {
            const auto row = i - 1;
            for (std::size_t column = row + 1; column < size; ++column)
            {
                result[row] -= coefficients[row * size + column] * result[column];
            }
            result[row] /= coefficients[row * size + row];
            if (!std::isfinite(result[row]))
            {
                throw std::overflow_error("Linear solve result exceeds the scalar range");
            }
        }
        return result;
    }
}
