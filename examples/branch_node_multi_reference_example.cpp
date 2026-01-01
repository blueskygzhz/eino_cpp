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
 * Complete example: BranchNode accessing multiple node outputs
 * 
 * Graph structure: A -> B -> C (BranchNode) -> [D1 | D2]
 * 
 * - Node A: User information processor (outputs age, name)
 * - Node B: Credit score evaluator (outputs score, credit_level)
 * - Node C: BranchNode that decides based on BOTH A and B outputs
 *   - Condition: age >= 18 AND score >= 700 -> Premium Service
 *   - Otherwise -> Standard Service
 * - Node D1: Premium service handler
 * - Node D2: Standard service handler
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

using MapType = std::map<std::string, std::any>;

// ============================================================================
// Custom Runnable Implementations
// ============================================================================

// Node A: User Information Processor
class UserInfoProcessor : public Runnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::cout << "[UserInfoProcessor] Processing user information..." << std::endl;
        
        // Extract user_id from input
        int64_t user_id = 0;
        if (input.count("user_id")) {
            user_id = std::any_cast<int64_t>(input.at("user_id"));
        }
        
        // Simulate user data lookup
        MapType output;
        output["user_id"] = user_id;
        output["age"] = static_cast<int64_t>(25);
        output["name"] = std::string("Alice");
        output["city"] = std::string("Shanghai");
        
        std::cout << "  → User: " << std::any_cast<std::string>(output["name"])
                  << ", Age: " << std::any_cast<int64_t>(output["age"]) << std::endl;
        
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
    
    std::string GetComponentType() const override {
        return "UserInfoProcessor";
    }
};

// Node B: Credit Score Evaluator
class CreditScoreEvaluator : public Runnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::cout << "[CreditScoreEvaluator] Evaluating credit score..." << std::endl;
        
        // In real scenario, this would use user_id to fetch credit score
        // Here we simulate the evaluation
        MapType output;
        output["score"] = static_cast<int64_t>(750);
        output["credit_level"] = std::string("Good");
        output["has_debt"] = false;
        
        std::cout << "  → Credit Score: " << std::any_cast<int64_t>(output["score"])
                  << ", Level: " << std::any_cast<std::string>(output["credit_level"]) << std::endl;
        
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
    
    std::string GetComponentType() const override {
        return "CreditScoreEvaluator";
    }
};

// Node D1: Premium Service Handler
class PremiumServiceHandler : public Runnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::cout << "[PremiumServiceHandler] Providing premium service..." << std::endl;
        
        MapType output;
        output["service_type"] = std::string("Premium");
        output["interest_rate"] = 3.5;
        output["loan_limit"] = static_cast<int64_t>(1000000);
        output["message"] = std::string("Welcome to our Premium Service! Low interest rate and high loan limit.");
        
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
    
    std::string GetComponentType() const override {
        return "PremiumServiceHandler";
    }
};

// Node D2: Standard Service Handler
class StandardServiceHandler : public Runnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::cout << "[StandardServiceHandler] Providing standard service..." << std::endl;
        
        MapType output;
        output["service_type"] = std::string("Standard");
        output["interest_rate"] = 5.5;
        output["loan_limit"] = static_cast<int64_t>(300000);
        output["message"] = std::string("Welcome to our Standard Service!");
        
        return output;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
    
    std::string GetComponentType() const override {
        return "StandardServiceHandler";
    }
};

// ============================================================================
// Main Example
// ============================================================================

void PrintSeparator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(60, '=') << "\n" << std::endl;
}

