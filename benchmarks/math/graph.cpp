#include <benchmark/benchmark.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "math/graphs/network-flow.hpp"
#include "math/graphs/shortest-path.hpp"
#include "math/graphs/weighted-graph.hpp"

namespace
{
    using namespace math;

    enum class Layout
    {
        Adjacency,
        Packed,
        Dense
    };

    enum class Query
    {
        PointToPoint,
        Targets,
        SingleSource,
        EditAndQuery
    };

    template <typename Weight>
    WeightedGraph<Weight> make_graph(std::size_t count, std::uint32_t percent)
    {
        WeightedGraph<Weight> graph(count);
        std::uint32_t random = 8;
        for (NodeId node = 0; node < count; ++node)
        {
            for (NodeId target = 0; target < count; ++target)
            {
                random = random * 1664525u + 1013904223u;
                if (target != node && (target == (node + 1) % count || (random >> 16) % 100 < percent))
                {
                    const auto weight = 1u + (random & 1023u);
                    if constexpr (std::integral<Weight>)
                    {
                        graph.add_edge(node, target, static_cast<Weight>(weight * 1000u));
                    }
                    else
                    {
                        graph.add_edge(node, target, static_cast<Weight>(weight));
                    }
                }
            }
        }
        return graph;
    }

    template <Query Operation, typename Graph>
    void run_queries(benchmark::State &state, const Graph &graph)
    {
        const auto size = graph.node_count();
        NodeId origin = 0;
        for (auto _ : state)
        {
            if constexpr (Operation == Query::PointToPoint)
            {
                auto result = shortest_path(graph, origin, (origin + size / 2) % size);
                benchmark::DoNotOptimize(result);
            }
            else if constexpr (Operation == Query::Targets)
            {
                std::array<NodeId, 8> targets;
                for (std::size_t i = 0; i < targets.size(); ++i)
                {
                    targets[i] = (origin + (i + 1) * size / (targets.size() + 1)) % size;
                }
                auto result = shortest_paths_to(graph, origin, targets);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                auto result = shortest_paths(graph, origin);
                benchmark::DoNotOptimize(result);
            }
            origin = (origin + 1) % size;
        }
    }

    template <Layout Storage, Query Operation, typename Weight>
    void bench_graph(benchmark::State &state)
    {
        const auto size = static_cast<std::size_t>(state.range(0));
        auto graph = make_graph<Weight>(size, static_cast<std::uint32_t>(state.range(1)));
        if constexpr (Operation == Query::EditAndQuery)
        {
            const std::vector<WeightedEdge<Weight>> edges(graph.outgoing(0).begin(), graph.outgoing(0).end());
            const auto run_edits = [&](auto &editable)
            {
                std::size_t index = 0;
                for (auto _ : state)
                {
                    const auto &edge = edges[index];
                    const auto set_weight = [&](Weight weight)
                    {
                        if constexpr (Storage == Layout::Dense)
                        {
                            editable.set_edge_weight(0, edge.target, weight);
                        }
                        else
                        {
                            editable.set_weight(0, edge.id, weight);
                        }
                    };
                    set_weight(edge.weight + Weight{1});
                    if constexpr (Storage == Layout::Packed)
                    {
                        const PackedWeightedGraph snapshot(editable);
                        auto result = shortest_path(snapshot, 0, size / 2);
                        benchmark::DoNotOptimize(result);
                    }
                    else
                    {
                        auto result = shortest_path(editable, 0, size / 2);
                        benchmark::DoNotOptimize(result);
                    }
                    set_weight(edge.weight);
                    index = (index + 1) % edges.size();
                }
            };
            if constexpr (Storage == Layout::Dense)
            {
                DenseWeightedGraph editable(graph);
                run_edits(editable);
            }
            else
            {
                run_edits(graph);
            }
        }
        else if constexpr (Storage == Layout::Packed)
        {
            run_queries<Operation>(state, PackedWeightedGraph{graph});
        }
        else if constexpr (Storage == Layout::Dense)
        {
            run_queries<Operation>(state, DenseWeightedGraph{graph});
        }
        else
        {
            run_queries<Operation>(state, graph);
        }
    }

    void graph_sizes(benchmark::Benchmark *benchmark)
    {
        benchmark->Args({256, 35})->Args({256, 80})->Args({2048, 35})->Args({2048, 80});
    }

    void edit_sizes(benchmark::Benchmark *benchmark)
    {
        benchmark->Args({256, 35})->Args({256, 80});
    }

    template <bool MinimumCost>
    void bench_flow(benchmark::State &state)
    {
        const auto width = static_cast<std::size_t>(state.range(0));
        const std::size_t sink = width * 2 + 1;
        FlowNetwork network(sink + 1);
        for (std::size_t i = 0; i < width; ++i)
        {
            network.add_arc(0, i + 1, 8);
            network.add_arc(i + width + 1, sink, 7);
            for (std::size_t j = 0; j < width; ++j)
            {
                network.add_arc(i + 1, j + width + 1, 3, static_cast<FlowValue>((i * 67 + j * 37) % 17));
            }
        }
        for (auto _ : state)
        {
            if constexpr (MinimumCost)
            {
                auto result = min_cost_flow(network, 0, sink);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                auto result = max_flow(network, 0, sink);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK(bench_graph<Layout::Adjacency, Query::PointToPoint, double>)->Name("bm_graph/point/adjacency")->Apply(graph_sizes);
    BENCHMARK(bench_graph<Layout::Packed, Query::PointToPoint, double>)->Name("bm_graph/point/packed")->Apply(graph_sizes);
    BENCHMARK(bench_graph<Layout::Dense, Query::PointToPoint, double>)->Name("bm_graph/point/dense")->Apply(graph_sizes);
    BENCHMARK(bench_graph<Layout::Adjacency, Query::Targets, double>)->Name("bm_graph/targets/adjacency")->Apply(graph_sizes);
    BENCHMARK(bench_graph<Layout::Packed, Query::Targets, double>)->Name("bm_graph/targets/packed")->Apply(graph_sizes);
    BENCHMARK(bench_graph<Layout::Dense, Query::Targets, double>)->Name("bm_graph/targets/dense")->Apply(graph_sizes);
    BENCHMARK(bench_graph<Layout::Adjacency, Query::SingleSource, double>)->Name("bm_graph/source/adjacency")->Apply(graph_sizes);
    BENCHMARK(bench_graph<Layout::Packed, Query::SingleSource, double>)->Name("bm_graph/source/packed")->Apply(graph_sizes);
    BENCHMARK(bench_graph<Layout::Dense, Query::SingleSource, double>)->Name("bm_graph/source/dense")->Apply(graph_sizes);
    BENCHMARK(bench_graph<Layout::Adjacency, Query::SingleSource, std::int64_t>)->Name("bm_graph/source/adjacency_i64")->Apply(graph_sizes);
    BENCHMARK(bench_graph<Layout::Adjacency, Query::EditAndQuery, double>)->Name("bm_graph/edit/adjacency")->Apply(edit_sizes);
    BENCHMARK(bench_graph<Layout::Packed, Query::EditAndQuery, double>)->Name("bm_graph/edit/packed")->Apply(edit_sizes);
    BENCHMARK(bench_graph<Layout::Dense, Query::EditAndQuery, double>)->Name("bm_graph/edit/dense")->Apply(edit_sizes);
    BENCHMARK(bench_flow<false>)->Name("bm_flow/max")->Arg(16)->Arg(64);
    BENCHMARK(bench_flow<true>)->Name("bm_flow/min_cost")->Arg(16)->Arg(64);
}
