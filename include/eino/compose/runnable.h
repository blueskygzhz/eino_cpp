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

#ifndef EINO_CPP_COMPOSE_RUNNABLE_H_
#define EINO_CPP_COMPOSE_RUNNABLE_H_

#include <functional>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <typeinfo>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <chrono>
#include "../schema/types.h"

namespace eino {
namespace compose {

using json = nlohmann::json;

// Forward declarations
template<typename T> class StreamReader;
template<typename I, typename O> class Runnable;
template<typename I, typename O> class ComposableRunnable;

// Forward declarations for callback functions (will be defined in utils.h)
template <typename T>
std::pair<Context, T> OnStart(const Context& ctx, const T& input);

template <typename T>
std::pair<Context, T> OnEnd(const Context& ctx, const T& output);

std::pair<Context, std::string> OnError(const Context& ctx, const std::string& error);

template <typename T>
std::pair<Context, std::shared_ptr<StreamReader<T>>> OnStartWithStreamInput(
    const Context& ctx,
    std::shared_ptr<StreamReader<T>> input);

template <typename T>
std::pair<Context, std::shared_ptr<StreamReader<T>>> OnEndWithStreamOutput(
    const Context& ctx,
    std::shared_ptr<StreamReader<T>> output);

// Context is a simple context implementation (similar to Go context.Context)
// In a more complete implementation, this would support cancellation and deadlines
class Context {
public:
    static std::shared_ptr<Context> Background() {
        return std::make_shared<Context>();
    }
    
    Context() = default;
    virtual ~Context() = default;
    
    // Store arbitrary values
    void SetValue(const std::string& key, const json& value) {
        values_[key] = value;
    }
    
    bool GetValue(const std::string& key, json& value) const {
        auto it = values_.find(key);
        if (it != values_.end()) {
            value = it->second;
            return true;
        }
        return false;
    }
    
private:
    std::map<std::string, json> values_;
};

// Option represents runtime options for invocation
using Option = std::map<std::string, json>;

// InvokeOptions represents options for invoking a runnable
struct InvokeOptions {
    std::map<std::string, json> extra_data;
    std::vector<Option> options;
    
    InvokeOptions() = default;
};

// StreamReader provides an interface for reading streamed values
template<typename T>
class StreamReader {
public:
    virtual ~StreamReader() = default;
    
    // Read returns the next value from the stream
    // Returns false if the stream is exhausted
    virtual bool Read(T& value) = 0;
    
    // Peek returns the next value without consuming it
    // Returns false if the stream is exhausted
    virtual bool Peek(T& value) = 0;
    
    // Close closes the stream
    virtual void Close() = 0;
    
    // IsClosed returns whether the stream is closed
    virtual bool IsClosed() const = 0;
};

// SimpleStreamReader is a basic implementation of StreamReader backed by a vector
template<typename T>
class SimpleStreamReader : public StreamReader<T> {
public:
    SimpleStreamReader() 
        : data_(), position_(0), closed_(false) {}
    
    explicit SimpleStreamReader(const std::vector<T>& data) 
        : data_(data), position_(0), closed_(false) {}
    
    void Add(const T& item) {
        data_.push_back(item);
    }
    
    void Add(const std::vector<T>& items) {
        data_.insert(data_.end(), items.begin(), items.end());
    }
    
    bool Read(T& value) override {
        if (closed_ || position_ >= data_.size()) {
            return false;
        }
        value = data_[position_++];
        return true;
    }
    
    bool Peek(T& value) override {
        if (closed_ || position_ >= data_.size()) {
            return false;
        }
        value = data_[position_];
        return true;
    }
    
    void Close() override {
        closed_ = true;
    }
    
    bool IsClosed() const override {
        return closed_;
    }
    
    // GetRemaining returns the number of items remaining in the stream
    size_t GetRemaining() const {
        if (position_ >= data_.size()) {
            return 0;
        }
        return data_.size() - position_;
    }
    
    // Reset resets the stream to the beginning
    void Reset() {
        position_ = 0;
        closed_ = false;
    }
    
    // GetData returns the underlying data vector
    const std::vector<T>& GetData() const {
        return data_;
    }
    
private:
    std::vector<T> data_;
    size_t position_;
    bool closed_;
};

// Runnable is the base interface for all executable components
// Implements 4 different streaming paradigms:
// - Invoke: input (non-stream) => output (non-stream)
// - Stream: input (non-stream) => output (stream)
// - Collect: input (stream) => output (non-stream)
// - Transform: input (stream) => output (stream)
template<typename I, typename O>
class Runnable {
public:
    virtual ~Runnable() = default;
    
