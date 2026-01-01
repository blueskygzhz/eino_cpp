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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "eino/compose/compose.h"

namespace eino {
namespace compose {

// Component type enumeration
enum class Component {
    ChatModel,
    ChatTemplate,
    ToolsNode,
    Retriever,
    Embedding,
    Loader,
    Indexer,
    DocumentTransformer,
    Lambda,
    Passthrough,
    Graph,
    Unknown
};

// Executor metadata - information about the original executable object
struct ExecutorMeta {
    // Component type automatically identified
    Component component = Component::Unknown;
    
    // Whether the component can execute callbacks itself
    bool is_component_callback_enabled = false;
    
    // Component implementation type name
    std::string component_impl_type;
    
    ExecutorMeta() = default;
    
    ExecutorMeta(Component comp, bool callback_enabled, const std::string& impl_type)
        : component(comp),
          is_component_callback_enabled(callback_enabled),
          component_impl_type(impl_type) {}
};

// Forward declarations
class ComposableRunnable;
class AnyGraph;
class GraphCompileOptions;

// Node information for display and configuration
struct NodeInfo {
    // Display name, not necessarily unique
    std::string name;
    
    // Input/output key for map-based I/O
    std::string input_key;
    std::string output_key;
    
    // Pre/post processors
    std::shared_ptr<ComposableRunnable> pre_processor;
    std::shared_ptr<ComposableRunnable> post_processor;
    
    // Compile options for sub-graphs
    std::shared_ptr<GraphCompileOptions> compile_option;
    
    NodeInfo() = default;
};

// Graph node - complete information of a node in the graph
class GraphNode {
public:
    GraphNode() = default;
    
    // Constructor for component node
    GraphNode(std::shared_ptr<ComposableRunnable> cr,
              std::shared_ptr<NodeInfo> info,
              std::shared_ptr<ExecutorMeta> meta)
        : cr_(std::move(cr)),
          node_info_(std::move(info)),
          executor_meta_(std::move(meta)) {}
    
    // Constructor for graph node
    GraphNode(std::shared_ptr<AnyGraph> g,
              std::shared_ptr<NodeInfo> info,
              std::shared_ptr<ExecutorMeta> meta)
        : g_(std::move(g)),
          node_info_(std::move(info)),
          executor_meta_(std::move(meta)) {}
    
    // Get generic helper for type handling
    // GenericHelper* GetGenericHelper() const;
    
    // Get input type
    // std::type_info InputType() const;
    
    // Get output type
    // std::type_info OutputType() const;
    
    // Compile the graph if needed and return the runnable
    std::shared_ptr<ComposableRunnable> CompileIfNeeded(const Context& ctx);
    
    // Get node info
    std::shared_ptr<NodeInfo> GetNodeInfo() const { return node_info_; }
    
    // Get executor meta
    std::shared_ptr<ExecutorMeta> GetExecutorMeta() const { return executor_meta_; }
    
    // Get composable runnable
    std::shared_ptr<ComposableRunnable> GetComposableRunnable() const { return cr_; }
    
    // Get graph
    std::shared_ptr<AnyGraph> GetGraph() const { return g_; }
    
    // Set instance (for introspection)
    void SetInstance(const Any& instance) { instance_ = instance; }
    
    // Get instance
    const Any& GetInstance() const { return instance_; }
    
    // Set options
    void SetOptions(const std::vector<GraphAddNodeOpt>& opts) { opts_ = opts; }
    
    // Get options
    const std::vector<GraphAddNodeOpt>& GetOptions() const { return opts_; }

private:
    // Either cr_ or g_ should be set
    std::shared_ptr<ComposableRunnable> cr_;  // Component runnable
    std::shared_ptr<AnyGraph> g_;              // Sub-graph
    
    std::shared_ptr<NodeInfo> node_info_;
    std::shared_ptr<ExecutorMeta> executor_meta_;
    
    Any instance_;  // Original instance for introspection
    std::vector<GraphAddNodeOpt> opts_;  // Node options
};

// Helper functions to create ExecutorMeta from components
ExecutorMeta ParseExecutorInfoFromComponent(Component c, const Any& executor);

// Helper to create NodeInfo from options
std::shared_ptr<NodeInfo> GetNodeInfo(const std::vector<GraphAddNodeOpt>& opts);

// Helper to wrap composable runnable with input/output keys
std::shared_ptr<ComposableRunnable> InputKeyedComposableRunnable(
    const std::string& key,
    std::shared_ptr<ComposableRunnable> inner);

std::shared_ptr<ComposableRunnable> OutputKeyedComposableRunnable(
    const std::string& key,
    std::shared_ptr<ComposableRunnable> inner);

} // namespace compose
} // namespace eino
