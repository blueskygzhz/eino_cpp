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

/**
 * Test: BranchNode accessing multiple predecessor node outputs in a Graph
 * 
 * Scenario: A -> B -> C (BranchNode)
 * - Node A outputs user info
 * - Node B outputs score info
 * - BranchNode C needs to access both A and B outputs for decision making
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

using MapType = std::map<std::string, std::any>;

// ============================================================================
// Test Runnables
// ============================================================================

// Node A: Outputs user information
class UserInfoNode : public Runnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        MapType output;
        output["age"] = static_cast<int64_t>(25);
        output["name"] = std::string("Alice");
        output["city"] = std::string("Beijing");
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
};

// Node B: Outputs score information
class ScoreNode : public Runnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        MapType output;
        output["score"] = static_cast<int64_t>(85);
        output["level"] = std::string("gold");
        output["vip"] = true;
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
};

// Service handlers
class VIPServiceNode : public Runnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        MapType output = input;
        output["service"] = std::string("VIP");
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
};

class StandardServiceNode : public Runnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        MapType output = input;
        output["service"] = std::string("Standard");
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
};

// ============================================================================
// Test Cases
// ============================================================================

/**
 * Test: BranchNode with references to multiple predecessor nodes
 * Graph: A -> B -> BranchNode
 * BranchNode uses both A's and B's outputs
 */
TEST(GraphBranchNodeMultiReferenceTest, BasicMultiNodeReference) {
    Graph<MapType, MapType> graph;
    
    // Add node A (user info)
    auto node_a = std::make_shared<UserInfoNode>();
    graph.AddNode("node_a", node_a);
    
    // Add node B (score info)
    auto node_b = std::make_shared<ScoreNode>();
    graph.AddNode("node_b", node_b);
    
    // Configure BranchNode with references to both node_a and node_b
    BranchNodeConfig branch_config;
    
    // Condition 0: node_a.age >= 18 AND node_b.score >= 80
    branch_config.AddMultiConditionWithOperands({
        SingleClauseConfig(
            BranchOperator::GreaterOrEqual,
            OperandConfig::FromNode("node_a", {"age"}),
            OperandConfig::FromLiteral(static_cast<int64_t>(18))
        ),
        SingleClauseConfig(
            BranchOperator::GreaterOrEqual,
            OperandConfig::FromNode("node_b", {"score"}),
            OperandConfig::FromLiteral(static_cast<int64_t>(80))
        )
    }, ClauseRelation::AND);
    
    // Condition 1: node_a.age >= 18 (but score < 80)
    branch_config.AddConditionWithOperands(
        BranchOperator::GreaterOrEqual,
        OperandConfig::FromNode("node_a", {"age"}),
        OperandConfig::FromLiteral(static_cast<int64_t>(18))
    );
    
    graph.AddBranchNode("branch_decision", branch_config);
    
    // Add service handlers
    auto vip_service = std::make_shared<VIPServiceNode>();
    auto standard_service = std::make_shared<StandardServiceNode>();
    
    graph.AddNode("vip_service", vip_service);
    graph.AddNode("standard_service", standard_service);
    
    // Connect nodes: START -> node_a -> node_b -> branch_decision
    graph.AddEdge(Graph<MapType, MapType>::START_NODE, "node_a");
    graph.AddEdge("node_a", "node_b");
    graph.AddEdge("node_b", "branch_decision");
    
    // Add branch edges
    graph.AddBranchEdge("branch_decision", 0, "vip_service");       // age >= 18 AND score >= 80
    graph.AddBranchEdge("branch_decision", 1, "standard_service");  // age >= 18 (default)
    
    // Connect to END
    graph.AddEdge("vip_service", Graph<MapType, MapType>::END_NODE);
    graph.AddEdge("standard_service", Graph<MapType, MapType>::END_NODE);
    
    // Compile and execute
    EXPECT_NO_THROW(graph.Compile());
    
    MapType input = {{"user_id", 123}};
    MapType result;
    
    EXPECT_NO_THROW({
        result = graph.Invoke(Context::Background(), input);
    });
    
    // Verify: should route to VIP service (age=25 >= 18 AND score=85 >= 80)
    EXPECT_TRUE(result.count("service"));
    EXPECT_EQ(std::any_cast<std::string>(result.at("service")), "VIP");
}

/**
 * Test: BranchNode accessing single field from each of multiple nodes
 */
TEST(GraphBranchNodeMultiReferenceTest, SingleFieldFromMultipleNodes) {
    Graph<MapType, MapType> graph;
    
    auto node_a = std::make_shared<UserInfoNode>();
    auto node_b = std::make_shared<ScoreNode>();
    
    graph.AddNode("node_a", node_a);
    graph.AddNode("node_b", node_b);
    
    // Condition: node_a.age >= 20 OR node_b.vip == true
    BranchNodeConfig branch_config;
    branch_config.AddMultiConditionWithOperands({
        SingleClauseConfig(
            BranchOperator::GreaterOrEqual,
            OperandConfig::FromNode("node_a", {"age"}),
            OperandConfig::FromLiteral(static_cast<int64_t>(20))
        ),
        SingleClauseConfig(
            BranchOperator::IsTrue,
            OperandConfig::FromNode("node_b", {"vip"}),
            OperandConfig()  // IsTrue is unary
        )
    }, ClauseRelation::OR);
    
    graph.AddBranchNode("branch_decision", branch_config);
    
    auto vip_service = std::make_shared<VIPServiceNode>();
    auto standard_service = std::make_shared<StandardServiceNode>();
    
    graph.AddNode("vip_service", vip_service);
    graph.AddNode("standard_service", standard_service);
    
    graph.AddEdge(Graph<MapType, MapType>::START_NODE, "node_a");
    graph.AddEdge("node_a", "node_b");
    graph.AddEdge("node_b", "branch_decision");
    
    graph.AddBranchEdge("branch_decision", 0, "vip_service");
    graph.AddBranchEdge("branch_decision", 1, "standard_service");
    
    graph.AddEdge("vip_service", Graph<MapType, MapType>::END_NODE);
    graph.AddEdge("standard_service", Graph<MapType, MapType>::END_NODE);
    
    graph.Compile();
    
    MapType input = {};
    auto result = graph.Invoke(Context::Background(), input);
    
    // Should match: age=25 >= 20 OR vip=true (both true)
    EXPECT_TRUE(result.count("service"));
    EXPECT_EQ(std::any_cast<std::string>(result.at("service")), "VIP");
}

