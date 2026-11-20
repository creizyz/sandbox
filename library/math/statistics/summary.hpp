#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace math
{
    template <std::floating_point T = double>
    class RunningStatistics
    {
        template <std::floating_point>
        friend class RunningCovariance;

        std::uint64_t m_count{0};
        T m_origin{0};
        T m_mean_offset{0};
        T m_squared_deviation{0};

        [[nodiscard]] T difference_from_mean(T value) const noexcept
        {
            return (value - m_origin) - m_mean_offset;
        }

        [[nodiscard]] T mean_difference(const RunningStatistics &other) const noexcept
        {
            return (other.m_origin - m_origin) + (other.m_mean_offset - m_mean_offset);
        }

        void update(std::uint64_t count, T mean_offset, T squared_deviation)
        {
            if (!std::isfinite(squared_deviation))
            {
                throw std::overflow_error("Accumulated squared deviation exceeds the scalar range");
            }
            m_count = count;
            m_mean_offset = mean_offset;
            m_squared_deviation = squared_deviation;
        }

    public:
        void add(T value)
        {
            if (!std::isfinite(value))
            {
                throw std::invalid_argument("Statistics require finite samples");
            }
            if (m_count == std::numeric_limits<std::uint64_t>::max())
            {
                throw std::overflow_error("Statistics sample count overflow");
            }
            if (m_count == 0)
            {
                m_origin = value;
                m_count = 1;
                return;
            }
            const auto count = m_count + 1;
            const T delta = difference_from_mean(value);
            const T mean_offset = m_mean_offset + delta / static_cast<T>(count);
            update(count, mean_offset, m_squared_deviation + delta * ((value - m_origin) - mean_offset));
        }

        // Results are reproducible for a fixed sample and merge order.
        void merge(const RunningStatistics &other)
        {
            if (other.m_count == 0)
            {
                return;
            }
            if (m_count == 0)
            {
                *this = other;
                return;
            }
            if (other.m_count > std::numeric_limits<std::uint64_t>::max() - m_count)
            {
                throw std::overflow_error("Statistics sample count overflow");
            }
            const auto count = m_count + other.m_count;
            const auto right_fraction = static_cast<T>(other.m_count) / static_cast<T>(count);
            const auto delta = mean_difference(other);
            const auto mean_offset = m_mean_offset + delta * right_fraction;
            const auto correction = delta * (delta * (static_cast<T>(m_count) * right_fraction));
            update(count, mean_offset, m_squared_deviation + other.m_squared_deviation + correction);
        }

        [[nodiscard]] std::uint64_t count() const noexcept
        {
            return m_count;
        }

        [[nodiscard]] std::optional<T> mean() const noexcept
        {
            return m_count == 0 ? std::nullopt : std::optional<T>{m_origin + m_mean_offset};
        }

        [[nodiscard]] std::optional<T> population_variance() const noexcept
        {
            return m_count == 0 ? std::nullopt : std::optional<T>{m_squared_deviation / static_cast<T>(m_count)};
        }

        [[nodiscard]] std::optional<T> sample_variance() const noexcept
        {
            return m_count < 2 ? std::nullopt : std::optional<T>{m_squared_deviation / static_cast<T>(m_count - 1)};
        }

        [[nodiscard]] std::optional<T> sample_standard_deviation() const noexcept
        {
            if (const auto variance = sample_variance())
            {
                return std::sqrt(*variance);
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<T> standard_error_of_mean() const noexcept
        {
            if (const auto deviation = sample_standard_deviation())
            {
                return *deviation / std::sqrt(static_cast<T>(m_count));
            }
            return std::nullopt;
        }
    };

    // Linearly interpolate between the order statistics around p * (n - 1).
    template <std::floating_point T>
    [[nodiscard]] std::optional<T> quantile(std::span<const T> samples, double probability)
    {
        if (!(probability >= 0.0 && probability <= 1.0))
        {
            throw std::invalid_argument("Quantile probability must be in [0, 1]");
        }
        if (samples.empty())
        {
            return std::nullopt;
        }
        if (!std::ranges::all_of(samples, [](T value)
                                 { return std::isfinite(value); }))
        {
            throw std::invalid_argument("Quantiles require finite samples");
        }
        if (probability == 0.0)
        {
            return *std::ranges::min_element(samples);
        }
        if (probability == 1.0)
        {
            return *std::ranges::max_element(samples);
        }
        std::vector<T> ordered(samples.begin(), samples.end());
        const auto last = ordered.size() - 1;
        const auto rank = static_cast<long double>(probability) * static_cast<long double>(last);
        const auto lower = std::min(static_cast<std::size_t>(rank), last);
        auto middle = ordered.begin() + static_cast<std::ptrdiff_t>(lower);
        std::nth_element(ordered.begin(), middle, ordered.end());
        if (lower == last || rank == static_cast<long double>(lower))
        {
            return *middle;
        }
        return std::lerp(*middle, *std::min_element(middle + 1, ordered.end()), static_cast<T>(rank - lower));
    }

    template <std::floating_point T>
    struct LinearFit
    {
        T slope;
        T intercept;
    };

    template <std::floating_point T = double>
    class RunningCovariance
    {
        RunningStatistics<T> m_x;
        RunningStatistics<T> m_y;
        T m_cross{0};

        void update(RunningStatistics<T> x, RunningStatistics<T> y, T cross)
        {
            if (!std::isfinite(cross))
            {
                throw std::overflow_error("Accumulated cross deviation exceeds the scalar range");
            }
            m_x = x;
            m_y = y;
            m_cross = cross;
        }

    public:
        void add(T x, T y)
        {
            auto next_x = m_x;
            auto next_y = m_y;
            next_x.add(x);
            next_y.add(y);
            const T cross = m_x.count() == 0 ? T{0} : m_cross + m_x.difference_from_mean(x) * next_y.difference_from_mean(y);
            update(next_x, next_y, cross);
        }

        void merge(const RunningCovariance &other)
        {
            if (other.count() == 0)
            {
                return;
            }
            if (count() == 0)
            {
                *this = other;
                return;
            }
            auto next_x = m_x;
            auto next_y = m_y;
            next_x.merge(other.m_x);
            next_y.merge(other.m_y);
            const auto factor = static_cast<T>(count()) * (static_cast<T>(other.count()) / static_cast<T>(next_x.count()));
            const auto change_x = m_x.mean_difference(other.m_x);
            const auto change_y = m_y.mean_difference(other.m_y);
            update(next_x, next_y, m_cross + other.m_cross + change_x * (change_y * factor));
        }

        [[nodiscard]] std::uint64_t count() const noexcept
        {
            return m_x.count();
        }

        [[nodiscard]] std::optional<T> population_covariance() const noexcept
        {
            return count() == 0 ? std::nullopt : std::optional<T>{m_cross / static_cast<T>(count())};
        }

        [[nodiscard]] std::optional<T> sample_covariance() const noexcept
        {
            return count() < 2 ? std::nullopt : std::optional<T>{m_cross / static_cast<T>(count() - 1)};
        }

        [[nodiscard]] std::optional<T> correlation() const noexcept
        {
            const auto x = m_x.sample_standard_deviation();
            const auto y = m_y.sample_standard_deviation();
            if (!x || !y || *x == T{0} || *y == T{0})
            {
                return std::nullopt;
            }
            return std::clamp(*sample_covariance() / std::max(*x, *y) / std::min(*x, *y), T{-1}, T{1});
        }

        [[nodiscard]] std::optional<LinearFit<T>> linear_fit() const
        {
            const auto variance = m_x.sample_variance();
            if (!variance || *variance == T{0})
            {
                return std::nullopt;
            }
            const T slope = *sample_covariance() / *variance;
            const T intercept = std::fma(-slope, *m_x.mean(), *m_y.mean());
            if (!std::isfinite(slope) || !std::isfinite(intercept))
            {
                throw std::overflow_error("Linear fit exceeds the scalar range");
            }
            return LinearFit<T>{slope, intercept};
        }
    };

    // The weights describe a population. Variance divides by total weight.
    template <std::floating_point T = double>
    class WeightedStatistics
    {
        T m_weight{0};
        T m_mean{0};
        T m_squared_deviation{0};

        void combine(T weight, T mean, T squared_deviation)
        {
            if (weight == T{0})
            {
                return;
            }
            if (m_weight == T{0})
            {
                m_weight = weight;
                m_mean = mean;
                m_squared_deviation = squared_deviation;
                return;
            }
            const T total = m_weight + weight;
            if (!std::isfinite(total))
            {
                throw std::overflow_error("Statistics weight exceeds the scalar range");
            }
            const T delta = mean - m_mean;
            const T correction = delta * (delta * (std::min(weight, m_weight) * (std::max(weight, m_weight) / total)));
            const T next_squared_deviation = m_squared_deviation + squared_deviation + correction;
            if (!std::isfinite(next_squared_deviation))
            {
                throw std::overflow_error("Weighted squared deviation exceeds the scalar range");
            }
            m_mean = weight <= m_weight ? std::lerp(m_mean, mean, weight / total)
                                        : std::lerp(mean, m_mean, m_weight / total);
            m_weight = total;
            m_squared_deviation = next_squared_deviation;
        }

    public:
        void add(T value, T weight)
        {
            if (!std::isfinite(value) || !std::isfinite(weight) || weight < T{0})
            {
                throw std::invalid_argument("Weighted statistics require finite values and nonnegative weights");
            }
            combine(weight, value, T{0});
        }

        void merge(const WeightedStatistics &other)
        {
            combine(other.m_weight, other.m_mean, other.m_squared_deviation);
        }

        [[nodiscard]] T weight() const noexcept
        {
            return m_weight;
        }

        [[nodiscard]] std::optional<T> mean() const noexcept
        {
            return m_weight == T{0} ? std::nullopt : std::optional<T>{m_mean};
        }

        [[nodiscard]] std::optional<T> population_variance() const noexcept
        {
            return m_weight == T{0} ? std::nullopt : std::optional<T>{m_squared_deviation / m_weight};
        }
    };

    // Pearson correlation between equal-length leading and lagged subsequences.
    template <std::floating_point T>
    [[nodiscard]] std::optional<T> lagged_correlation(std::span<const T> samples, std::size_t lag)
    {
        if (lag >= samples.size())
        {
            return std::nullopt;
        }
        RunningCovariance<T> paired;
        for (std::size_t i = lag; i < samples.size(); ++i)
        {
            paired.add(samples[i - lag], samples[i]);
        }
        return paired.correlation();
    }

    struct Histogram
    {
        std::vector<std::size_t> bins;
        std::size_t underflow{0};
        std::size_t overflow{0};
    };

    // Bins include their left edge, and the last bin also includes its right edge.
    template <std::floating_point T>
    [[nodiscard]] Histogram histogram(std::span<const T> samples, std::span<const T> edges)
    {
        if (edges.size() < 2 || !std::ranges::all_of(edges, [](T edge)
                                                     { return std::isfinite(edge); }) ||
            std::adjacent_find(edges.begin(), edges.end(), [](T a, T b)
                               { return a >= b; }) != edges.end())
        {
            throw std::invalid_argument("Histogram requires finite, strictly increasing edges");
        }
        Histogram result{std::vector<std::size_t>(edges.size() - 1)};
        for (const T value : samples)
        {
            if (!std::isfinite(value))
            {
                throw std::invalid_argument("Histogram samples must be finite");
            }
            if (value < edges.front())
            {
                ++result.underflow;
            }
            else if (value > edges.back())
            {
                ++result.overflow;
            }
            else
            {
                const auto position = std::upper_bound(edges.begin() + 1, edges.end() - 1, value);
                ++result.bins[static_cast<std::size_t>(position - edges.begin() - 1)];
            }
        }
        return result;
    }

    // Sort once when selecting several probabilities. The result follows the supplied order.
    template <std::floating_point T>
    [[nodiscard]] std::optional<std::vector<T>> quantiles(std::span<const T> samples, std::span<const double> probabilities)
    {
        for (const double p : probabilities)
        {
            if (!(p >= 0.0 && p <= 1.0))
            {
                throw std::invalid_argument("Quantile probability must be in [0, 1]");
            }
        }
        std::vector<T> results;
        if (probabilities.empty())
        {
            return results;
        }
        if (samples.empty())
        {
            return std::nullopt;
        }
        if (!std::ranges::all_of(samples, [](T value)
                                 { return std::isfinite(value); }))
        {
            throw std::invalid_argument("Quantile samples must be finite");
        }
        std::vector<T> ordered(samples.begin(), samples.end());
        std::ranges::sort(ordered);
        const auto last = ordered.size() - 1;
        results.reserve(probabilities.size());
        for (const double probability : probabilities)
        {
            const auto rank = static_cast<long double>(probability) * static_cast<long double>(last);
            const auto lower = std::min(static_cast<std::size_t>(rank), last);
            const auto upper = lower + (lower < last ? 1 : 0);
            results.push_back(std::lerp(ordered[lower], ordered[upper], static_cast<T>(rank - lower)));
        }
        return results;
    }
}
