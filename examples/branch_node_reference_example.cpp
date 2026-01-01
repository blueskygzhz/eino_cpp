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

// ============================================================================
// BranchNode Node Reference Example
// ============================================================================
// This example demonstrates how to use BranchNode with node reference capability,
// allowing the IF node to reference outputs from preceding LLM nodes.
//
// Scenario:
//   Node A (LLM) -> Output: {age: 25, name: "Alice"}
//   Node B (LLM) -> Output: {score: 85, vip: true}
//   Node C (Branch) -> References A and B's outputs for conditional logic
//
// Aligns with: coze-studio's block input reference mechanism

#include <iostream>
#include <map>
#include <string>
#include <any>
#include <memory>
#include "../include/eino/compose/branch_node.h"

using namespace eino::compose;

// ============================================================================
// Example 1: Basic Node Reference (IF age >= 18)
// ============================================================================

void example1_basic_reference() {
    std::cout << "\n========== Example 1: Basic Node Reference ==========\n";
    
    // Step 1: Configure BranchNode with node reference
    BranchNodeConfig config;
    
    // Condition: node_a.age >= 18
    config.AddConditionWithOperands(
        BranchOperator::GreaterOrEqual,
        OperandConfig::FromNode("node_a", {"age"}),   // Reference node_a's output.age
        OperandConfig::FromLiteral(static_cast<int64_t>(18))  // Compare with literal 18
    );
    
    // Step 2: Create BranchNode
    auto branch_node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    // Step 3: Simulate input from preceding nodes
    std::map<std::string, std::any> input;
    
    // Simulate node_a (LLM) output
    std::map<std::string, std::any> node_a_output;
    node_a_output["age"] = static_cast<int64_t>(25);
    node_a_output["name"] = std::string("Alice");
    input["node_a"] = node_a_output;
    
    // Step 4: Execute BranchNode
    auto output = branch_node->Invoke(nullptr, input);
    int64_t selected = std::any_cast<int64_t>(output["selected"]);
    
    std::cout << "Input: node_a.age = 25\n";
    std::cout << "Condition: age >= 18\n";
    std::cout << "Result: Branch " << selected << " selected ";
    std::cout << (selected == 0 ? "(Condition matched)" : "(Default branch)") << "\n";
}

// ============================================================================
// Example 2: Multi-Node Reference (age >= 18 AND vip == true)
// ============================================================================

void example2_multi_node_reference() {
    std::cout << "\n========== Example 2: Multi-Node Reference ==========\n";
    
    // Configure BranchNode
    BranchNodeConfig config;
    
    // Condition: node_a.age >= 18 AND node_b.vip == true
    std::vector<SingleClauseConfig> clauses = {
        SingleClauseConfig(
            BranchOperator::GreaterOrEqual,
            OperandConfig::FromNode("node_a", {"age"}),
            OperandConfig::FromLiteral(static_cast<int64_t>(18))
        ),
        SingleClauseConfig(
            BranchOperator::Equal,
            OperandConfig::FromNode("node_b", {"vip"}),
            OperandConfig::FromLiteral(true)
        )
    };
    config.AddMultiConditionWithOperands(clauses, ClauseRelation::AND);
    
    // Create BranchNode
    auto branch_node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    // Simulate input from two LLM nodes
    std::map<std::string, std::any> input;
    
    std::map<std::string, std::any> node_a_output;
    node_a_output["age"] = static_cast<int64_t>(25);
    node_a_output["name"] = std::string("Alice");
    input["node_a"] = node_a_output;
    
    std::map<std::string, std::any> node_b_output;
    node_b_output["score"] = static_cast<int64_t>(85);
    node_b_output["vip"] = true;
    input["node_b"] = node_b_output;
    
    // Execute
    auto output = branch_node->Invoke(nullptr, input);
    int64_t selected = std::any_cast<int64_t>(output["selected"]);
    
    std::cout << "Input:\n";
    std::cout << "  node_a.age = 25\n";
    std::cout << "  node_b.vip = true\n";
    std::cout << "Condition: (age >= 18) AND (vip == true)\n";
    std::cout << "Result: Branch " << selected << " selected ";
    std::cout << (selected == 0 ? "(Both conditions matched)" : "(Default branch)") << "\n";
}

// ============================================================================
// Example 3: Multiple Branches with Node References
// ============================================================================

