/*
 * Copyright 2024 CloudWeGo Authors
 *
 * Licensed under the Apache License, Version 2.0 (the \"License\");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an \"AS IS\" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EINO_CPP_COMPOSE_STATE_H_
#define EINO_CPP_COMPOSE_STATE_H_

#include <memory>
#include <mutex>
#include <functional>
#include <stdexcept>
#include <typeinfo>
#include <string>
#include "eino/adk/context.h"

namespace eino {
namespace compose {

// InternalState wraps user state with mutex for thread-safe access
// Aligns with eino compose.internalState
// Go reference: eino/compose/state.go lines 34-37
struct InternalState {
    std::shared_ptr<void> state;  // Type-erased state
    std::mutex mu;
    
    InternalState() = default;
    
    template<typename S>
    InternalState(std::shared_ptr<S> s) : state(s) {}
};

// StateKey for storing state in context
// Aligns with eino compose.stateKey
struct StateKey {};

// StatePreHandler is called before node execution
// Aligns with eino compose.StatePreHandler
// Go reference: eino/compose/state.go lines 39-41
template<typename I, typename S>
using StatePreHandler = std::function<I(void* ctx, const I& input, S* state)>;

// StatePostHandler is called after node execution
// Aligns with eino compose.StatePostHandler
// Go reference: eino/compose/state.go lines 43-45
template<typename O, typename S>
using StatePostHandler = std::function<O(void* ctx, const O& output, S* state)>;

// StreamStatePreHandler for stream input/output
// Aligns with eino compose.StreamStatePreHandler
// Go reference: eino/compose/state.go lines 47-48
template<typename I, typename S>
using StreamStatePreHandler = std::function<
    std::shared_ptr<StreamReader<I>>(void* ctx, 
                                     std::shared_ptr<StreamReader<I>> input, 
                                     S* state)>;

// StreamStatePostHandler for stream input/output
// Aligns with eino compose.StreamStatePostHandler
// Go reference: eino/compose/state.go lines 50-51
template<typename O, typename S>
using StreamStatePostHandler = std::function<
    std::shared_ptr<StreamReader<O>>(void* ctx, 
                                      std::shared_ptr<StreamReader<O>> output, 
                                      S* state)>;

// GetState retrieves and casts state from context with mutex
// Aligns with eino compose.getState
// Go reference: eino/compose/state.go lines 143-166
template<typename S>
std::pair<S*, std::mutex*> GetState(void* ctx) {
    auto internal_state = context::GetContextValue<InternalState>(ctx, StateKey{});
    if (!internal_state) {
        throw std::runtime_error("state not found in context");
    }
    
    if (!internal_state->state) {
        throw std::runtime_error("state is nil");
    }
    
    // Cast to target type
    S* state = static_cast<S*>(internal_state->state.get());
    if (!state) {
        throw std::runtime_error(
            std::string("unexpected state type. expected: ") + 
            typeid(S).name() + ", got: " + 
            typeid(internal_state->state.get()).name());
    }
    
    return {state, &internal_state->mu};
}

// ProcessState processes state in a thread-safe way
// Aligns with eino compose.ProcessState
// Go reference: eino/compose/state.go lines 113-141
//
// Example usage:
//   auto lambda = [](void* ctx, const std::string& input) {
//       ProcessState<MyState>(ctx, [](void* ctx, MyState* state) {
//           state->count++;
//           return nullptr;
//       });
//       return input;
//   };
template<typename S>
void ProcessState(void* ctx, std::function<void(void* ctx, S* state)> handler) {
    auto [state, mu] = GetState<S>(ctx);
    
    std::lock_guard<std::mutex> lock(*mu);
    handler(ctx, state);
}

// SetState sets state in context
// Helper function for initializing state in context
template<typename S>
void* SetState(void* ctx, std::shared_ptr<S> state) {
    auto internal = std::make_shared<InternalState>(state);
    return context::SetContextValue(ctx, StateKey{}, internal);
}

// WithGenLocalState option for creating state generator
// Aligns with eino compose.WithGenLocalState
template<typename S>
struct GenLocalStateOption {
    std::function<std::shared_ptr<S>(void* ctx)> generator;
    
    GenLocalStateOption(std::function<std::shared_ptr<S>(void* ctx)> gen)
        : generator(gen) {}
};

// ConvertPreHandler converts StatePreHandler to runnable
// Aligns with eino compose.convertPreHandler
// Go reference: eino/compose/state.go lines 53-66
template<typename I, typename S>
std::function<I(void*, const I&)> ConvertPreHandler(StatePreHandler<I, S> handler) {
    return [handler](void* ctx, const I& input) -> I {
        auto [state, mu] = GetState<S>(ctx);
        std::lock_guard<std::mutex> lock(*mu);
        return handler(ctx, input, state);
    };
}

// ConvertPostHandler converts StatePostHandler to runnable
// Aligns with eino compose.convertPostHandler
// Go reference: eino/compose/state.go lines 68-81
template<typename O, typename S>
std::function<O(void*, const O&)> ConvertPostHandler(StatePostHandler<O, S> handler) {
    return [handler](void* ctx, const O& output) -> O {
        auto [state, mu] = GetState<S>(ctx);
        std::lock_guard<std::mutex> lock(*mu);
        return handler(ctx, output, state);
    };
}

// StreamConvertPreHandler converts StreamStatePreHandler to runnable
// Aligns with eino compose.streamConvertPreHandler
// Go reference: eino/compose/state.go lines 83-96
template<typename I, typename S>
std::function<std::shared_ptr<StreamReader<I>>(void*, std::shared_ptr<StreamReader<I>>)>
StreamConvertPreHandler(StreamStatePreHandler<I, S> handler) {
    return [handler](void* ctx, std::shared_ptr<StreamReader<I>> input) 
        -> std::shared_ptr<StreamReader<I>> {
        auto [state, mu] = GetState<S>(ctx);
        std::lock_guard<std::mutex> lock(*mu);
        return handler(ctx, input, state);
    };
}

// StreamConvertPostHandler converts StreamStatePostHandler to runnable
// Aligns with eino compose.streamConvertPostHandler
// Go reference: eino/compose/state.go lines 98-111
template<typename O, typename S>
std::function<std::shared_ptr<StreamReader<O>>(void*, std::shared_ptr<StreamReader<O>>)>
StreamConvertPostHandler(StreamStatePostHandler<O, S> handler) {
    return [handler](void* ctx, std::shared_ptr<StreamReader<O>> output) 
        -> std::shared_ptr<StreamReader<O>> {
        auto [state, mu] = GetState<S>(ctx);
        std::lock_guard<std::mutex> lock(*mu);
        return handler(ctx, output, state);
    };
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_STATE_H_
