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
#include <memory>
#include <string>
#include <vector>

#include "eino/compose/compose.h"

namespace eino {
namespace compose {

// Forward declarations
class ComposableRunnable;
class GraphCompileOptions;

// Options for node processors
struct ProcessorOptions {
    std::shared_ptr<ComposableRunnable> state_pre_handler;
    std::shared_ptr<ComposableRunnable> state_post_handler;
    
    ProcessorOptions() = default;
};

// Options for node configuration
struct NodeOptions {
    std::string node_name;
    std::string input_key;
    std::string output_key;
    std::vector<std::shared_ptr<GraphCompileOptions>> graph_compile_options;
    
    NodeOptions() = default;
};

// Combined add node options
struct GraphAddNodeOpts {
    ProcessorOptions processor;
    NodeOptions node_options;
    
    GraphAddNodeOpts() = default;
};

// Option type for AddNode
using GraphAddNodeOpt = std::function<void(GraphAddNodeOpts&)>;

// WithNodeName sets the display name of the node
GraphAddNodeOpt WithNodeName(const std::string& name);

// WithInputKey sets the input key for map-based input
GraphAddNodeOpt WithInputKey(const std::string& key);

// WithOutputKey sets the output key for map-based output
GraphAddNodeOpt WithOutputKey(const std::string& key);

// WithStatePreHandler sets a pre-processor for the node
GraphAddNodeOpt WithStatePreHandler(std::shared_ptr<ComposableRunnable> handler);

// WithStatePostHandler sets a post-processor for the node
GraphAddNodeOpt WithStatePostHandler(std::shared_ptr<ComposableRunnable> handler);

// WithGraphCompileOptions sets compile options for sub-graphs
GraphAddNodeOpt WithGraphCompileOptions(std::shared_ptr<GraphCompileOptions> opts);

// Helper to get GraphAddNodeOpts from options
GraphAddNodeOpts GetGraphAddNodeOpts(const std::vector<GraphAddNodeOpt>& opts);

} // namespace compose
} // namespace eino
