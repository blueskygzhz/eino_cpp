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

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "eino/internal/core/address.h"
#include "eino/internal/core/interrupt.h"

namespace eino {
namespace compose {

// Type aliases from internal/core
// Aligned with Go: compose package re-exports from internal/core
using Address = internal::core::Address;
using AddressSegment = internal::core::AddressSegment;
using AddressSegmentType = internal::core::AddressSegmentType;
using InterruptCtx = internal::core::InterruptCtx;
using InterruptSignal = internal::core::InterruptSignal;
using InterruptSignalError = internal::core::InterruptSignalError;
using ExecutionContext = internal::core::ExecutionContext;

// Address segment type constants
// Aligned with Go: compose.AddressSegmentNode, compose.AddressSegmentTool, compose.AddressSegmentRunnable
static const AddressSegmentType kAddressSegmentNode = "node";
static const AddressSegmentType kAddressSegmentTool = "tool";
static const AddressSegmentType kAddressSegmentRunnable = "runnable";

// GraphCompileOption function type for interrupt configuration
// Aligned with Go: compose.GraphCompileOption
struct GraphCompileOptions {
    std::vector<std::string> interrupt_before_nodes;
    std::vector<std::string> interrupt_after_nodes;
};

using GraphCompileOption = std::function<void(GraphCompileOptions&)>;

// WithInterruptBeforeNodes instructs to interrupt before the given nodes.
// Aligned with Go: compose.WithInterruptBeforeNodes()
inline GraphCompileOption WithInterruptBeforeNodes(const std::vector<std::string>& nodes) {
    return [nodes](GraphCompileOptions& options) {
        options.interrupt_before_nodes = nodes;
    };
}

// WithInterruptAfterNodes instructs to interrupt after the given nodes.
// Aligned with Go: compose.WithInterruptAfterNodes()
inline GraphCompileOption WithInterruptAfterNodes(const std::vector<std::string>& nodes) {
    return [nodes](GraphCompileOptions& options) {
        options.interrupt_after_nodes = nodes;
    };
}

// InterruptInfo aggregates interrupt metadata for composite or nested runs.
// Aligned with Go: compose.InterruptInfo
struct ComposeInterruptInfo {
    std::any state;
    std::vector<std::string> before_nodes;
    std::vector<std::string> after_nodes;
    std::vector<std::string> rerun_nodes;
    std::map<std::string, std::any> rerun_nodes_extra;
    std::map<std::string, ComposeInterruptInfo> sub_graphs;
    std::vector<std::shared_ptr<InterruptCtx>> interrupt_contexts;

    ComposeInterruptInfo() = default;
};

// InterruptError wraps interrupt information.
// Aligned with Go: compose.interruptError
class InterruptError : public std::runtime_error {
public:
    explicit InterruptError(std::shared_ptr<ComposeInterruptInfo> info)
        : std::runtime_error(info ? "interrupt happened" : "interrupt happened (no info)"),
          info_(std::move(info)) {}

    std::shared_ptr<ComposeInterruptInfo> GetInfo() const { return info_; }

    std::vector<std::shared_ptr<InterruptCtx>> GetInterruptContexts() const {
        if (info_) return info_->interrupt_contexts;
        return {};
    }

private:
    std::shared_ptr<ComposeInterruptInfo> info_;
};

// SubGraphInterruptError for nested graph interruptions.
// Aligned with Go: compose.subGraphInterruptError
class SubGraphInterruptError : public std::runtime_error {
public:
    SubGraphInterruptError(std::shared_ptr<ComposeInterruptInfo> info,
                           std::any checkpoint,
                           std::shared_ptr<InterruptSignal> signal = nullptr)
        : std::runtime_error("sub-graph interrupt"),
          info_(std::move(info)),
          checkpoint_(std::move(checkpoint)),
          signal_(std::move(signal)) {}