    // Invoke runs with non-stream input and returns non-stream output
    // This is the most common invocation pattern
    virtual O Invoke(
        std::shared_ptr<Context> ctx,
        const I& input, 
        const std::vector<Option>& opts = std::vector<Option>()) = 0;
    
    // Stream runs with non-stream input and returns a stream of outputs
    // Useful for streaming responses from LLMs
    virtual std::shared_ptr<StreamReader<O>> Stream(
        std::shared_ptr<Context> ctx,
        const I& input, 
        const std::vector<Option>& opts = std::vector<Option>()) = 0;
    
    // Collect runs with stream input and returns non-stream output
    // Useful for aggregating multiple inputs
    virtual O Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) = 0;
    
    // Transform runs with stream input and returns stream output
    // Useful for processing streams of data
    virtual std::shared_ptr<StreamReader<O>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) = 0;
};

// ComposableRunnable extends Runnable with composition capabilities
// Allows chaining and composing multiple runnables together
template<typename I, typename O>
class ComposableRunnable : public Runnable<I, O> {
public:
    virtual ~ComposableRunnable() = default;
    
    // GetInputType returns the input type information
    virtual const std::type_info& GetInputType() const = 0;
    
    // GetOutputType returns the output type information
    virtual const std::type_info& GetOutputType() const = 0;
    
    // GetComponentType returns the component type (e.g., "Lambda", "Chain", "Graph")
    virtual std::string GetComponentType() const {
        return "Runnable";
    }
};

// Helper for automatic type conversion between different invocation modes
// Allows Invoke to work even if only Transform is implemented, etc.
template<typename I, typename O>
class RunnableHelper {
public:
    // Auto-implement Invoke from Transform
    static O InvokeFromTransform(
        std::shared_ptr<Runnable<I, O>> r,
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts) {
        std::vector<I> inputs{input};
        auto stream_in = std::make_shared<SimpleStreamReader<I>>(inputs);
        auto stream_out = r->Transform(ctx, stream_in, opts);
        O value;
        if (stream_out && stream_out->Read(value)) {
            return value;
        }
        throw std::runtime_error("Failed to read from transformed stream");
    }
    
    // Auto-implement Invoke from Stream
    static O InvokeFromStream(
        std::shared_ptr<Runnable<I, O>> r,
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts) {
        auto stream_out = r->Stream(ctx, input, opts);
        O value;
        if (stream_out && stream_out->Read(value)) {
            return value;
        }
        throw std::runtime_error("Failed to read from stream");
    }
};

// Lambda function types for different invocation patterns (with Context support)
template<typename I, typename O>
using InvokeFunc = std::function<O(
    std::shared_ptr<Context>,
    const I&,
    const std::vector<Option>&)>;

template<typename I, typename O>
using StreamFunc = std::function<std::shared_ptr<StreamReader<O>>(
    std::shared_ptr<Context>,
    const I&,
    const std::vector<Option>&)>;

template<typename I, typename O>
using CollectFunc = std::function<O(
    std::shared_ptr<Context>,
    std::shared_ptr<StreamReader<I>>,
    const std::vector<Option>&)>;

template<typename I, typename O>
using TransformFunc = std::function<std::shared_ptr<StreamReader<O>>(
    std::shared_ptr<Context>,
    std::shared_ptr<StreamReader<I>>,
    const std::vector<Option>&)>;

