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

#ifndef EINO_CPP_CALLBACKS_ASPECT_INJECT_H_
#define EINO_CPP_CALLBACKS_ASPECT_INJECT_H_

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include <map>

namespace eino {
namespace callbacks {

using json = nlohmann::json;

// CallbackTiming enumerates all the timing of callback aspects
enum class CallbackTiming {
    // OnStart is called before the runnable is invoked
    kOnStart = 0,
    // OnEnd is called after the runnable completes
    kOnEnd = 1,
    // OnError is called when an error occurs
    kOnError = 2,
    // OnStartWithStreamInput is called before a transform/collect invocation
    kOnStartWithStreamInput = 3,
    // OnEndWithStreamOutput is called after a transform/collect completes
    kOnEndWithStreamOutput = 4,
};

// TimingChecker checks if the handler is needed for the given callback aspect timing
// This is recommended for callback handlers to implement for optimization
class TimingChecker {
public:
    virtual ~TimingChecker() = default;
    
    // Check returns true if the handler needs to be called for the given timing
    virtual bool Check(CallbackTiming timing) = 0;
};

// Handler with TimingChecker support
class HandlerWithTiming : public TimingChecker {
public:
    virtual ~HandlerWithTiming() = default;
    
    // OnStart is called before the runnable is invoked
    virtual void OnStart(const RunInfo& info, const CallbackInput& input) {}
    
    // OnEnd is called after the runnable completes
    virtual void OnEnd(const RunInfo& info, const CallbackOutput& output) {}
    
    // OnError is called when an error occurs
    virtual void OnError(const RunInfo& info, const std::string& error) {}
    
    // OnStartWithStreamInput is called before stream processing
    virtual void OnStartWithStreamInput(const RunInfo& info, const CallbackInput& input) {}
    
    // OnEndWithStreamOutput is called after stream processing
    virtual void OnEndWithStreamOutput(const RunInfo& info, const CallbackOutput& output) {}
    
    // Default implementation always returns true (all timings enabled)
    bool Check(CallbackTiming timing) override {
        return true;
    }
};

// AspectInterceptor manages callback handler chains
// Supports multiple handlers, error handling, and timing-based optimization
class AspectInterceptor {
public:
    AspectInterceptor() = default;
    virtual ~AspectInterceptor() = default;
    
    // AddHandler adds a handler to the interceptor chain
    void AddHandler(std::shared_ptr<HandlerWithTiming> handler) {
        handlers_.push_back(handler);
    }
    
    // AddHandlers adds multiple handlers
    void AddHandlers(const std::vector<std::shared_ptr<HandlerWithTiming>>& handlers) {
        for (const auto& h : handlers) {
            handlers_.push_back(h);
        }
    }
    
    // GetHandlers returns all handlers
    const std::vector<std::shared_ptr<HandlerWithTiming>>& GetHandlers() const {
        return handlers_;
    }
    
    // RemoveHandler removes a handler from the chain
    void RemoveHandler(std::shared_ptr<HandlerWithTiming> handler) {
        auto it = std::find(handlers_.begin(), handlers_.end(), handler);
        if (it != handlers_.end()) {
            handlers_.erase(it);
        }
    }
    
    // ClearHandlers removes all handlers
    void ClearHandlers() {
        handlers_.clear();
    }
    
    // HasHandlersForTiming checks if any handler needs this timing
    bool HasHandlersForTiming(CallbackTiming timing) const {
        for (const auto& handler : handlers_) {
            if (handler && handler->Check(timing)) {
                return true;
            }
        }
        return false;
    }
    
    // OnStart calls all registered OnStart handlers
    void OnStart(const RunInfo& info, const CallbackInput& input) {
        if (!HasHandlersForTiming(CallbackTiming::kOnStart)) {
            return;
        }
        for (const auto& handler : handlers_) {
            if (handler && handler->Check(CallbackTiming::kOnStart)) {
                try {
                    handler->OnStart(info, input);
                } catch (const std::exception& e) {
                    // Error in callback handler should not break the flow
                }
            }
        }
    }
    
