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

#ifndef EINO_CPP_INTERNAL_CORE_RESUME_H_
#define EINO_CPP_INTERNAL_CORE_RESUME_H_

#include <tuple>

#include "eino/internal/core/address.h"
#include "eino/internal/core/interrupt.h"

namespace eino {
namespace internal {
namespace core {

// GetInterruptState provides a type-safe way to check for and retrieve the persisted state
// from a previous interruption.
// Aligned with Go: core.GetInterruptState[T]()
//
// Returns: (wasInterrupted, hasState, state)
//   - wasInterrupted: True if the node was part of a previous interruption.
//   - hasState: True if state was provided and successfully cast to type T.
//   - state: The typed state object.
template <typename T>
std::tuple<bool, bool, T> GetInterruptState(const ExecutionContext& ctx) {
    auto addr_ctx = ctx.GetAddressContext();
    if (!addr_ctx || !addr_ctx->interrupt_state) {
        return {false, false, T{}};
    }

    bool was_interrupted = true;
    if (!addr_ctx->interrupt_state->state.has_value()) {
        return {was_interrupted, false, T{}};
    }

    try {
        T state = std::any_cast<T>(addr_ctx->interrupt_state->state);
        return {was_interrupted, true, state};
    } catch (const std::bad_any_cast&) {
        return {was_interrupted, false, T{}};
    }
}

// GetResumeContext checks if the current component is the target of a resume operation
// and retrieves any data provided by the user for that resumption.
// Aligned with Go: core.GetResumeContext[T]()
//
// Returns: (isResumeTarget, hasData, data)
//   - isResumeTarget: True if the current component or any descendant is a resume target.
//   - hasData: True if data was provided for this specific component.
//   - data: The typed data provided by the user.
//
// ### Strategy 1: Implicit "Resume All"
// A component can just use GetInterruptState to see if wasInterrupted is true and proceed.
//
// ### Strategy 2: Explicit "Targeted Resume" (Most Common)
//   - If isResumeTarget is true: Your component (or a descendant) is the target.
//   - If isResumeTarget is false: You MUST re-interrupt to preserve your state.
template <typename T>
std::tuple<bool, bool, T> GetResumeContext(const ExecutionContext& ctx) {
    auto addr_ctx = ctx.GetAddressContext();
    if (!addr_ctx) {
        return {false, false, T{}};
    }

    bool is_resume_target = addr_ctx->is_resume_target;
    if (!is_resume_target) {
        return {false, false, T{}};
    }

    if (!addr_ctx->resume_data.has_value()) {
        return {true, false, T{}};
    }

    try {
        T data = std::any_cast<T>(addr_ctx->resume_data);
        return {true, true, data};
    } catch (const std::bad_any_cast&) {
        return {true, false, T{}};
    }
}

}  // namespace core
}  // namespace internal
}  // namespace eino

#endif  // EINO_CPP_INTERNAL_CORE_RESUME_H_
