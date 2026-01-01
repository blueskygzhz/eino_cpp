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

#ifndef EINO_CPP_COMPOSE_INTERRUPT_H_
#define EINO_CPP_COMPOSE_INTERRUPT_H_

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <functional>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>

namespace eino {
namespace compose {

// Forward declarations
class Context;

// InterruptAndRerunError indicates that graph execution should be interrupted and rerun
// Aligns with eino compose.InterruptAndRerun error
class InterruptAndRerunError : public std::runtime_error {
public:
    explicit InterruptAndRerunError(const std::string& message, const std::any& extra = {})
        : std::runtime_error(message), extra_(extra) {}

    const std::any& GetExtra() const { return extra_; }

private:
    std::any extra_;
};

// InterruptInfo contains information about where the graph was interrupted
// Aligns with eino compose.InterruptInfo structure
struct InterruptInfo {
    std::any state;  // Current state at interruption point
    std::vector<std::string> before_nodes;      // Nodes to interrupt before execution
    std::vector<std::string> after_nodes;       // Nodes to interrupt after execution
    std::vector<std::string> rerun_nodes;       // Nodes to rerun on resume
    std::map<std::string, std::any> rerun_nodes_extra;  // Extra data for rerun nodes
    std::map<std::string, InterruptInfo> sub_graphs;    // Info for nested subgraphs

    InterruptInfo() = default;
};

// InterruptError wraps interrupt information
class InterruptError : public std::runtime_error {
public:
    explicit InterruptError(const std::string& message, const std::shared_ptr<InterruptInfo>& info)
        : std::runtime_error(message), info_(info) {}

    std::shared_ptr<InterruptInfo> GetInfo() const { return info_; }

private:
    std::shared_ptr<InterruptInfo> info_;
};

// SubGraphInterruptError for nested graph interruptions
class SubGraphInterruptError : public InterruptError {
public:
    SubGraphInterruptError(const std::string& message, 
                          const std::shared_ptr<InterruptInfo>& info,
                          const std::any& checkpoint)
        : InterruptError(message, info), checkpoint_(checkpoint) {}

    const std::any& GetCheckpoint() const { return checkpoint_; }

private:
    std::any checkpoint_;
};

// Helper functions to extract interrupt information
inline bool IsInterruptError(const std::exception_ptr& eptr) {
    if (!eptr) return false;
    try {
        std::rethrow_exception(eptr);
    } catch (const InterruptError&) {
        return true;
    } catch (const SubGraphInterruptError&) {
        return true;
    } catch (const InterruptAndRerunError&) {
        return true;
    } catch (...) {
        return false;
    }
}

inline std::shared_ptr<InterruptInfo> ExtractInterruptInfo(const std::exception_ptr& eptr) {
    if (!eptr) return nullptr;
    try {
        std::rethrow_exception(eptr);
    } catch (const InterruptError& e) {
        return e.GetInfo();
    } catch (const SubGraphInterruptError& e) {
        return e.GetInfo();
    } catch (...) {
        return nullptr;
    }
}

// GraphInterruptOptions for interrupt configuration
struct GraphInterruptOptions {
    // Timeout before forcing an interrupt (0 = no timeout)
    std::chrono::milliseconds timeout{0};

    // Flag to enable interrupt support
    bool enable_interrupt = true;

    GraphInterruptOptions() = default;
};

// InterruptHandle provides interface to interrupt graph execution
class InterruptHandle {
public:
    InterruptHandle() = default;
    virtual ~InterruptHandle() = default;

    // Interrupt triggers an interrupt with optional timeout
    virtual void Interrupt(const std::shared_ptr<GraphInterruptOptions>& opts = nullptr) = 0;

    // IsInterrupted returns true if interrupt was triggered
    virtual bool IsInterrupted() const = 0;

    // WaitForInterrupt blocks until interrupt signal is received
    virtual bool WaitForInterrupt(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(0)) = 0;

private:
    InterruptHandle(const InterruptHandle&) = delete;
    InterruptHandle& operator=(const InterruptHandle&) = delete;
};

// Default implementation of InterruptHandle
class DefaultInterruptHandle : public InterruptHandle {
public:
    DefaultInterruptHandle() : interrupted_(false) {}

