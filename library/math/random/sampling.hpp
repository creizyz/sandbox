#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <random>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace math
{
    // Own a stream per logical task when draw order must survive worker scheduling.
    class RandomStream
    {
        std::mt19937_64 m_engine;
        std::optional<double> m_normal_spare;

        [[nodiscard]] double standard_normal()
        {
            if (m_normal_spare)
            {
                const double result = *m_normal_spare;
                m_normal_spare.reset();
                return result;
            }
            double x;
            double y;
            double radius_squared;
            do
            {
                x = 2.0 * unit() - 1.0;
                y = 2.0 * unit() - 1.0;
                radius_squared = x * x + y * y;
            } while (!(radius_squared > 0.0 && radius_squared < 1.0));
            const double scale = std::sqrt(-2.0 * std::log(radius_squared) / radius_squared);
            m_normal_spare = y * scale;
            return x * scale;
        }

    public:
        using result_type = std::uint64_t;

        [[nodiscard]] static constexpr result_type min() noexcept
        {
            return 0;
        }

        [[nodiscard]] static constexpr result_type max() noexcept
        {
            return std::numeric_limits<result_type>::max();
        }

        explicit RandomStream(std::uint64_t seed)
            : m_engine(seed)
        {
        }

        RandomStream(std::uint64_t seed, std::uint64_t stream_id)
        {
            std::seed_seq sequence{
                static_cast<std::uint32_t>(seed), static_cast<std::uint32_t>(seed >> 32),
                static_cast<std::uint32_t>(stream_id), static_cast<std::uint32_t>(stream_id >> 32)};
            m_engine.seed(sequence);
        }

        [[nodiscard]] std::uint64_t next()
        {
            return m_engine();
        }

        [[nodiscard]] result_type operator()()
        {
            return next();
        }

        [[nodiscard]] std::uint64_t below(std::uint64_t bound)
        {
            if (bound == 0)
            {
                throw std::invalid_argument("Random bound must be positive");
            }
            const auto threshold = (std::uint64_t{0} - bound) % bound;
            auto sample = next();
            while (sample < threshold)
            {
                sample = next();
            }
            return sample % bound;
        }

        [[nodiscard]] double unit()
        {
            return static_cast<double>(next() >> 11) * 0x1p-53;
        }

        // Closed integer interval, including intervals spanning the entire signed range.
        [[nodiscard]] std::int64_t uniform_integer(std::int64_t minimum, std::int64_t maximum)
        {
            if (minimum > maximum)
            {
                throw std::invalid_argument("Random interval must be ordered");
            }
            const auto first = static_cast<std::uint64_t>(minimum);
            const auto size = static_cast<std::uint64_t>(maximum) - first + 1;
            if (size == 1)
            {
                return minimum;
            }
            return static_cast<std::int64_t>(first + (size == 0 ? next() : below(size)));
        }

        // The upper endpoint is excluded except for a constant interval.
        [[nodiscard]] double uniform_real(double minimum, double maximum)
        {
            if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum > maximum)
            {
                throw std::invalid_argument("Random interval must be finite and ordered");
            }
            if (minimum == maximum)
            {
                return minimum;
            }
            const double value = std::lerp(minimum, maximum, unit());
            return value < maximum ? value : std::nextafter(maximum, minimum);
        }

        [[nodiscard]] bool bernoulli(double probability)
        {
            if (!(probability >= 0.0 && probability <= 1.0))
            {
                throw std::invalid_argument("Bernoulli probability must be in [0, 1]");
            }
            return probability == 1.0 || (probability > 0.0 && unit() < probability);
        }

        [[nodiscard]] double normal(double mean = 0.0, double deviation = 1.0)
        {
            if (!std::isfinite(mean) || !std::isfinite(deviation) || deviation < 0.0)
            {
                throw std::invalid_argument("Normal parameters must be finite with a nonnegative deviation");
            }
            if (deviation == 0.0)
            {
                return mean;
            }
            const double value = std::fma(deviation, standard_normal(), mean);
            if (!std::isfinite(value))
            {
                throw std::overflow_error("Normal draw exceeds the scalar range");
            }
            return value;
        }

        [[nodiscard]] double exponential(double rate)
        {
            if (!std::isfinite(rate) || !(rate > 0.0))
            {
                throw std::invalid_argument("Exponential rate must be finite and positive");
            }
            const double value = -std::log1p(-unit()) / rate;
            if (!std::isfinite(value))
            {
                throw std::overflow_error("Exponential draw exceeds the scalar range");
            }
            return value;
        }

        // Moves a uniform sample without replacement into the first count positions.
        template <typename Value, std::size_t Extent>
        std::span<Value> sample_prefix(std::span<Value, Extent> values, std::size_t count)
        {
            if (count > values.size())
            {
                throw std::out_of_range("Sample size exceeds the input size");
            }
            for (std::size_t i = 0; i < count && i + 1 < values.size(); ++i)
            {
                const auto index = i + static_cast<std::size_t>(below(values.size() - i));
                using std::swap;
                swap(values[i], values[index]);
            }
            return values.first(count);
        }

        template <typename Value, std::size_t Extent>
        void shuffle(std::span<Value, Extent> values)
        {
            sample_prefix(values, values.size());
        }
    };

    class WeightedSampler
    {
        std::vector<long double> m_cumulative;

    public:
        explicit WeightedSampler(std::span<const double> weights)
        {
            double maximum = 0.0;
            for (const double weight : weights)
            {
                if (!std::isfinite(weight) || weight < 0.0)
                {
                    throw std::invalid_argument("Sampling weights must be finite and nonnegative");
                }
                maximum = std::max(maximum, weight);
            }
            if (maximum == 0.0)
            {
                throw std::invalid_argument("At least one sampling weight must be positive");
            }
            m_cumulative.reserve(weights.size());
            long double total = 0.0L;
            for (const double weight : weights)
            {
                total += static_cast<long double>(weight) / maximum;
                m_cumulative.push_back(total);
            }
        }

        [[nodiscard]] std::size_t draw(RandomStream &stream) const
        {
            if (m_cumulative.empty())
            {
                throw std::logic_error("WeightedSampler has no weights");
            }
            const long double total = m_cumulative.back();
            long double value = static_cast<long double>(stream.unit()) * total;
            if (value >= total)
            {
                value = std::nextafter(total, 0.0L);
            }
            const auto position = std::upper_bound(m_cumulative.begin(), m_cumulative.end(), value);
            return static_cast<std::size_t>(position - m_cumulative.begin());
        }
    };
}
