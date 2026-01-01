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

#include "eino/compose/state.h"
#include "eino/compose/graph.h"
#include <stdexcept>

namespace eino {
namespace compose {

// State Handler Execution Framework
// Aligns with eino/compose/state.go + graph_run.go state handling

// Execute pre-handler chain
// Aligns with eino/compose/graph_run.go:220-245 (pre-processing)
template<typename I, typename S>
I ExecutePreHandlers(
    void* ctx,
    const I& input,
    S* state,
    const std::vector<StatePreHandler<I, S>>& handlers) {
    
    I current_input = input;
    
    for (const auto& handler : handlers) {
        if (handler.handler_) {
            try {
                current_input = handler.handler_(ctx, current_input, state);
            } catch (const std::exception& e) {
                // Log error and rethrow
                throw std::runtime_error(
                    std::string("PreHandler execution failed: ") + e.what());
            }
        }
    }
    
    return current_input;
}

// Execute post-handler chain
// Aligns with eino/compose/graph_run.go:250-270 (post-processing)
template<typename O, typename S>
O ExecutePostHandlers(
    void* ctx,
    const O& output,
    S* state,
    const std::vector<StatePostHandler<O, S>>& handlers) {
    
    O current_output = output;
    
    for (const auto& handler : handlers) {
        if (handler.handler_) {
            try {
                current_output = handler.handler_(ctx, current_output, state);
            } catch (const std::exception& e) {
                throw std::runtime_error(
                    std::string("PostHandler execution failed: ") + e.what());
            }
        }
    }
    
    return current_output;
}

// Node execution with full state handler support
// Aligns with eino/compose/graph_run.go:195-280
template<typename I, typename O, typename S>
class NodeExecutor {
public:
    using PreHandler = std::function<I(void*, const I&, S*)>;
    using PostHandler = std::function<O(void*, const O&, S*)>;
    using RunnableFunc = std::function<O(void*, const I&)>;
    
    NodeExecutor(
        const std::string& node_name,
        RunnableFunc runnable,
        const std::vector<PreHandler>& pre_handlers = {},
        const std::vector<PostHandler>& post_handlers = {})
        : node_name_(node_name)
        , runnable_(runnable)
        , pre_handlers_(pre_handlers)
        , post_handlers_(post_handlers) {}
    
    O Execute(void* ctx, const I& input, S* state) {
        // Step 1: Execute pre-handlers
        // Aligns with eino/compose/graph_run.go:220-235
        I processed_input = input;
        for (const auto& pre_handler : pre_handlers_) {
            processed_input = pre_handler(ctx, processed_input, state);
        }
        
        // Step 2: Execute node runnable
        // Aligns with eino/compose/graph_run.go:240-245
        O output;
        try {
            output = runnable_(ctx, processed_input);
        } catch (const std::exception& e) {
            throw std::runtime_error(
                "Node " + node_name_ + " execution failed: " + e.what());
        }
        
        // Step 3: Execute post-handlers
        // Aligns with eino/compose/graph_run.go:250-265
        O processed_output = output;
        for (const auto& post_handler : post_handlers_) {
            processed_output = post_handler(ctx, processed_output, state);
        }
        
        return processed_output;
    }
    
    // Streaming version
    std::shared_ptr<StreamReader<O>> ExecuteStream(
        void* ctx,
        const I& input,
        S* state) {
        
        // For streaming, apply pre-handler, then stream through node
        I processed_input = input;
        for (const auto& pre_handler : pre_handlers_) {
            processed_input = pre_handler(ctx, processed_input, state);
        }
        
        // Stream execution (would need streaming runnable)
        // For now, use Invoke and wrap in stream
        O output = runnable_(ctx, processed_input);
        
        // Apply post-handlers to final output
        for (const auto& post_handler : post_handlers_) {
            output = post_handler(ctx, output, state);
        }
        
        std::vector<O> results{output};
        return std::make_shared<VectorStreamReader<O>>(results);
    }
    
private:
    std::string node_name_;
    RunnableFunc runnable_;
    std::vector<PreHandler> pre_handlers_;
    std::vector<PostHandler> post_handlers_;
};

// State injection/extraction helpers
// Aligns with eino/compose/state.go:80-150

template<typename S>
class StateManager {
public:
    StateManager() = default;
    
    // Initialize state with generator
    void InitializeState(void* ctx, std::function<std::shared_ptr<S>(void*)> generator) {
        state_ = generator(ctx);
    }
    
    // Thread-safe state access
    void WithState(std::function<void(S*)> callback) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (state_) {
            callback(state_.get());
        }
    }
    
    // Read-only state access
    void WithStateReadOnly(std::function<void(const S*)> callback) const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (state_) {
            callback(state_.get());
        }
    }
    
    // Get state pointer (not thread-safe, use carefully)
    S* GetState() {
        return state_.get();
    }
    
    const S* GetState() const {
        return state_.get();
    }
    
    bool HasState() const {
        return state_ != nullptr;
    }
    
private:
    std::shared_ptr<S> state_;
    mutable std::mutex state_mutex_;
};

// Explicit template instantiations for common types
template class NodeExecutor<
    std::vector<schema::Message>,
    schema::Message,
    ReactState>;

template class NodeExecutor<
    schema::Message,
    std::vector<schema::Message>,
    ReactState>;

template class StateManager<ReactState>;
template class StateManager<ChatModelAgentState>;

} // namespace compose
} // namespace eino
