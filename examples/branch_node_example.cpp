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

#include "../include/eino/compose/branch_node.h"
#include "../include/eino/compose/graph.h"
#include <iostream>
#include <map>
#include <any>

using namespace eino::compose;

// ============================================================================
// Example 1: Simple Age Check (Single Condition)
// ============================================================================
void Example_SimpleAgeCheck() {
    std::cout << "\n=== Example 1: Simple Age Check ===" << std::endl;
    
    // Create BranchNode configuration
    // Condition: age >= 18
    BranchNodeConfig config;
    config.AddSingleCondition(BranchOperator::GreaterOrEqual);
    
    // Create BranchNode
    auto node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    // Test case 1: age = 25 (should match)
    {
        std::map<std::string, std::any> input;
        std::map<std::string, std::any> condition_0;
        condition_0["left"] = static_cast<int64_t>(25);    // age = 25
        condition_0["right"] = static_cast<int64_t>(18);   // threshold = 18
        input["0"] = condition_0;
        
        auto output = node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "Test 1 - Age 25: Branch " << selected;
        std::cout << (selected == 0 ? " (Adult - TRUE branch)" : " (ERROR)") << std::endl;
    }
    
    // Test case 2: age = 15 (should NOT match)
    {
        std::map<std::string, std::any> input;
        std::map<std::string, std::any> condition_0;
        condition_0["left"] = static_cast<int64_t>(15);    // age = 15
        condition_0["right"] = static_cast<int64_t>(18);   // threshold = 18
        input["0"] = condition_0;
        
        auto output = node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "Test 2 - Age 15: Branch " << selected;
        std::cout << (selected == 1 ? " (Minor - FALSE branch)" : " (ERROR)") << std::endl;
    }
}

// ============================================================================
// Example 2: Multiple Conditions (Age AND Score)
// ============================================================================
void Example_MultipleConditions() {
    std::cout << "\n=== Example 2: Multiple Conditions (Age AND Score) ===" << std::endl;
    
    // Create BranchNode configuration
    // Condition 0: age >= 18
    // Condition 1: score > 60
    BranchNodeConfig config;
    config.AddSingleCondition(BranchOperator::GreaterOrEqual);  // age >= 18
    config.AddSingleCondition(BranchOperator::Greater);         // score > 60
    
    // Create BranchNode
    auto node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    // Test case: age = 25, score = 85
    {
        std::map<std::string, std::any> input;
        
        // Condition 0: age >= 18
        std::map<std::string, std::any> condition_0;
        condition_0["left"] = static_cast<int64_t>(25);    // age = 25
        condition_0["right"] = static_cast<int64_t>(18);   // threshold = 18
        input["0"] = condition_0;
        
        // Condition 1: score > 60
        std::map<std::string, std::any> condition_1;
        condition_1["left"] = static_cast<int64_t>(85);    // score = 85
        condition_1["right"] = static_cast<int64_t>(60);   // threshold = 60
        input["1"] = condition_1;
        
        auto output = node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "Test - Age 25, Score 85: Branch " << selected;
        std::cout << " (First condition matched: age >= 18)" << std::endl;
    }
}