    // OnEnd calls all registered OnEnd handlers
    void OnEnd(const RunInfo& info, const CallbackOutput& output) {
        if (!HasHandlersForTiming(CallbackTiming::kOnEnd)) {
            return;
        }
        for (const auto& handler : handlers_) {
            if (handler && handler->Check(CallbackTiming::kOnEnd)) {
                try {
                    handler->OnEnd(info, output);
                } catch (const std::exception& e) {
                    // Error in callback handler should not break the flow
                }
            }
        }
    }
    
    // OnError calls all registered OnError handlers
    void OnError(const RunInfo& info, const std::string& error) {
        if (!HasHandlersForTiming(CallbackTiming::kOnError)) {
            return;
        }
        for (const auto& handler : handlers_) {
            if (handler && handler->Check(CallbackTiming::kOnError)) {
                try {
                    handler->OnError(info, error);
                } catch (const std::exception& e) {
                    // Error in callback handler should not break the flow
                }
            }
        }
    }
    
    // OnStartWithStreamInput calls all registered OnStartWithStreamInput handlers
    void OnStartWithStreamInput(const RunInfo& info, const CallbackInput& input) {
        if (!HasHandlersForTiming(CallbackTiming::kOnStartWithStreamInput)) {
            return;
        }
        for (const auto& handler : handlers_) {
            if (handler && handler->Check(CallbackTiming::kOnStartWithStreamInput)) {
                try {
                    handler->OnStartWithStreamInput(info, input);
                } catch (const std::exception& e) {
                    // Error in callback handler should not break the flow
                }
            }
        }
    }
    
    // OnEndWithStreamOutput calls all registered OnEndWithStreamOutput handlers
    void OnEndWithStreamOutput(const RunInfo& info, const CallbackOutput& output) {
        if (!HasHandlersForTiming(CallbackTiming::kOnEndWithStreamOutput)) {
            return;
        }
        for (const auto& handler : handlers_) {
            if (handler && handler->Check(CallbackTiming::kOnEndWithStreamOutput)) {
                try {
                    handler->OnEndWithStreamOutput(info, output);
                } catch (const std::exception& e) {
                    // Error in callback handler should not break the flow
                }
            }
        }
    }
    
private:
    std::vector<std::shared_ptr<HandlerWithTiming>> handlers_;
};

// Global handler management
class GlobalHandlerManager {
public:
    static GlobalHandlerManager& GetInstance() {
        static GlobalHandlerManager instance;
        return instance;
    }
    
    void AppendGlobalHandlers(const std::vector<std::shared_ptr<HandlerWithTiming>>& handlers) {
        for (const auto& h : handlers) {
            global_handlers_.push_back(h);
        }
    }
    
    void AppendGlobalHandler(std::shared_ptr<HandlerWithTiming> handler) {
        global_handlers_.push_back(handler);
    }
    
    const std::vector<std::shared_ptr<HandlerWithTiming>>& GetGlobalHandlers() const {
        return global_handlers_;
    }
    
    void ClearGlobalHandlers() {
        global_handlers_.clear();
    }
    
private:
    GlobalHandlerManager() = default;
    std::vector<std::shared_ptr<HandlerWithTiming>> global_handlers_;
};

// Convenience functions for global handler management
inline void AppendGlobalHandlers(const std::vector<std::shared_ptr<HandlerWithTiming>>& handlers) {
    GlobalHandlerManager::GetInstance().AppendGlobalHandlers(handlers);
}

inline void AppendGlobalHandler(std::shared_ptr<HandlerWithTiming> handler) {
    GlobalHandlerManager::GetInstance().AppendGlobalHandler(handler);
}

} // namespace callbacks
} // namespace eino

#endif // EINO_CPP_CALLBACKS_ASPECT_INJECT_H_