// LambdaRunnable allows creating Runnable from lambda functions
// Supports automatic conversion between different invocation modes
template<typename I, typename O>
class LambdaRunnable : public ComposableRunnable<I, O>,
                       public std::enable_shared_from_this<LambdaRunnable<I, O>> {
public:
    LambdaRunnable() 
        : invoke_func_(nullptr), 
          stream_func_(nullptr),
          collect_func_(nullptr),
          transform_func_(nullptr) {}
    
    explicit LambdaRunnable(InvokeFunc<I, O> func) 
        : invoke_func_(func), 
          stream_func_(nullptr),
          collect_func_(nullptr),
          transform_func_(nullptr) {}
    
    explicit LambdaRunnable(TransformFunc<I, O> func)
        : invoke_func_(nullptr),
          stream_func_(nullptr),
          collect_func_(nullptr),
          transform_func_(func) {}
    
    LambdaRunnable(
        InvokeFunc<I, O> i,
        StreamFunc<I, O> s,
        CollectFunc<I, O> c,
        TransformFunc<I, O> t)
        : invoke_func_(i),
          stream_func_(s),
          collect_func_(c),
          transform_func_(t) {}
    
    // ⭐ NEW: Capability detection methods
    bool HasInvokeFunc() const { return invoke_func_ != nullptr; }
    bool HasStreamFunc() const { return stream_func_ != nullptr; }
    bool HasCollectFunc() const { return collect_func_ != nullptr; }
    bool HasTransformFunc() const { return transform_func_ != nullptr; }
    
    O Invoke(
        std::shared_ptr<Context> ctx,
        const I& input, 
        const std::vector<Option>& opts = std::vector<Option>()) override {
        // ⭐ Callback wrapping - Aligns with eino/compose/runnable.go:343
        // i = invokeWithCallbacks(i)
        
        // Step 1: OnStart
        Context ctx_val = ctx ? *ctx : Context();
        auto [ctx1, input1] = compose::OnStart(ctx_val, input);
        auto ctx1_ptr = std::make_shared<Context>(ctx1);
        
        // Step 2: Execute the actual function
        O output;
        try {
            if (invoke_func_) {
                output = invoke_func_(ctx1_ptr, input1, opts);
            } else if (stream_func_) {
                auto self = std::enable_shared_from_this<LambdaRunnable<I, O>>::shared_from_this();
                output = RunnableHelper<I, O>::InvokeFromStream(self, ctx1_ptr, input1, opts);
            } else if (transform_func_) {
                auto self = std::enable_shared_from_this<LambdaRunnable<I, O>>::shared_from_this();
                output = RunnableHelper<I, O>::InvokeFromTransform(self, ctx1_ptr, input1, opts);
            } else {
                throw std::runtime_error("LambdaRunnable: no Invoke implementation");
            }
        } catch (const std::exception& e) {
            // Step 2b: OnError
            auto [ctx2, err] = compose::OnError(ctx1, std::string(e.what()));
            throw;
        }
        
        // Step 3: OnEnd
        auto [ctx3, output1] = compose::OnEnd(ctx1, output);
        
        // Update the original context pointer if needed
        if (ctx) {
            *ctx = ctx3;
        }
        
        return output1;
    }
    
    std::shared_ptr<StreamReader<O>> Stream(
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        // ⭐ Callback wrapping - Aligns with eino/compose/runnable.go:347
        // s = streamWithCallbacks(s)
        
        // Step 1: OnStart
        Context ctx_val = ctx ? *ctx : Context();
        auto [ctx1, input1] = compose::OnStart(ctx_val, input);
        auto ctx1_ptr = std::make_shared<Context>(ctx1);
        
        // Step 2: Execute the actual function
        std::shared_ptr<StreamReader<O>> output;
        try {
            if (stream_func_) {
                output = stream_func_(ctx1_ptr, input1, opts);
            } else if (invoke_func_) {
                O result = invoke_func_(ctx1_ptr, input1, opts);
                std::vector<O> results{result};
                output = std::make_shared<SimpleStreamReader<O>>(results);
            } else if (transform_func_) {
                std::vector<I> inputs{input1};
                auto stream_in = std::make_shared<SimpleStreamReader<I>>(inputs);
                output = transform_func_(ctx1_ptr, stream_in, opts);
            } else {
                throw std::runtime_error("LambdaRunnable: no Stream implementation");
            }
        } catch (const std::exception& e) {
            // Step 2b: OnError
            auto [ctx2, err] = compose::OnError(ctx1, std::string(e.what()));
            throw;
        }
        
        // Step 3: OnEndWithStreamOutput
        auto [ctx3, output1] = compose::OnEndWithStreamOutput(ctx1, output);
        
        // Update the original context pointer if needed
        if (ctx) {
            *ctx = ctx3;
        }
        
        return output1;
    }
    
    O Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        // ⭐ Callback wrapping - Aligns with eino/compose/runnable.go:351
        // c = collectWithCallbacks(c)
        
        // Step 1: OnStartWithStreamInput
        Context ctx_val = ctx ? *ctx : Context();
        auto [ctx1, input1] = compose::OnStartWithStreamInput(ctx_val, input);
        auto ctx1_ptr = std::make_shared<Context>(ctx1);
        
        // Step 2: Execute the actual function
        O output;
        try {
            if (collect_func_) {
                output = collect_func_(ctx1_ptr, input1, opts);
            } else if (invoke_func_) {
                I value;
                if (input1 && input1->Read(value)) {
                    output = invoke_func_(ctx1_ptr, value, opts);
                } else {
                    throw std::runtime_error("LambdaRunnable: no input to collect");
                }
            } else if (transform_func_) {
                auto stream_out = transform_func_(ctx1_ptr, input1, opts);
                O value;
                if (stream_out && stream_out->Read(value)) {
                    output = value;
                } else {
                    throw std::runtime_error("LambdaRunnable: failed to collect from transform");
                }
            } else {
                throw std::runtime_error("LambdaRunnable: no Collect implementation");
            }
        } catch (const std::exception& e) {
            // Step 2b: OnError
            auto [ctx2, err] = compose::OnError(ctx1, std::string(e.what()));
            throw;
        }
        
        // Step 3: OnEnd
        auto [ctx3, output1] = compose::OnEnd(ctx1, output);
        
        // Update the original context pointer if needed
        if (ctx) {
            *ctx = ctx3;
        }
        
        return output1;
    }
    
    std::shared_ptr<StreamReader<O>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        // ⭐ Callback wrapping - Aligns with eino/compose/runnable.go:355
        // t = transformWithCallbacks(t)
        
        // Step 1: OnStartWithStreamInput
        Context ctx_val = ctx ? *ctx : Context();
        auto [ctx1, input1] = compose::OnStartWithStreamInput(ctx_val, input);
        auto ctx1_ptr = std::make_shared<Context>(ctx1);
        
        // Step 2: Execute the actual function
        std::shared_ptr<StreamReader<O>> output;
        try {
            if (transform_func_) {
                output = transform_func_(ctx1_ptr, input1, opts);
            } else if (stream_func_) {
                I value;
                if (input1 && input1->Read(value)) {
                    output = stream_func_(ctx1_ptr, value, opts);
                } else {
                    throw std::runtime_error("LambdaRunnable: no input to transform");
                }
            } else if (invoke_func_) {
                std::vector<O> results;
                I value;
                while (input1 && input1->Read(value)) {
                    results.push_back(invoke_func_(ctx1_ptr, value, opts));
                }
                output = std::make_shared<SimpleStreamReader<O>>(results);
            } else {
                throw std::runtime_error("LambdaRunnable: no Transform implementation");
            }
        } catch (const std::exception& e) {
            // Step 2b: OnError
            auto [ctx2, err] = compose::OnError(ctx1, std::string(e.what()));
            throw;
        }
        
        // Step 3: OnEndWithStreamOutput
        auto [ctx3, output1] = compose::OnEndWithStreamOutput(ctx1, output);
        
        // Update the original context pointer if needed
        if (ctx) {
            *ctx = ctx3;
        }
        
        return output1;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(I);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(O);
    }
    
    std::string GetComponentType() const override {
        return "Lambda";
    }
    
    // Setters for lazy initialization
    void SetInvokeFunc(InvokeFunc<I, O> func) {
        invoke_func_ = func;
    }
    
    void SetStreamFunc(StreamFunc<I, O> func) {
        stream_func_ = func;
    }
    
    void SetCollectFunc(CollectFunc<I, O> func) {
        collect_func_ = func;
    }
    
    void SetTransformFunc(TransformFunc<I, O> func) {
        transform_func_ = func;
    }
    
    // ⭐ NEW: Get capabilities for smart method selection
    struct Capabilities {
        bool has_invoke;
        bool has_stream;
        bool has_collect;
        bool has_transform;
    };
    
    Capabilities GetCapabilities() const {
        return Capabilities{
            .has_invoke = invoke_func_ != nullptr,
            .has_stream = stream_func_ != nullptr,
            .has_collect = collect_func_ != nullptr,
            .has_transform = transform_func_ != nullptr
        };
    }
    