/**
 * Test: BranchNode with nested field access from multiple nodes
 */
TEST(GraphBranchNodeMultiReferenceTest, NestedFieldAccess) {
    // This test would require nodes that output nested structures
    // For now, we test the basic capability
    SUCCEED() << "Nested field access test placeholder";
}

/**
 * Test: Multiple BranchNodes in sequence, each referencing different nodes
 */
TEST(GraphBranchNodeMultiReferenceTest, SequentialBranchNodes) {
    Graph<MapType, MapType> graph;
    
    auto node_a = std::make_shared<UserInfoNode>();
    auto node_b = std::make_shared<ScoreNode>();
    
    graph.AddNode("node_a", node_a);
    graph.AddNode("node_b", node_b);
    
    // First BranchNode: checks node_a
    BranchNodeConfig config1;
    config1.AddConditionWithOperands(
        BranchOperator::GreaterOrEqual,
        OperandConfig::FromNode("node_a", {"age"}),
        OperandConfig::FromLiteral(static_cast<int64_t>(18))
    );
    graph.AddBranchNode("branch1", config1);
    
    // Second BranchNode: checks node_b
    BranchNodeConfig config2;
    config2.AddConditionWithOperands(
        BranchOperator::GreaterOrEqual,
        OperandConfig::FromNode("node_b", {"score"}),
        OperandConfig::FromLiteral(static_cast<int64_t>(80))
    );
    graph.AddBranchNode("branch2", config2);
    
    auto vip_service = std::make_shared<VIPServiceNode>();
    auto standard_service = std::make_shared<StandardServiceNode>();
    
    graph.AddNode("vip_service", vip_service);
    graph.AddNode("standard_service", standard_service);
    
    // Connect: node_a -> node_b -> branch1 -> branch2 -> services
    graph.AddEdge(Graph<MapType, MapType>::START_NODE, "node_a");
    graph.AddEdge("node_a", "node_b");
    graph.AddEdge("node_b", "branch1");
    
    // branch1 routes
    graph.AddBranchEdge("branch1", 0, "branch2");        // age >= 18 -> check score
    graph.AddBranchEdge("branch1", 1, "standard_service"); // age < 18 -> standard
    
    // branch2 routes
    graph.AddBranchEdge("branch2", 0, "vip_service");      // score >= 80 -> VIP
    graph.AddBranchEdge("branch2", 1, "standard_service"); // score < 80 -> standard
    
    graph.AddEdge("vip_service", Graph<MapType, MapType>::END_NODE);
    graph.AddEdge("standard_service", Graph<MapType, MapType>::END_NODE);
    
    EXPECT_NO_THROW(graph.Compile());
    
    MapType input = {};
    auto result = graph.Invoke(Context::Background(), input);
    
    // Should route through both branches to VIP service
    EXPECT_TRUE(result.count("service"));
    EXPECT_EQ(std::any_cast<std::string>(result.at("service")), "VIP");
}

/**
 * Test: Verify BranchNode can access START node input
 */
TEST(GraphBranchNodeMultiReferenceTest, AccessStartNodeInput) {
    Graph<MapType, MapType> graph;
    
    // BranchNode directly after START, referencing START input
    BranchNodeConfig branch_config;
    branch_config.AddConditionWithOperands(
        BranchOperator::GreaterOrEqual,
        OperandConfig::FromNode("__START__", {"user_age"}),
        OperandConfig::FromLiteral(static_cast<int64_t>(18))
    );
    
    graph.AddBranchNode("branch_decision", branch_config);
    
    auto vip_service = std::make_shared<VIPServiceNode>();
    auto standard_service = std::make_shared<StandardServiceNode>();
    
    graph.AddNode("vip_service", vip_service);
    graph.AddNode("standard_service", standard_service);
    
    graph.AddEdge(Graph<MapType, MapType>::START_NODE, "branch_decision");
    graph.AddBranchEdge("branch_decision", 0, "vip_service");
    graph.AddBranchEdge("branch_decision", 1, "standard_service");
    
    graph.AddEdge("vip_service", Graph<MapType, MapType>::END_NODE);
    graph.AddEdge("standard_service", Graph<MapType, MapType>::END_NODE);
    
    graph.Compile();
    
    // Input with user_age field
    MapType input = {
        {"user_age", static_cast<int64_t>(25)},
        {"user_name", std::string("Bob")}
    };
    
    auto result = graph.Invoke(Context::Background(), input);
    
    // Should route to VIP (age 25 >= 18)
    EXPECT_TRUE(result.count("service"));
    EXPECT_EQ(std::any_cast<std::string>(result.at("service")), "VIP");
}
