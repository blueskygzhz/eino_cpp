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
#include <string>
#include <vector>

#include \"eino/callbacks/callback.h\"
#include \"eino/compose/compose.h\"
#include \"eino/compose/graph_node.h\"
#include \"eino/compose/stream_reader.h\"
#include \"eino/compose/type_registry.h\"

namespace eino {
namespace compose {

// Callback wrapper functions

// OnStart wrapper
template <typename T>
std::pair<Context, T> OnStart(const Context& ctx, const T& input) {
    // Get callbacks from context and invoke OnStart
    auto handlers = callbacks::GetHandlersFromContext(ctx);
    if (handlers.empty()) {
        return {ctx, input};
    }
    
    Context new_ctx = ctx;
    for (auto& handler : handlers) {
        new_ctx = handler->OnStart(new_ctx, input);
    }
    
    return {new_ctx, input};
}

// OnEnd wrapper
template <typename T>
std::pair<Context, T> OnEnd(const Context& ctx, const T& output) {
    auto handlers = callbacks::GetHandlersFromContext(ctx);
    if (handlers.empty()) {
        return {ctx, output};
    }
    
    Context new_ctx = ctx;
    for (auto& handler : handlers) {
        new_ctx = handler->OnEnd(new_ctx, output);
    }
    
    return {new_ctx, output};
}

// OnError wrapper
std::pair<Context, std::string> OnError(const Context& ctx, const std::string& error);

// OnStartWithStreamInput wrapper
template <typename T>
std::pair<Context, std::shared_ptr<schema::StreamReader<T>>> OnStartWithStreamInput(
    const Context& ctx,
    std::shared_ptr<schema::StreamReader<T>> input) {
    
    auto handlers = callbacks::GetHandlersFromContext(ctx);
    if (handlers.empty()) {
        return {ctx, input};
    }
    
    Context new_ctx = ctx;
    for (auto& handler : handlers) {
        new_ctx = handler->OnStartWithStreamInput(new_ctx, input);
    }
    
    return {new_ctx, input};
}

// OnEndWithStreamOutput wrapper
template <typename T>
std::pair<Context, std::shared_ptr<schema::StreamReader<T>>> OnEndWithStreamOutput(
    const Context& ctx,
    std::shared_ptr<schema::StreamReader<T>> output) {
    
    auto handlers = callbacks::GetHandlersFromContext(ctx);
    if (handlers.empty()) {
        return {ctx, output};
    }
    
    Context new_ctx = ctx;
    for (auto& handler : handlers) {
        new_ctx = handler->OnEndWithStreamOutput(new_ctx, output);
    }
    
    return {new_ctx, output};
}

// Generic graph callbacks
std::pair<Context, Any> OnGraphStart(const Context& ctx, const Any& input, bool is_stream);
std::pair<Context, Any> OnGraphEnd(const Context& ctx, const Any& output, bool is_stream);
std::pair<Context, std::string> OnGraphError(const Context& ctx, const std::string& error);

// RunWithCallbacks wraps a function with callback hooks
template <typename I, typename O, typename... Options>
std::function<O(const Context&, const I&, Options...)> RunWithCallbacks(
    std::function<O(const Context&, const I&, Options...)> fn) {
    
    return [fn](const Context& ctx, const I& input, Options... opts) -> O {
        // OnStart
        auto [ctx1, input1] = OnStart(ctx, input);
        
        // Execute function
        O output;
        try {
            output = fn(ctx1, input1, opts...);
        } catch (const std::exception& e) {
            // OnError
            auto [ctx2, err] = OnError(ctx1, e.what());
            throw;
        }
        
        // OnEnd
        auto [ctx3, output1] = OnEnd(ctx1, output);
        
        return output1;
    };
}

// Callback initialization for graphs
Context InitGraphCallbacks(const Context& ctx,
                            const NodeInfo* info,
                            const ExecutorMeta* meta,
                            const std::vector<Option>& opts);

// Callback initialization for nodes
Context InitNodeCallbacks(const Context& ctx,
                           const std::string& key,
                           const NodeInfo* info,
                           const ExecutorMeta* meta,
                           const std::vector<Option>& opts);

// Type checking utilities

enum class AssignableType {
    MustNot,  // Definitely not assignable
    Must,     // Definitely assignable
    May       // May be assignable (interface)
};

// Check if input type is assignable to argument type
AssignableType CheckAssignable(const std::type_info& input, const std::type_info& arg);

// Option extraction utilities

// Extract options for specific nodes from global options
std::map<std::string, std::vector<Any>> ExtractOptions(
    const std::map<std::string, std::shared_ptr<ChannelCall>>& nodes,
    const std::vector<Option>& opts);

// Convert map to list
std::vector<Any> MapToList(const std::map<std::string, Any>& m);

// Convert list to any
template <typename T>
std::vector<Any> ToAnyList(const std::vector<T>& in) {
    std::vector<Any> result;
    result.reserve(in.size());
    for (const auto& v : in) {
        result.push_back(v);
    }
    return result;
}

// Stream conversion for callbacks
template <typename T>
callbacks::CallbackOutput StreamChunkConvertForCBOutput(const T& o) {
    return o;
}

template <typename T>
callbacks::CallbackInput StreamChunkConvertForCBInput(const T& i) {
    return i;
}

} // namespace compose
} // namespace eino
