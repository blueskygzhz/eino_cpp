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

#ifndef EINO_CPP_COMPOSE_GRAPH_METHODS_H_
#define EINO_CPP_COMPOSE_GRAPH_METHODS_H_

#include <memory>
#include <vector>
#include <string>
#include "graph.h"
#include "component_to_graph_node.h"
#include "tool_node.h"
#include "branch.h"
#include "types_lambda.h"
#include "eino/components/model.h"

namespace eino {
namespace compose {

// Graph method implementations aligned with eino/compose/graph.go

// AddChatModelNode adds a node that implements BaseChatModel
// Aligns with eino/compose/graph.go:350-353 (AddChatModelNode)
// Example:
//   auto chat_model = openai::NewChatModel(ctx, config);
//   graph->AddChatModelNode("chat_model", chat_model);
template<typename I, typename O>
void AddChatModelNode(
    Graph<I, O>* graph,
    const std::string& key,
    std::shared_ptr<components::BaseChatModel> chat_model,
    const std::vector<GraphAddNodeOpt>& opts = {}) {
    
    if (!graph) {
        throw std::invalid_argument("Graph cannot be null");
    }
    
    if (!chat_model) {
        throw std::invalid_argument("ChatModel cannot be null");
    }
    
    // Convert ChatModel to GraphNode
    // Aligns with eino/compose/component_to_graph_node.go:92-97 (toChatModelNode)
    auto [graph_node, options] = ToChatModelNode<I, O>(chat_model, opts);
    
    // Add node to graph
    // Aligns with eino/compose/graph.go:351
    graph->AddNodeInternal(key, graph_node, options);
}

// AddToolsNode adds a node that implements ToolsNode
// Aligns with eino/compose/graph.go:375-378 (AddToolsNode)
// Example:
//   auto tools_node = NewToolsNode(ctx, config);
//   graph->AddToolsNode("tools", tools_node);
template<typename I, typename O>
void AddToolsNode(
    Graph<I, O>* graph,
    const std::string& key,
    std::shared_ptr<ToolsNode> tools_node,
    const std::vector<GraphAddNodeOpt>& opts = {}) {
    
    if (!graph) {
        throw std::invalid_argument("Graph cannot be null");
    }
    
    if (!tools_node) {
        throw std::invalid_argument("ToolsNode cannot be null");
    }
    
    // Convert ToolsNode to GraphNode
    // Aligns with eino/compose/component_to_graph_node.go:113-120 (toToolsNode)
    auto [graph_node, options] = ToToolsNode(tools_node, opts);
    
    // Add node to graph
    // Aligns with eino/compose/graph.go:376
    graph->AddNodeInternal(key, graph_node, options);
}

// AddLambdaNode adds a node that implements at least one of:
// Invoke[I, O], Stream[I, O], Collect[I, O], Transform[I, O]
// Aligns with eino/compose/graph.go:398-407 (AddLambdaNode)
// Example:
//   auto lambda = InvokableLambda<Input, Output>([](ctx, input) {
//       return process(input);
//   });
//   graph->AddLambdaNode("processor", lambda);
template<typename I, typename O>
void AddLambdaNode(
    Graph<I, O>* graph,
    const std::string& key,
    std::shared_ptr<Lambda> lambda,
    const std::vector<GraphAddNodeOpt>& opts = {}) {
    
    if (!graph) {
        throw std::invalid_argument("Graph cannot be null");
    }
    
    if (!lambda) {
        throw std::invalid_argument("Lambda cannot be null");
    }
    
    // Convert Lambda to GraphNode
    // Aligns with eino/compose/component_to_graph_node.go:122-128 (toLambdaNode)
    auto [graph_node, options] = ToLambdaNode(lambda, opts);
    
    // Add node to graph
    // Aligns with eino/compose/graph.go:399
    graph->AddNodeInternal(key, graph_node, options);
}

// AddBranch adds a conditional branch to the graph
// Aligns with eino/compose/graph.go:444-447 (AddBranch)
// Example:
//   auto condition = [](ctx, msg) -> std::string {
//       return msg.tool_calls.empty() ? END : "ToolNode";
//   };
//   auto branch = NewGraphBranch(condition, {"ToolNode", END});
//   graph->AddBranch("ChatModel", branch);
template<typename I, typename O>
void AddBranch(
    Graph<I, O>* graph,
    const std::string& start_node,
    std::shared_ptr<GraphBranch> branch) {
    
    if (!graph) {
        throw std::invalid_argument("Graph cannot be null");
    }
    
    if (start_node.empty()) {
        throw std::invalid_argument("Start node cannot be empty");
    }
    
    if (!branch) {
        throw std::invalid_argument("Branch cannot be null");
    }
    
    // Add branch to graph
    // Aligns with eino/compose/graph.go:446
    graph->AddBranchInternal(start_node, branch);
}

// Helper: Create state generator for stateful graphs
// Aligns with eino/compose/graph.go state generation pattern
template<typename S>
std::function<std::shared_ptr<S>(void*)> MakeStateGenerator() {
    return [](void* ctx) -> std::shared_ptr<S> {
        return std::make_shared<S>();
    };
}

// Helper: Add edge with field mapping
// Aligns with eino/compose/graph.go:511-530 (AddEdge with mappings)
template<typename I, typename O>
void AddEdgeWithMapping(
    Graph<I, O>* graph,
    const std::string& from,
    const std::string& to,
    const std::vector<FieldMapping>& mappings = {}) {
    
    if (!graph) {
        throw std::invalid_argument("Graph cannot be null");
    }
    
    std::vector<std::shared_ptr<FieldMapping>> mapping_ptrs;
    for (const auto& m : mappings) {
        mapping_ptrs.push_back(std::make_shared<FieldMapping>(m));
    }
    
    graph->AddEdge(from, to, false, false, mapping_ptrs);
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_GRAPH_METHODS_H_
