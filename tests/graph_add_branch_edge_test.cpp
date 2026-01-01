/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "eino/compose/graph.h"
#include "eino/compose/branch_node.h"
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <any>
#include <memory>

using namespace eino;
using namespace eino::compose;

// Simple test runnable for graph testing
class SimpleMapRunnable : public Runnable<std::map<std::string, std::any>, std::map<std::string, std::any>> {
public:
    std::map<std::string, std::any> Invoke(
        std::shared_ptr<Context> ctx,
        const std::map<std::string, std::any>& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        return input;  // Pass through
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(std::map<std::string, std::any>);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(std::map<std::string, std::any>);
    }
};

/**
 * Test: AddBranchEdge basic functionality
 */
TEST(GraphAddBranchEdgeTest, BasicAddBranchEdge) {
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;
    
    // Add nodes
    auto branch_node = std::make_shared<SimpleMapRunnable>();
    auto target1 = std::make_shared<SimpleMapRunnable>();
    auto target2 = std::make_shared<SimpleMapRunnable>();
    
    graph.AddNode("branch_decision", branch_node);
    graph.AddNode("target_1", target1);
    graph.AddNode("target_2", target2);
    
    // Add branch edges
    EXPECT_NO_THROW(graph.AddBranchEdge("branch_decision", 0, "target_1"));
    EXPECT_NO_THROW(graph.AddBranchEdge("branch_decision", 1, "target_2"));
    
    // Verify branch edges were stored
    auto branch_edges = graph.GetBranchEdges("branch_decision");
    EXPECT_EQ(branch_edges.size(), 2);
    EXPECT_EQ(branch_edges[0], "target_1");
    EXPECT_EQ(branch_edges[1], "target_2");
}

/**
 * Test: AddBranchEdge with missing branch node
 */
TEST(GraphAddBranchEdgeTest, MissingBranchNode) {
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;
    
    auto target = std::make_shared<SimpleMapRunnable>();
    graph.AddNode("target", target);
    
    // Try to add branch edge with non-existent branch node
    EXPECT_THROW({
        graph.AddBranchEdge("non_existent_branch", 0, "target");
    }, std::runtime_error);
}

/**
 * Test: AddBranchEdge with missing target node
 */
TEST(GraphAddBranchEdgeTest, MissingTargetNode) {
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;
    
    auto branch_node = std::make_shared<SimpleMapRunnable>();
    graph.AddNode("branch", branch_node);
    
    // Try to add branch edge with non-existent target node
    EXPECT_THROW({
        graph.AddBranchEdge("branch", 0, "non_existent_target");
    }, std::runtime_error);
}

/**
 * Test: AddBranchEdge with END_NODE as target
 */
TEST(GraphAddBranchEdgeTest, BranchToEndNode) {
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;
    
    auto branch_node = std::make_shared<SimpleMapRunnable>();
    graph.AddNode("branch", branch_node);
    
    // Should allow branching directly to END_NODE
    EXPECT_NO_THROW(graph.AddBranchEdge("branch", 0, Graph<std::map<std::string, std::any>, std::map<std::string, std::any>>::END_NODE));
    
    auto target = graph.GetBranchTarget("branch", 0);
    EXPECT_EQ(target, Graph<std::map<std::string, std::any>, std::map<std::string, std::any>>::END_NODE);
}

/**
 * Test: AddBranchEdge with invalid branch index
 */
TEST(GraphAddBranchEdgeTest, NegativeBranchIndex) {
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;
    
    auto branch_node = std::make_shared<SimpleMapRunnable>();
    auto target = std::make_shared<SimpleMapRunnable>();
    
    graph.AddNode("branch", branch_node);
    graph.AddNode("target", target);
    
    // Negative branch index should throw
    EXPECT_THROW({
        graph.AddBranchEdge("branch", -1, "target");
    }, std::invalid_argument);
}

/**
 * Test: AddBranchEdge with empty names
 */
TEST(GraphAddBranchEdgeTest, EmptyNames) {
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;
    
    auto branch_node = std::make_shared<SimpleMapRunnable>();
    auto target = std::make_shared<SimpleMapRunnable>();
    
    graph.AddNode("branch", branch_node);
    graph.AddNode("target", target);
    
    // Empty branch node name
    EXPECT_THROW({
        graph.AddBranchEdge("", 0, "target");
    }, std::invalid_argument);
    
    // Empty target node name
    EXPECT_THROW({
        graph.AddBranchEdge("branch", 0, "");
    }, std::invalid_argument);
}

/**
 * Test: AddBranchEdge after compilation
 */
TEST(GraphAddBranchEdgeTest, AddAfterCompilation) {
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;
    
    auto branch_node = std::make_shared<SimpleMapRunnable>();
    auto target = std::make_shared<SimpleMapRunnable>();
    
    graph.AddNode("branch", branch_node);
    graph.AddNode("target", target);
    
    graph.AddEdge(Graph<std::map<std::string, std::any>, std::map<std::string, std::any>>::START_NODE, "branch");
    graph.AddEdge("branch", Graph<std::map<std::string, std::any>, std::map<std::string, std::any>>::END_NODE);
    
    // Compile the graph
    graph.Compile();
    
    // Try to add branch edge after compilation
    EXPECT_THROW({
        graph.AddBranchEdge("branch", 0, "target");
    }, std::runtime_error);
}

/**
 * Test: Multiple branch edges from same node
 */
TEST(GraphAddBranchEdgeTest, MultipleBranchEdges) {
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;
    
    auto branch_node = std::make_shared<SimpleMapRunnable>();
    auto target1 = std::make_shared<SimpleMapRunnable>();
    auto target2 = std::make_shared<SimpleMapRunnable>();
    auto target3 = std::make_shared<SimpleMapRunnable>();
    
    graph.AddNode("branch", branch_node);
    graph.AddNode("target_1", target1);
    graph.AddNode("target_2", target2);
    graph.AddNode("target_3", target3);
    
    // Add multiple branch edges
    graph.AddBranchEdge("branch", 0, "target_1");
    graph.AddBranchEdge("branch", 1, "target_2");
    graph.AddBranchEdge("branch", 2, "target_3");
    
    // Verify all branch edges
    EXPECT_EQ(graph.GetBranchTarget("branch", 0), "target_1");
    EXPECT_EQ(graph.GetBranchTarget("branch", 1), "target_2");
    EXPECT_EQ(graph.GetBranchTarget("branch", 2), "target_3");
    
    auto all_edges = graph.GetBranchEdges("branch");
    EXPECT_EQ(all_edges.size(), 3);
}

/**
 * Test: GetBranchTarget with non-existent branch
 */
TEST(GraphAddBranchEdgeTest, GetNonExistentBranchTarget) {
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;
    
    auto branch_node = std::make_shared<SimpleMapRunnable>();
    auto target = std::make_shared<SimpleMapRunnable>();
    
    graph.AddNode("branch", branch_node);
    graph.AddNode("target", target);
    
    graph.AddBranchEdge("branch", 0, "target");
    
    // Query non-existent branch index
    EXPECT_EQ(graph.GetBranchTarget("branch", 999), "");
    
    // Query non-existent branch node
    EXPECT_EQ(graph.GetBranchTarget("non_existent", 0), "");
}

/**
 * Test: GetBranchEdges with non-existent node
 */
TEST(GraphAddBranchEdgeTest, GetNonExistentBranchEdges) {
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;
    
    auto edges = graph.GetBranchEdges("non_existent");
    EXPECT_TRUE(edges.empty());
}
