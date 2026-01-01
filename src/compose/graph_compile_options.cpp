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

#include "eino/compose/graph_compile_options.h"

#include "eino/compose/checkpoint.h"
#include "eino/compose/graph_manager.h"

namespace eino {
namespace compose {

GraphCompileOption WithCheckPointer(std::shared_ptr<CheckPointer> checkpointer) {
    return [checkpointer](GraphCompileOptions& opts) {
        opts.checkpointer = checkpointer;
    };
}

GraphCompileOption WithEdgeHandler(std::shared_ptr<EdgeHandlerManager> manager) {
    return [manager](GraphCompileOptions& opts) {
        opts.edge_handler_manager = manager;
    };
}

GraphCompileOption WithPreNodeHandler(std::shared_ptr<PreNodeHandlerManager> manager) {
    return [manager](GraphCompileOptions& opts) {
        opts.pre_node_handler_manager = manager;
    };
}

GraphCompileOption WithFanInMergeConfig(const std::string& node_key, const FanInMergeConfig& config) {
    return [node_key, config](GraphCompileOptions& opts) {
        opts.fan_in_merge_config[node_key] = config;
    };
}

GraphCompileOption WithMaxParallelism(size_t max_parallelism) {
    return [max_parallelism](GraphCompileOptions& opts) {
        opts.max_parallelism = max_parallelism;
    };
}

GraphCompileOption WithGraphName(const std::string& graph_name) {
    return [graph_name](GraphCompileOptions& opts) {
        opts.graph_name = graph_name;
    };
}

GraphCompileOption WithMaxRunSteps(int max_steps) {
    return [max_steps](GraphCompileOptions& opts) {
        opts.max_run_steps = max_steps;
    };
}

std::shared_ptr<GraphCompileOptions> NewGraphCompileOptions(
    const std::vector<GraphCompileOption>& opts) {
    auto options = std::make_shared<GraphCompileOptions>();
    for (const auto& opt : opts) {
        if (opt) {
            opt(*options);
        }
    }
    return options;
}

} // namespace compose
} // namespace eino
