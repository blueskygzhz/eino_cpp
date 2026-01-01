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

#ifndef EINO_CPP_CALLBACKS_WRAPPER_H_
#define EINO_CPP_CALLBACKS_WRAPPER_H_

#include <functional>
#include <memory>
#include <exception>

#include "eino/callbacks/callback.h"
#include "eino/callbacks/manager.h"

namespace eino {
namespace callbacks {

// invokeWithCallbacks wraps a function to add callback hooks
// This is the C++ equivalent of Go's invokeWithCallbacks
//
// Usage:
//   auto wrapped = invokeWithCallbacks<Input, Output>(original_func);
//   Output result = wrapped(ctx, input);
//
// Execution flow:
//   1. OnStart(ctx, input)
//   2. output = original_func(ctx, input)
//   3. OnEnd(ctx, output)  OR  OnError(ctx, error)
//
template<typename Input, typename Output>
std::function<Output(const Context&, const Input&)> invokeWithCallbacks(
    std::function<Output(const Context&, const Input&)> fn) {
    
    return [fn](const Context& ctx, const Input& input) -> Output {
        Context ctx1 = ctx;
        Input input1 = input;
        
        // Step 1: OnStart
        try {
            std::tie(ctx1, input1) = OnStart<Input>(ctx1, input1);
        } catch (const std::exception& e) {
            // If OnStart fails, still try to execute the function
            // This matches Go's error handling philosophy
        }
        
        // Step 2: Execute original function
        Output output;
        try {
            output = fn(ctx1, input1);
        } catch (const std::exception& e) {
            // Step 3a: OnError
            try {
                auto [ctx2, _] = OnError(ctx1, e.what());
            } catch (...) {
                // Ignore callback errors
            }
            throw;  // Re-throw original exception
        }
        
        // Step 3b: OnEnd
        try {
            auto [ctx3, output1] = OnEnd<Output>(ctx1, output);
            return output1;
        } catch (const std::exception& e) {
            // If OnEnd fails, return original output
            return output;
        }
    };
}

// invokeWithCallbacks - overload with extra context parameter
template<typename Input, typename Output>
std::function<Output(Context&, const Input&)> invokeWithCallbacks(
    std::function<Output(Context&, const Input&)> fn) {
    
    return [fn](Context& ctx, const Input& input) -> Output {
        Input input1 = input;
        
        // Step 1: OnStart
        try {
            std::tie(ctx, input1) = OnStart<Input>(ctx, input1);
        } catch (const std::exception& e) {
            // Ignore OnStart errors
        }
        
        // Step 2: Execute original function
        Output output;
        try {
            output = fn(ctx, input1);
        } catch (const std::exception& e) {
            // Step 3a: OnError
            try {
                std::tie(ctx, std::ignore) = OnError(ctx, e.what());
            } catch (...) {
                // Ignore callback errors
            }
            throw;
        }
        
        // Step 3b: OnEnd
        try {
            Output output1;
            std::tie(ctx, output1) = OnEnd<Output>(ctx, output);
            return output1;
        } catch (const std::exception& e) {
            return output;
        }
    };
}

// streamWithCallbacks wraps a streaming function to add callback hooks
// This handles streaming inputs/outputs differently from regular invocations
//
// Usage:
//   auto wrapped = streamWithCallbacks<Input, Output>(stream_func);
//   StreamReader<Output> stream = wrapped(ctx, input);
//
// Execution flow:
//   1. OnStartWithStreamInput(ctx, input_stream) - if input is stream
//   2. output_stream = original_func(ctx, input)
//   3. OnEndWithStreamOutput(ctx, output_stream) - if output is stream
//
template<typename Input, typename Output>
std::function<std::shared_ptr<Output>(const Context&, const Input&)> streamWithCallbacks(
    std::function<std::shared_ptr<Output>(const Context&, const Input&)> fn) {
    
    return [fn](const Context& ctx, const Input& input) -> std::shared_ptr<Output> {
        Context ctx1 = ctx;
        Input input1 = input;
        
        // Step 1: Check if input is a stream and call OnStartWithStreamInput
        // Note: In C++, we need to check if Input is a stream type
        // This is simplified - in practice, you'd use SFINAE or concepts
        try {
            // For now, we just call OnStart
            std::tie(ctx1, input1) = OnStart<Input>(ctx1, input1);
        } catch (const std::exception& e) {
            // Ignore OnStart errors
        }
        
        // Step 2: Execute original function
        std::shared_ptr<Output> output;
        try {
            output = fn(ctx1, input1);
        } catch (const std::exception& e) {
            // OnError
            try {
                auto [ctx2, _] = OnError(ctx1, e.what());
            } catch (...) {
                // Ignore callback errors
            }
            throw;
        }
        
        // Step 3: Check if output is a stream and call OnEndWithStreamOutput
        try {
            // For stream outputs, we would call OnEndWithStreamOutput
            // For now, simplified version
            auto [ctx3, output1] = OnEnd<std::shared_ptr<Output>>(ctx1, output);
            return output1;
        } catch (const std::exception& e) {
            return output;
        }
    };
}

// collectWithCallbacks - wraps a collect operation with callbacks
// Used for collecting stream chunks into a single result
template<typename Input, typename Output>
std::function<Output(const Context&, std::shared_ptr<Input>)> collectWithCallbacks(
    std::function<Output(const Context&, std::shared_ptr<Input>)> fn) {
    
    return [fn](const Context& ctx, std::shared_ptr<Input> input) -> Output {
        Context ctx1 = ctx;
        
        // OnStartWithStreamInput
        try {
            auto [ctx2, input1] = OnStartWithStreamInput<Input>(ctx1, input);
            ctx1 = ctx2;
        } catch (const std::exception& e) {
            // Ignore errors
        }
        
        // Execute collect
        Output output;
        try {
            output = fn(ctx1, input);
        } catch (const std::exception& e) {
            try {
                auto [ctx2, _] = OnError(ctx1, e.what());
            } catch (...) {}
            throw;
        }
        
        // OnEnd
        try {
            auto [ctx3, output1] = OnEnd<Output>(ctx1, output);
            return output1;
        } catch (const std::exception& e) {
            return output;
        }
    };
}

// transformWithCallbacks - wraps a transform operation with callbacks
// Used for transforming stream elements
template<typename Input, typename Output>
std::function<std::shared_ptr<Output>(const Context&, std::shared_ptr<Input>)> transformWithCallbacks(
    std::function<std::shared_ptr<Output>(const Context&, std::shared_ptr<Input>)> fn) {
    
    return [fn](const Context& ctx, std::shared_ptr<Input> input) -> std::shared_ptr<Output> {
        Context ctx1 = ctx;
        
        // OnStartWithStreamInput
        try {
            auto [ctx2, input1] = OnStartWithStreamInput<Input>(ctx1, input);
            ctx1 = ctx2;
        } catch (const std::exception& e) {
            // Ignore errors
        }
        
        // Execute transform
        std::shared_ptr<Output> output;
        try {
            output = fn(ctx1, input);
        } catch (const std::exception& e) {
            try {
                auto [ctx2, _] = OnError(ctx1, e.what());
            } catch (...) {}
            throw;
        }
        
        // OnEndWithStreamOutput
        try {
            auto [ctx3, output1] = OnEndWithStreamOutput<Output>(ctx1, output);
            return output1;
        } catch (const std::exception& e) {
            return output;
        }
    };
}

// runWithCallbacks - generic wrapper that combines invoke and stream logic
// Automatically determines whether to use invoke or stream based on return type
template<typename Func>
auto runWithCallbacks(Func fn) -> Func {
    // In C++20, we could use concepts to detect stream types
    // For now, we return the function as-is and let the caller use specific wrappers
    return fn;
}

} // namespace callbacks
} // namespace eino

#endif // EINO_CPP_CALLBACKS_WRAPPER_H_
