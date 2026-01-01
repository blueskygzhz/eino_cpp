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

#include "eino/compose/utils.h"

namespace eino {
namespace compose {

std::pair<Context, std::string> OnError(const Context& ctx, const std::string& error) {
    auto handlers = callbacks::GetHandlersFromContext(ctx);
    if (handlers.empty()) {
        return {ctx, error};
    }
    
    Context new_ctx = ctx;
    for (auto& handler : handlers) {
        new_ctx = handler->OnError(new_ctx, error);
    }
    
    return {new_ctx, error};
}

std::pair<Context, Any> OnGraphStart(const Context& ctx, const Any& input, bool is_stream) {
    if (is_stream) {
        // Handle stream input
        try {
            auto stream = std::any_cast<std::shared_ptr<IStreamReader>>(input);
            // Generic stream handling
            return {ctx, input};
        } catch (const std::bad_any_cast&) {
            // Not a stream, fall through
        }
    }
    
    // Handle regular input
    auto handlers = callbacks::GetHandlersFromContext(ctx);
    if (handlers.empty()) {
        return {ctx, input};
    }
    
    Context new_ctx = ctx;
    // Invoke callbacks
    return {new_ctx, input};
}

std::pair<Context, Any> OnGraphEnd(const Context& ctx, const Any& output, bool is_stream) {
    if (is_stream) {
        // Handle stream output
        try {
            auto stream = std::any_cast<std::shared_ptr<IStreamReader>>(output);
            return {ctx, output};
        } catch (const std::bad_any_cast&) {
            // Not a stream, fall through
        }
    }
    
    auto handlers = callbacks::GetHandlersFromContext(ctx);
    if (handlers.empty()) {
        return {ctx, output};
    }
    
    Context new_ctx = ctx;
    return {new_ctx, output};
}

std::pair<Context, std::string> OnGraphError(const Context& ctx, const std::string& error) {
    return OnError(ctx, error);
}

Context InitGraphCallbacks(const Context& ctx,
                            const NodeInfo* info,
                            const ExecutorMeta* meta,
                            const std::vector<Option>& opts) {
    callbacks::RunInfo ri;
    
    if (meta) {
        ri.component = static_cast<int>(meta->component);
        ri.type = meta->component_impl_type;
    }
    
    if (info) {
        ri.name = info->name;
    }
    
    // Collect handlers from options
    std::vector<std::shared_ptr<callbacks::Handler>> handlers;
    for (const auto& opt : opts) {
        if (!opt.handlers.empty() && opt.paths.empty()) {
            handlers.insert(handlers.end(), opt.handlers.begin(), opt.handlers.end());
        }
    }
    
    if (handlers.empty()) {
        return callbacks::ReuseHandlers(ctx, ri);
    }
    
    return callbacks::AppendHandlers(ctx, ri, handlers);
}

Context InitNodeCallbacks(const Context& ctx,
                           const std::string& key,
                           const NodeInfo* info,
                           const ExecutorMeta* meta,
                           const std::vector<Option>& opts) {
    callbacks::RunInfo ri;
    
    if (meta) {
        ri.component = static_cast<int>(meta->component);
        ri.type = meta->component_impl_type;
    }
    
    if (info) {
        ri.name = info->name;
    }
    
    // Collect handlers for this specific node
    std::vector<std::shared_ptr<callbacks::Handler>> handlers;
    for (const auto& opt : opts) {
        if (!opt.handlers.empty() && !opt.paths.empty()) {
            for (const auto& path : opt.paths) {
                if (path.size() == 1 && path[0] == key) {
                    handlers.insert(handlers.end(), opt.handlers.begin(), opt.handlers.end());
                    break;
                }
            }
        }
    }
    
    if (handlers.empty()) {
        return callbacks::ReuseHandlers(ctx, ri);
    }
    
    return callbacks::AppendHandlers(ctx, ri, handlers);
}

AssignableType CheckAssignable(const std::type_info& input, const std::type_info& arg) {
    // Aligns with eino/compose/utils.go:233-253 (checkAssignable)
    
    // Check for null types
    // Go: if arg == nil || input == nil { return assignableTypeMustNot }
    if (&input == nullptr || &arg == nullptr) {
        return AssignableType::MustNot;
    }
    
    // Check if types are exactly the same
    // Go: if arg == input { return assignableTypeMust }
    if (input == arg) {
        return AssignableType::Must;
    }
    
    // Check interface compatibility using TypeRegistry
    // Go: if arg.Kind() == reflect.Interface && input.Implements(arg)
    auto& registry = TypeRegistry::Instance();
    if (registry.Implements(input, arg)) {
        return AssignableType::Must;
    }
    
    // Reverse check: if input is interface and arg implements it
    // Go: if input.Kind() == reflect.Interface
    if (registry.Implements(arg, input)) {
        return AssignableType::May;
    }
    
    // Check for std::any (can accept anything)
    if (arg == typeid(std::any) || arg == typeid(void*)) {
        return AssignableType::Must;
    }
    
    // Check pointer/reference compatibility
    std::string input_name = input.name();
    std::string arg_name = arg.name();
    
    // Strip pointer/reference qualifiers
    auto strip_qualifiers = [](const std::string& type_name) -> std::string {
        std::string result = type_name;
        // Remove 'P' prefix (pointer) and 'R' prefix (reference)
        while (!result.empty() && (result[0] == 'P' || result[0] == 'R')) {
            result = result.substr(1);
        }
        return result;
    };
    
    if (strip_qualifiers(input_name) == strip_qualifiers(arg_name)) {
        return AssignableType::Must;
    }
    
    // Types are incompatible
    return AssignableType::MustNot;
}

std::map<std::string, std::vector<Any>> ExtractOptions(
    const std::map<std::string, std::shared_ptr<ChannelCall>>& nodes,
    const std::vector<Option>& opts) {
    
    std::map<std::string, std::vector<Any>> opt_map;
    
    for (const auto& opt : opts) {
        if (opt.paths.empty()) {
            // Common options - filter by type
            if (opt.options.empty()) {
                continue;
            }
            
            for (const auto& [name, call] : nodes) {
                // For subgraphs, add the whole option
                // For components, check type compatibility
                opt_map[name].push_back(opt);
            }
        } else {
            // Path-specific options
            for (const auto& path : opt.paths) {
                if (path.empty()) {
                    throw std::runtime_error("option has designated an empty path");
                }
                
                auto it = nodes.find(path[0]);
                if (it == nodes.end()) {
                    throw std::runtime_error("option has designated an unknown node: " + path[0]);
                }
                
                if (path.size() == 1) {
                    // Direct node option
                    if (!opt.options.empty()) {
                        opt_map[path[0]].insert(
                            opt_map[path[0]].end(),
                            opt.options.begin(),
                            opt.options.end());
                    }
                } else {
                    // Sub-graph option
                    Option new_opt = opt;
                    new_opt.paths = {std::vector<std::string>(path.begin() + 1, path.end())};
                    opt_map[path[0]].push_back(new_opt);
                }
            }
        }
    }
    
    return opt_map;
}

std::vector<Any> MapToList(const std::map<std::string, Any>& m) {
    std::vector<Any> result;
    result.reserve(m.size());
    for (const auto& [k, v] : m) {
        result.push_back(v);
    }
    return result;
}

} // namespace compose
} // namespace eino
