#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "math/detail/checked-integer.hpp"

namespace math
{
    using FlowValue = std::int64_t;

    struct FlowArc
    {
        std::size_t source;
        std::size_t target;
        FlowValue capacity;
        FlowValue cost;
    };

    class FlowNetwork
    {
        std::size_t m_node_count;
        std::vector<FlowArc> m_arcs;

    public:
        explicit FlowNetwork(std::size_t node_count) noexcept
            : m_node_count(node_count)
        {
        }

        [[nodiscard]] std::size_t node_count() const noexcept
        {
            return m_node_count;
        }

        [[nodiscard]] std::span<const FlowArc> arcs() const noexcept
        {
            return m_arcs;
        }

        // The returned ID indexes the input-order flow vector in a solve result.
        std::size_t add_arc(std::size_t source, std::size_t target, FlowValue capacity, FlowValue cost = 0)
        {
            if (source >= m_node_count || target >= m_node_count)
            {
                throw std::out_of_range("Flow arc endpoint is out of range");
            }
            if (capacity < 0 || cost < 0)
            {
                throw std::invalid_argument("Flow capacities and input costs must be nonnegative");
            }
            m_arcs.push_back({source, target, capacity, cost});
            return m_arcs.size() - 1;
        }
    };

    struct FlowResult
    {
        FlowValue value;
        std::vector<FlowValue> arc_flows;
    };

    struct CostFlowResult
    {
        FlowResult flow;
        FlowValue cost;
    };

    namespace detail
    {
        inline void validate_flow_query(const FlowNetwork &network, std::size_t source, std::size_t sink, FlowValue limit)
        {
            if (source >= network.node_count() || sink >= network.node_count())
            {
                throw std::out_of_range("Flow query endpoint is out of range");
            }
            if (source == sink || limit < 0)
            {
                throw std::invalid_argument("Flow requires distinct endpoints and a nonnegative limit");
            }
        }

        class ResidualNetwork
        {
            struct Arc
            {
                std::size_t target;
                std::size_t reverse_index;
                FlowValue capacity;
                FlowValue cost;
            };

            std::vector<std::vector<Arc>> m_rows;
            std::vector<std::size_t> m_forward_indices;

        public:
            explicit ResidualNetwork(const FlowNetwork &network)
                : m_rows(network.node_count())
            {
                m_forward_indices.reserve(network.arcs().size());
                for (const auto &arc : network.arcs())
                {
                    auto &from = m_rows[arc.source];
                    auto &to = m_rows[arc.target];
                    const auto forward_index = from.size();
                    const auto reverse_index = to.size() + (arc.source == arc.target ? 1 : 0);
                    m_forward_indices.push_back(forward_index);
                    from.push_back({arc.target, reverse_index, arc.capacity, arc.cost});
                    to.push_back({arc.source, forward_index, 0, -arc.cost});
                }
            }

            [[nodiscard]] const auto &outgoing(std::size_t node) const noexcept
            {
                return m_rows[node];
            }

            void push(std::size_t node, std::size_t index, FlowValue amount) noexcept
            {
                auto &arc = m_rows[node][index];
                arc.capacity -= amount;
                m_rows[arc.target][arc.reverse_index].capacity += amount;
            }

            [[nodiscard]] FlowResult result(const FlowNetwork &network, FlowValue total) const
            {
                FlowResult result{total, {}};
                result.arc_flows.reserve(network.arcs().size());
                std::size_t index = 0;
                for (const auto &arc : network.arcs())
                {
                    result.arc_flows.push_back(arc.capacity - m_rows[arc.source][m_forward_indices[index++]].capacity);
                }
                return result;
            }
        };
    }

