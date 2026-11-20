#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "math/graphs/network-flow.hpp"
#include "math/graphs/shortest-path.hpp"
#include "math/graphs/weighted-graph.hpp"

namespace math::tests
{
    TEST(WeightedShortestPath, QueryFormsAndLayoutsAgreeOnParallelEdgesAndTies)
    {
        WeightedGraph<int> graph(6);
        graph.add_edge(0, 2, 1);
        const auto first = graph.add_edge(0, 1, 1);
        graph.add_edge(0, 1, 1);
        graph.add_edge(1, 3, 7);
        const auto second = graph.add_edge(1, 3, 2);
        graph.add_edge(2, 3, 2);
        graph.add_edge(3, 1, 0);
        graph.add_edge(3, 4, 0);
        graph.add_edge(4, 3, 0);

        const auto verify = [&](const auto &layout)
        {
            const auto path = shortest_path(layout, 0, 3);
            ASSERT_TRUE(path);
            EXPECT_EQ(path->distance, 3);
            EXPECT_EQ(path->nodes, (std::vector<NodeId>{0, 1, 3}));
            EXPECT_EQ(path->edges, (std::vector<EdgeId>{first, second}));
            const auto all = shortest_paths(layout, 0);
            EXPECT_EQ(all.distance_to(4), 3);
            EXPECT_EQ(all.path_to(3)->edges, path->edges);
            EXPECT_FALSE(all.path_to(5));
            const std::array<NodeId, 4> requested{4, 5, 4, 0};
            const auto results = shortest_paths_to(layout, 0, requested);
            ASSERT_EQ(results.size(), requested.size());
            ASSERT_TRUE(results[0] && results[2] && results[3]);
            EXPECT_EQ(results[0]->nodes, (std::vector<NodeId>{0, 1, 3, 4}));
            EXPECT_EQ(results[2]->nodes, results[0]->nodes);
            EXPECT_FALSE(results[1]);
            EXPECT_EQ(results[3]->distance, 0);
            EXPECT_TRUE(results[3]->edges.empty());
        };

        verify(graph);
        verify(PackedWeightedGraph{graph});
        verify(DenseWeightedGraph{graph});
    }

    TEST(WeightedShortestPath, DynamicEditsPreserveIdsAndPriorSnapshots)
    {
        WeightedGraph<double> graph(3);
        const auto direct = graph.add_edge(0, 2, 5.0);
        const auto first = graph.add_edge(0, 1, 2.0);
        graph.add_edge(1, 2, 2.0);
        const PackedWeightedGraph snapshot(graph);
        const auto earlier = shortest_paths(graph, 0);

        ASSERT_TRUE(graph.set_weight(0, direct, 1.0));
        EXPECT_EQ(shortest_path(graph, 0, 2)->edges, (std::vector<EdgeId>{direct}));
        graph.remove_node(1);
        EXPECT_FALSE(graph.remove_edge(0, first));
        EXPECT_EQ(graph.add_node(), 3u);
        EXPECT_EQ(shortest_paths(graph, 0).distance_to(2), 1.0);
        EXPECT_FALSE(shortest_paths(graph, 0).distance_to(1));
        EXPECT_EQ(earlier.distance_to(2), 4.0);
        EXPECT_EQ(shortest_paths(snapshot, 0).distance_to(2), 4.0);
        EXPECT_THROW((void)shortest_path(graph, 0, 1), std::out_of_range);
        EXPECT_THROW((void)shortest_paths(graph, 1), std::out_of_range);
        EXPECT_THROW(graph.add_edge(1, 3, 1.0), std::out_of_range);
    }

    TEST(WeightedShortestPath, DenseEdgesCanBeEditedWithoutRebuildingTheGraph)
    {
        DenseWeightedGraph<double> graph(2);
        const auto id = graph.add_edge(0, 1, 3.0);
        EXPECT_THROW(graph.add_edge(0, 1, 1.0), std::invalid_argument);
        EXPECT_THROW(graph.set_edge_weight(0, 1, -1.0), std::invalid_argument);
        EXPECT_THROW(graph.set_edge_weight(0, 2, 1.0), std::out_of_range);
        EXPECT_TRUE(graph.set_edge_weight(0, 1, 2.0));
        const auto changed = shortest_path(graph, 0, 1);
        ASSERT_TRUE(changed);
        EXPECT_EQ(changed->distance, 2.0);
        EXPECT_EQ(changed->edges.front(), id);
        EXPECT_TRUE(graph.erase_edge(0, 1));
        EXPECT_FALSE(graph.set_edge_weight(0, 1, 1.0));
        EXPECT_FALSE(shortest_path(graph, 0, 1));
        EXPECT_NE(graph.add_edge(0, 1, 1.0), id);
    }

