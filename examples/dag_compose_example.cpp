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

/**
 * @file dag_compose_example.cpp
 * @brief Comprehensive DAG (Directed Acyclic Graph) composition examples using eino_cpp
 * 
 * This example demonstrates:
 * - Basic DAG construction with nodes and edges
 * - Linear pipelines and branching/merging patterns
 * - Different node trigger modes (AllPredecessor, AnyPredecessor)
 * - Lambda nodes for quick transformations
 * - Custom runnable nodes
 * - Stream-based execution
 * - Execution tracing and debugging
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <nlohmann/json.hpp>
#include "eino/compose/graph.h"
#include "eino/compose/runnable.h"
#include "eino/compose/types.h"

using json = nlohmann::json;
using namespace eino::compose;

// ============================================================================
// Custom Runnable Nodes
// ============================================================================

/**
 * @brief String transformation node - converts to uppercase
 */
class ToUpperNode : public ComposableRunnable<std::string, std::string> {
public:
    ToUpperNode(const std::string& name = "ToUpper") : name_(name) {}
    
    std::string Invoke(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::string result = input;
        for (auto& c : result) {
            c = std::toupper(c);
        }
        std::cout << "  [" << name_ << "] '" << input << "' -> '" << result << "'" << std::endl;
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
        return (input && input->Read(value)) ? Invoke(ctx, value, opts) : "";
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
    
    const std::type_info& GetInputType() const override { return typeid(std::string); }
    const std::type_info& GetOutputType() const override { return typeid(std::string); }
    std::string GetComponentType() const override { return "ToUpperNode"; }
    
private:
    std::string name_;
};

/**
 * @brief String reversal node
 */
class ReverseNode : public ComposableRunnable<std::string, std::string> {
public:
    ReverseNode(const std::string& name = "Reverse") : name_(name) {}
    
    std::string Invoke(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::string result = input;
        std::reverse(result.begin(), result.end());
        std::cout << "  [" << name_ << "] '" << input << "' -> '" << result << "'" << std::endl;
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
        return (input && input->Read(value)) ? Invoke(ctx, value, opts) : "";
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
    
    const std::type_info& GetInputType() const override { return typeid(std::string); }
    const std::type_info& GetOutputType() const override { return typeid(std::string); }
    std::string GetComponentType() const override { return "ReverseNode"; }
    
private:
    std::string name_;
};

/**
 * @brief Prefix appender node
 */
class PrefixNode : public ComposableRunnable<std::string, std::string> {
public:
    PrefixNode(const std::string& prefix, const std::string& name = "Prefix") 
        : prefix_(prefix), name_(name) {}
    
    std::string Invoke(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::string result = prefix_ + input;
        std::cout << "  [" << name_ << "] '" << input << "' -> '" << result << "'" << std::endl;
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
        return (input && input->Read(value)) ? Invoke(ctx, value, opts) : "";
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
    
    const std::type_info& GetInputType() const override { return typeid(std::string); }
    const std::type_info& GetOutputType() const override { return typeid(std::string); }
    std::string GetComponentType() const override { return "PrefixNode"; }
    
private:
    std::string prefix_;
    std::string name_;
};

/**
 * @brief Merge/concatenate node - combines multiple inputs
 */
class MergeNode : public ComposableRunnable<std::string, std::string> {
public:
    MergeNode(const std::string& separator = " + ", const std::string& name = "Merge") 
        : separator_(separator), name_(name) {}
    
