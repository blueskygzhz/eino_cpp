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

#include "eino/compose/graph_add_node_options.h"

namespace eino {
namespace compose {

GraphAddNodeOpt WithNodeName(const std::string& name) {
    return [name](GraphAddNodeOpts& opts) {
        opts.node_options.node_name = name;
    };
}

GraphAddNodeOpt WithInputKey(const std::string& key) {
    return [key](GraphAddNodeOpts& opts) {
        opts.node_options.input_key = key;
    };
}

GraphAddNodeOpt WithOutputKey(const std::string& key) {
    return [key](GraphAddNodeOpts& opts) {
        opts.node_options.output_key = key;
    };
}

GraphAddNodeOpt WithStatePreHandler(std::shared_ptr<ComposableRunnable> handler) {
    return [handler](GraphAddNodeOpts& opts) {
        opts.processor.state_pre_handler = handler;
    };
}

GraphAddNodeOpt WithStatePostHandler(std::shared_ptr<ComposableRunnable> handler) {
    return [handler](GraphAddNodeOpts& opts) {
        opts.processor.state_post_handler = handler;
    };
}

GraphAddNodeOpt WithGraphCompileOptions(std::shared_ptr<GraphCompileOptions> compile_opts) {
    return [compile_opts](GraphAddNodeOpts& opts) {
        opts.node_options.graph_compile_options.push_back(compile_opts);
    };
}

GraphAddNodeOpts GetGraphAddNodeOpts(const std::vector<GraphAddNodeOpt>& opts) {
    GraphAddNodeOpts result;
    for (const auto& opt : opts) {
        if (opt) {
            opt(result);
        }
    }
    return result;
}

} // namespace compose
} // namespace eino
