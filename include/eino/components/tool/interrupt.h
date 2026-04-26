/*
 * Copyright 2025 CloudWeGo Authors
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

#ifndef EINO_CPP_COMPONENTS_TOOL_INTERRUPT_H_
#define EINO_CPP_COMPONENTS_TOOL_INTERRUPT_H_

// Tool-level interrupt/resume support.
// Aligned with Go: components/tool/interrupt.go
//
// Provides Interrupt, StatefulInterrupt, CompositeInterrupt, GetInterruptState,
// and GetResumeContext functions specifically for tool implementations.

#include <any>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "eino/internal/core/address.h"
#include "eino/internal/core/interrupt.h"
#include "eino/internal/core/resume.h"

namespace eino {
namespace components {
namespace tool {

// Interrupt pauses tool execution and signals the orchestration layer to checkpoint.
// The tool can be resumed later with optional data.
//
// Parameters:
//   - ctx: The execution context passed to InvokableRun/StreamableRun
//   - info: User-facing information about why the tool is interrupting
//
// Returns an InterruptSignalError that should be thrown from InvokableRun/StreamableRun.
//
// Example:
//   std::string InvokableRun(const ExecutionContext& ctx, const std::string& args) {
//       if (needsConfirmation(args)) {
//           throw tool::Interrupt(ctx, std::string("Please confirm this action"));
//       }
//       return doWork(args);
//   }
//
// Aligned with Go: tool.Interrupt()
inline internal::core::InterruptSignalError Interrupt(
    const internal::core::ExecutionContext& ctx,
    const std::any& info) {
    auto signal = internal::core::Interrupt(ctx, info, std::any{});
    return internal::core::InterruptSignalError(signal);
}

// StatefulInterrupt pauses tool execution with state preservation.
// Use this when the tool has internal state that must be restored on resume.
//
// Aligned with Go: tool.StatefulInterrupt()
inline internal::core::InterruptSignalError StatefulInterrupt(
    const internal::core::ExecutionContext& ctx,
    const std::any& info,
    const std::any& state) {
    auto signal = internal::core::Interrupt(ctx, info, state);
    return internal::core::InterruptSignalError(signal);
}

// CompositeInterrupt creates an interrupt that aggregates multiple sub-interrupts.
// Use this when a tool internally executes a graph or other interruptible components.
//
// Aligned with Go: tool.CompositeInterrupt()
inline internal::core::InterruptSignalError CompositeInterrupt(
    const internal::core::ExecutionContext& ctx,
    const std::any& info,
    const std::any& state,
    const std::vector<std::shared_ptr<internal::core::InterruptSignal>>& sub_errors = {}) {

    if (sub_errors.empty()) {
        return StatefulInterrupt(ctx, info, state);
    }

    auto signal = internal::core::Interrupt(ctx, info, state, sub_errors);
    return internal::core::InterruptSignalError(signal);
}

// GetInterruptState checks if the tool was previously interrupted and retrieves saved state.
//
// Returns: (wasInterrupted, hasState, state)
//
// Aligned with Go: tool.GetInterruptState[T]()
template <typename T>
std::tuple<bool, bool, T> GetInterruptState(const internal::core::ExecutionContext& ctx) {
    return internal::core::GetInterruptState<T>(ctx);
}

// GetResumeContext checks if this tool is the explicit target of a resume operation.
//
// Returns: (isResumeTarget, hasData, data)
//
// Aligned with Go: tool.GetResumeContext[T]()
template <typename T>
std::tuple<bool, bool, T> GetResumeContext(const internal::core::ExecutionContext& ctx) {
    return internal::core::GetResumeContext<T>(ctx);
}

// IsInterruptError checks if an exception is an interrupt error.
// Aligned with Go: tool.IsInterruptError()
inline bool IsInterruptError(const std::exception_ptr& eptr) {
    if (!eptr) return false;
    try {
        std::rethrow_exception(eptr);
    } catch (const internal::core::InterruptSignalError&) {
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace tool
}  // namespace components
}  // namespace eino

#endif  // EINO_CPP_COMPONENTS_TOOL_INTERRUPT_H_
