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

#include "eino/compose/component_to_graph_node.h"

#include "eino/compose/tool_node.h"

namespace eino {
namespace compose {

std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToToolsNode(
    std::shared_ptr<ToolsNode> node,
    const std::vector<GraphAddNodeOpt>& opts) {
    
    if (!node) {
        throw std::invalid_argument("ToToolsNode: node cannot be null");
    }
    
    auto meta = std::make_shared<ExecutorMeta>(
        Component::ToolsNode,
        false,  // callback enabled
        "ToolsNode");
    
    auto info = GetNodeInfo(opts);
    
    // Create composable runnable from ToolsNode
    // This requires wrapping the ToolsNode's Invoke/Stream methods
    // For now, store the node directly
    auto gn = ToNode(info, nullptr, nullptr, meta, node, opts);
    
    auto options = GetGraphAddNodeOpts(opts);
    
    return {gn, options};
}

std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToLambdaNode(
    std::shared_ptr<Lambda> node,
    const std::vector<GraphAddNodeOpt>& opts) {
    
    if (!node) {
        throw std::invalid_argument("ToLambdaNode: node cannot be null");
    }
    
    auto info = GetNodeInfo(opts);
    
    // Lambda already contains executor and meta
    auto gn = ToNode(info, node->GetExecutor(), nullptr, node->GetMeta(), node, opts);
    
    auto options = GetGraphAddNodeOpts(opts);
    
    return {gn, options};
}

std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToAnyGraphNode(
    std::shared_ptr<AnyGraph> node,
    const std::vector<GraphAddNodeOpt>& opts) {
    
    if (!node) {
        throw std::invalid_argument("ToAnyGraphNode: node cannot be null");
    }
    
    auto meta = std::make_shared<ExecutorMeta>(
        Component::Graph,
        false,
        "Graph");
    
    auto info = GetNodeInfo(opts);
    
    auto gn = ToNode(info, nullptr, node, meta, node, opts);
    
    auto options = GetGraphAddNodeOpts(opts);
    
    return {gn, options};
}

std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToPassthroughNode(
    const std::vector<GraphAddNodeOpt>& opts) {
    
    // Create passthrough composable runnable
    // A passthrough simply forwards input to output
    class PassthroughRunnable : public ComposableRunnable {
    public:
        Any Invoke(const Context& ctx, const Any& input) override {
            return input;
        }
        
        StreamReader Stream(const Context& ctx, const Any& input) override {
            // For stream, wrap the input as a single-item stream
            // This requires stream construction utilities
            throw std::runtime_error("Passthrough stream not yet implemented");
        }
    };
    
    auto passthrough = std::make_shared<PassthroughRunnable>();
    
    auto meta = std::make_shared<ExecutorMeta>(
        Component::Passthrough,
        false,
        "Passthrough");
    
    auto info = GetNodeInfo(opts);
    
    auto gn = ToNode(info, passthrough, nullptr, meta, passthrough, opts);
    
    auto options = GetGraphAddNodeOpts(opts);
    
    return {gn, options};
}

std::shared_ptr<GraphNode> ToNode(
    std::shared_ptr<NodeInfo> node_info,
    std::shared_ptr<ComposableRunnable> executor,
    std::shared_ptr<AnyGraph> graph,
    std::shared_ptr<ExecutorMeta> meta,
    const Any& instance,
    const std::vector<GraphAddNodeOpt>& opts) {
    
    if (!meta) {
        meta = std::make_shared<ExecutorMeta>();
    }
    
    auto gn = std::make_shared<GraphNode>();
    
    if (executor) {
        *gn = GraphNode(executor, node_info, meta);
    } else if (graph) {
        *gn = GraphNode(graph, node_info, meta);
    } else {
        throw std::invalid_argument("ToNode: either executor or graph must be provided");
    }
    
    gn->SetInstance(instance);
    gn->SetOptions(opts);
    
    return gn;
}

} // namespace compose
} // namespace eino
