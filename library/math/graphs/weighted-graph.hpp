#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace math
{
    template <typename T>
    concept GraphWeight = (std::integral<T> || std::floating_point<T>) && !std::same_as<T, bool> && std::same_as<T, std::remove_cv_t<T>>;

    using NodeId = std::size_t;
    using EdgeId = std::size_t;

    template <GraphWeight Weight>
    struct WeightedEdge
    {
        NodeId target;
        Weight weight;
        EdgeId id;
    };

    namespace detail
    {
        template <GraphWeight Weight>
        void validate_graph_weight(Weight weight)
        {
            if constexpr (std::floating_point<Weight>)
            {
                if (!std::isfinite(weight))
                {
                    throw std::invalid_argument("Graph weights must be finite");
                }
            }
            if constexpr (!std::unsigned_integral<Weight>)
            {
                if (weight < Weight{0})
                {
                    throw std::invalid_argument("Graph weights must be nonnegative");
                }
            }
        }
    }

    // Node and edge IDs remain stable across edits. Removed node IDs are not reused.
    template <GraphWeight Weight>
    class WeightedGraph
    {
        struct Node
        {
            std::vector<WeightedEdge<Weight>> edges;
            bool active{true};
        };

        std::vector<Node> m_nodes;
        EdgeId m_next_edge{0};

        [[nodiscard]] Node &require_node(NodeId node)
        {
            if (!has_node(node))
            {
                throw std::out_of_range("WeightedGraph node ID is inactive or out of range");
            }
            return m_nodes[node];
        }

    public:
        using weight_type = Weight;

        explicit WeightedGraph(std::size_t node_count = 0)
            : m_nodes(node_count)
        {
        }

        // This counts ID slots, including removed nodes.
        [[nodiscard]] std::size_t node_count() const noexcept
        {
            return m_nodes.size();
        }

        [[nodiscard]] bool has_node(NodeId node) const noexcept
        {
            return node < node_count() && m_nodes[node].active;
        }

        NodeId add_node()
        {
            m_nodes.emplace_back();
            return node_count() - 1;
        }

        void remove_node(NodeId node)
        {
            auto &removed = require_node(node);
            removed.edges.clear();
            removed.active = false;
            for (auto &entry : m_nodes)
            {
                std::erase_if(entry.edges, [node](const auto &edge)
                              { return edge.target == node; });
            }
        }

        EdgeId add_edge(NodeId source, NodeId target, Weight weight)
        {
            detail::validate_graph_weight(weight);
            auto &origin = require_node(source);
            static_cast<void>(require_node(target));
            if (m_next_edge == std::numeric_limits<EdgeId>::max())
            {
                throw std::length_error("WeightedGraph exhausted its edge IDs");
            }
            origin.edges.push_back({target, weight, m_next_edge});
            return m_next_edge++;
        }

        bool set_weight(NodeId source, EdgeId id, Weight weight)
        {
            detail::validate_graph_weight(weight);
            for (auto &edge : require_node(source).edges)
            {
                if (edge.id == id)
                {
                    edge.weight = weight;
                    return true;
                }
            }
            return false;
        }

        bool remove_edge(NodeId source, EdgeId id)
        {
            return std::erase_if(require_node(source).edges, [id](const auto &edge)
                                 { return edge.id == id; }) != 0;
        }

        // Borrowed until the next edit. Callers must synchronize edits with readers.
        [[nodiscard]] std::span<const WeightedEdge<Weight>> outgoing(NodeId node) const
        {
            if (!has_node(node))
            {
                throw std::out_of_range("WeightedGraph node ID is inactive or out of range");
            }
            return m_nodes[node].edges;
        }

        void for_each_outgoing(NodeId node, auto &&visit) const
        {
            for (const auto &edge : outgoing(node))
            {
                visit(edge);
            }
        }
    };

    // Immutable snapshot with contiguous outgoing edges and independent ownership.
    template <GraphWeight Weight>
    class PackedWeightedGraph
    {
        std::vector<bool> m_active;
        std::vector<std::size_t> m_offsets;
        std::vector<WeightedEdge<Weight>> m_edges;

    public:
        using weight_type = Weight;

        explicit PackedWeightedGraph(const WeightedGraph<Weight> &graph)
            : m_active(graph.node_count())
        {
            m_offsets.reserve(node_count() + 1);
            std::size_t edge_count = 0;
            for (NodeId node = 0; node < node_count(); ++node)
            {
                m_offsets.push_back(edge_count);
                m_active[node] = graph.has_node(node);
                if (m_active[node])
                {
                    if (graph.outgoing(node).size() > m_edges.max_size() - edge_count)
                    {
                        throw std::length_error("PackedWeightedGraph is too large");
                    }
                    edge_count += graph.outgoing(node).size();
                }
            }
            m_offsets.push_back(edge_count);
            m_edges.reserve(edge_count);
            for (NodeId node = 0; node < node_count(); ++node)
            {
                if (m_active[node])
                {
                    const auto edges = graph.outgoing(node);
                    m_edges.insert(m_edges.end(), edges.begin(), edges.end());
                }
            }
        }

        [[nodiscard]] std::size_t node_count() const noexcept
        {
            return m_active.size();
        }

        [[nodiscard]] bool has_node(NodeId node) const noexcept
        {
            return node < node_count() && m_active[node];
        }

        [[nodiscard]] std::span<const WeightedEdge<Weight>> outgoing(NodeId node) const
        {
            if (!has_node(node))
            {
                throw std::out_of_range("PackedWeightedGraph node ID is inactive or out of range");
            }
            return std::span<const WeightedEdge<Weight>>{m_edges}.subspan(m_offsets[node], m_offsets[node + 1] - m_offsets[node]);
        }

        void for_each_outgoing(NodeId node, auto &&visit) const
        {
            for (const auto &edge : outgoing(node))
            {
                visit(edge);
            }
        }
    };

    // Fixed node slots and constant-time edge edits. A snapshot keeps the cheapest edge per pair.
    template <GraphWeight Weight>
    class DenseWeightedGraph
    {
        static constexpr auto absent = std::numeric_limits<EdgeId>::max();

        struct Cell
        {
            Weight weight{};
            EdgeId id{absent};
        };

        std::vector<bool> m_active;
        std::vector<Cell> m_cells;
        EdgeId m_next_edge{0};

        [[nodiscard]] Cell &require_cell(NodeId source, NodeId target)
        {
            if (!has_node(source) || !has_node(target))
            {
                throw std::out_of_range("DenseWeightedGraph endpoint is inactive or out of range");
            }
            return m_cells[source * node_count() + target];
        }

    public:
        using weight_type = Weight;

        explicit DenseWeightedGraph(std::size_t size)
            : m_active(size, true)
        {
            if (size != 0 && size > m_cells.max_size() / size)
            {
                throw std::length_error("DenseWeightedGraph is too large");
            }
            m_cells.resize(size * size);
        }

        explicit DenseWeightedGraph(const WeightedGraph<Weight> &graph)
            : DenseWeightedGraph(graph.node_count())
        {
            const auto size = node_count();
            for (NodeId node = 0; node < size; ++node)
            {
                m_active[node] = graph.has_node(node);
                if (m_active[node])
                {
                    for (const auto &edge : graph.outgoing(node))
                    {
                        auto &cell = m_cells[node * size + edge.target];
                        m_next_edge = std::max(m_next_edge, edge.id + 1);
                        if (cell.id == absent || edge.weight < cell.weight || (edge.weight == cell.weight && edge.id < cell.id))
                        {
                            cell = {edge.weight, edge.id};
                        }
                    }
                }
            }
        }

        [[nodiscard]] std::size_t node_count() const noexcept
        {
            return m_active.size();
        }

        [[nodiscard]] bool has_node(NodeId node) const noexcept
        {
            return node < node_count() && m_active[node];
        }

        EdgeId add_edge(NodeId source, NodeId target, Weight weight)
        {
            detail::validate_graph_weight(weight);
            auto &cell = require_cell(source, target);
            if (cell.id != absent)
            {
                throw std::invalid_argument("DenseWeightedGraph already has an edge between these endpoints");
            }
            if (m_next_edge == absent)
            {
                throw std::length_error("DenseWeightedGraph exhausted its edge IDs");
            }
            cell = {weight, m_next_edge};
            return m_next_edge++;
        }

        bool set_edge_weight(NodeId source, NodeId target, Weight weight)
        {
            detail::validate_graph_weight(weight);
            auto &cell = require_cell(source, target);
            if (cell.id == absent)
            {
                return false;
            }
            cell.weight = weight;
            return true;
        }

        bool erase_edge(NodeId source, NodeId target)
        {
            auto &cell = require_cell(source, target);
            const bool had_edge = cell.id != absent;
            cell = {};
            return had_edge;
        }

        void for_each_outgoing(NodeId node, auto &&visit) const
        {
            if (!has_node(node))
            {
                throw std::out_of_range("DenseWeightedGraph node ID is inactive or out of range");
            }
            const auto size = node_count();
            for (NodeId target = 0; target < size; ++target)
            {
                const auto &cell = m_cells[node * size + target];
                if (cell.id != absent)
                {
                    visit(WeightedEdge<Weight>{target, cell.weight, cell.id});
                }
            }
        }
    };
}
