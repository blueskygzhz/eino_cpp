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

#include "eino/callbacks/callback.h"

namespace eino {
namespace compose {

// NodePath represents a path to a node in the graph hierarchy
using NodePath = std::vector<std::string>;

// Helper to create node path
NodePath NewNodePath(const std::vector<std::string>& path);

// Options for graph execution
struct Option {
    // Callback handlers
    std::vector<std::shared_ptr<callbacks::Handler>> handlers;
    
    // Node paths for targeted options
    std::vector<NodePath> paths;
    
    // Component-specific options
    std::vector<Any> options;
    
    Option() = default;
    
    // Deep copy the option
    Option DeepCopy() const;
};

// Option function type
using GraphCallOption = std::function<void(Option&)>;

// WithCallbacks adds callbacks to the graph execution
GraphCallOption WithCallbacks(const std::vector<std::shared_ptr<callbacks::Handler>>& handlers);

// WithNodeCallbacks adds callbacks for specific nodes
GraphCallOption WithNodeCallbacks(
    const NodePath& path,
    const std::vector<std::shared_ptr<callbacks::Handler>>& handlers);

// WithNodeOptions adds options for specific nodes
template <typename T>
GraphCallOption WithNodeOptions(const NodePath& path, const std::vector<T>& opts) {
    return [path, opts](Option& option) {
        option.paths.push_back(path);
        for (const auto& opt : opts) {
            option.options.push_back(opt);
        }
    };
}

// WithComponentOptions adds options for components
template <typename T>
GraphCallOption WithComponentOptions(const std::vector<T>& opts) {
    return [opts](Option& option) {
        for (const auto& opt : opts) {
            option.options.push_back(opt);
        }
    };
}

// Helper to get Option from options
std::vector<Option> GetGraphCallOptions(const std::vector<GraphCallOption>& opts);

} // namespace compose
} // namespace eino
