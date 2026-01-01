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

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace eino {
namespace compose {

// Forward declarations
class CheckPointer;
class EdgeHandlerManager;
class PreNodeHandlerManager;
class Serializer;  // From checkpoint.h

// Fan-in merge configuration
struct FanInMergeConfig {
    bool stream_merge_with_source_eof = false;
    
    FanInMergeConfig() = default;
};

// Compile options for graph
struct GraphCompileOptions {
    // Graph name for debugging and logging
    std::string graph_name;
    
    // Maximum number of steps for graph execution
    int max_run_steps = -1;  // -1 means no limit
    
    // Checkpointer for state persistence
    std::shared_ptr<CheckPointer> checkpointer;
    
    // Serializer for checkpoint data (used with checkpointer)
    std::shared_ptr<Serializer> serializer;
    
    // Edge handlers
    std::shared_ptr<EdgeHandlerManager> edge_handler_manager;
    
    // Pre-node handlers
    std::shared_ptr<PreNodeHandlerManager> pre_node_handler_manager;
    
    // Fan-in merge configuration per node
    std::map<std::string, FanInMergeConfig> fan_in_merge_config;
    
    // Maximum parallelism
    size_t max_parallelism = 0;  // 0 means unlimited
    
    GraphCompileOptions() = default;
};

// Option function type
using GraphCompileOption = std::function<void(GraphCompileOptions&)>;

// WithCheckPointer sets the checkpointer
GraphCompileOption WithCheckPointer(std::shared_ptr<CheckPointer> checkpointer);

// WithEdgeHandler adds an edge handler
GraphCompileOption WithEdgeHandler(std::shared_ptr<EdgeHandlerManager> manager);

// WithPreNodeHandler adds a pre-node handler
GraphCompileOption WithPreNodeHandler(std::shared_ptr<PreNodeHandlerManager> manager);

// WithFanInMergeConfig sets fan-in merge configuration for a node
GraphCompileOption WithFanInMergeConfig(const std::string& node_key, const FanInMergeConfig& config);

// WithMaxParallelism sets maximum parallelism
GraphCompileOption WithMaxParallelism(size_t max_parallelism);

// WithGraphName sets a name for the graph
// Aligns with: eino/compose/graph_compile_options.go:65-68
GraphCompileOption WithGraphName(const std::string& graph_name);

// WithMaxRunSteps sets the maximum number of steps that a graph can run
// Aligns with: eino/compose/graph_compile_options.go:56-60
GraphCompileOption WithMaxRunSteps(int max_steps);

// Helper to create GraphCompileOptions from options
std::shared_ptr<GraphCompileOptions> NewGraphCompileOptions(
    const std::vector<GraphCompileOption>& opts);

} // namespace compose
} // namespace eino
