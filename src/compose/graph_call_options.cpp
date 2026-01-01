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

#include "eino/compose/graph_call_options.h"

namespace eino {
namespace compose {

NodePath NewNodePath(const std::vector<std::string>& path) {
    return path;
}

Option Option::DeepCopy() const {
    Option copy;
    copy.handlers = handlers;
    copy.paths = paths;
    copy.options = options;
    return copy;
}

GraphCallOption WithCallbacks(const std::vector<std::shared_ptr<callbacks::Handler>>& handlers) {
    return [handlers](Option& option) {
        option.handlers.insert(option.handlers.end(), handlers.begin(), handlers.end());
    };
}

GraphCallOption WithNodeCallbacks(
    const NodePath& path,
    const std::vector<std::shared_ptr<callbacks::Handler>>& handlers) {
    return [path, handlers](Option& option) {
        option.paths.push_back(path);
        option.handlers.insert(option.handlers.end(), handlers.begin(), handlers.end());
    };
}

std::vector<Option> GetGraphCallOptions(const std::vector<GraphCallOption>& opts) {
    std::vector<Option> result;
    Option current;
    
    for (const auto& opt : opts) {
        if (opt) {
            opt(current);
        }
    }
    
    if (!current.handlers.empty() || !current.paths.empty() || !current.options.empty()) {
        result.push_back(current);
    }
    
    return result;
}

} // namespace compose
} // namespace eino
