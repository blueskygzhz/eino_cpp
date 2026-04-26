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

#ifndef EINO_CPP_COMPOSE_RESUME_H_
#define EINO_CPP_COMPOSE_RESUME_H_

#include <any>
#include <map>
#include <string>

#include "eino/internal/core/address.h"
#include "eino/internal/core/interrupt.h"
#include "eino/internal/core/resume.h"

namespace eino {
namespace compose {

// Type aliases from internal/core, aligned with eino Go's compose package re-exports
using Address = internal::core::Address;
using AddressSegment = internal::core::AddressSegment;
using AddressSegmentType = internal::core::AddressSegmentType;
using InterruptCtx = internal::core::InterruptCtx;
using ExecutionContext = internal::core::ExecutionContext;

// Address segment type constants
// Aligned with Go: compose.AddressSegmentNode, compose.AddressSegmentTool, compose.AddressSegmentRunnable
static const AddressSegmentType AddressSegmentNode = "node";
static const AddressSegmentType AddressSegmentTool = "tool";
static const AddressSegmentType AddressSegmentRunnable = "runnable";

// GetInterruptState provides a type-safe way to check for and retrieve the persisted state
// from a previous interruption.
// Aligned with Go: compose.GetInterruptState[T]()
//
// Returns: (wasInterrupted, hasState, state)
template <typename T>
std::tuple<bool, bool, T> GetInterruptState(const ExecutionContext& ctx) {
    return internal::core::GetInterruptState<T>(ctx);
}

// GetResumeContext checks if the current component is the target of a resume operation
// and retrieves any data provided by the user for that resumption.
// Aligned with Go: compose.GetResumeContext[T]()
//
// Returns: (isResumeFlow, hasData, data)
template <typename T>
std::tuple<bool, bool, T> GetResumeContext(const ExecutionContext& ctx) {
    return internal::core::GetResumeContext<T>(ctx);
}

// GetCurrentAddress returns the hierarchical address of the currently executing component.
// Aligned with Go: compose.GetCurrentAddress()
inline Address GetCurrentAddress(const ExecutionContext& ctx) {
    return ctx.GetCurrentAddress();
}

// Resume prepares a context for an "Explicit Targeted Resume" operation by targeting one or more
// components without providing data. Convenience wrapper around BatchResumeWithData.
// Aligned with Go: compose.Resume()
inline ExecutionContext Resume(const ExecutionContext& ctx,
                               const std::vector<std::string>& interrupt_ids) {
    std::map<std::string, std::any> resume_data;
    for (const auto& id : interrupt_ids) {
        resume_data[id] = std::any{};
    }
    return ctx.BatchResumeWithData(resume_data);
}

// ResumeWithData prepares a context to resume a single, specific component with data.
// Aligned with Go: compose.ResumeWithData()
inline ExecutionContext ResumeWithData(const ExecutionContext& ctx,
                                       const std::string& interrupt_id,
                                       const std::any& data) {
    return ctx.BatchResumeWithData({{interrupt_id, data}});
}

// BatchResumeWithData is the core function for preparing a resume context.
// Aligned with Go: compose.BatchResumeWithData()
inline ExecutionContext BatchResumeWithData(const ExecutionContext& ctx,
                                            const std::map<std::string, std::any>& resume_data) {
    return ctx.BatchResumeWithData(resume_data);
}

// AppendAddressSegment creates a new execution context for a sub-component.
// Aligned with Go: compose.AppendAddressSegment()
inline ExecutionContext AppendAddressSegment(const ExecutionContext& ctx,
                                             AddressSegmentType seg_type,
                                             const std::string& seg_id) {
    return ctx.AppendAddressSegment(seg_type, seg_id, "");
}

}  // namespace compose
}  // namespace eino

#endif  // EINO_CPP_COMPOSE_RESUME_H_
