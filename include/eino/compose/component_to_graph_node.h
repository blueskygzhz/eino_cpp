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

#pragma once

#include <memory>
#include <vector>

#include "eino/compose/graph_node.h"
#include "eino/compose/graph_add_node_options.h"
#include "eino/compose/types_lambda.h"

namespace eino {
namespace compose {

// Forward declarations
class ToolsNode;
class ComposableRunnable;

// Convert component to graph node with metadata
// This is the generic template for all component types
template <typename I, typename O, typename TOption = InvokeOptions>
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToComponentNode(
    const Any& node,
    Component component_type,
    Invoke<I, O, TOption> invoke_fn,
    Stream<I, O, TOption> stream_fn,
    Collect<I, O, TOption> collect_fn,
    Transform<I, O, TOption> transform_fn,
    const std::vector<GraphAddNodeOpt>& opts = {});

// Specific conversion functions for each component type

// Convert chat model to graph node
template <typename I, typename O, typename TOption = InvokeOptions>
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToChatModelNode(
    const Any& node,
    const std::vector<GraphAddNodeOpt>& opts = {});

// Convert chat template to graph node
template <typename I, typename O, typename TOption = InvokeOptions>
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToChatTemplateNode(
    const Any& node,
    const std::vector<GraphAddNodeOpt>& opts = {});

// Convert embedding to graph node
template <typename I, typename O, typename TOption = InvokeOptions>
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToEmbeddingNode(
    const Any& node,
    const std::vector<GraphAddNodeOpt>& opts = {});

// Convert retriever to graph node
template <typename I, typename O, typename TOption = InvokeOptions>
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToRetrieverNode(
    const Any& node,
    const std::vector<GraphAddNodeOpt>& opts = {});

// Convert loader to graph node
template <typename I, typename O, typename TOption = InvokeOptions>
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToLoaderNode(
    const Any& node,
    const std::vector<GraphAddNodeOpt>& opts = {});

// Convert indexer to graph node
template <typename I, typename O, typename TOption = InvokeOptions>
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToIndexerNode(
    const Any& node,
    const std::vector<GraphAddNodeOpt>& opts = {});

// Convert document transformer to graph node
template <typename I, typename O, typename TOption = InvokeOptions>
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToDocumentTransformerNode(
    const Any& node,
    const std::vector<GraphAddNodeOpt>& opts = {});

// Convert tools node to graph node
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToToolsNode(
    std::shared_ptr<ToolsNode> node,
    const std::vector<GraphAddNodeOpt>& opts = {});

// Convert lambda to graph node
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToLambdaNode(
    std::shared_ptr<Lambda> node,
    const std::vector<GraphAddNodeOpt>& opts = {});

// Convert any graph to graph node
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToAnyGraphNode(
    std::shared_ptr<AnyGraph> node,
    const std::vector<GraphAddNodeOpt>& opts = {});

// Convert passthrough to graph node
std::pair<std::shared_ptr<GraphNode>, GraphAddNodeOpts> ToPassthroughNode(
    const std::vector<GraphAddNodeOpt>& opts = {});

// Helper to create graph node from components
std::shared_ptr<GraphNode> ToNode(
    std::shared_ptr<NodeInfo> node_info,
    std::shared_ptr<ComposableRunnable> executor,
    std::shared_ptr<AnyGraph> graph,
    std::shared_ptr<ExecutorMeta> meta,
    const Any& instance,
    const std::vector<GraphAddNodeOpt>& opts = {});

} // namespace compose
} // namespace eino
