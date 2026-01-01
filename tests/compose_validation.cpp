/*
 * Copyright 2024 CloudWeGo Authors
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

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "eino/compose/types.h"
#include "eino/compose/runnable.h"
#include "eino/compose/chain.h"
#include "eino/compose/graph.h"
#include "eino/compose/branch.h"
#include "eino/compose/chain_branch.h"

using namespace eino::compose;

int main() {
    int test_count = 0;
    int passed_count = 0;
    
    std::cout << "\n========== Compose Module Tests ==========\n" << std::endl;
    
    auto ctx = Context::Background();
    auto empty_opts = std::vector<Option>();
    
    // Test 1: Context creation
    test_count++;
    std::cout << "Test 1: Context creation... ";
    {
        auto ctx = Context::Background();
        if (ctx != nullptr) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 2: ComponentType::Chain
    test_count++;
    std::cout << "Test 2: ComponentType::Chain... ";
    {
        std::string chain_str = ComponentTypeToString(ComponentType::Chain);
        if (chain_str == "Chain") {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 3: ComponentType::Graph
    test_count++;
    std::cout << "Test 3: ComponentType::Graph... ";
    {
        std::string graph_str = ComponentTypeToString(ComponentType::Graph);
        if (graph_str == "Graph") {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 4: NodeTriggerMode::AnyPredecessor
    test_count++;
    std::cout << "Test 4: NodeTriggerMode::AnyPredecessor... ";
    {
        std::string mode_str = NodeTriggerModeToString(NodeTriggerMode::AnyPredecessor);
        if (mode_str == "any_predecessor") {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 5: NodeTriggerMode::AllPredecessor
    test_count++;
    std::cout << "Test 5: NodeTriggerMode::AllPredecessor... ";
    {
        std::string mode_str = NodeTriggerModeToString(NodeTriggerMode::AllPredecessor);
        if (mode_str == "all_predecessor") {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 6: SimpleStreamReader creation
    test_count++;
    std::cout << "Test 6: SimpleStreamReader creation... ";
    {
        std::vector<int> data{1, 2, 3};
        auto reader = std::make_shared<SimpleStreamReader<int>>(data);
        if (reader != nullptr && !reader->IsClosed()) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 7: SimpleStreamReader Read
    test_count++;
    std::cout << "Test 7: SimpleStreamReader Read... ";
    {
        std::vector<int> data{1, 2, 3};
        auto reader = std::make_shared<SimpleStreamReader<int>>(data);
        int value;
        bool success = reader->Read(value);
        if (success && value == 1) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 8: SimpleStreamReader Peek
    test_count++;
    std::cout << "Test 8: SimpleStreamReader Peek... ";
    {
        std::vector<int> data{10, 20, 30};
        auto reader = std::make_shared<SimpleStreamReader<int>>(data);
        int value;
        bool success = reader->Peek(value);
        if (success && value == 10) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 9: SimpleStreamReader Close
    test_count++;
    std::cout << "Test 9: SimpleStreamReader Close... ";
    {
        std::vector<int> data{1, 2, 3};
        auto reader = std::make_shared<SimpleStreamReader<int>>(data);
        reader->Close();
        if (reader->IsClosed()) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 10: SimpleStreamReader GetRemaining
    test_count++;
    std::cout << "Test 10: SimpleStreamReader GetRemaining... ";
    {
        std::vector<int> data{1, 2, 3};
        auto reader = std::make_shared<SimpleStreamReader<int>>(data);
        int v;
        reader->Read(v);
        if (reader->GetRemaining() == 2) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 11: LambdaRunnable creation
    test_count++;
    std::cout << "Test 11: LambdaRunnable creation... ";
    {
        auto step = NewLambdaRunnable<int, int>(
            [](std::shared_ptr<Context> ctx, const int& x, const std::vector<Option>& opts) {
                return x * 2;
            }
        );
        if (step != nullptr) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 12: LambdaRunnable Invoke
    test_count++;
    std::cout << "Test 12: LambdaRunnable Invoke... ";
    {
        auto step = NewLambdaRunnable<int, int>(
            [](std::shared_ptr<Context> ctx, const int& x, const std::vector<Option>& opts) {
                return x * 2;
            }
        );
        try {
            int result = step->Invoke(ctx, 5, empty_opts);
            if (result == 10) {
                std::cout << "PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "FAIL" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "FAIL (exception)" << std::endl;
        }
    }
    
    // Test 13: SimpleChain creation
    test_count++;
    std::cout << "Test 13: SimpleChain creation... ";
    {
        auto step1 = NewLambdaRunnable<int, int>(
            [](std::shared_ptr<Context> ctx, const int& x, const std::vector<Option>& opts) {
                return x * 2;
            }
        );
        auto step2 = NewLambdaRunnable<int, int>(
            [](std::shared_ptr<Context> ctx, const int& x, const std::vector<Option>& opts) {
                return x + 1;
            }
        );
        auto chain = NewChain<int, int, int>(step1, step2);
        if (chain != nullptr) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 14: SimpleChain Invoke
    test_count++;
    std::cout << "Test 14: SimpleChain Invoke... ";
    {
        auto step1 = NewLambdaRunnable<int, int>(
            [](std::shared_ptr<Context> ctx, const int& x, const std::vector<Option>& opts) {
                return x * 2;
            }
        );
        auto step2 = NewLambdaRunnable<int, int>(
            [](std::shared_ptr<Context> ctx, const int& x, const std::vector<Option>& opts) {
                return x + 1;
            }
        );
        auto chain = NewChain<int, int, int>(step1, step2);
        chain->Compile();
        try {
            int result = chain->Invoke(ctx, 5, empty_opts);
            if (result == 11) {
                std::cout << "PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "FAIL" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "FAIL (exception)" << std::endl;
        }
    }
    
    // Test 15: Graph creation
    test_count++;
    std::cout << "Test 15: Graph creation... ";
    {
        auto graph = std::make_shared<Graph<int, int>>();
        if (graph != nullptr) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 16: Graph AddNode
    test_count++;
    std::cout << "Test 16: Graph AddNode... ";
    {
        auto graph = std::make_shared<Graph<int, int>>();
        auto step = NewLambdaRunnable<int, int>(
            [](std::shared_ptr<Context> ctx, const int& x, const std::vector<Option>& opts) {
                return x * 2;
            }
        );
        try {
            graph->AddNode("step1", step);
            std::cout << "PASS" << std::endl;
            passed_count++;
        } catch (const std::exception& e) {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 17: Graph GetNodeCount
    test_count++;
    std::cout << "Test 17: Graph GetNodeCount... ";
    {
        auto graph = std::make_shared<Graph<int, int>>();
        auto step = NewLambdaRunnable<int, int>(
            [](std::shared_ptr<Context> ctx, const int& x, const std::vector<Option>& opts) {
                return x * 2;
            }
        );
        graph->AddNode("step1", step);
        if (graph->GetNodeCount() == 1) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 18: Simple Lambda with Graph
    test_count++;
    std::cout << "Test 18: Lambda graph execution... ";
    {
        auto graph = std::make_shared<Graph<std::string, std::string>>();
        auto step = NewLambdaRunnable<std::string, std::string>(
            [](std::shared_ptr<Context> ctx, const std::string& x, const std::vector<Option>& opts) {
                return x + "_processed";
            }
        );
        graph->AddNode("process", step);
        graph->AddEdge("__START__", "process");
        graph->AddEdge("process", "__END__");
        graph->Compile();
        
        try {
            std::string result = graph->Invoke(ctx, "test", empty_opts);
            if (result == "test_processed") {
                std::cout << "PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "FAIL" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 19: Graph node count
    test_count++;
    std::cout << "Test 19: Graph node count... ";
    {
        auto graph = std::make_shared<Graph<int, int>>();
        auto step1 = NewLambdaRunnable<int, int>(
            [](std::shared_ptr<Context> ctx, const int& x, const std::vector<Option>& opts) {
                return x + 1;
            }
        );
        auto step2 = NewLambdaRunnable<int, int>(
            [](std::shared_ptr<Context> ctx, const int& x, const std::vector<Option>& opts) {
                return x + 2;
            }
        );
        graph->AddNode("step1", step1);
        graph->AddNode("step2", step2);
        
        if (graph->GetNodeCount() == 2) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 20: ChainBranch creation
    test_count++;
    std::cout << "Test 20: ChainBranch creation... ";
    {
        auto branch = NewChainBranch<std::string>();
        if (branch != nullptr && !branch->HasError()) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 21: ChainBranch AddLambda
    test_count++;
    std::cout << "Test 21: ChainBranch AddLambda... ";
    {
        auto step = NewLambdaRunnable<std::string, std::string>(
            [](std::shared_ptr<Context> ctx, const std::string& x, const std::vector<Option>& opts) {
                return x + x;
            }
        );
        
        auto branch = NewChainBranch<std::string>();
        branch->AddLambda("key1", step);
        
        if (!branch->HasError() && branch->Validate()) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 22: ChainBranch with multiple lambdas
    test_count++;
    std::cout << "Test 22: ChainBranch with multiple lambdas... ";
    {
        auto step1 = NewLambdaRunnable<std::string, std::string>(
            [](std::shared_ptr<Context> ctx, const std::string& x, const std::vector<Option>& opts) {
                return x + "_1";
            }
        );
        
        auto step2 = NewLambdaRunnable<std::string, std::string>(
            [](std::shared_ptr<Context> ctx, const std::string& x, const std::vector<Option>& opts) {
                return x + "_2";
            }
        );
        
        auto branch = NewChainBranch<std::string>();
        branch->AddLambda("key1", step1);
        branch->AddLambda("key2", step2);
        
        const auto& nodes = branch->GetBranchNodes();
        if (!branch->HasError() && nodes.size() == 2) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 23: ChainBranch Validate
    test_count++;
    std::cout << "Test 23: ChainBranch Validate... ";
    {
        auto step = NewLambdaRunnable<std::string, std::string>(
            [](std::shared_ptr<Context> ctx, const std::string& x, const std::vector<Option>& opts) {
                return x + "_processed";
            }
        );
        
        auto branch = NewChainBranch<std::string>();
        branch->AddLambda("process", step);
        
        if (branch->Validate() && !branch->HasError()) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 24: ChainMultiBranch creation
    test_count++;
    std::cout << "Test 24: ChainMultiBranch creation... ";
    {
        auto branch = NewChainMultiBranch<std::string>();
        if (branch != nullptr && !branch->HasError()) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    // Test 25: ChainBranch validation
    test_count++;
    std::cout << "Test 25: ChainBranch validation... ";
    {
        auto step1 = NewLambdaRunnable<std::string, std::string>(
            [](std::shared_ptr<Context> ctx, const std::string& x, const std::vector<Option>& opts) {
                return x + "_1";
            }
        );
        auto step2 = NewLambdaRunnable<std::string, std::string>(
            [](std::shared_ptr<Context> ctx, const std::string& x, const std::vector<Option>& opts) {
                return x + "_2";
            }
        );
        
        auto branch = NewChainBranch<std::string>();
        branch->AddLambda("branch1", step1);
        branch->AddLambda("branch2", step2);
        
        if (branch->Validate() && !branch->HasError()) {
            std::cout << "PASS" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }
    
    std::cout << "\n========== Results ==========\n";
    std::cout << "Passed: " << passed_count << "/" << test_count << std::endl;
    std::cout << "Failed: " << (test_count - passed_count) << "/" << test_count << std::endl;
    std::cout << "\n";
    
    return (test_count - passed_count) == 0 ? 0 : 1;
}