    std::string Invoke(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        // In a real merge node, this would accumulate from multiple sources
        // For simplicity, just pass through with a marker
        std::string result = "[merged:" + input + "]";
        std::cout << "  [" << name_ << "] Received: '" << input << "' -> '" << result << "'" << std::endl;
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
        return (input && input->Read(value)) ? Invoke(ctx, value, opts) : "";
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
    
    const std::type_info& GetInputType() const override { return typeid(std::string); }
    const std::type_info& GetOutputType() const override { return typeid(std::string); }
    std::string GetComponentType() const override { return "MergeNode"; }
    
private:
    std::string separator_;
    std::string name_;
};

// ============================================================================
// Helper Functions
// ============================================================================

void PrintSeparator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(70, '=') << std::endl;
}

void PrintGraphInfo(const Graph<std::string, std::string>& graph) {
    std::cout << "\n[Graph Info]" << std::endl;
    std::cout << "  Nodes: " << graph.GetNodeCount() << std::endl;
    std::cout << "  Edges: " << graph.GetEdgeCount() << std::endl;
    
    auto node_names = graph.GetNodeNames();
    std::cout << "  Node names: ";
    for (size_t i = 0; i < node_names.size(); ++i) {
        std::cout << node_names[i];
        if (i < node_names.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    auto start_nodes = graph.GetStartNodes();
    std::cout << "  Start nodes: ";
    for (size_t i = 0; i < start_nodes.size(); ++i) {
        std::cout << start_nodes[i];
        if (i < start_nodes.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    auto end_nodes = graph.GetEndNodes();
    std::cout << "  End nodes: ";
    for (size_t i = 0; i < end_nodes.size(); ++i) {
        std::cout << end_nodes[i];
        if (i < end_nodes.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

// ============================================================================
// Example 1: Simple Linear DAG
// ============================================================================

void Example1_SimpleLinearDAG() {
    PrintSeparator("Example 1: Simple Linear DAG Pipeline");
    
    std::cout << "\n[Description]" << std::endl;
    std::cout << "A simple linear pipeline: Input -> ToUpper -> Reverse -> Output" << std::endl;
    std::cout << "This demonstrates the most basic DAG structure." << std::endl;
    
    // Create graph
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // Create nodes
    auto upper = std::make_shared<ToUpperNode>("ToUpper");
    auto reverse = std::make_shared<ReverseNode>("Reverse");
    
    // Add nodes to graph
    graph->AddNode("upper", upper);
    graph->AddNode("reverse", reverse);
    
    // Add edges: START -> upper -> reverse -> END
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "upper");
    graph->AddEdge("upper", "reverse");
    graph->AddEdge("reverse", Graph<std::string, std::string>::END_NODE);
    
    PrintGraphInfo(*graph);
    
    // Compile graph
    GraphCompileOptions compile_opts;
    compile_opts.graph_name = "LinearPipeline";
    graph->Compile(compile_opts);
    std::cout << "\n[Status] Graph compiled successfully!" << std::endl;
    
    // Execute
    auto ctx = Context::Background();
    std::string input = "hello";
    
    std::cout << "\n[Execution]" << std::endl;
    std::cout << "Input: \"" << input << "\"" << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    std::string output = graph->Invoke(ctx, input);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "\nOutput: \"" << output << "\"" << std::endl;
    std::cout << "Execution time: " << duration.count() << " μs" << std::endl;
}

// ============================================================================
// Example 2: Branching DAG (Fan-out)
// ============================================================================

void Example2_BranchingDAG() {
    PrintSeparator("Example 2: Branching DAG (Fan-out Pattern)");
    
    std::cout << "\n[Description]" << std::endl;
    std::cout << "Demonstrates branching where one node's output feeds multiple nodes:" << std::endl;
    std::cout << "                   -> branch1 (ToUpper) ->" << std::endl;
    std::cout << "  Input -> source                          -> merge -> Output" << std::endl;
    std::cout << "                   -> branch2 (Reverse) ->" << std::endl;
    
    // Create graph
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // Create nodes
    auto source = std::make_shared<PrefixNode>("SOURCE:", "Source");
    auto branch1 = std::make_shared<ToUpperNode>("Branch1_Upper");
    auto branch2 = std::make_shared<ReverseNode>("Branch2_Reverse");
    auto merge = std::make_shared<MergeNode>(" | ", "Merge");
    
    // Add nodes
    graph->AddNode("source", source);
    graph->AddNode("branch1", branch1);
    graph->AddNode("branch2", branch2);
    graph->AddNode("merge", merge, NodeTriggerMode::AnyPredecessor);  // Trigger on any predecessor
    
    // Add edges - branching pattern
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "source");
    graph->AddEdge("source", "branch1");  // Fan-out to branch1
    graph->AddEdge("source", "branch2");  // Fan-out to branch2
    graph->AddEdge("branch1", "merge");   // Converge to merge
    graph->AddEdge("branch2", "merge");   // Converge to merge
    graph->AddEdge("merge", Graph<std::string, std::string>::END_NODE);
    
    PrintGraphInfo(*graph);
    
    // Compile
    GraphCompileOptions compile_opts;
    compile_opts.graph_name = "BranchingDAG";
    graph->Compile(compile_opts);
    std::cout << "\n[Status] Graph compiled successfully!" << std::endl;
    
    // Execute
    auto ctx = Context::Background();
    std::string input = "test";
    
    std::cout << "\n[Execution]" << std::endl;
    std::cout << "Input: \"" << input << "\"" << std::endl;
    std::cout << "Note: Branch1 and Branch2 run in parallel (logically)" << std::endl;
    
    std::string output = graph->Invoke(ctx, input);
    
    std::cout << "\nFinal Output: \"" << output << "\"" << std::endl;
}

// ============================================================================
// Example 3: Lambda Nodes
// ============================================================================

void Example3_LambdaNodes() {
    PrintSeparator("Example 3: Using Lambda Nodes for Quick Transformations");
    
    std::cout << "\n[Description]" << std::endl;
    std::cout << "Lambda nodes allow inline transformations without defining custom classes." << std::endl;
    std::cout << "Pipeline: Input -> trim -> lowercase -> add_suffix -> Output" << std::endl;
    
    // Create graph
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // Create lambda nodes
    auto trim_lambda = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) {
            std::string result = input;
            // Trim leading/trailing spaces
            result.erase(0, result.find_first_not_of(" \t\n\r"));
            result.erase(result.find_last_not_of(" \t\n\r") + 1);
            std::cout << "  [TrimLambda] '" << input << "' -> '" << result << "'" << std::endl;
            return result;
        }
    );
    
    auto lowercase_lambda = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) {
            std::string result = input;
            for (auto& c : result) {
                c = std::tolower(c);
            }
            std::cout << "  [LowercaseLambda] '" << input << "' -> '" << result << "'" << std::endl;
            return result;
        }
    );
    
    auto suffix_lambda = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) {
            std::string result = input + "_processed";
            std::cout << "  [SuffixLambda] '" << input << "' -> '" << result << "'" << std::endl;
            return result;
        }
    );
    