int main() {
    try {
        PrintSeparator("BranchNode Multi-Reference Example");
        
        std::cout << "This example demonstrates how a BranchNode can reference" << std::endl;
        std::cout << "outputs from multiple predecessor nodes (A and B) to make" << std::endl;
        std::cout << "routing decisions.\n" << std::endl;
        
        // ========================================================================
        // Step 1: Create Graph
        // ========================================================================
        
        std::cout << "Step 1: Creating graph..." << std::endl;
        Graph<MapType, MapType> graph;
        
        // ========================================================================
        // Step 2: Add Nodes
        // ========================================================================
        
        std::cout << "Step 2: Adding nodes..." << std::endl;
        
        // Node A: User info processor
        auto user_processor = std::make_shared<UserInfoProcessor>();
        graph.AddNode("user_processor", user_processor);
        std::cout << "  ✓ Added: user_processor" << std::endl;
        
        // Node B: Credit evaluator
        auto credit_evaluator = std::make_shared<CreditScoreEvaluator>();
        graph.AddNode("credit_evaluator", credit_evaluator);
        std::cout << "  ✓ Added: credit_evaluator" << std::endl;
        
        // Node C: BranchNode (decision maker)
        std::cout << "  ✓ Configuring BranchNode..." << std::endl;
        BranchNodeConfig branch_config;
        
        // Condition 0: age >= 18 AND score >= 700
        // This condition references BOTH user_processor (for age) and credit_evaluator (for score)
        branch_config.AddMultiConditionWithOperands({
            SingleClauseConfig(
                BranchOperator::GreaterOrEqual,
                OperandConfig::FromNode("user_processor", {"age"}),      // Reference node A
                OperandConfig::FromLiteral(static_cast<int64_t>(18))
            ),
            SingleClauseConfig(
                BranchOperator::GreaterOrEqual,
                OperandConfig::FromNode("credit_evaluator", {"score"}),  // Reference node B
                OperandConfig::FromLiteral(static_cast<int64_t>(700))
            )
        }, ClauseRelation::AND);
        
        graph.AddBranchNode("service_router", branch_config);
        std::cout << "    → Condition 0: user_processor.age >= 18 AND credit_evaluator.score >= 700" << std::endl;
        std::cout << "  ✓ Added: service_router (BranchNode)" << std::endl;
        
        // Node D1: Premium service
        auto premium_service = std::make_shared<PremiumServiceHandler>();
        graph.AddNode("premium_service", premium_service);
        std::cout << "  ✓ Added: premium_service" << std::endl;
        
        // Node D2: Standard service
        auto standard_service = std::make_shared<StandardServiceHandler>();
        graph.AddNode("standard_service", standard_service);
        std::cout << "  ✓ Added: standard_service" << std::endl;
        
        // ========================================================================
        // Step 3: Connect Nodes
        // ========================================================================
        
        std::cout << "\nStep 3: Connecting nodes..." << std::endl;
        
        // Main flow: START -> A -> B -> C
        graph.AddEdge(Graph<MapType, MapType>::START_NODE, "user_processor");
        std::cout << "  ✓ START → user_processor" << std::endl;
        
        graph.AddEdge("user_processor", "credit_evaluator");
        std::cout << "  ✓ user_processor → credit_evaluator" << std::endl;
        
        graph.AddEdge("credit_evaluator", "service_router");
        std::cout << "  ✓ credit_evaluator → service_router" << std::endl;
        
        // Branch edges: C -> [D1 | D2]
        graph.AddBranchEdge("service_router", 0, "premium_service");
        std::cout << "  ✓ service_router [Branch 0] → premium_service" << std::endl;
        
        graph.AddBranchEdge("service_router", 1, "standard_service");
        std::cout << "  ✓ service_router [Branch 1] → standard_service" << std::endl;
        
        // Connect to END
        graph.AddEdge("premium_service", Graph<MapType, MapType>::END_NODE);
        graph.AddEdge("standard_service", Graph<MapType, MapType>::END_NODE);
        std::cout << "  ✓ Connected to END" << std::endl;
        
        // ========================================================================
        // Step 4: Compile Graph
        // ========================================================================
        
        std::cout << "\nStep 4: Compiling graph..." << std::endl;
        graph.Compile();
        std::cout << "  ✓ Graph compiled successfully!" << std::endl;
        
        // ========================================================================
        // Step 5: Execute Graph
        // ========================================================================
        
        PrintSeparator("Executing Graph");
        
        MapType input = {
            {"user_id", static_cast<int64_t>(12345)}
        };
        
        std::cout << "Input: user_id = " << std::any_cast<int64_t>(input["user_id"]) << "\n" << std::endl;
        
        auto result = graph.Invoke(Context::Background(), input);
        
        // ========================================================================
        // Step 6: Display Results
        // ========================================================================
        
        PrintSeparator("Results");
        
        std::cout << "Service Type: " << std::any_cast<std::string>(result["service_type"]) << std::endl;
        std::cout << "Interest Rate: " << std::any_cast<double>(result["interest_rate"]) << "%" << std::endl;
        std::cout << "Loan Limit: ¥" << std::any_cast<int64_t>(result["loan_limit"]) << std::endl;
        std::cout << "Message: " << std::any_cast<std::string>(result["message"]) << std::endl;
        
        // ========================================================================
        // Explanation
        // ========================================================================
        
        PrintSeparator("How It Works");
        
        std::cout << "1. Graph executes nodes in topological order:" << std::endl;
        std::cout << "   user_processor → credit_evaluator → service_router → service" << std::endl;
        std::cout << std::endl;
        
        std::cout << "2. When service_router (BranchNode) executes:" << std::endl;
        std::cout << "   - Graph detects it's a BranchNode" << std::endl;
        std::cout << "   - Provides ALL executed node outputs as input:" << std::endl;
        std::cout << "     {" << std::endl;
        std::cout << "       \"user_processor\": {\"age\": 25, \"name\": \"Alice\", ...}," << std::endl;
        std::cout << "       \"credit_evaluator\": {\"score\": 750, \"credit_level\": \"Good\", ...}" << std::endl;
        std::cout << "     }" << std::endl;
        std::cout << std::endl;
        
        std::cout << "3. BranchNode resolves NodeReferences:" << std::endl;
        std::cout << "   - FromNode(\"user_processor\", {\"age\"}) → 25" << std::endl;
        std::cout << "   - FromNode(\"credit_evaluator\", {\"score\"}) → 750" << std::endl;
        std::cout << std::endl;
        
        std::cout << "4. BranchNode evaluates condition:" << std::endl;
        std::cout << "   - 25 >= 18 AND 750 >= 700 → TRUE" << std::endl;
        std::cout << "   - Returns branch index 0 → premium_service" << std::endl;
        std::cout << std::endl;
        
        std::cout << "5. Graph routes to premium_service based on branch index" << std::endl;
        
        PrintSeparator("Example Complete");
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
