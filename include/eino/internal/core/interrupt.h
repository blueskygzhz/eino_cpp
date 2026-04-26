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

#ifndef EINO_CPP_INTERNAL_CORE_INTERRUPT_H_
#define EINO_CPP_INTERNAL_CORE_INTERRUPT_H_

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "eino/internal/core/address.h"

namespace eino {
namespace internal {
namespace core {

// CheckPointStore is the interface for checkpoint persistence.
// Aligned with Go: core.CheckPointStore
class CheckPointStore {
public:
    virtual ~CheckPointStore() = default;

    // Get retrieves a checkpoint by ID. Returns (data, found, error).
    virtual std::tuple<std::vector<uint8_t>, bool, std::string> Get(
        const std::string& checkpoint_id) = 0;

    // Set stores a checkpoint by ID.
    virtual std::string Set(const std::string& checkpoint_id,
                            const std::vector<uint8_t>& checkpoint) = 0;
};

// InterruptInfo holds user-facing information about an interrupt.
// Aligned with Go: core.InterruptInfo
struct InterruptInfo {
    std::any info;           // User-facing information about the interrupt
    bool is_root_cause = false;  // Whether this is the root cause of the interruption

    std::string String() const {
        std::ostringstream oss;
        oss << "interrupt info: IsRootCause=" << (is_root_cause ? "true" : "false");
        return oss.str();
    }
};

// InterruptState holds the persisted state for an interrupt point.
// Aligned with Go: core.InterruptState
struct InterruptState {
    std::any state;                    // Component's persisted state
    std::any layer_specific_payload;   // Layer-specific metadata

    std::string String() const {
        return "interrupt state";
    }
};

// InterruptConfig holds optional parameters for creating an interrupt.
// Aligned with Go: core.InterruptConfig
struct InterruptConfig {
    std::any layer_payload;
};

// InterruptOption is a function that configures an InterruptConfig.
// Aligned with Go: core.InterruptOption
using InterruptOption = std::function<void(InterruptConfig&)>;

// WithLayerPayload creates an option to attach layer-specific metadata.
// Aligned with Go: core.WithLayerPayload()
inline InterruptOption WithLayerPayload(std::any payload) {
    return [payload = std::move(payload)](InterruptConfig& config) {
        config.layer_payload = payload;
    };
}

// InterruptSignal is the core error type for interrupt-and-rerun.
// Aligned with Go: core.InterruptSignal
struct InterruptSignal {
    std::string id;
    Address address;
    InterruptInfo interrupt_info;
    InterruptState interrupt_state;
    std::vector<std::shared_ptr<InterruptSignal>> subs;

    std::string Error() const {
        std::ostringstream oss;
        oss << "interrupt signal: ID=" << id
            << ", Addr=" << AddressToString(address)
            << ", " << interrupt_info.String()
            << ", " << interrupt_state.String()
            << ", SubsLen=" << subs.size();
        return oss.str();
    }
};

// InterruptSignalError wraps InterruptSignal as a C++ exception.
class InterruptSignalError : public std::runtime_error {
public:
    explicit InterruptSignalError(std::shared_ptr<InterruptSignal> signal)
        : std::runtime_error(signal ? signal->Error() : "interrupt signal"),
          signal_(std::move(signal)) {}

    const std::shared_ptr<InterruptSignal>& GetSignal() const { return signal_; }

private:
    std::shared_ptr<InterruptSignal> signal_;
};

// InterruptCtx provides a complete, user-facing context for a single, resumable interrupt point.
// Aligned with Go: core.InterruptCtx
struct InterruptCtx {
    // ID is the unique, fully-qualified address of the interrupt point.
    // Constructed by joining Address segments, e.g., "agent:A;node:graph_a;tool:tool_call_123".
    // Used when providing resume data via ResumeWithData.
    std::string id;
    // Address is the structured sequence of AddressSegment segments.
    Address address;
    // Info is the user-facing information associated with the interrupt.
    std::any info;
    // IsRootCause indicates whether the interrupt point is the exact root cause.
    bool is_root_cause = false;
    // Parent points to the context of the parent component in the interrupt chain.
    std::shared_ptr<InterruptCtx> parent;