// ============================================================================
// Example 3: Multi-Clause with AND/OR Logic
// ============================================================================
void Example_MultiClauseLogic() {
    std::cout << "\n=== Example 3: Multi-Clause with AND Logic ===" << std::endl;
    
    // Create BranchNode configuration
    // Condition: (age >= 18) AND (score > 60)
    BranchNodeConfig config;
    config.AddMultiCondition(
        {BranchOperator::GreaterOrEqual, BranchOperator::Greater},
        ClauseRelation::AND
    );
    
    // Create BranchNode
    auto node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    // Test case 1: age = 25, score = 85 (both TRUE)
    {
        std::map<std::string, std::any> input;
        std::map<std::string, std::any> condition_0;
        
        // Sub-condition 0: age >= 18
        std::map<std::string, std::any> sub_0;
        sub_0["left"] = static_cast<int64_t>(25);
        sub_0["right"] = static_cast<int64_t>(18);
        condition_0["0"] = sub_0;
        
        // Sub-condition 1: score > 60
        std::map<std::string, std::any> sub_1;
        sub_1["left"] = static_cast<int64_t>(85);
        sub_1["right"] = static_cast<int64_t>(60);
        condition_0["1"] = sub_1;
        
        input["0"] = condition_0;
        
        auto output = node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "Test 1 - Age 25, Score 85: Branch " << selected;
        std::cout << (selected == 0 ? " (Both conditions TRUE - AND matched)" : " (ERROR)") << std::endl;
    }
    
    // Test case 2: age = 25, score = 50 (first TRUE, second FALSE)
    {
        std::map<std::string, std::any> input;
        std::map<std::string, std::any> condition_0;
        
        // Sub-condition 0: age >= 18
        std::map<std::string, std::any> sub_0;
        sub_0["left"] = static_cast<int64_t>(25);
        sub_0["right"] = static_cast<int64_t>(18);
        condition_0["0"] = sub_0;
        
        // Sub-condition 1: score > 60
        std::map<std::string, std::any> sub_1;
        sub_1["left"] = static_cast<int64_t>(50);
        sub_1["right"] = static_cast<int64_t>(60);
        condition_0["1"] = sub_1;
        
        input["0"] = condition_0;
        
        auto output = node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "Test 2 - Age 25, Score 50: Branch " << selected;
        std::cout << (selected == 1 ? " (AND failed - default branch)" : " (ERROR)") << std::endl;
    }
}

// ============================================================================
// Example 4: String Operations
// ============================================================================
void Example_StringOperations() {
    std::cout << "\n=== Example 4: String Operations ===" << std::endl;
    
    // Create BranchNode configuration
    // Condition 0: name == "Alice"
    // Condition 1: name contains "Bob"
    // Condition 2: name is empty
    BranchNodeConfig config;
    config.AddSingleCondition(BranchOperator::Equal);
    config.AddSingleCondition(BranchOperator::Contain);
    config.AddSingleCondition(BranchOperator::Empty);
    
    auto node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    // Test case 1: name = "Alice" (exact match)
    {
        std::map<std::string, std::any> input;
        
        std::map<std::string, std::any> cond_0;
        cond_0["left"] = std::string("Alice");
        cond_0["right"] = std::string("Alice");
        input["0"] = cond_0;
        
        std::map<std::string, std::any> cond_1;
        cond_1["left"] = std::string("Alice");
        cond_1["right"] = std::string("Bob");
        input["1"] = cond_1;
        
        std::map<std::string, std::any> cond_2;
        cond_2["left"] = std::string("Alice");
        input["2"] = cond_2;
        
        auto output = node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "Test 1 - Name 'Alice': Branch " << selected;
        std::cout << " (Exact match)" << std::endl;
    }
    
    // Test case 2: name = "Bobby" (contains "Bob")
    {
        std::map<std::string, std::any> input;
        
        std::map<std::string, std::any> cond_0;
        cond_0["left"] = std::string("Bobby");
        cond_0["right"] = std::string("Alice");
        input["0"] = cond_0;
        
        std::map<std::string, std::any> cond_1;
        cond_1["left"] = std::string("Bobby");
        cond_1["right"] = std::string("Bob");
        input["1"] = cond_1;
        
        std::map<std::string, std::any> cond_2;
        cond_2["left"] = std::string("Bobby");
        input["2"] = cond_2;
        
        auto output = node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "Test 2 - Name 'Bobby': Branch " << selected;
        std::cout << " (Contains 'Bob')" << std::endl;
    }
    
    // Test case 3: name = "" (empty)
    {
        std::map<std::string, std::any> input;
        
        std::map<std::string, std::any> cond_0;
        cond_0["left"] = std::string("");
        cond_0["right"] = std::string("Alice");
        input["0"] = cond_0;
        
        std::map<std::string, std::any> cond_1;
        cond_1["left"] = std::string("");
        cond_1["right"] = std::string("Bob");
        input["1"] = cond_1;
        
        std::map<std::string, std::any> cond_2;
        cond_2["left"] = std::string("");
        input["2"] = cond_2;
        
        auto output = node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "Test 3 - Name empty: Branch " << selected;
        std::cout << " (Empty string)" << std::endl;
    }
}