void example3_multiple_branches() {
    std::cout << "\n========== Example 3: Multiple Branches ==========\n";
    
    BranchNodeConfig config;
    
    // Branch 0: VIP users (node_b.vip == true)
    config.AddConditionWithOperands(
        BranchOperator::Equal,
        OperandConfig::FromNode("node_b", {"vip"}),
        OperandConfig::FromLiteral(true)
    );
    
    // Branch 1: High score (node_b.score >= 80)
    config.AddConditionWithOperands(
        BranchOperator::GreaterOrEqual,
        OperandConfig::FromNode("node_b", {"score"}),
        OperandConfig::FromLiteral(static_cast<int64_t>(80))
    );
    
    // Branch 2: Adult (node_a.age >= 18)
    config.AddConditionWithOperands(
        BranchOperator::GreaterOrEqual,
        OperandConfig::FromNode("node_a", {"age"}),
        OperandConfig::FromLiteral(static_cast<int64_t>(18))
    );
    
    auto branch_node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    // Test Case 1: VIP user
    {
        std::map<std::string, std::any> input;
        
        std::map<std::string, std::any> node_a_output;
        node_a_output["age"] = static_cast<int64_t>(16);
        input["node_a"] = node_a_output;
        
        std::map<std::string, std::any> node_b_output;
        node_b_output["score"] = static_cast<int64_t>(50);
        node_b_output["vip"] = true;
        input["node_b"] = node_b_output;
        
        auto output = branch_node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "\nTest Case 1: VIP user (age=16, score=50, vip=true)\n";
        std::cout << "Result: Branch " << selected << " (VIP path)\n";
    }
    
    // Test Case 2: High score non-VIP
    {
        std::map<std::string, std::any> input;
        
        std::map<std::string, std::any> node_a_output;
        node_a_output["age"] = static_cast<int64_t>(25);
        input["node_a"] = node_a_output;
        
        std::map<std::string, std::any> node_b_output;
        node_b_output["score"] = static_cast<int64_t>(90);
        node_b_output["vip"] = false;
        input["node_b"] = node_b_output;
        
        auto output = branch_node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "\nTest Case 2: High score non-VIP (age=25, score=90, vip=false)\n";
        std::cout << "Result: Branch " << selected << " (High score path)\n";
    }
    
    // Test Case 3: No match - default branch
    {
        std::map<std::string, std::any> input;
        
        std::map<std::string, std::any> node_a_output;
        node_a_output["age"] = static_cast<int64_t>(15);
        input["node_a"] = node_a_output;
        
        std::map<std::string, std::any> node_b_output;
        node_b_output["score"] = static_cast<int64_t>(50);
        node_b_output["vip"] = false;
        input["node_b"] = node_b_output;
        
        auto output = branch_node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "\nTest Case 3: No match (age=15, score=50, vip=false)\n";
        std::cout << "Result: Branch " << selected << " (Default path)\n";
    }
}

// ============================================================================
// Example 4: Compare Two Node Outputs
// ============================================================================

void example4_compare_nodes() {
    std::cout << "\n========== Example 4: Compare Two Node Outputs ==========\n";
    
    BranchNodeConfig config;
    
    // Condition: node_a.score > node_b.score
    config.AddConditionWithOperands(
        BranchOperator::Greater,
        OperandConfig::FromNode("node_a", {"score"}),  // Left: node_a.score
        OperandConfig::FromNode("node_b", {"score"})   // Right: node_b.score
    );
    
    auto branch_node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    // Test: node_a.score (85) > node_b.score (75)
    std::map<std::string, std::any> input;
    
    std::map<std::string, std::any> node_a_output;
    node_a_output["score"] = static_cast<int64_t>(85);
    input["node_a"] = node_a_output;
    
    std::map<std::string, std::any> node_b_output;
    node_b_output["score"] = static_cast<int64_t>(75);
    input["node_b"] = node_b_output;
    
    auto output = branch_node->Invoke(nullptr, input);
    int64_t selected = std::any_cast<int64_t>(output["selected"]);
    
    std::cout << "Input:\n";
    std::cout << "  node_a.score = 85\n";
    std::cout << "  node_b.score = 75\n";
    std::cout << "Condition: node_a.score > node_b.score\n";
    std::cout << "Result: Branch " << selected << " selected ";
    std::cout << (selected == 0 ? "(A wins)" : "(B wins)") << "\n";
}

// ============================================================================
// Example 5: String Operations with Node References
// ============================================================================

void example5_string_operations() {
    std::cout << "\n========== Example 5: String Operations ==========\n";
    
    BranchNodeConfig config;
    
    // Condition: node_a.name contains "Alice"
    config.AddConditionWithOperands(
        BranchOperator::Contain,
        OperandConfig::FromNode("node_a", {"name"}),
        OperandConfig::FromLiteral(std::string("Alice"))
    );
    
    auto branch_node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    std::map<std::string, std::any> input;
    
    std::map<std::string, std::any> node_a_output;
    node_a_output["name"] = std::string("Hello Alice World");
    input["node_a"] = node_a_output;
    
    auto output = branch_node->Invoke(nullptr, input);
    int64_t selected = std::any_cast<int64_t>(output["selected"]);
    
    std::cout << "Input: node_a.name = \"Hello Alice World\"\n";
    std::cout << "Condition: name contains \"Alice\"\n";
    std::cout << "Result: Branch " << selected << " selected ";
    std::cout << (selected == 0 ? "(Contains Alice)" : "(Does not contain)") << "\n";
}

// ============================================================================
// Example 6: Backward Compatibility (Legacy Mode)
// ============================================================================

void example6_backward_compatibility() {
    std::cout << "\n========== Example 6: Backward Compatibility ==========\n";
    
    // Old API still works (using literal values directly in input)
    BranchNodeConfig config;
    config.AddSingleCondition(BranchOperator::GreaterOrEqual);
    
    auto branch_node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    // Old input format: {"0": {"left": 25, "right": 18}}
    std::map<std::string, std::any> input;
    std::map<std::string, std::any> condition_0;
    condition_0["left"] = static_cast<int64_t>(25);
    condition_0["right"] = static_cast<int64_t>(18);
    input["0"] = condition_0;
    
    auto output = branch_node->Invoke(nullptr, input);
    int64_t selected = std::any_cast<int64_t>(output["selected"]);
    
    std::cout << "Legacy input format: {\"0\": {\"left\": 25, \"right\": 18}}\n";
    std::cout << "Result: Branch " << selected << " selected (backward compatible)\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "==========================================================\n";
    std::cout << "BranchNode Node Reference Examples\n";
    std::cout << "==========================================================\n";
    std::cout << "\nThese examples demonstrate how BranchNode can reference\n";
    std::cout << "outputs from preceding nodes (e.g., LLM nodes A and B).\n";
    
    try {
        example1_basic_reference();
        example2_multi_node_reference();
        example3_multiple_branches();
        example4_compare_nodes();
        example5_string_operations();
        example6_backward_compatibility();
        
        std::cout << "\n==========================================================\n";
        std::cout << "All examples completed successfully!\n";
        std::cout << "==========================================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