    // Dinic's algorithm. A finite limit stops the solve once that much flow is found.
    [[nodiscard]] inline FlowResult max_flow(const FlowNetwork &network, std::size_t source, std::size_t sink,
                                             FlowValue limit = std::numeric_limits<FlowValue>::max())
    {
        detail::validate_flow_query(network, source, sink, limit);
        detail::ResidualNetwork residual(network);
        const auto size = network.node_count();
        std::vector<std::size_t> level(size);
        std::vector<std::size_t> next_arc(size);
        std::vector<std::size_t> queue;
        queue.reserve(size);
        FlowValue total = 0;
        while (total < limit)
        {
            std::fill(level.begin(), level.end(), size);
            level[source] = 0;
            queue.clear();
            queue.push_back(source);
            for (std::size_t i = 0; i < queue.size(); ++i)
            {
                const auto node = queue[i];
                for (const auto &arc : residual.outgoing(node))
                {
                    if (arc.capacity > 0 && level[arc.target] == size)
                    {
                        level[arc.target] = level[node] + 1;
                        queue.push_back(arc.target);
                    }
                }
            }
            if (level[sink] == size)
            {
                break;
            }
            std::fill(next_arc.begin(), next_arc.end(), 0);
            const auto send = [&](auto &&self, std::size_t node, FlowValue available) -> FlowValue
            {
                if (node == sink)
                {
                    return available;
                }
                const auto &row = residual.outgoing(node);
                for (auto &index = next_arc[node]; index < row.size(); ++index)
                {
                    const auto &arc = row[index];
                    if (arc.capacity > 0 && level[arc.target] == level[node] + 1)
                    {
                        const auto sent = self(self, arc.target, std::min(available, arc.capacity));
                        if (sent > 0)
                        {
                            residual.push(node, index, sent);
                            return sent;
                        }
                    }
                }
                return 0;
            };
            while (total < limit)
            {
                const auto sent = send(send, source, limit - total);
                if (sent == 0)
                {
                    break;
                }
                total += sent;
            }
        }
        return residual.result(network, total);
    }

    // Successive shortest augmenting paths with reduced-cost potentials.
    // Returns the cheapest maximum flow up to limit. Costs and intermediates must fit FlowValue.
    [[nodiscard]] inline CostFlowResult min_cost_flow(const FlowNetwork &network, std::size_t source, std::size_t sink,
                                                      FlowValue limit = std::numeric_limits<FlowValue>::max())
    {
        detail::validate_flow_query(network, source, sink, limit);
        detail::ResidualNetwork residual(network);
        const auto size = network.node_count();
        std::vector<FlowValue> potential(size);
        std::vector<std::optional<FlowValue>> distance(size);
        std::vector<std::pair<std::size_t, std::size_t>> incoming(size);
        using Visit = std::pair<FlowValue, std::size_t>;
        FlowValue total = 0;
        FlowValue cost = 0;
        while (total < limit)
        {
            std::fill(distance.begin(), distance.end(), std::nullopt);
            distance[source] = 0;
            std::priority_queue<Visit, std::vector<Visit>, std::greater<Visit>> frontier;
            frontier.emplace(0, source);
            while (!frontier.empty())
            {
                const auto [current, node] = frontier.top();
                frontier.pop();
                if (current != distance[node])
                {
                    continue;
                }
                const auto &outgoing = residual.outgoing(node);
                for (std::size_t i = 0; i < outgoing.size(); ++i)
                {
                    const auto &arc = outgoing[i];
                    if (arc.capacity == 0 || (distance[arc.target] && *distance[arc.target] <= current))
                    {
                        continue;
                    }
                    const auto reduced = detail::checked_add(arc.cost, detail::checked_subtract(potential[node], potential[arc.target]));
                    assert(reduced >= 0);
                    const auto next = detail::checked_add(current, reduced);
                    if (!distance[arc.target] || next < *distance[arc.target])
                    {
                        distance[arc.target] = next;
                        incoming[arc.target] = {node, i};
                        frontier.emplace(next, arc.target);
                    }
                }
            }
            if (!distance[sink])
            {
                break;
            }
            for (std::size_t node = 0; node < size; ++node)
            {
                if (distance[node])
                {
                    potential[node] = detail::checked_add(potential[node], *distance[node]);
                }
            }

            FlowValue amount = limit - total;
            for (auto node = sink; node != source; node = incoming[node].first)
            {
                const auto [parent, index] = incoming[node];
                amount = std::min(amount, residual.outgoing(parent)[index].capacity);
            }
            cost = detail::checked_add(cost, detail::checked_scaled_product(amount, potential[sink], 1));
            for (auto node = sink; node != source; node = incoming[node].first)
            {
                const auto [parent, index] = incoming[node];
                residual.push(parent, index, amount);
            }
            total += amount;
        }
        return {residual.result(network, total), cost};
    }
}