    // Add nodes
    graph->AddNode("trim", trim_lambda);
    graph->AddNode("lowercase", lowercase_lambda);
    graph->AddNode("suffix", suffix_lambda);
    
    // Add edges
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "trim");
    graph->AddEdge("trim", "lowercase");
    graph->AddEdge("lowercase", "suffix");
    graph->AddEdge("suffix", Graph<std::string, std::string>::END_NODE);
    
    PrintGraphInfo(*graph);
    
    // Compile
    graph->Compile();
    std::cout << "\n[Status] Graph compiled!" << std::endl;
    
    // Execute
    auto ctx = Context::Background();
    std::string input = "  HELLO WORLD  ";
    
    std::cout << "\n[Execution]" << std::endl;
    std::cout << "Input: \"" << input << "\" (with spaces)" << std::endl;
    
    std::string output = graph->Invoke(ctx, input);
    
    std::cout << "\nOutput: \"" << output << "\"" << std::endl;
}

// ============================================================================
// Example 4: Complex Multi-Path DAG
// ============================================================================

void Example4_ComplexMultiPathDAG() {
    PrintSeparator("Example 4: Complex Multi-Path DAG");
    
    std::cout << "\n[Description]" << std::endl;
    std::cout << "A more complex DAG with multiple paths and convergence points:" << std::endl;
    std::cout << "                -> pathA_upper -> pathA_reverse ->" << std::endl;
    std::cout << "  Input -> split                                    -> final -> Output" << std::endl;
    std::cout << "                -> pathB_prefix ---------------->" << std::endl;
    
    // Create graph
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // Create nodes
    auto split = std::make_shared<PrefixNode>("[", "Split");
    auto pathA_upper = std::make_shared<ToUpperNode>("PathA_Upper");
    auto pathA_reverse = std::make_shared<ReverseNode>("PathA_Reverse");
    auto pathB_prefix = std::make_shared<PrefixNode>("***", "PathB_Prefix");
    auto final = std::make_shared<PrefixNode>("]", "Final");
    
    // Add nodes with different trigger modes
    graph->AddNode("split", split);
    graph->AddNode("pathA_upper", pathA_upper);
    graph->AddNode("pathA_reverse", pathA_reverse);
    graph->AddNode("pathB_prefix", pathB_prefix);
    graph->AddNode("final", final, NodeTriggerMode::AnyPredecessor);  // Trigger on first completion
    
    // Add edges - complex routing
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "split");
    
