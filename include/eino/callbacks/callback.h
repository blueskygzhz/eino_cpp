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

#ifndef EINO_CPP_CALLBACKS_CALLBACK_H_
#define EINO_CPP_CALLBACKS_CALLBACK_H_

#include <functional>
#include <vector>
#include <memory>
#include <exception>

#include "eino/callbacks/manager.h"
#include "eino/callbacks/interface.h"
#include "eino/callbacks/aspect_inject.h"

namespace eino {
namespace callbacks {

// Forward declaration for stream support
namespace schema {
    template<typename T>
    class StreamReader;
}

// Handle function type for On[T]
template<typename T>
using HandleFunc = std::function<std::pair<Context, T>(
    const Context&, T, const RunInfo*, const std::vector<std::shared_ptr<Handler>>&)>;

// Generic On function - core callback dispatcher
// This is the main entry point for all callback executions
template<typename T>
std::pair<Context, T> On(
    const Context& ctx,
    T in_out,
    HandleFunc<T> handle,
    CallbackTiming timing,
    bool start) {
    
    // Get manager from context
    auto mgr = ManagerFromCtx(ctx);
    if (!mgr) {
        return {ctx, in_out};
    }
    
    // Create a copy of manager
    auto n_mgr = std::make_shared<CallbackManager>(*mgr);
    
    RunInfo* info = nullptr;
    Context new_ctx = ctx;
    
    if (start) {
        // At start, extract RunInfo and store it
        info = const_cast<RunInfo*>(&n_mgr->GetRunInfo());
        new_ctx = CtxWithRunInfo(new_ctx, *info);
        
        // Clear RunInfo in manager to prevent reuse
        n_mgr = n_mgr->WithRunInfo(RunInfo{});
    } else {
        // At end, retrieve stored RunInfo
        info = RunInfoFromCtx(new_ctx);
        if (!info) {
            // Fallback to manager's RunInfo
            info = const_cast<RunInfo*>(&n_mgr->GetRunInfo());
        }
    }
    
    // Filter handlers by timing
    std::vector<std::shared_ptr<Handler>> filtered_handlers;
    auto all_handlers = n_mgr->GetAllHandlers();
    
    for (const auto& handler : all_handlers) {
        // Check if handler supports this timing
        auto timing_handler = std::dynamic_pointer_cast<HandlerWithTiming>(handler);
        if (!timing_handler || timing_handler->Check(timing)) {
            filtered_handlers.push_back(handler);
        }
    }
    
    // Execute handle function
    T out;
    std::tie(new_ctx, out) = handle(new_ctx, in_out, info, filtered_handlers);
    
    // Update context with manager
    new_ctx = CtxWithManager(new_ctx, n_mgr);
    
    return {new_ctx, out};
}

// OnStartHandle - Execute all OnStart callbacks
template<typename T>
std::pair<Context, T> OnStartHandle(
    const Context& ctx,
    T input,
    const RunInfo* run_info,
    const std::vector<std::shared_ptr<Handler>>& handlers) {
    
    Context new_ctx = ctx;
    
    // Execute in reverse order (Go style)
    for (int i = static_cast<int>(handlers.size()) - 1; i >= 0; --i) {
        try {
            CallbackInput cb_input;
            cb_input.input = input;
            
            handlers[i]->OnStart(*run_info, cb_input);
        } catch (const std::exception& e) {
            // Callback errors should not break the flow
            // Log or handle silently
        }
    }
    
    return {new_ctx, input};
}

// OnEndHandle - Execute all OnEnd callbacks
template<typename T>
std::pair<Context, T> OnEndHandle(
    const Context& ctx,
    T output,
    const RunInfo* run_info,
    const std::vector<std::shared_ptr<Handler>>& handlers) {
    
    Context new_ctx = ctx;
    
    // Execute in forward order
    for (const auto& handler : handlers) {
        try {
            CallbackOutput cb_output;
            cb_output.output = output;
            
            handler->OnEnd(*run_info, cb_output);
        } catch (const std::exception& e) {
            // Callback errors should not break the flow
        }
    }
    
    return {new_ctx, output};
}

// OnErrorHandle - Execute all OnError callbacks
inline std::pair<Context, std::string> OnErrorHandle(
    const Context& ctx,
    const std::string& error,
    const RunInfo* run_info,
    const std::vector<std::shared_ptr<Handler>>& handlers) {
    
    Context new_ctx = ctx;
    
    for (const auto& handler : handlers) {
        try {
            handler->OnError(*run_info, error);
        } catch (const std::exception& e) {
            // Callback errors should not break the flow
        }
    }
    
    return {new_ctx, error};
}

// Stream-specific callback handlers

// OnStartWithStreamInputHandle
template<typename T>
std::pair<Context, std::shared_ptr<T>> OnStartWithStreamInputHandle(
    const Context& ctx,
    std::shared_ptr<T> input,
    const RunInfo* run_info,
    const std::vector<std::shared_ptr<Handler>>& handlers) {
    
    Context new_ctx = ctx;
    
    // Execute in reverse order
    for (int i = static_cast<int>(handlers.size()) - 1; i >= 0; --i) {
        try {
            CallbackInput cb_input;
            // For streams, we pass the stream reader itself
            
            handlers[i]->OnStartWithStreamInput(*run_info, cb_input);
        } catch (const std::exception& e) {
            // Callback errors should not break the flow
        }
    }
    
    return {new_ctx, input};
}

// OnEndWithStreamOutputHandle
template<typename T>
std::pair<Context, std::shared_ptr<T>> OnEndWithStreamOutputHandle(
    const Context& ctx,
    std::shared_ptr<T> output,
    const RunInfo* run_info,
    const std::vector<std::shared_ptr<Handler>>& handlers) {
    
    Context new_ctx = ctx;
    
    // Execute in forward order
    for (const auto& handler : handlers) {
        try {
            CallbackOutput cb_output;
            // For streams, we pass the stream reader itself
            
            handler->OnEndWithStreamOutput(*run_info, cb_output);
        } catch (const std::exception& e) {
            // Callback errors should not break the flow
        }
    }
    
    return {new_ctx, output};
}

// Convenience wrappers that use On() internally

// OnStart - convenience wrapper for start callbacks
template<typename T>
std::pair<Context, T> OnStart(const Context& ctx, T input) {
    return On<T>(ctx, input, OnStartHandle<T>, CallbackTiming::kOnStart, true);
}

// OnEnd - convenience wrapper for end callbacks
template<typename T>
std::pair<Context, T> OnEnd(const Context& ctx, T output) {
    return On<T>(ctx, output, OnEndHandle<T>, CallbackTiming::kOnEnd, false);
}

// OnError - convenience wrapper for error callbacks
inline std::pair<Context, std::string> OnError(const Context& ctx, const std::string& error) {
    return On<std::string>(ctx, error, OnErrorHandle, CallbackTiming::kOnError, false);
}

// OnStartWithStreamInput - convenience wrapper for stream input
template<typename T>
std::pair<Context, std::shared_ptr<T>> OnStartWithStreamInput(
    const Context& ctx,
    std::shared_ptr<T> input) {
    
    return On<std::shared_ptr<T>>(
        ctx, input,
        OnStartWithStreamInputHandle<T>,
        CallbackTiming::kOnStartWithStreamInput,
        true);
}

// OnEndWithStreamOutput - convenience wrapper for stream output
template<typename T>
std::pair<Context, std::shared_ptr<T>> OnEndWithStreamOutput(
    const Context& ctx,
    std::shared_ptr<T> output) {
    
    return On<std::shared_ptr<T>>(
        ctx, output,
        OnEndWithStreamOutputHandle<T>,
        CallbackTiming::kOnEndWithStreamOutput,
        false);
}

} // namespace callbacks
} // namespace eino

#endif // EINO_CPP_CALLBACKS_CALLBACK_H_
