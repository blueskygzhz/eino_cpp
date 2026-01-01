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
 * Complete example demonstrating AddBranchEdge usage in a Graph
 * 
 * Scenario: User service routing based on user attributes
 * - VIP users (score >= 80) -> VIP Service
 * - Standard users (score >= 50 && score < 80) -> Standard Service  
 * - Guest users (score < 50) -> Guest Service
 */

#include "eino/compose/graph.h"
#include "eino/compose/branch_node.h"
#include <iostream>
#include <map>
#include <string>
#include <any>
#include <memory>

using namespace eino;
using namespace eino::compose;

// ============================================================================
// Helper Runnables for demonstration
// ============================================================================

// Runnable that processes user input and outputs user score
class UserScoreEvaluator : public Runnable<std::map<std::string, std::any>, std::map<std::string, std::any>> {
public:
    std::map<std::string, std::any> Invoke(
        std::shared_ptr<Context> ctx,
        const std::map<std::string, std::any>& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::cout << "[UserScoreEvaluator] Evaluating user..." << std::endl;
        
        std::map<std::string, std::any> output;
        
        // Extract user info from input
        if (input.count("user_name") && input.count("user_score")) {
            output["user_name"] = input.at("user_name");
            output["user_score"] = input.at("user_score");
            
            int64_t score = std::any_cast<int64_t>(input.at("user_score"));
            std::string name = std::any_cast<std::string>(input.at("user_name"));
            
            std::cout << "  User: " << name << ", Score: " << score << std::endl;
        }
        
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(std::map<std::string, std::any>);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(std::map<std::string, std::any>);
    }
};

// Service handler for VIP users
class VIPServiceHandler : public Runnable<std::map<std::string, std::any>, std::map<std::string, std::any>> {
public:
    std::map<std::string, std::any> Invoke(
        std::shared_ptr<Context> ctx,
        const std::map<std::string, std::any>& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::map<std::string, std::any> output = input;
        output["service_level"] = std::string("VIP");
        output["message"] = std::string("Welcome to VIP Service! Premium support available 24/7.");
        
        std::cout << "[VIPServiceHandler] Handling VIP user" << std::endl;
        
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(std::map<std::string, std::any>);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(std::map<std::string, std::any>);
    }
};

// Service handler for Standard users
class StandardServiceHandler : public Runnable<std::map<std::string, std::any>, std::map<std::string, std::any>> {
public:
    std::map<std::string, std::any> Invoke(
        std::shared_ptr<Context> ctx,
        const std::map<std::string, std::any>& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::map<std::string, std::any> output = input;
        output["service_level"] = std::string("Standard");
        output["message"] = std::string("Welcome to Standard Service! Regular support available.");
        
        std::cout << "[StandardServiceHandler] Handling Standard user" << std::endl;
        
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(std::map<std::string, std::any>);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(std::map<std::string, std::any>);
    }
};

// Service handler for Guest users
class GuestServiceHandler : public Runnable<std::map<std::string, std::any>, std::map<std::string, std::any>> {
public:
    std::map<std::string, std::any> Invoke(
        std::shared_ptr<Context> ctx,
        const std::map<std::string, std::any>& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::map<std::string, std::any> output = input;
        output["service_level"] = std::string("Guest");
        output["message"] = std::string("Welcome! Basic support available during business hours.");
        
        std::cout << "[GuestServiceHandler] Handling Guest user" << std::endl;
        
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(std::map<std::string, std::any>);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(std::map<std::string, std::any>);
    }
};

// ============================================================================
// Main Example
// ============================================================================