private:
    InvokeFunc<I, O> invoke_func_;
    StreamFunc<I, O> stream_func_;
    CollectFunc<I, O> collect_func_;
    TransformFunc<I, O> transform_func_;
};

// PassthroughRunnable is a simple runnable that passes input through as output\n// Useful for connecting incompatible types or implementing branches\ntemplate<typename I, typename O>\nclass PassthroughRunnable : public ComposableRunnable<I, O> {\npublic:\n    PassthroughRunnable() : passthrough_func_(nullptr) {}\n    \n    explicit PassthroughRunnable(\n        std::function<O(\n            std::shared_ptr<Context>,\n            const I&,\n            const std::vector<Option>&)> func)\n        : passthrough_func_(func) {}\n    \n    O Invoke(\n        std::shared_ptr<Context> ctx,\n        const I& input, \n        const std::vector<Option>& opts = std::vector<Option>()) override {\n        if (passthrough_func_) {\n            return passthrough_func_(ctx, input, opts);\n        }\n        throw std::runtime_error(\"PassthroughRunnable: conversion not implemented\");\n    }\n    \n    std::shared_ptr<StreamReader<O>> Stream(\n        std::shared_ptr<Context> ctx,\n        const I& input,\n        const std::vector<Option>& opts = std::vector<Option>()) override {\n        O result = Invoke(ctx, input, opts);\n        std::vector<O> results{result};\n        return std::make_shared<SimpleStreamReader<O>>(results);\n    }\n    \n    O Collect(\n        std::shared_ptr<Context> ctx,\n        std::shared_ptr<StreamReader<I>> input,\n        const std::vector<Option>& opts = std::vector<Option>()) override {\n        I value;\n        if (input && input->Read(value)) {\n            return Invoke(ctx, value, opts);\n        }\n        throw std::runtime_error(\"PassthroughRunnable: no input to collect\");\n    }\n    \n    std::shared_ptr<StreamReader<O>> Transform(\n        std::shared_ptr<Context> ctx,\n        std::shared_ptr<StreamReader<I>> input,\n        const std::vector<Option>& opts = std::vector<Option>()) override {\n        std::vector<O> results;\n        I value;\n        while (input && input->Read(value)) {\n            results.push_back(Invoke(ctx, value, opts));\n        }\n        return std::make_shared<SimpleStreamReader<O>>(results);\n    }\n    \n    const std::type_info& GetInputType() const override {\n        return typeid(I);\n    }\n    \n    const std::type_info& GetOutputType() const override {\n        return typeid(O);\n    }\n    \n    std::string GetComponentType() const override {\n        return \"Passthrough\";\n    }\n    \n    void SetPassthroughFunc(\n        std::function<O(\n            std::shared_ptr<Context>,\n            const I&,\n            const std::vector<Option>&)> func) {\n        passthrough_func_ = func;\n    }\n    \nprivate:\n    std::function<O(\n        std::shared_ptr<Context>,\n        const I&,\n        const std::vector<Option>&)> passthrough_func_;\n};

