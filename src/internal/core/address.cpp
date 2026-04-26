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

#include "eino/internal/core/address.h"

namespace eino {
namespace internal {
namespace core {

ExecutionContext ExecutionContext::AppendAddressSegment(
    AddressSegmentType seg_type,
    const std::string& seg_id,
    const std::string& sub_id) const {

    // Get current address
    Address current_address = GetCurrentAddress();
    if (current_address.empty()) {
        current_address = {AddressSegment{seg_id, seg_type, sub_id}};
    } else {
        current_address.push_back(AddressSegment{seg_id, seg_type, sub_id});
    }

    auto run_ctx = std::make_shared<AddressContext>();
    run_ctx->addr = current_address;

    if (!resume_info_) {
        return ExecutionContext(run_ctx, nullptr);
    }

    // Check if any interrupt state matches the new address
    std::string matched_id;
    {
        for (const auto& kv : resume_info_->id2_addr) {
            if (AddressEquals(kv.second, current_address)) {
                std::lock_guard<std::mutex> lock(resume_info_->mu);
                auto used_it = resume_info_->id2_state_used.find(kv.first);
                if (used_it == resume_info_->id2_state_used.end() || !used_it->second) {
                    auto state_it = resume_info_->id2_state.find(kv.first);
                    if (state_it != resume_info_->id2_state.end()) {
                        run_ctx->interrupt_state =
                            std::make_shared<InterruptState>(state_it->second);
                    }
                    resume_info_->id2_state_used[kv.first] = true;
                    matched_id = kv.first;
                    break;
                }
            }
        }
    }

    // Take resume data for the new address if there is any
    {
        std::lock_guard<std::mutex> lock(resume_info_->mu);
        auto used_it = resume_info_->id2_resume_data_used.find(matched_id);
        bool used = (used_it != resume_info_->id2_resume_data_used.end()) && used_it->second;
        if (!used) {
            auto data_it = resume_info_->id2_resume_data.find(matched_id);
            if (data_it != resume_info_->id2_resume_data.end()) {
                resume_info_->id2_resume_data_used[matched_id] = true;
                run_ctx->resume_data = data_it->second;
                run_ctx->is_resume_target = true;
            }
        }

        // Also mark as resume target if any descendant address is a resume target
        if (!run_ctx->is_resume_target) {
            for (const auto& kv : resume_info_->id2_addr) {
                if (kv.second.size() > current_address.size() &&
                    AddressHasPrefix(kv.second, current_address)) {
                    auto data_used_it = resume_info_->id2_resume_data_used.find(kv.first);
                    if (data_used_it == resume_info_->id2_resume_data_used.end() ||
                        !data_used_it->second) {
                        run_ctx->is_resume_target = true;
                        break;
                    }
                }
            }
        }
    }

    return ExecutionContext(run_ctx, resume_info_);
}

ExecutionContext ExecutionContext::BatchResumeWithData(
    const std::map<std::string, std::any>& resume_data) const {

    if (!resume_info_) {
        // Create a new GlobalResumeInfo
        auto new_info = std::make_shared<GlobalResumeInfo>();
        new_info->id2_resume_data = resume_data;
        return ExecutionContext(addr_ctx_, new_info);
    }

    std::lock_guard<std::mutex> lock(resume_info_->mu);
    for (const auto& kv : resume_data) {
        resume_info_->id2_resume_data[kv.first] = kv.second;
    }
    return *this;
}

ExecutionContext ExecutionContext::PopulateInterruptState(
    const std::map<std::string, Address>& id2_addr,
    const std::map<std::string, InterruptState>& id2_state) const {

    auto info = resume_info_;
    ExecutionContext result = *this;

    if (info) {
        std::lock_guard<std::mutex> lock(info->mu);
        for (const auto& kv : id2_addr) {
            info->id2_addr[kv.first] = kv.second;
        }
        info->id2_state = id2_state;
    } else {
        info = std::make_shared<GlobalResumeInfo>();
        info->id2_addr = id2_addr;
        info->id2_state = id2_state;
        result = ExecutionContext(addr_ctx_, info);
    }

    // Check if current address matches any interrupt state
    if (addr_ctx_) {
        for (const auto& kv : id2_addr) {
            if (AddressEquals(kv.second, addr_ctx_->addr)) {
                std::lock_guard<std::mutex> lock(info->mu);
                auto used_it = info->id2_state_used.find(kv.first);
                if (used_it == info->id2_state_used.end() || !used_it->second) {
                    auto state_it = id2_state.find(kv.first);
                    if (state_it != id2_state.end()) {
                        addr_ctx_->interrupt_state =
                            std::make_shared<InterruptState>(state_it->second);
                    }
                    info->id2_state_used[kv.first] = true;
                }

                auto data_used_it = info->id2_resume_data_used.find(kv.first);
                if (data_used_it == info->id2_resume_data_used.end() ||
                    !data_used_it->second) {
                    addr_ctx_->is_resume_target = true;
                    auto data_it = info->id2_resume_data.find(kv.first);
                    if (data_it != info->id2_resume_data.end()) {
                        addr_ctx_->resume_data = data_it->second;
                    }
                    info->id2_resume_data_used[kv.first] = true;
                }
                break;
            }
        }
    }

    return result;
}

std::map<std::string, bool> ExecutionContext::GetNextResumptionPoints() const {
    Address parent_addr = GetCurrentAddress();

    if (!resume_info_) {
        return {};
    }

    std::map<std::string, bool> next_points;
    size_t parent_addr_len = parent_addr.size();

    for (const auto& kv : resume_info_->id2_addr) {
        const Address& addr = kv.second;
        if (addr.size() <= parent_addr_len) continue;

        bool is_prefix = (parent_addr_len == 0) ||
                         AddressHasPrefix(addr, parent_addr);
        if (!is_prefix) continue;

        const std::string& child_id = addr[parent_addr_len].id;
        next_points[child_id] = true;
    }

    return next_points;
}

}  // namespace core
}  // namespace internal
}  // namespace eino