    void Interrupt(const std::shared_ptr<GraphInterruptOptions>& opts = nullptr) override {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            interrupted_ = true;
            if (opts && opts->timeout.count() > 0) {
                timeout_ms_ = opts->timeout.count();
            }
        }
        cv_.notify_all();
    }

    bool IsInterrupted() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return interrupted_;
    }

    bool WaitForInterrupt(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(0)) override {
        std::unique_lock<std::mutex> lock(mutex_);
        if (interrupted_) return true;
        
        if (timeout.count() == 0) {
            cv_.wait(lock, [this] { return interrupted_; });
            return true;
        } else {
            return cv_.wait_for(lock, timeout, [this] { return interrupted_; });
        }
    }

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    std::atomic<bool> interrupted_;
    long long timeout_ms_{0};
};

// GraphInterruptContext wraps interrupt capabilities into Context
class GraphInterruptContext {
public:
    explicit GraphInterruptContext(std::shared_ptr<Context> parent_ctx)
        : parent_context_(parent_ctx), 
          interrupt_handle_(std::make_shared<DefaultInterruptHandle>()) {}

    std::shared_ptr<Context> GetParentContext() const { return parent_context_; }

    std::shared_ptr<InterruptHandle> GetInterruptHandle() const { return interrupt_handle_; }

    void SetInterruptHandle(std::shared_ptr<InterruptHandle> handle) {
        if (handle) {
            interrupt_handle_ = handle;
        }
    }

    // Check if execution should be interrupted
    bool ShouldInterrupt() const {
        return interrupt_handle_ && interrupt_handle_->IsInterrupted();
    }

private:
    std::shared_ptr<Context> parent_context_;
    std::shared_ptr<InterruptHandle> interrupt_handle_;
};

// WithGraphInterrupt creates a context with interrupt support
inline std::pair<std::shared_ptr<Context>, std::function<void()>> 
WithGraphInterrupt(std::shared_ptr<Context> parent_ctx = nullptr) {
    if (!parent_ctx) {
        parent_ctx = Context::Background();
    }
    
    auto interrupt_ctx = std::make_shared<GraphInterruptContext>(parent_ctx);
    auto interrupt_handle = interrupt_ctx->GetInterruptHandle();
    
    auto interrupt_fn = [interrupt_handle]() {
        if (interrupt_handle) {
            interrupt_handle->Interrupt();
        }
    };
    
    return {std::static_pointer_cast<Context>(parent_ctx), interrupt_fn};
}

// WithGraphInterruptTimeout sets a timeout for interrupt
inline std::function<void()> 
WithGraphInterruptTimeout(std::shared_ptr<InterruptHandle> handle,
                         const std::chrono::milliseconds& timeout) {
    auto opts = std::make_shared<GraphInterruptOptions>();
    opts->timeout = timeout;
    
    return [handle, opts]() {
        if (handle) {
            handle->Interrupt(opts);
        }
    };
}

// Helper to check if error is interrupt and extract info
inline std::pair<std::shared_ptr<InterruptInfo>, bool> 
TryExtractInterruptInfo(const std::exception_ptr& eptr) {
    auto info = ExtractInterruptInfo(eptr);
    return {info, info != nullptr};
}

// Interrupt builder for fluent API
class InterruptBuilder {
public:
    InterruptBuilder() : options_(std::make_shared<GraphInterruptOptions>()) {}

    InterruptBuilder& WithTimeout(const std::chrono::milliseconds& timeout) {
        options_->timeout = timeout;
        return *this;
    }

    InterruptBuilder& EnableInterrupt(bool enable = true) {
        options_->enable_interrupt = enable;
        return *this;
    }

    std::shared_ptr<GraphInterruptOptions> Build() const { return options_; }

private:
    std::shared_ptr<GraphInterruptOptions> options_;
};

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_INTERRUPT_H_