    bool EqualsWithoutID(const InterruptCtx& other) const {
        if (!AddressEquals(address, other.address)) return false;
        if (is_root_cause != other.is_root_cause) return false;
        // Note: std::any comparison is not straightforward in C++,
        // so we skip deep comparison of info here.
        if ((parent == nullptr) != (other.parent == nullptr)) return false;
        if (parent && other.parent) {
            return parent->EqualsWithoutID(*other.parent);
        }
        return true;
    }
};

// InterruptContextsProvider is an interface for errors that contain interrupt contexts.
// Aligned with Go: core.InterruptContextsProvider
class InterruptContextsProvider {
public:
    virtual ~InterruptContextsProvider() = default;
    virtual std::vector<std::shared_ptr<InterruptCtx>> GetInterruptContexts() const = 0;
};

// Interrupt creates an InterruptSignal from the current execution context.
// Aligned with Go: core.Interrupt()
inline std::shared_ptr<InterruptSignal> Interrupt(
    const ExecutionContext& ctx,
    const std::any& info,
    const std::any& state,
    const std::vector<std::shared_ptr<InterruptSignal>>& sub_contexts = {},
    const std::vector<InterruptOption>& opts = {}) {

    Address addr = ctx.GetCurrentAddress();

    InterruptConfig config;
    for (const auto& opt : opts) {
        opt(config);
    }

    InterruptInfo my_point;
    my_point.info = info;

    auto signal = std::make_shared<InterruptSignal>();
    // Generate a simple unique ID (in production, use a proper UUID library)
    static int counter = 0;
    signal->id = "interrupt_" + std::to_string(++counter);
    signal->address = addr;
    signal->interrupt_info = my_point;
    signal->interrupt_state.state = state;
    signal->interrupt_state.layer_specific_payload = config.layer_payload;

    if (sub_contexts.empty()) {
        signal->interrupt_info.is_root_cause = true;
    } else {
        signal->subs = sub_contexts;
    }

    return signal;
}

// FromInterruptContexts reconstructs a single InterruptSignal tree from a list of
// user-facing InterruptCtx objects. It correctly merges common ancestors.
// Aligned with Go: core.FromInterruptContexts()
inline std::shared_ptr<InterruptSignal> FromInterruptContexts(
    const std::vector<std::shared_ptr<InterruptCtx>>& contexts) {
    if (contexts.empty()) return nullptr;

    std::map<std::string, std::shared_ptr<InterruptSignal>> signal_map;
    std::shared_ptr<InterruptSignal> root_signal;

    std::function<std::shared_ptr<InterruptSignal>(const std::shared_ptr<InterruptCtx>&)>
        get_or_create_signal;

    get_or_create_signal = [&](const std::shared_ptr<InterruptCtx>& ictx)
        -> std::shared_ptr<InterruptSignal> {
        if (!ictx) return nullptr;

        auto it = signal_map.find(ictx->id);
        if (it != signal_map.end()) {
            return it->second;
        }

        auto new_signal = std::make_shared<InterruptSignal>();
        new_signal->id = ictx->id;
        new_signal->address = ictx->address;
        new_signal->interrupt_info.info = ictx->info;
        new_signal->interrupt_info.is_root_cause = ictx->is_root_cause;
        signal_map[ictx->id] = new_signal;

        auto parent_signal = get_or_create_signal(ictx->parent);
        if (parent_signal) {
            parent_signal->subs.push_back(new_signal);
        } else {
            root_signal = new_signal;
        }
        return new_signal;
    };

    for (const auto& ctx : contexts) {
        get_or_create_signal(ctx);
    }

    return root_signal;
}

// ToInterruptContexts converts the internal InterruptSignal tree into a list of
// user-facing InterruptCtx objects for the root causes of the interruption.
// Aligned with Go: core.ToInterruptContexts()
inline std::vector<std::shared_ptr<InterruptCtx>> ToInterruptContexts(
    const std::shared_ptr<InterruptSignal>& is,
    const std::vector<AddressSegmentType>& allowed_segment_types = {}) {
    if (!is) return {};

    std::vector<std::shared_ptr<InterruptCtx>> root_cause_contexts;

    std::function<void(const std::shared_ptr<InterruptSignal>&,
                       const std::shared_ptr<InterruptCtx>&)> build_contexts;

    build_contexts = [&](const std::shared_ptr<InterruptSignal>& signal,
                         const std::shared_ptr<InterruptCtx>& parent_ctx) {
        auto current_ctx = std::make_shared<InterruptCtx>();
        current_ctx->id = signal->id;
        current_ctx->address = signal->address;
        current_ctx->info = signal->interrupt_info.info;
        current_ctx->is_root_cause = signal->interrupt_info.is_root_cause;
        current_ctx->parent = parent_ctx;

        if (current_ctx->is_root_cause) {
            root_cause_contexts.push_back(current_ctx);
        }

        for (const auto& sub_signal : signal->subs) {
            build_contexts(sub_signal, current_ctx);
        }
    };

    build_contexts(is, nullptr);

    // Apply segment type filtering if specified
    if (!allowed_segment_types.empty()) {
        std::map<AddressSegmentType, bool> allowed_set;
        for (const auto& t : allowed_segment_types) {
            allowed_set[t] = true;
        }

        for (auto& ctx : root_cause_contexts) {
            // Filter parent chain
            auto current = ctx;
            while (current) {
                auto parent = current->parent;
                while (parent) {
                    if (!parent->address.empty() &&
                        allowed_set.count(parent->address.back().type)) {
                        break;
                    }
                    parent = parent->parent;
                }
                current->parent = parent;
                current = parent;
            }

            // Filter addresses
            for (auto c = ctx; c; c = c->parent) {
                Address new_addr;
                for (const auto& seg : c->address) {
                    if (allowed_set.count(seg.type)) {
                        new_addr.push_back(seg);
                    }
                }
                c->address = new_addr;
            }
        }
    }

    return root_cause_contexts;
}

// SignalToPersistenceMaps flattens an InterruptSignal tree into two maps suitable for persistence.
// Aligned with Go: core.SignalToPersistenceMaps()
inline std::pair<std::map<std::string, Address>, std::map<std::string, InterruptState>>
SignalToPersistenceMaps(const std::shared_ptr<InterruptSignal>& is) {
    std::map<std::string, Address> id2addr;
    std::map<std::string, InterruptState> id2state;

    if (!is) return {id2addr, id2state};

    std::function<void(const std::shared_ptr<InterruptSignal>&)> traverse;
    traverse = [&](const std::shared_ptr<InterruptSignal>& signal) {
        id2addr[signal->id] = signal->address;
        id2state[signal->id] = signal->interrupt_state;
        for (const auto& sub : signal->subs) {
            traverse(sub);
        }
    };

    traverse(is);
    return {id2addr, id2state};
}

}  // namespace core
}  // namespace internal
}  // namespace eino

#endif  // EINO_CPP_INTERNAL_CORE_INTERRUPT_H_
