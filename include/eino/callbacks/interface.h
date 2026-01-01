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

#ifndef EINO_CPP_CALLBACKS_INTERFACE_H_
#define EINO_CPP_CALLBACKS_INTERFACE_H_

#include <memory>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include <map>

namespace eino {
namespace callbacks {

using json = nlohmann::json;

// RunInfo contains information about a run
struct RunInfo {
    std::string run_id;
    std::string name;
    std::string run_type;  // "chain", "llm", "tool", etc.
    std::map<std::string, std::string> extra;  // Extra metadata
};

// CallbackInput represents input to a callback
struct CallbackInput {
    json input;
    std::map<std::string, json> extra;  // Extra context
};

// CallbackOutput represents output from a callback
struct CallbackOutput {
    json output;
    std::map<std::string, json> extra;  // Extra context
};

// Handler is the base interface for callbacks
class Handler {
public:
    virtual ~Handler() = default;
    
    // Called before the runnable is invoked
    virtual void OnStart(const RunInfo& info, const CallbackInput& input) {}
    
    // Called after the runnable completes
    virtual void OnEnd(const RunInfo& info, const CallbackOutput& output) {}
    
    // Called when an error occurs
    virtual void OnError(const RunInfo& info, const std::string& error) {}
    
    // Called before stream input processing
    virtual void OnStartWithStreamInput(const RunInfo& info, const CallbackInput& input) {}
    
    // Called after stream output processing
    virtual void OnEndWithStreamOutput(const RunInfo& info, const CallbackOutput& output) {}
};

// HandlerBuilder helps construct handlers with fluent API
class HandlerBuilder {
public:
    using OnStartFn = std::function<void(const RunInfo&, const CallbackInput&)>;
    using OnEndFn = std::function<void(const RunInfo&, const CallbackOutput&)>;
    using OnErrorFn = std::function<void(const RunInfo&, const std::string&)>;
    using OnStartWithStreamInputFn = std::function<void(const RunInfo&, const CallbackInput&)>;
    using OnEndWithStreamOutputFn = std::function<void(const RunInfo&, const CallbackOutput&)>;
    
    HandlerBuilder& WithOnStart(OnStartFn fn) {
        on_start_ = fn;
        return *this;
    }
    
    HandlerBuilder& WithOnEnd(OnEndFn fn) {
        on_end_ = fn;
        return *this;
    }
    
    HandlerBuilder& WithOnError(OnErrorFn fn) {
        on_error_ = fn;
        return *this;
    }
    
    HandlerBuilder& WithOnStartWithStreamInput(OnStartWithStreamInputFn fn) {
        on_start_with_stream_input_ = fn;
        return *this;
    }
    
    HandlerBuilder& WithOnEndWithStreamOutput(OnEndWithStreamOutputFn fn) {
        on_end_with_stream_output_ = fn;
        return *this;
    }
    
    std::shared_ptr<Handler> Build();
    
private:
    OnStartFn on_start_;
    OnEndFn on_end_;
    OnErrorFn on_error_;
    OnStartWithStreamInputFn on_start_with_stream_input_;
    OnEndWithStreamOutputFn on_end_with_stream_output_;
};

// Convenience function for creating handlers
inline std::shared_ptr<Handler> NewHandler(
    const std::function<void(const RunInfo&, const CallbackInput&)>& on_start = nullptr,
    const std::function<void(const RunInfo&, const CallbackOutput&)>& on_end = nullptr,
    const std::function<void(const RunInfo&, const std::string&)>& on_error = nullptr) {
    HandlerBuilder builder;
    if (on_start) builder.WithOnStart(on_start);
    if (on_end) builder.WithOnEnd(on_end);
    if (on_error) builder.WithOnError(on_error);
    return builder.Build();
}

} // namespace callbacks
} // namespace eino

#endif // EINO_CPP_CALLBACKS_INTERFACE_H_