    // Path A: split -> upper -> reverse -> final
    graph->AddEdge("split", "pathA_upper");
    graph->AddEdge("pathA_upper", "pathA_reverse");
    graph->AddEdge("pathA_reverse", "final");
    
    // Path B: split -> prefix -> final (shorter path)
    graph->AddEdge("split", "pathB_prefix");
    graph->AddEdge("pathB_prefix", "final");
    
    graph->AddEdge("final", Graph<std::string, std::string>::END_NODE);
    
    PrintGraphInfo(*graph);
    
    // Compile
    GraphCompileOptions compile_opts;
    compile_opts.graph_name = "ComplexMultiPath";
    compile_opts.max_run_steps = 10;
    graph->Compile(compile_opts);
    std::cout << "\n[Status] Graph compiled!" << std::endl;
    
    // Execute
    auto ctx = Context::Background();
    std::string input = "dag";
    
    std::cout << "\n[Execution]" << std::endl;
    std::cout << "Input: \"" << input << "\"" << std::endl;
    std::cout << "Note: PathB is shorter and will likely complete first" << std::endl;
    std::cout << "      'final' node uses AnyPredecessor trigger mode" << std::endl;
    
    std::string output = graph->Invoke(ctx, input);
    
    std::cout << "\nFinal Output: \"" << output << "\"" << std::endl;
}

// ============================================================================
// Example 5: Stream Processing with DAG
// ============================================================================

void Example5_StreamProcessing() {
    PrintSeparator("Example 5: Stream Processing with DAG");
    
    std::cout << "\n[Description]" << std::endl;
    std::cout << "Process a stream of inputs through a DAG pipeline." << std::endl;
    std::cout << "Each item flows through: ToUpper -> Reverse" << std::endl;
    
    // Create graph
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // Create nodes
    auto upper = std::make_shared<ToUpperNode>("StreamUpper");
    auto reverse = std::make_shared<ReverseNode>("StreamReverse");
    
    // Add nodes
    graph->AddNode("upper", upper);
    graph->AddNode("reverse", reverse);
    
    // Add edges
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "upper");
    graph->AddEdge("upper", "reverse");
    graph->AddEdge("reverse", Graph<std::string, std::string>::END_NODE);
    
    // Compile
    graph->Compile();
    std::cout << "\n[Status] Graph compiled!" << std::endl;
    
    // Create input stream
    auto input_stream = std::make_shared<SimpleStreamReader<std::string>>();
    input_stream->Add("apple");
    input_stream->Add("banana");
    input_stream->Add("cherry");
    input_stream->Add("date");
    
    std::cout << "\n[Execution]" << std::endl;
    std::cout << "Processing stream with " << input_stream->GetRemaining() << " items..." << std::endl;
    
    // Execute with stream
    auto ctx = Context::Background();
    auto output_stream = graph->Transform(ctx, input_stream);
    
    std::cout << "\n[Results]" << std::endl;
    std::string result;
    int count = 1;
    while (output_stream && output_stream->Read(result)) {
        std::cout << "  Item " << count++ << ": \"" << result << "\"" << std::endl;
    }
}

// ============================================================================
// Example 6: JSON Processing DAG
// ============================================================================

