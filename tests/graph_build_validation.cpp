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

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include "eino/compose/graph.h"
#include "eino/compose/runnable.h"
#include "eino/compose/types.h"

using namespace eino::compose;

// Test data structure
struct DataPacket {
    std::string value;
    int count;
    
    DataPacket() : count(0) {}
    DataPacket(const std::string& v, int c = 0) : value(v), count(c) {}
};

// Mock Runnable implementations
class UpperRunnable : public ComposableRunnable<DataPacket, DataPacket> {
public:
    DataPacket Invoke(
        std::shared_ptr<Context> ctx,
        const DataPacket& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        DataPacket output = input;
        for (auto& c : output.value) {
            c = std::toupper(c);
        }
        output.count++;
        return output;
    }
    
    std::shared_ptr<StreamReader<DataPacket>> Stream(
        std::shared_ptr<Context> ctx,
        const DataPacket& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<DataPacket> results{Invoke(ctx, input, opts)};
        return std::make_shared<SimpleStreamReader<DataPacket>>(results);
    }
    
    DataPacket Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<DataPacket>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        DataPacket value;
        if (input && input->Read(value)) {
            return Invoke(ctx, value, opts);
        }
        return DataPacket();
    }
    
    std::shared_ptr<StreamReader<DataPacket>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<DataPacket>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<DataPacket> results;
        DataPacket value;
        while (input && input->Read(value)) {
            results.push_back(Invoke(ctx, value, opts));
        }
        return std::make_shared<SimpleStreamReader<DataPacket>>(results);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(DataPacket);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(DataPacket);
    }
    
    std::string GetComponentType() const override {
        return "UpperRunnable";
    }
};

class ReverseRunnable : public ComposableRunnable<DataPacket, DataPacket> {
public:
    DataPacket Invoke(
        std::shared_ptr<Context> ctx,
        const DataPacket& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        DataPacket output = input;
        std::reverse(output.value.begin(), output.value.end());
        output.count++;
        return output;
    }
    
    std::shared_ptr<StreamReader<DataPacket>> Stream(
        std::shared_ptr<Context> ctx,
        const DataPacket& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<DataPacket> results{Invoke(ctx, input, opts)};
        return std::make_shared<SimpleStreamReader<DataPacket>>(results);
    }
    
    DataPacket Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<DataPacket>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        DataPacket value;
        if (input && input->Read(value)) {
            return Invoke(ctx, value, opts);
        }
        return DataPacket();
    }
    
    std::shared_ptr<StreamReader<DataPacket>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<DataPacket>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<DataPacket> results;
        DataPacket value;
        while (input && input->Read(value)) {
            results.push_back(Invoke(ctx, value, opts));
        }
        return std::make_shared<SimpleStreamReader<DataPacket>>(results);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(DataPacket);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(DataPacket);
    }
    
    std::string GetComponentType() const override {
        return "ReverseRunnable";
    }
};

class CounterRunnable : public ComposableRunnable<DataPacket, DataPacket> {
public:
    DataPacket Invoke(
        std::shared_ptr<Context> ctx,
        const DataPacket& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        DataPacket output = input;
        output.count += 10;
        return output;
    }
    
    std::shared_ptr<StreamReader<DataPacket>> Stream(
        std::shared_ptr<Context> ctx,
        const DataPacket& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<DataPacket> results{Invoke(ctx, input, opts)};
        return std::make_shared<SimpleStreamReader<DataPacket>>(results);
    }
    
    DataPacket Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<DataPacket>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        DataPacket value;
        if (input && input->Read(value)) {
            return Invoke(ctx, value, opts);
        }
        return DataPacket();
    }
    
    std::shared_ptr<StreamReader<DataPacket>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<DataPacket>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<DataPacket> results;
        DataPacket value;
        while (input && input->Read(value)) {
            results.push_back(Invoke(ctx, value, opts));
        }
        return std::make_shared<SimpleStreamReader<DataPacket>>(results);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(DataPacket);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(DataPacket);
    }
    
    std::string GetComponentType() const override {
        return "CounterRunnable";
    }
};