    TEST(WeightedShortestPath, FloatingPointParallelEdgesChooseTheLowerWeightAfterRounding)
    {
        WeightedGraph<double> graph(3);
        graph.add_edge(0, 1, 0x1p54);
        graph.add_edge(1, 2, 1.0);
        const auto cheaper = graph.add_edge(1, 2, 0.5);
        EXPECT_EQ(shortest_path(graph, 0, 2)->edges.back(), cheaper);
        EXPECT_EQ(shortest_path(DenseWeightedGraph{graph}, 0, 2)->edges.back(), cheaper);
    }

    TEST(WeightedShortestPath, RejectsInvalidWeightsAndChecksPathOverflow)
    {
        WeightedGraph<double> graph(2);
        EXPECT_THROW(graph.add_edge(0, 1, -1.0), std::invalid_argument);
        EXPECT_THROW(graph.add_edge(0, 1, std::numeric_limits<double>::infinity()), std::invalid_argument);
        EXPECT_THROW(graph.add_edge(0, 1, std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);

        WeightedGraph<std::uint64_t> integers(5);
        constexpr auto limit = std::numeric_limits<std::uint64_t>::max();
        integers.add_edge(0, 1, limit);
        integers.add_edge(1, 2, 1);
        integers.add_edge(2, 3, 1);
        EXPECT_EQ(shortest_path(integers, 0, 1)->distance, limit);
        const auto paths = shortest_paths(integers, 0);
        EXPECT_THROW((void)paths.distance_to(2), std::overflow_error);
        EXPECT_THROW((void)paths.path_to(3), std::overflow_error);
        EXPECT_FALSE(paths.distance_to(4));

        integers.add_edge(0, 4, limit);
        integers.add_edge(4, 2, 0);
        EXPECT_EQ(shortest_path(integers, 0, 2)->distance, limit);
        EXPECT_THROW((void)shortest_path(integers, 0, 3), std::overflow_error);
    }

    TEST(WeightedShortestPath, LayoutsMatchAllPairsReferenceAcrossGeneratedGraphs)
    {
        constexpr std::size_t count = 6;
        constexpr int absent = 1000;
        std::uint32_t generator = 1;
        for (int trial = 0; trial < 24; ++trial)
        {
            WeightedGraph<int> graph(count);
            std::array<std::array<int, count>, count> distance{};
            for (NodeId source = 0; source < count; ++source)
            {
                for (NodeId target = 0; target < count; ++target)
                {
                    generator = 1664525u * generator + 1013904223u;
                    distance[source][target] = source == target ? 0 : absent;
                    if (source != target && (generator >> 24) % 3 != 0)
                    {
                        const auto weight = static_cast<int>((generator >> 16) % 9);
                        graph.add_edge(source, target, weight);
                        distance[source][target] = weight;
                    }
                }
            }
            for (NodeId middle = 0; middle < count; ++middle)
            {
                for (NodeId source = 0; source < count; ++source)
                {
                    for (NodeId target = 0; target < count; ++target)
                    {
                        distance[source][target] = std::min(distance[source][target], distance[source][middle] + distance[middle][target]);
                    }
                }
            }
            const PackedWeightedGraph packed(graph);
            const DenseWeightedGraph dense(graph);
            for (NodeId source = 0; source < count; ++source)
            {
                const auto mutable_paths = shortest_paths(graph, source);
                const auto packed_paths = shortest_paths(packed, source);
                const auto dense_paths = shortest_paths(dense, source);
                for (NodeId target = 0; target < count; ++target)
                {
                    EXPECT_EQ(mutable_paths.distance_to(target).value_or(absent), distance[source][target]);
                    EXPECT_EQ(packed_paths.distance_to(target), mutable_paths.distance_to(target));
                    EXPECT_EQ(dense_paths.distance_to(target), mutable_paths.distance_to(target));
                }
            }
        }
    }

    TEST(NetworkFlow, MaximumFlowRespectsParallelArcsAndLimits)
    {
        FlowNetwork graph(4);
        graph.add_arc(0, 1, 2);
        graph.add_arc(0, 1, 1);
        graph.add_arc(0, 2, 2);
        graph.add_arc(1, 3, 2);
        graph.add_arc(1, 2, 1);
        graph.add_arc(2, 3, 3);
        graph.add_arc(2, 2, 5);
        const auto result = max_flow(graph, 0, 3);
        EXPECT_EQ(result.value, 5);
        EXPECT_EQ(result.arc_flows, (std::vector<FlowValue>{2, 1, 2, 2, 1, 3, 0}));
        EXPECT_EQ(max_flow(graph, 0, 3, 3).value, 3);
        EXPECT_EQ(max_flow(graph, 3, 0).value, 0);
    }

    TEST(NetworkFlow, MinimumCostUsesReverseArcToImproveEarlierAllocation)
    {
        FlowNetwork graph(6);
        graph.add_arc(0, 1, 1);
        graph.add_arc(0, 2, 1);
        const auto initially_used = graph.add_arc(1, 3, 1, 2);
        graph.add_arc(1, 4, 1, 3);
        graph.add_arc(2, 3, 1, 3);
        graph.add_arc(3, 5, 1);
        graph.add_arc(4, 5, 1);
        EXPECT_EQ(min_cost_flow(graph, 0, 5, 1).cost, 2);
        const auto result = min_cost_flow(graph, 0, 5);
        EXPECT_EQ(result.flow.value, 2);
        EXPECT_EQ(result.cost, 6);
        EXPECT_EQ(result.flow.arc_flows[initially_used], 0);
        EXPECT_EQ(result.flow.arc_flows, (std::vector<FlowValue>{1, 1, 0, 1, 1, 1, 1}));
    }

    TEST(NetworkFlow, ValidatesInputAndChecksCostOverflow)
    {
        FlowNetwork graph(2);
        EXPECT_THROW(graph.add_arc(0, 2, 1), std::out_of_range);
        EXPECT_THROW(graph.add_arc(0, 1, -1), std::invalid_argument);
        EXPECT_THROW(graph.add_arc(0, 1, 1, -1), std::invalid_argument);
        EXPECT_THROW((void)max_flow(graph, 0, 0), std::invalid_argument);
        EXPECT_THROW((void)min_cost_flow(graph, 0, 1, -1), std::invalid_argument);
        graph.add_arc(0, 1, 2, std::numeric_limits<FlowValue>::max());
        EXPECT_EQ(min_cost_flow(graph, 0, 1, 1).cost, std::numeric_limits<FlowValue>::max());
        EXPECT_THROW((void)min_cost_flow(graph, 0, 1), std::overflow_error);
    }

    TEST(NetworkFlow, BothSolversMatchExhaustiveSmallNetworks)
    {
        constexpr std::array endpoints{
            std::array{0u, 1u}, std::array{0u, 2u}, std::array{1u, 2u},
            std::array{2u, 1u}, std::array{1u, 3u}, std::array{2u, 3u}, std::array{0u, 3u}};
        std::uint32_t random = 4;
        for (int trial = 0; trial < 20; ++trial)
        {
            FlowNetwork network(4);
            for (const auto &[from, to] : endpoints)
            {
                random = random * 1664525u + 1013904223u;
                network.add_arc(from, to, (random >> 16) % 3, (random >> 24) % 7);
            }
            const auto capacity_limit = static_cast<FlowValue>(trial % 5);
            std::array<FlowValue, 4> net{};
            FlowValue best = 0;
            FlowValue cheapest = 0;
            const auto enumerate = [&](auto &&self, std::size_t index, FlowValue cost) -> void
            {
                if (index == network.arcs().size())
                {
                    if (net[1] == 0 && net[2] == 0 && net[0] <= capacity_limit &&
                        (net[0] > best || (net[0] == best && cost < cheapest)))
                    {
                        best = net[0];
                        cheapest = cost;
                    }
                    return;
                }
                const auto &arc = network.arcs()[index];
                for (FlowValue amount = 0; amount <= arc.capacity; ++amount)
                {
                    net[arc.source] += amount;
                    net[arc.target] -= amount;
                    self(self, index + 1, cost + amount * arc.cost);
                    net[arc.source] -= amount;
                    net[arc.target] += amount;
                }
            };
            enumerate(enumerate, 0, 0);
            EXPECT_EQ(max_flow(network, 0, 3, capacity_limit).value, best);
            const auto result = min_cost_flow(network, 0, 3, capacity_limit);
            EXPECT_EQ(result.flow.value, best);
            EXPECT_EQ(result.cost, cheapest);
        }
    }
}
