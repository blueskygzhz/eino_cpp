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
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include "eino/compose/graph.h"
#include "eino/compose/graph_run.h"
#include "eino/compose/graph_advanced.h"

using namespace eino::compose;

// Example: Simple string transformation runnable
class ToUpperRunnable : public ComposableRunnable<std::string, std::string> {
public:
    std::string Invoke(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::string result = input;
        for (auto& c : result) {
            c = std::toupper(c);
        }
        std::cout << "  [ToUpperRunnable] '" << input << "' -> '" << result << "'" << std::endl;
        return result;
    }
    
    std::shared_ptr<StreamReader<std::string>> Stream(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<std::string> results{Invoke(ctx, input, opts)};
        return std::make_shared<SimpleStreamReader<std::string>>(results);
    }
    
    std::string Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::string>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::string value;
        if (input && input->Read(value)) {
            return Invoke(ctx, value, opts);
        }
        return "";
    }
    
    std::shared_ptr<StreamReader<std::string>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::string>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<std::string> results;
        std::string value;
        while (input && input->Read(value)) {
            results.push_back(Invoke(ctx, value, opts));
        }
        return std::make_shared<SimpleStreamReader<std::string>>(results);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(std::string);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(std::string);
    }
    
    std::string GetComponentType() const override {
        return "ToUpperRunnable";
    }
};

class ReverseRunnable : public ComposableRunnable<std::string, std::string> {
public:
    std::string Invoke(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::string result = input;
        std::reverse(result.begin(), result.end());
        std::cout << "  [ReverseRunnable] '" << input << "' -> '" << result << "'" << std::endl;
        return result;
    }
    
    std::shared_ptr<StreamReader<std::string>> Stream(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<std::string> results{Invoke(ctx, input, opts)};
        return std::make_shared<SimpleStreamReader<std::string>>(results);
    }
    
    std::string Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::string>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::string value;
        if (input && input->Read(value)) {
            return Invoke(ctx, value, opts);
        }
        return "";
    }
    
    std::shared_ptr<StreamReader<std::string>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::string>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<std::string> results;
        std::string value;
        while (input && input->Read(value)) {
            results.push_back(Invoke(ctx, value, opts));
        }
        return std::make_shared<SimpleStreamReader<std::string>>(results);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(std::string);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(std::string);
    }
    
    std::string GetComponentType() const override {
        return "ReverseRunnable";
    }
};

void ExampleBasicDAG() {
    std::cout << "\n=== Example 1: Basic DAG Execution ===" << std::endl;
    
    // Create graph
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    auto upper = std::make_shared<ToUpperRunnable>();
    auto reverse = std::make_shared<ReverseRunnable>();
    
    // Add nodes
    graph->AddNode("upper", upper);
    graph->AddNode("reverse", reverse);
    
    // Add edges
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "upper");
    graph->AddEdge("upper", "reverse");
    graph->AddEdge("reverse", Graph<std::string, std::string>::END_NODE);
    
    graph->Compile();
    
    // Create runner
    GraphRunOptions opts;
    opts.run_type = GraphRunType::DAG;
    auto runner = NewGraphRunner(graph, opts);
    
    // Execute
    auto ctx = Context::Background();
    std::string input = "hello";
    GraphExecutionTrace<std::string, std::string> trace;
    
    std::cout << "Input: " << input << std::endl;
    std::string result = runner->Run(ctx, input, {}, &trace);
    
    std::cout << "Output: " << result << std::endl;
    std::cout << "Execution trace:" << std::endl;
    std::cout << "  Total steps: " << trace.total_steps << std::endl;
    std::cout << "  Completed: " << (trace.completed ? "yes" : "no") << std::endl;
    std::cout << "  Status: " << trace.final_status << std::endl;
    std::cout << "  Execution time: " << trace.total_execution_time_ms << " ms" << std::endl;
    std::cout << "  Node executions: " << trace.node_infos.size() << std::endl;
}