int main() {
    int test_count = 0;
    int passed_count = 0;
    
    std::cout << "\n========== Compose Graph Build Tests ==========\n" << std::endl;
    
    // Test 1: Basic graph construction
    {
        test_count++;
        std::cout << "Test 1: Basic graph construction... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            graph->AddNode("upper", std::make_shared<UpperRunnable>());
            graph->AddNode("reverse", std::make_shared<ReverseRunnable>());
            
            graph->AddEdge(Graph<DataPacket, DataPacket>::START_NODE, "upper");
            graph->AddEdge("upper", "reverse");
            graph->AddEdge("reverse", Graph<DataPacket, DataPacket>::END_NODE);
            
            if (graph->GetNodeCount() == 2 && graph->GetEdgeCount() == 3) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (node count: " << graph->GetNodeCount() 
                         << ", edge count: " << graph->GetEdgeCount() << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 2: Graph compilation
    {
        test_count++;
        std::cout << "Test 2: Graph compilation... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            graph->AddNode("node1", std::make_shared<UpperRunnable>());
            graph->AddEdge(Graph<DataPacket, DataPacket>::START_NODE, "node1");
            graph->AddEdge("node1", Graph<DataPacket, DataPacket>::END_NODE);
            
            if (!graph->IsCompiled()) {
                graph->Compile();
                if (graph->IsCompiled()) {
                    std::cout << "✓ PASS" << std::endl;
                    passed_count++;
                } else {
                    std::cout << "✗ FAIL (compilation flag not set)" << std::endl;
                }
            } else {
                std::cout << "✗ FAIL (already compiled)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 3: Simple graph execution
    {
        test_count++;
        std::cout << "Test 3: Simple graph execution (upper)... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            graph->AddNode("upper", std::make_shared<UpperRunnable>());
            graph->AddEdge(Graph<DataPacket, DataPacket>::START_NODE, "upper");
            graph->AddEdge("upper", Graph<DataPacket, DataPacket>::END_NODE);
            graph->Compile();
            
            auto ctx = std::make_shared<Context>();
            DataPacket input("hello", 0);
            DataPacket output = graph->Invoke(ctx, input);
            
            if (output.value == "HELLO" && output.count == 1) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value: " << output.value << ", count: " 
                         << output.count << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 4: Multi-step graph execution
    {
        test_count++;
        std::cout << "Test 4: Multi-step graph execution (upper->reverse)... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            graph->AddNode("upper", std::make_shared<UpperRunnable>());
            graph->AddNode("reverse", std::make_shared<ReverseRunnable>());
            
            graph->AddEdge(Graph<DataPacket, DataPacket>::START_NODE, "upper");
            graph->AddEdge("upper", "reverse");
            graph->AddEdge("reverse", Graph<DataPacket, DataPacket>::END_NODE);
            graph->Compile();
            
            auto ctx = std::make_shared<Context>();
            DataPacket input("test", 0);
            DataPacket output = graph->Invoke(ctx, input);
            
            // Expected: "TEST" (from upper) -> "TSET" (from reverse), count = 2
            if (output.value == "TSET" && output.count == 2) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value: " << output.value << ", count: " 
                         << output.count << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 5: Complex 4-step graph
    {
        test_count++;
        std::cout << "Test 5: Complex 4-step graph execution... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            graph->AddNode("step1", std::make_shared<UpperRunnable>());
            graph->AddNode("step2", std::make_shared<CounterRunnable>());
            graph->AddNode("step3", std::make_shared<ReverseRunnable>());
            graph->AddNode("step4", std::make_shared<CounterRunnable>());
            
            graph->AddEdge(Graph<DataPacket, DataPacket>::START_NODE, "step1");
            graph->AddEdge("step1", "step2");
            graph->AddEdge("step2", "step3");
            graph->AddEdge("step3", "step4");
            graph->AddEdge("step4", Graph<DataPacket, DataPacket>::END_NODE);
            
            graph->Compile();
            
            auto ctx = std::make_shared<Context>();
            DataPacket input("abc", 0);
            DataPacket output = graph->Invoke(ctx, input);
            
            // Expected: "abc" -> "ABC" (upper, count=1) -> (counter, count=11) 
            //          -> "CBA" (reverse, count=12) -> (counter, count=22)
            if (output.value == "CBA" && output.count == 22) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value: " << output.value << ", count: " 
                         << output.count << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 6: Duplicate node detection
    {
        test_count++;
        std::cout << "Test 6: Duplicate node detection... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            graph->AddNode("node1", std::make_shared<UpperRunnable>());
            
            bool caught = false;
            try {
                graph->AddNode("node1", std::make_shared<ReverseRunnable>());
            } catch (const std::runtime_error&) {
                caught = true;
            }
            
            if (caught) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (exception not thrown)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 7: Reserved node name detection
    {
        test_count++;
        std::cout << "Test 7: Reserved node name detection... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            
            bool caught_start = false;
            try {
                graph->AddNode("__START__", std::make_shared<UpperRunnable>());
            } catch (const std::runtime_error&) {
                caught_start = true;
            }
            
            bool caught_end = false;
            try {
                graph->AddNode("__END__", std::make_shared<ReverseRunnable>());
            } catch (const std::runtime_error&) {
                caught_end = true;
            }
            
            if (caught_start && caught_end) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (not all reserved names caught)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 8: Invalid edge detection
    {
        test_count++;
        std::cout << "Test 8: Invalid edge detection... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            graph->AddNode("node1", std::make_shared<UpperRunnable>());
            
            bool caught = false;
            try {
                graph->AddEdge("node1", "non_existent");
            } catch (const std::runtime_error&) {
                caught = true;
            }
            
            if (caught) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (exception not thrown)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 9: Execution without compilation
    {
        test_count++;
        std::cout << "Test 9: Execution without compilation error... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            graph->AddNode("node1", std::make_shared<UpperRunnable>());
            graph->AddEdge(Graph<DataPacket, DataPacket>::START_NODE, "node1");
            graph->AddEdge("node1", Graph<DataPacket, DataPacket>::END_NODE);
            
            auto ctx = std::make_shared<Context>();
            DataPacket input("test", 0);
            
            bool caught = false;
            try {
                graph->Invoke(ctx, input);
            } catch (const std::runtime_error&) {
                caught = true;
            }
            
            if (caught) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (exception not thrown)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 10: Large pipeline
    {
        test_count++;
        std::cout << "Test 10: Large pipeline (10 nodes)... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            
            const int NUM_NODES = 10;
            for (int i = 0; i < NUM_NODES; ++i) {
                std::string node_name = "node_" + std::to_string(i);
                if (i % 2 == 0) {
                    graph->AddNode(node_name, std::make_shared<UpperRunnable>());
                } else {
                    graph->AddNode(node_name, std::make_shared<CounterRunnable>());
                }
            }
            
            graph->AddEdge(Graph<DataPacket, DataPacket>::START_NODE, "node_0");
            for (int i = 0; i < NUM_NODES - 1; ++i) {
                std::string from = "node_" + std::to_string(i);
                std::string to = "node_" + std::to_string(i + 1);
                graph->AddEdge(from, to);
            }
            graph->AddEdge("node_" + std::to_string(NUM_NODES - 1),
                         Graph<DataPacket, DataPacket>::END_NODE);
            
            graph->Compile();
            
            auto ctx = std::make_shared<Context>();
            DataPacket input("x", 0);
            DataPacket output = graph->Invoke(ctx, input);
            
            // 5 upper operations (counts to 5), 5 counter operations (adds 50)
            // Total count = 5 + 50 = 55
            if (output.value == "X" && output.count == 55) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value: " << output.value << ", count: " 
                         << output.count << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 11: Graph node names query
    {
        test_count++;
        std::cout << "Test 11: Get node names query... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            graph->AddNode("alpha", std::make_shared<UpperRunnable>());
            graph->AddNode("beta", std::make_shared<ReverseRunnable>());
            graph->AddNode("gamma", std::make_shared<CounterRunnable>());
            
            auto names = graph->GetNodeNames();
            
            bool has_alpha = std::find(names.begin(), names.end(), "alpha") != names.end();
            bool has_beta = std::find(names.begin(), names.end(), "beta") != names.end();
            bool has_gamma = std::find(names.begin(), names.end(), "gamma") != names.end();
            
            if (has_alpha && has_beta && has_gamma && names.size() == 3) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (missing or wrong nodes)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 12: Graph type information
    {
        test_count++;
        std::cout << "Test 12: Graph type information... ";
        try {
            auto graph = std::make_shared<Graph<DataPacket, DataPacket>>();
            graph->AddNode("node", std::make_shared<UpperRunnable>());
            graph->AddEdge(Graph<DataPacket, DataPacket>::START_NODE, "node");
            graph->AddEdge("node", Graph<DataPacket, DataPacket>::END_NODE);
            
            std::string component_type = graph->GetComponentType();
            
            if (component_type == "Graph") {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (component type: " << component_type << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Results
    std::cout << "\n========== Results ==========" << std::endl;
    std::cout << "Passed: " << passed_count << "/" << test_count << std::endl;
    std::cout << "Failed: " << (test_count - passed_count) << "/" << test_count << std::endl;
    
    if (passed_count == test_count) {
        std::cout << "\n✓ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ Some tests failed!" << std::endl;
        return 1;
    }
}