// ============================================================================
// Example 5: Complex Workflow Scenario (coze-studio style)
// ============================================================================
void Example_ComplexWorkflow() {
    std::cout << "\n=== Example 5: Complex Workflow (Coze-Studio Style) ===" << std::endl;
    std::cout << "Scenario: User eligibility check" << std::endl;
    std::cout << "  - Branch 0: VIP user (level >= 3 AND balance > 1000)" << std::endl;
    std::cout << "  - Branch 1: Regular user (level >= 1)" << std::endl;
    std::cout << "  - Default: Guest user" << std::endl;
    
    // Configuration
    BranchNodeConfig config;
    
    // Branch 0: VIP (level >= 3 AND balance > 1000)
    config.AddMultiCondition(
        {BranchOperator::GreaterOrEqual, BranchOperator::Greater},
        ClauseRelation::AND
    );
    
    // Branch 1: Regular (level >= 1)
    config.AddSingleCondition(BranchOperator::GreaterOrEqual);
    
    auto node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    // Test case 1: level = 5, balance = 2000 (VIP)
    {
        std::map<std::string, std::any> input;
        
        // Branch 0: VIP check
        std::map<std::string, std::any> branch_0;
        std::map<std::string, std::any> level_check;
        level_check["left"] = static_cast<int64_t>(5);
        level_check["right"] = static_cast<int64_t>(3);
        branch_0["0"] = level_check;
        
        std::map<std::string, std::any> balance_check;
        balance_check["left"] = static_cast<int64_t>(2000);
        balance_check["right"] = static_cast<int64_t>(1000);
        branch_0["1"] = balance_check;
        input["0"] = branch_0;
        
        // Branch 1: Regular check
        std::map<std::string, std::any> branch_1;
        branch_1["left"] = static_cast<int64_t>(5);
        branch_1["right"] = static_cast<int64_t>(1);
        input["1"] = branch_1;
        
        auto output = node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "\nTest 1 - Level 5, Balance 2000: Branch " << selected;
        std::cout << " → VIP User (full access)" << std::endl;
    }
    
    // Test case 2: level = 2, balance = 500 (Regular)
    {
        std::map<std::string, std::any> input;
        
        // Branch 0: VIP check (will fail)
        std::map<std::string, std::any> branch_0;
        std::map<std::string, std::any> level_check;
        level_check["left"] = static_cast<int64_t>(2);
        level_check["right"] = static_cast<int64_t>(3);
        branch_0["0"] = level_check;
        
        std::map<std::string, std::any> balance_check;
        balance_check["left"] = static_cast<int64_t>(500);
        balance_check["right"] = static_cast<int64_t>(1000);
        branch_0["1"] = balance_check;
        input["0"] = branch_0;
        
        // Branch 1: Regular check (will pass)
        std::map<std::string, std::any> branch_1;
        branch_1["left"] = static_cast<int64_t>(2);
        branch_1["right"] = static_cast<int64_t>(1);
        input["1"] = branch_1;
        
        auto output = node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "Test 2 - Level 2, Balance 500: Branch " << selected;
        std::cout << " → Regular User (limited access)" << std::endl;
    }
    
    // Test case 3: level = 0, balance = 0 (Guest)
    {
        std::map<std::string, std::any> input;
        
        // Branch 0: VIP check (will fail)
        std::map<std::string, std::any> branch_0;
        std::map<std::string, std::any> level_check;
        level_check["left"] = static_cast<int64_t>(0);
        level_check["right"] = static_cast<int64_t>(3);
        branch_0["0"] = level_check;
        
        std::map<std::string, std::any> balance_check;
        balance_check["left"] = static_cast<int64_t>(0);
        balance_check["right"] = static_cast<int64_t>(1000);
        branch_0["1"] = balance_check;
        input["0"] = branch_0;
        
        // Branch 1: Regular check (will fail)
        std::map<std::string, std::any> branch_1;
        branch_1["left"] = static_cast<int64_t>(0);
        branch_1["right"] = static_cast<int64_t>(1);
        input["1"] = branch_1;
        
        auto output = node->Invoke(nullptr, input);
        int64_t selected = std::any_cast<int64_t>(output["selected"]);
        
        std::cout << "Test 3 - Level 0, Balance 0: Branch " << selected;
        std::cout << " → Guest User (read-only)" << std::endl;
    }
}

// ============================================================================
// Main Function
// ============================================================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "BranchNode Examples (Coze-Studio Style)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        Example_SimpleAgeCheck();
        Example_MultipleConditions();
        Example_MultiClauseLogic();
        Example_StringOperations();
        Example_ComplexWorkflow();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "All examples completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