// ⭐ Create LambdaRunnable with callback wrapping enabled
// Aligns with eino/compose/runnable.go:336-400 (newRunnablePacker)
//
// NOTE: In C++, callback wrapping is done INSIDE LambdaRunnable methods
// (see Invoke/Stream/Collect/Transform implementations above).
// This is different from Go's approach where functions are wrapped at creation time,
// but achieves the same result.
//
// The enable_callback parameter is kept for API compatibility with Go,
// but in current implementation, callbacks are always active when handlers
// are present in the Context.
template<typename I, typename O>
std::shared_ptr<LambdaRunnable<I, O>> NewLambdaRunnableWithCallbacks(
    InvokeFunc<I, O> i,
    StreamFunc<I, O> s,
    CollectFunc<I, O> c,
    TransformFunc<I, O> t,
    bool enable_callback = true) {
    
    // In C++, we don't need to wrap functions here because:
    // 1. LambdaRunnable::Invoke/Stream/Collect/Transform already trigger callbacks internally
    // 2. This avoids the type erasure problem that would occur with function wrapping
    //
    // The enable_callback flag could be used in future to add a flag to LambdaRunnable
    // to skip callback triggering for performance-critical scenarios.
    
    // Simply create the LambdaRunnable with the provided functions
    // Callbacks will be triggered automatically in each method
    auto runner = std::make_shared<LambdaRunnable<I, O>>(i, s, c, t);
    
    // Future enhancement: Could add a flag to runner to control callback behavior
    // if (enable_callback) {
    //     runner->SetCallbackEnabled(true);
    // }
    
    return runner;
}

// Standard creation - callbacks are always enabled by default
// (controlled by presence of handlers in Context)
template<typename I, typename O>
std::shared_ptr<LambdaRunnable<I, O>> NewLambdaRunnable(InvokeFunc<I, O> func) {
    auto runner = std::make_shared<LambdaRunnable<I, O>>(func);
    return runner;
}

template<typename I, typename O>
std::shared_ptr<LambdaRunnable<I, O>> NewLambdaRunnable(
    InvokeFunc<I, O> i,
    StreamFunc<I, O> s,
    CollectFunc<I, O> c,
    TransformFunc<I, O> t) {
    auto runner = std::make_shared<LambdaRunnable<I, O>>(i, s, c, t);
    return runner;
}

} // namespace compose
} // namespace eino

// Include callback utilities implementation
// This must come after the class definitions to avoid circular dependencies
#include "eino/compose/utils.h"

#endif // EINO_CPP_COMPOSE_RUNNABLE_H_