void ExamplePregelMode() {
    std::cout << "\n=== Example 2: Pregel Mode (Iterative Execution) ===" << std::endl;
    
    // Create a simple iterative graph
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    auto upper = std::make_shared<ToUpperRunnable>();
    
    graph->AddNode("upper", upper);
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "upper");
    graph->AddEdge("upper", Graph<std::string, std::string>::END_NODE);
    
    graph->Compile();
    
    // Create runner in Pregel mode
    GraphRunOptions opts;
    opts.run_type = GraphRunType::Pregel;
    opts.max_steps = 3;
    auto runner = NewGraphRunner(graph, opts);
    
    // Execute
    auto ctx = Context::Background();
    std::string input = "a";
    GraphExecutionTrace<std::string, std::string> trace;
    
    std::cout << "Input: " << input << std::endl;
    std::string result = runner->Run(ctx, input, {}, &trace);
    
    std::cout << "Output: " << result << std::endl;
    std::cout << "Iterations: " << runner->GetStepCount() << std::endl;
    std::cout << "Final status: " << trace.final_status << std::endl;
}

void ExampleStreamExecution() {
    std::cout << "\n=== Example 3: Stream Execution ===" << std::endl;
    
    // Create graph
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    auto upper = std::make_shared<ToUpperRunnable>();
    
    graph->AddNode("upper", upper);
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "upper");
    graph->AddEdge("upper", Graph<std::string, std::string>::END_NODE);
    
    graph->Compile();
    
    // Create runner
    GraphRunOptions opts;
    auto runner = NewGraphRunner(graph, opts);
    
    // Create input stream
    auto input_stream = std::make_shared<SimpleStreamReader<std::string>>();
    input_stream->Add("hello");
    input_stream->Add("world");
    input_stream->Add("test");
    
    // Execute
    auto ctx = Context::Background();
    auto output_stream = runner->RunStream(ctx, input_stream);
    
    std::cout << "Processing stream:" << std::endl;
    std::string result;
    while (output_stream && output_stream->Read(result)) {
        std::cout << "Output: " << result << std::endl;
    }
}

void ExampleConditionalExecution() {
    std::cout << "\n=== Example 4: Conditional Execution ===" << std::endl;
    
    // Create conditional graph
    auto graph = std::make_shared<ConditionalGraph<std::string, std::string>>();
    
    auto upper = std::make_shared<ToUpperRunnable>();
    auto reverse = std::make_shared<ReverseRunnable>();
    
    graph->AddNode("upper", upper);
    graph->AddNode("reverse", reverse);
    
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "upper");
    
    // Add conditional edge: only execute reverse if output length > 3
    graph->AddConditionalEdge(
        "upper",
        "reverse",
        [](std::shared_ptr<Context> ctx, const std::string& output) {
            return output.length() > 3;
        }
    );
    
    graph->AddEdge("reverse", Graph<std::string, std::string>::END_NODE);
    graph->AddEdge("upper", Graph<std::string, std::string>::END_NODE);
    
    graph->Compile();
    
    // Create runner
    auto runner = NewGraphRunner(graph);
    
    // Test with different inputs
    auto ctx = Context::Background();
    
    std::cout << "Input: 'hi' (short)" << std::endl;
    try {
        std::string result = runner->Run(ctx, "hi");
        std::cout << "Output: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    std::cout << "Input: 'hello' (long)" << std::endl;
    try {
        std::string result = runner->Run(ctx, "hello");
        std::cout << "Output: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

void ExampleFluentAPI() {
    std::cout << "\n=== Example 5: Fluent API ===" << std::endl;
    
    auto upper = std::make_shared<ToUpperRunnable>();
    auto reverse = std::make_shared<ReverseRunnable>();
    
    // Build graph using fluent API
    GraphRunOptions opts;
    opts.run_type = GraphRunType::DAG;
    
    auto runner = BuildGraph<std::string, std::string>()
        .Node("upper", upper)
        .Node("reverse", reverse)
        .Start("upper")
        .Edge("upper", "reverse")
        .End("reverse")
        .BuildWithRunner(opts);
    
    // Execute
    auto ctx = Context::Background();
    std::string input = "hello";
    GraphExecutionTrace<std::string, std::string> trace;
    
    std::cout << "Input: " << input << std::endl;
    std::string result = runner->Run(ctx, input, {}, &trace);
    
    std::cout << "Output: " << result << std::endl;
    std::cout << "Total execution time: " << trace.total_execution_time_ms << " ms" << std::endl;
}

int main() {
    std::cout << "Graph Run Examples" << std::endl;
    std::cout << "==================" << std::endl;
    
    try {
        ExampleBasicDAG();
        ExamplePregelMode();
        ExampleStreamExecution();
        ExampleConditionalExecution();
        ExampleFluentAPI();
        
        std::cout << "\n=== All examples completed successfully ===" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
