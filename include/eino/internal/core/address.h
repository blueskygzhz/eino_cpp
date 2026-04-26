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

#ifndef EINO_CPP_INTERNAL_CORE_ADDRESS_H_
#define EINO_CPP_INTERNAL_CORE_ADDRESS_H_

#include <any>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace eino {
namespace internal {
namespace core {

// AddressSegmentType defines the type of a segment in an execution address.
// Aligned with Go: core.AddressSegmentType
using AddressSegmentType = std::string;

// AddressSegment represents a single segment in the hierarchical address of an execution point.
// A sequence of AddressSegments uniquely identifies a location within a potentially nested structure.
// Aligned with Go: core.AddressSegment
struct AddressSegment {
    // ID is the unique identifier for this segment, e.g., the node's key or the tool's name.
    std::string id;
    // Type indicates whether this address segment is a graph node, a tool call, an agent, etc.
    AddressSegmentType type;
    // In some cases, ID alone are not unique enough, we need this SubID to guarantee uniqueness.
    // e.g. parallel tool calls with the same name but different tool call IDs.
    std::string sub_id;

    bool operator==(const AddressSegment& other) const {
        return type == other.type && id == other.id && sub_id == other.sub_id;
    }

    bool operator!=(const AddressSegment& other) const {
        return !(*this == other);
    }
};

// Address represents a full, hierarchical address to a point in the execution structure.
// Aligned with Go: core.Address ([]AddressSegment)
using Address = std::vector<AddressSegment>;

// Convert an Address into its unique string representation.
// Aligned with Go: Address.String()
inline std::string AddressToString(const Address& addr) {
    if (addr.empty()) return "";
    std::string result;
    for (size_t i = 0; i < addr.size(); ++i) {
        result += addr[i].type;
        result += ":";
        result += addr[i].id;
        if (!addr[i].sub_id.empty()) {
            result += ":";
            result += addr[i].sub_id;
        }
        if (i != addr.size() - 1) {
            result += ";";
        }
    }
    return result;
}

// Check if two addresses are equal.
// Aligned with Go: Address.Equals()
inline bool AddressEquals(const Address& a, const Address& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// Check if 'prefix' is a prefix of 'addr'.
inline bool AddressHasPrefix(const Address& addr, const Address& prefix) {
    if (prefix.size() > addr.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (addr[i] != prefix[i]) return false;
    }
    return true;
}

// Forward declarations
struct InterruptState;

// AddressContext holds the address and interrupt/resume state for the current execution point.
// Aligned with Go: addrCtx struct
struct AddressContext {
    Address addr;
    std::shared_ptr<InterruptState> interrupt_state;
    bool is_resume_target = false;
    std::any resume_data;
};

// GlobalResumeInfo holds the global resume information for a run.
// Aligned with Go: globalResumeInfo struct
struct GlobalResumeInfo {
    mutable std::mutex mu;
    std::map<std::string, std::any> id2_resume_data;
    std::map<std::string, bool> id2_resume_data_used;
    std::map<std::string, InterruptState> id2_state;
    std::map<std::string, bool> id2_state_used;
    std::map<std::string, Address> id2_addr;
};

// Context carries address and resume information through the execution chain.
// This is the C++ equivalent of Go's context.Context with address-related values.
class ExecutionContext {
public:
    ExecutionContext() = default;

    explicit ExecutionContext(std::shared_ptr<AddressContext> addr_ctx,
                             std::shared_ptr<GlobalResumeInfo> resume_info = nullptr)
        : addr_ctx_(std::move(addr_ctx)),
          resume_info_(std::move(resume_info)) {}

    // GetCurrentAddress returns the hierarchical address of the currently executing component.
    // Aligned with Go: core.GetCurrentAddress()
    Address GetCurrentAddress() const {
        if (addr_ctx_) {
            return addr_ctx_->addr;
        }
        return {};
    }

    // GetAddressContext returns the current address context.
    std::shared_ptr<AddressContext> GetAddressContext() const {
        return addr_ctx_;
    }

    // GetResumeInfo returns the global resume info.
    std::shared_ptr<GlobalResumeInfo> GetResumeInfo() const {
        return resume_info_;
    }

    // HasResumeInfo checks if resume info is available.
    bool HasResumeInfo() const {
        return resume_info_ != nullptr;
    }

    // AppendAddressSegment creates a new execution context for a sub-component.
    // Aligned with Go: core.AppendAddressSegment()
    ExecutionContext AppendAddressSegment(AddressSegmentType seg_type,
                                          const std::string& seg_id,
                                          const std::string& sub_id = "") const;

    // BatchResumeWithData injects resume targets and data into the context.
    // Aligned with Go: core.BatchResumeWithData()
    ExecutionContext BatchResumeWithData(const std::map<std::string, std::any>& resume_data) const;

    // PopulateInterruptState populates interrupt state from checkpoint data.
    // Aligned with Go: core.PopulateInterruptState()
    ExecutionContext PopulateInterruptState(
        const std::map<std::string, Address>& id2_addr,
        const std::map<std::string, InterruptState>& id2_state) const;

    // GetNextResumptionPoints finds the immediate child resumption points.
    // Aligned with Go: core.GetNextResumptionPoints()
    std::map<std::string, bool> GetNextResumptionPoints() const;

private:
    std::shared_ptr<AddressContext> addr_ctx_;
    std::shared_ptr<GlobalResumeInfo> resume_info_;
};

}  // namespace core
}  // namespace internal
}  // namespace eino

#endif  // EINO_CPP_INTERNAL_CORE_ADDRESS_H_
