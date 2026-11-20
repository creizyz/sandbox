#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "math/detail/checked-integer.hpp"
#include "fixed-point.hpp"

namespace math
{
    namespace detail
    {
        template <typename T>
        inline constexpr bool is_fixed_point = false;

        template <std::int64_t Scale>
        inline constexpr bool is_fixed_point<FixedPoint<Scale>> = true;

        template <typename T>
        concept DiscreteScalar = std::floating_point<T> || std::same_as<T, std::int64_t> || is_fixed_point<T>;

        template <DiscreteScalar T>
        constexpr bool finite(T value) noexcept
        {
            if constexpr (std::floating_point<T>)
            {
                return std::isfinite(value);
            }
            return true;
        }

        template <DiscreteScalar T>
        constexpr T finite_result(T value)
        {
            if (!finite(value))
            {
                throw std::overflow_error("Discrete update exceeds the scalar range");
            }
            return value;
        }
    }

    template <detail::DiscreteScalar T>
    [[nodiscard]] constexpr T integrate_rate(T rate, T duration)
    {
        if (!detail::finite(rate) || !detail::finite(duration) || duration < T{})
        {
            throw std::invalid_argument("Rate integration requires a finite rate and a finite nonnegative duration");
        }
        if constexpr (std::same_as<T, std::int64_t>)
        {
            return detail::checked_scaled_product(rate, duration, 1);
        }
        else
        {
            return detail::finite_result(rate * duration);
        }
    }

    // Subtract the nonnegative flows before adjusting the balance to avoid overflowing a temporary sum.
    template <detail::DiscreteScalar T>
    [[nodiscard]] constexpr T advance_balance(T balance, T inflow, T outflow)
    {
        if (!detail::finite(balance) || !detail::finite(inflow) || !detail::finite(outflow) ||
            inflow < T{} || outflow < T{})
        {
            throw std::invalid_argument("Balance updates require a finite balance and finite nonnegative flows");
        }
        if constexpr (std::same_as<T, std::int64_t>)
        {
            return detail::checked_add(balance, inflow - outflow);
        }
        else
        {
            return detail::finite_result(balance + (inflow - outflow));
        }
    }

    using Tick = std::uint64_t;

    // Equal ticks are delivered in the order they were scheduled.
    template <typename Value>
        requires(std::is_nothrow_move_constructible_v<Value> && std::is_nothrow_move_assignable_v<Value>)
    class TickQueue
    {
    public:
        struct Event
        {
            Tick tick;
            Value value;
        };

    private:
        struct Entry
        {
            Event event;
            std::uint64_t sequence;
        };

        static bool later(const Entry &left, const Entry &right) noexcept
        {
            return left.event.tick == right.event.tick ? left.sequence > right.sequence : left.event.tick > right.event.tick;
        }

        std::vector<Entry> m_events;
        std::uint64_t m_sequence{0};

    public:
        [[nodiscard]] std::size_t size() const noexcept
        {
            return m_events.size();
        }

        [[nodiscard]] std::optional<Tick> next_tick() const noexcept
        {
            return m_events.empty() ? std::nullopt : std::optional<Tick>{m_events.front().event.tick};
        }

        void schedule(Tick tick, Value value)
        {
            if (m_sequence == std::numeric_limits<std::uint64_t>::max())
            {
                throw std::overflow_error("TickQueue exhausted its sequence numbers");
            }
            m_events.push_back({Event{tick, std::move(value)}, m_sequence});
            ++m_sequence;
            std::push_heap(m_events.begin(), m_events.end(), later);
        }

        void schedule_after(Tick now, Tick delay, Value value)
        {
            if (delay > std::numeric_limits<Tick>::max() - now)
            {
                throw std::overflow_error("Scheduled tick exceeds the tick range");
            }
            schedule(now + delay, std::move(value));
        }

        [[nodiscard]] std::optional<Event> pop_ready(Tick now)
        {
            if (m_events.empty() || m_events.front().event.tick > now)
            {
                return std::nullopt;
            }
            std::pop_heap(m_events.begin(), m_events.end(), later);
            Event event = std::move(m_events.back().event);
            m_events.pop_back();
            return event;
        }
    };
}