void Example6_JSONProcessing() {
    PrintSeparator("Example 6: JSON Data Processing DAG");
    
    std::cout << "\n[Description]" << std::endl;
    std::cout << "Process JSON objects through a DAG pipeline." << std::endl;
    std::cout << "Pipeline: extract -> transform -> enrich" << std::endl;
    
    // Create graph for JSON processing
    auto graph = std::make_shared<Graph<json, json>>();
    
    // Lambda to extract name field
    auto extract = NewLambdaRunnable<json, json>(
        [](std::shared_ptr<Context> ctx, const json& input, const std::vector<Option>& opts) {
            json result;
            std::string name_key = "name";
            std::string extracted_key = "extracted_name";
            if (input.contains(name_key)) {
                result[extracted_key] = input[name_key];
            }
            std::cout << "  [Extract] Input: " << input.dump() << std::endl;
            std::cout << "            Output: " << result.dump() << std::endl;
            return result;
        }
    );
    
    // Lambda to transform name to uppercase
    auto transform = NewLambdaRunnable<json, json>(
        [](std::shared_ptr<Context> ctx, const json& input, const std::vector<Option>& opts) {
            json result = input;
            std::string extracted_key = "extracted_name";
            std::string transformed_key = "transformed_name";
            if (result.contains(extracted_key) && result[extracted_key].is_string()) {
                std::string name = result[extracted_key];
                for (auto& c : name) {
                    c = std::toupper(c);
                }
                result[transformed_key] = name;
            }
            std::cout << "  [Transform] Output: " << result.dump() << std::endl;
            return result;
        }
    );
    
    // Lambda to enrich with metadata
    auto enrich = NewLambdaRunnable<json, json>(
        [](std::shared_ptr<Context> ctx, const json& input, const std::vector<Option>& opts) {
            json result = input;
            std::string processed_key = "processed";
            std::string timestamp_key = "timestamp";
            std::string pipeline_key = "pipeline";
            result[processed_key] = true;
            result[timestamp_key] = "2024-01-01T00:00:00Z";
            result[pipeline_key] = "DAG_JSON_Processor";
            std::cout << "  [Enrich] Final: " << result.dump() << std::endl;
            return result;
        }
    );
    
    // Add nodes
    graph->AddNode("extract", extract);
    graph->AddNode("transform", transform);
    graph->AddNode("enrich", enrich);
    
    // Add edges
    graph->AddEdge(Graph<json, json>::START_NODE, "extract");
    graph->AddEdge("extract", "transform");
    graph->AddEdge("transform", "enrich");
    graph->AddEdge("enrich", Graph<json, json>::END_NODE);
    
    // Compile
    graph->Compile();
    std::cout << "\n[Status] Graph compiled!" << std::endl;
    
    // Execute
    auto ctx = Context::Background();
    json input;
    std::string key_name = "name";
    std::string key_age = "age";
    std::string key_city = "city";
    input[key_name] = "alice";
    input[key_age] = 30;
    input[key_city] = "wonderland";
    
    std::cout << "\n[Execution]" << std::endl;
    std::cout << "Input JSON: " << input.dump(2) << std::endl;
    
    json output = graph->Invoke(ctx, input);
    
    std::cout << "\n[Final Output]" << std::endl;
    std::cout << output.dump(2) << std::endl;
}

// ============================================================================
// Main Function
// ============================================================================

int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════════╗
║                                                                      ║
║         Eino C++ DAG Compose Module - Comprehensive Examples        ║
║                                                                      ║
║  Demonstrates DAG construction, execution, and composition patterns  ║
║                                                                      ║
╚══════════════════════════════════════════════════════════════════════╝
)" << std::endl;
    
    try {
        // Run all examples
        Example1_SimpleLinearDAG();
        Example2_BranchingDAG();
        Example3_LambdaNodes();
        Example4_ComplexMultiPathDAG();
        Example5_StreamProcessing();
        Example6_JSONProcessing();
        
        PrintSeparator("All Examples Completed Successfully! ✓");
        
        std::cout << "\n[Summary]" << std::endl;
        std::cout << "This demo covered:" << std::endl;
        std::cout << "  ✓ Linear DAG pipelines" << std::endl;
        std::cout << "  ✓ Branching and merging (fan-out/fan-in)" << std::endl;
        std::cout << "  ✓ Lambda nodes for inline transformations" << std::endl;
        std::cout << "  ✓ Complex multi-path DAG structures" << std::endl;
        std::cout << "  ✓ Stream-based processing" << std::endl;
        std::cout << "  ✓ JSON data processing" << std::endl;
        std::cout << "  ✓ Different node trigger modes" << std::endl;
        std::cout << "  ✓ Graph compilation and execution" << std::endl;
        
        std::cout << "\n[Next Steps]" << std::endl;
        std::cout << "  - Try modifying the examples with your own nodes" << std::endl;
        std::cout << "  - Experiment with different trigger modes" << std::endl;
        std::cout << "  - Add conditional edges for dynamic routing" << std::endl;
        std::cout << "  - Integrate with checkpoints for state management" << std::endl;
        std::cout << "  - Use interrupts for debugging and control flow" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