void RunCompleteBranchEdgeExample() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Complete AddBranchEdge Example" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // 1. Create Graph
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;
    
    // 2. Add evaluator node
    auto evaluator = std::make_shared<UserScoreEvaluator>();
    graph.AddNode("user_evaluator", evaluator);
    
    // 3. Create BranchNode with conditions
    // Branch logic:
    // - Branch 0: score >= 80 (VIP)
    // - Branch 1: score >= 50 (Standard)
    // - Branch 2: default (Guest - when no condition matches)
    
    BranchNodeConfig branch_config;
    
    // Condition 0: score >= 80
    branch_config.AddConditionWithOperands(
        BranchOperator::GreaterOrEqual,
        OperandConfig::FromNode("user_evaluator", {"user_score"}),
        OperandConfig::FromLiteral(static_cast<int64_t>(80))
    );
    
    // Condition 1: score >= 50
    branch_config.AddConditionWithOperands(
        BranchOperator::GreaterOrEqual,
        OperandConfig::FromNode("user_evaluator", {"user_score"}),
        OperandConfig::FromLiteral(static_cast<int64_t>(50))
    );
    
    // Add BranchNode to graph
    graph.AddBranchNode("service_router", branch_config);
    
    // 4. Add service handler nodes
    auto vip_service = std::make_shared<VIPServiceHandler>();
    auto standard_service = std::make_shared<StandardServiceHandler>();
    auto guest_service = std::make_shared<GuestServiceHandler>();
    
    graph.AddNode("vip_service", vip_service);
    graph.AddNode("standard_service", standard_service);
    graph.AddNode("guest_service", guest_service);
    
    // 5. Connect nodes with regular edges
    graph.AddEdge(Graph<std::map<std::string, std::any>, std::map<std::string, std::any>>::START_NODE, 
                  "user_evaluator");
    graph.AddEdge("user_evaluator", "service_router");
    
    // 6. Use AddBranchEdge to define branch routing
    std::cout << "Setting up branch routing:" << std::endl;
    std::cout << "  Branch 0 (score >= 80) -> VIP Service" << std::endl;
    std::cout << "  Branch 1 (score >= 50) -> Standard Service" << std::endl;
    std::cout << "  Branch 2 (default)     -> Guest Service" << std::endl;
    
    graph.AddBranchEdge("service_router", 0, "vip_service");
    graph.AddBranchEdge("service_router", 1, "standard_service");
    graph.AddBranchEdge("service_router", 2, "guest_service");
    
    // 7. Connect to END
    graph.AddEdge("vip_service", Graph<std::map<std::string, std::any>, std::map<std::string, std::any>>::END_NODE);
    graph.AddEdge("standard_service", Graph<std::map<std::string, std::any>, std::map<std::string, std::any>>::END_NODE);
    graph.AddEdge("guest_service", Graph<std::map<std::string, std::any>, std::map<std::string, std::any>>::END_NODE);
    
    // 8. Verify branch edges
    std::cout << "\nVerifying branch edges configuration:" << std::endl;
    auto branch_edges = graph.GetBranchEdges("service_router");
    std::cout << "  Total branch edges: " << branch_edges.size() << std::endl;
    for (const auto& [index, target] : branch_edges) {
        std::cout << "  Branch " << index << " -> " << target << std::endl;
    }
    
    // 9. Compile graph
    std::cout << "\nCompiling graph..." << std::endl;
    graph.Compile();
    std::cout << "Graph compiled successfully!" << std::endl;
    
    // 10. Test with different user scores
    std::cout << "\n========================================" << std::endl;
    std::cout << "Testing Branch Routing" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Test Case 1: VIP User (score >= 80)
    {
        std::cout << "\n--- Test Case 1: VIP User (score = 95) ---" << std::endl;
        std::map<std::string, std::any> input;
        input["user_name"] = std::string("Alice");
        input["user_score"] = static_cast<int64_t>(95);
        
        auto result = graph.Invoke(Context::Background(), input);
        
        if (result.count("service_level")) {
            std::cout << "Result: " << std::any_cast<std::string>(result["service_level"]) << std::endl;
            std::cout << "Message: " << std::any_cast<std::string>(result["message"]) << std::endl;
        }
    }
    
    // Test Case 2: Standard User (50 <= score < 80)
    {
        std::cout << "\n--- Test Case 2: Standard User (score = 65) ---" << std::endl;
        std::map<std::string, std::any> input;
        input["user_name"] = std::string("Bob");
        input["user_score"] = static_cast<int64_t>(65);
        
        auto result = graph.Invoke(Context::Background(), input);
        
        if (result.count("service_level")) {
            std::cout << "Result: " << std::any_cast<std::string>(result["service_level"]) << std::endl;
            std::cout << "Message: " << std::any_cast<std::string>(result["message"]) << std::endl;
        }
    }
    
    // Test Case 3: Guest User (score < 50)
    {
        std::cout << "\n--- Test Case 3: Guest User (score = 30) ---" << std::endl;
        std::map<std::string, std::any> input;
        input["user_name"] = std::string("Charlie");
        input["user_score"] = static_cast<int64_t>(30);
        
        auto result = graph.Invoke(Context::Background(), input);
        
        if (result.count("service_level")) {
            std::cout << "Result: " << std::any_cast<std::string>(result["service_level"]) << std::endl;
            std::cout << "Message: " << std::any_cast<std::string>(result["message"]) << std::endl;
        }
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Example completed successfully!" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

int main() {
    try {
        RunCompleteBranchEdgeExample();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
