#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "weighted-graph.hpp"

namespace math
{
    template <GraphWeight Weight>
    struct WeightedPath
    {
        Weight distance;
        std::vector<NodeId> nodes;
        std::vector<EdgeId> edges;
    };

    namespace detail
    {
        template <GraphWeight Weight>
        struct PathPredecessor
        {
            NodeId node;
            EdgeId edge;
            Weight edge_weight;
        };

        template <GraphWeight Weight>
        struct PathSearch
        {
            NodeId source;
            std::vector<std::optional<Weight>> distances;
            std::vector<PathPredecessor<Weight>> predecessors;
            std::vector<bool> overflowed;
        };

        template <GraphWeight Weight>
        [[nodiscard]] std::optional<Weight> add_path_weights(Weight current, Weight edge)
        {
            if constexpr (std::integral<Weight>)
            {
                if (edge > std::numeric_limits<Weight>::max() - current)
                {
                    return std::nullopt;
                }
            }
            const auto total = static_cast<Weight>(current + edge);
            if constexpr (std::floating_point<Weight>)
            {
                if (!std::isfinite(total))
                {
                    return std::nullopt;
                }
            }
            return total;
        }

        template <typename Graph>
        [[nodiscard]] auto search_shortest_paths(const Graph &graph, NodeId source, std::span<const NodeId> targets)
        {
            using Weight = typename Graph::weight_type;
            using Visit = std::pair<Weight, NodeId>;
            constexpr bool dense = std::is_same_v<Graph, DenseWeightedGraph<Weight>>;
            const auto size = graph.node_count();
            if (!graph.has_node(source))
            {
                throw std::out_of_range("Shortest path source is inactive or out of range");
            }

            std::vector<bool> wanted;
            std::size_t remaining = 0;
            if (!targets.empty())
            {
                wanted.resize(size);
                for (const auto target : targets)
                {
                    if (!graph.has_node(target))
                    {
                        throw std::out_of_range("Shortest path destination is inactive or out of range");
                    }
                    if (!wanted[target])
                    {
                        wanted[target] = true;
                        ++remaining;
                    }
                }
            }

            PathSearch<Weight> result{source, std::vector<std::optional<Weight>>(size), std::vector<PathPredecessor<Weight>>(size), {}};
            std::vector<bool> settled(size);
            std::priority_queue<Visit, std::vector<Visit>, std::greater<Visit>> frontier;
            result.distances[source] = Weight{0};
            if constexpr (!dense)
            {
                frontier.emplace(Weight{0}, source);
            }

            const auto next_node = [&]() -> std::optional<Visit>
            {
                if constexpr (dense)
                {
                    std::optional<Visit> next;
                    for (NodeId node = 0; node < size; ++node)
                    {
                        if (!settled[node] && result.distances[node] && (!next || *result.distances[node] < next->first))
                        {
                            next = Visit{*result.distances[node], node};
                        }
                    }
                    return next;
                }
                else
                {
                    while (!frontier.empty())
                    {
                        const auto next = frontier.top();
                        frontier.pop();
                        if (!settled[next.second])
                        {
                            return next;
                        }
                    }
                    return std::nullopt;
                }
            };

            // Both frontiers settle equal distances in node ID order.
            while (const auto next = next_node())
            {
                const auto [distance, node] = *next;
                settled[node] = true;
                if (!wanted.empty() && wanted[node] && --remaining == 0)
                {
                    break;
                }
                graph.for_each_outgoing(node, [&](const WeightedEdge<Weight> &edge)
                                        {
                    auto &known = result.distances[edge.target];
                    auto &predecessor = result.predecessors[edge.target];
                    if (settled[edge.target] || (known && (*known < distance || (*known == distance && predecessor.node != node))))
                    {
                        return;
                    }
                    const auto candidate = add_path_weights(distance, edge.weight);
                    if (!candidate)
                    {
                        if (!known)
                        {
                            if (result.overflowed.empty())
                            {
                                result.overflowed.resize(size);
                            }
                            result.overflowed[edge.target] = true;
                        }
                        return;
                    }
                    if (!known || *candidate < *known)
                    {
                        known = *candidate;
                        predecessor = {node, edge.id, edge.weight};
                        if constexpr (!dense)
                        {
                            frontier.emplace(*candidate, edge.target);
                        }
                    }
                    else if (*candidate == *known && predecessor.node == node && edge.weight < predecessor.edge_weight)
                    {
                        predecessor = {node, edge.id, edge.weight};
                    } });
            }

            if (!result.overflowed.empty())
            {
                std::vector<NodeId> overflow_frontier;
                for (NodeId node = 0; node < size; ++node)
                {
                    if (result.distances[node])
                    {
                        result.overflowed[node] = false;
                    }
                    if (result.overflowed[node])
                    {
                        overflow_frontier.push_back(node);
                    }
                }
                for (std::size_t i = 0; i < overflow_frontier.size(); ++i)
                {
                    graph.for_each_outgoing(overflow_frontier[i], [&](const WeightedEdge<Weight> &edge)
                                            {
                        if (!result.distances[edge.target] && !result.overflowed[edge.target])
                        {
                            result.overflowed[edge.target] = true;
                            overflow_frontier.push_back(edge.target);
                        } });
                }
            }
            return result;
        }
    }

    // Owns distances and path predecessors for the graph state at query time.
    template <GraphWeight Weight>
    class ShortestPaths
    {
        detail::PathSearch<Weight> m_search;

    public:
        explicit ShortestPaths(detail::PathSearch<Weight> search)
            : m_search(std::move(search))
        {
        }

        [[nodiscard]] std::optional<Weight> distance_to(NodeId target) const
        {
            const auto result = m_search.distances.at(target);
            if (!m_search.overflowed.empty() && m_search.overflowed[target])
            {
                throw std::overflow_error("Shortest path exceeds the weight range");
            }
            return result;
        }

        [[nodiscard]] std::optional<WeightedPath<Weight>> path_to(NodeId target) const
        {
            const auto distance = distance_to(target);
            if (!distance)
            {
                return std::nullopt;
            }

            WeightedPath<Weight> result{*distance, {target}, {}};
            while (target != m_search.source)
            {
                const auto &predecessor = m_search.predecessors[target];
                result.edges.push_back(predecessor.edge);
                result.nodes.push_back(predecessor.node);
                target = predecessor.node;
            }
            std::reverse(result.nodes.begin(), result.nodes.end());
            std::reverse(result.edges.begin(), result.edges.end());
            return result;
        }
    };

    template <typename Graph>
    [[nodiscard]] auto shortest_paths(const Graph &graph, NodeId source)
    {
        return ShortestPaths(detail::search_shortest_paths(graph, source, {}));
    }

    template <typename Graph>
    [[nodiscard]] auto shortest_path(const Graph &graph, NodeId source, NodeId target)
    {
        const ShortestPaths paths(detail::search_shortest_paths(graph, source, std::span{&target, 1}));
        return paths.path_to(target);
    }

    template <typename Graph>
    [[nodiscard]] auto shortest_paths_to(const Graph &graph, NodeId source, std::span<const NodeId> targets)
    {
        std::vector<std::optional<WeightedPath<typename Graph::weight_type>>> result;
        if (!targets.empty())
        {
            const ShortestPaths paths(detail::search_shortest_paths(graph, source, targets));
            result.reserve(targets.size());
            for (const auto target : targets)
            {
                result.push_back(paths.path_to(target));
            }
        }
        return result;
    }
}