    std::shared_ptr<ComposeInterruptInfo> GetInfo() const { return info_; }
    const std::any& GetCheckpoint() const { return checkpoint_; }
    std::shared_ptr<InterruptSignal> GetSignal() const { return signal_; }

private:
    std::shared_ptr<ComposeInterruptInfo> info_;
    std::any checkpoint_;
    std::shared_ptr<InterruptSignal> signal_;
};

// Interrupt creates a special error that signals the execution engine to interrupt
// the current run at the component's specific address and save a checkpoint.
// Aligned with Go: compose.Interrupt()
inline InterruptSignalError Interrupt(const ExecutionContext& ctx, const std::any& info) {
    auto signal = internal::core::Interrupt(ctx, info, std::any{});
    return InterruptSignalError(signal);
}

// StatefulInterrupt creates a special error with persisted state.
// Aligned with Go: compose.StatefulInterrupt()
inline InterruptSignalError StatefulInterrupt(const ExecutionContext& ctx,
                                               const std::any& info,
                                               const std::any& state) {
    auto signal = internal::core::Interrupt(ctx, info, state);
    return InterruptSignalError(signal);
}

// CompositeInterrupt creates a special error that signals a composite interruption.
// Designed for "composite" nodes (like ToolsNode) that manage multiple, independent,
// interruptible sub-processes.
// Aligned with Go: compose.CompositeInterrupt()
inline InterruptSignalError CompositeInterrupt(
    const ExecutionContext& ctx,
    const std::any& info,
    const std::any& state,
    const std::vector<std::shared_ptr<InterruptSignal>>& sub_errors = {}) {

    if (sub_errors.empty()) {
        auto signal = internal::core::Interrupt(ctx, info, state);
        return InterruptSignalError(signal);
    }

    auto signal = internal::core::Interrupt(ctx, info, state, sub_errors);
    return InterruptSignalError(signal);
}

// IsInterruptRerunError reports whether the error represents an interrupt-and-rerun
// and returns any attached info.
// Aligned with Go: compose.IsInterruptRerunError()
inline std::pair<std::any, bool> IsInterruptRerunError(const std::exception_ptr& eptr) {
    if (!eptr) return {std::any{}, false};
    try {
        std::rethrow_exception(eptr);
    } catch (const InterruptSignalError& e) {
        auto signal = e.GetSignal();
        if (signal) {
            return {signal->interrupt_info.info, true};
        }
        return {std::any{}, true};
    } catch (const InterruptError& e) {
        return {std::any{}, true};
    } catch (...) {
        return {std::any{}, false};
    }
}

// ExtractInterruptInfo extracts ComposeInterruptInfo from an error if present.
// Aligned with Go: compose.ExtractInterruptInfo()
inline std::pair<std::shared_ptr<ComposeInterruptInfo>, bool>
ExtractInterruptInfo(const std::exception_ptr& eptr) {
    if (!eptr) return {nullptr, false};
    try {
        std::rethrow_exception(eptr);
    } catch (const InterruptError& e) {
        return {e.GetInfo(), true};
    } catch (const SubGraphInterruptError& e) {
        return {e.GetInfo(), true};
    } catch (...) {
        return {nullptr, false};
    }
}

// IsInterruptError checks if an exception is any kind of interrupt error.
// Aligned with Go: compose.isInterruptError()
inline bool IsInterruptError(const std::exception_ptr& eptr) {
    if (!eptr) return false;
    try {
        std::rethrow_exception(eptr);
    } catch (const InterruptSignalError&) {
        return true;
    } catch (const InterruptError&) {
        return true;
    } catch (const SubGraphInterruptError&) {
        return true;
    } catch (...) {
        return false;
    }
}

// ========================================================================
// Legacy API (kept for backward compatibility)
// Deprecated: prefer Interrupt/StatefulInterrupt/CompositeInterrupt
// ========================================================================

// Deprecated: InterruptAndRerunError - use Interrupt() instead
class InterruptAndRerunError : public std::runtime_error {
public:
    explicit InterruptAndRerunError(const std::string& message, const std::any& extra = {})
        : std::runtime_error(message), extra_(extra) {}

    const std::any& GetExtra() const { return extra_; }

private:
    std::any extra_;
};

// Deprecated: NewInterruptAndRerunErr - use Interrupt() instead
// Aligned with Go: compose.NewInterruptAndRerunErr()
inline InterruptSignalError NewInterruptAndRerunErr(const std::any& extra) {
    internal::core::InterruptInfo info;
    info.info = extra;
    info.is_root_cause = true;
    auto signal = std::make_shared<InterruptSignal>();
    signal->interrupt_info = info;
    return InterruptSignalError(signal);
}

}  // namespace compose
}  // namespace eino

#endif  // EINO_CPP_COMPOSE_INTERRUPT_H_
