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

// Interrupt and Resume Mechanism
// ===============================
// Aligns with eino/adk/interrupt.go
//
// Provides checkpoint-based interrupt/resume functionality:
// - CheckPoint storage: serialize and restore execution state
// - InterruptInfo: metadata about interruption point
// - ResumeInfo: information needed to resume execution
// - Serialization support: encode/decode execution state

#include "eino/adk/types.h"
#include "eino/adk/interrupt.h"
#include "eino/compose/checkpoint.h"
#include <memory>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace eino {
namespace adk {

using json = nlohmann::json;

// ============================================================================
// Serialization Support (align with eino marshaller/unmarshaller)
// ============================================================================

// Serialize InterruptInfo to JSON
json SerializeInterruptInfo(const std::shared_ptr<InterruptInfo>& info) {
    if (!info) {
        return json::object();
    }
    
    json j;
    j["interrupt_type"] = static_cast<int>(info->interrupt_type);
    j["reason"] = info->reason;
    j["state_key"] = info->state_key;
    j["context"] = info->context;
    j["extra"] = json(info->extra);
    
    return j;
}

// Deserialize InterruptInfo from JSON
std::shared_ptr<InterruptInfo> DeserializeInterruptInfo(const json& j) {
    auto info = std::make_shared<InterruptInfo>();
    
    if (j.contains("interrupt_type")) {
        info->interrupt_type = static_cast<InterruptType>(j["interrupt_type"].get<int>());
    }
    if (j.contains("reason")) {
        info->reason = j["reason"].get<std::string>();
    }
    if (j.contains("state_key")) {
        info->state_key = j["state_key"].get<std::string>();
    }
    if (j.contains("context")) {
        info->context = j["context"];
    }
    if (j.contains("extra")) {
        info->extra = j["extra"].get<std::map<std::string, json>>();
    }
    
    return info;
}

// Serialize CheckPoint data (RunContext + InterruptInfo)
std::vector<uint8_t> SerializeCheckPoint(
    const std::shared_ptr<void>& run_ctx,
    const std::shared_ptr<InterruptInfo>& info) {
    
    json j;
    
    // Serialize interrupt info
    if (info) {
        j["interrupt_info"] = SerializeInterruptInfo(info);
    }
    
    // Serialize run context (if needed)
    // Note: RunContext is typically opaque, may need custom serialization
    j["run_ctx"] = json::object();  // Placeholder
    
    // Convert JSON to bytes
    std::string json_str = j.dump();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

// Deserialize CheckPoint data
std::tuple<std::shared_ptr<void>, std::shared_ptr<InterruptInfo>, std::string>
DeserializeCheckPoint(const std::vector<uint8_t>& data) {
    try {
        // Parse JSON from bytes
        std::string json_str(data.begin(), data.end());
        json j = json::parse(json_str);
        
        // Deserialize interrupt info
        std::shared_ptr<InterruptInfo> info;
        if (j.contains("interrupt_info")) {
            info = DeserializeInterruptInfo(j["interrupt_info"]);
        }
        
        // Deserialize run context (placeholder)
        std::shared_ptr<void> run_ctx = std::make_shared<int>(0);  // Placeholder
        
        return {run_ctx, info, ""};
        
    } catch (const std::exception& e) {
        return {nullptr, nullptr, std::string("Failed to deserialize checkpoint: ") + e.what()};
    }
}

// ============================================================================
// ResumeInfo Implementation
// ============================================================================

ResumeInfo::ResumeInfo()
    : enable_streaming(false), interrupt_info(nullptr) {
}

ResumeInfo::~ResumeInfo() {
}

// ============================================================================
// InterruptInfo Implementation
// ============================================================================

InterruptInfo::InterruptInfo() : data(nullptr) {
}

InterruptInfo::~InterruptInfo() {
}

// ============================================================================
// CheckPoint Storage Helpers (align with eino/adk/interrupt.go)
// ============================================================================

const std::string kMockCheckPointID = "adk_react_mock_key";

// Get checkpoint from store
std::tuple<std::shared_ptr<RunContext>, std::shared_ptr<ResumeInfo>, bool> GetCheckPoint(
    void* ctx,
    compose::CheckPointStore* store,
    const std::string& key) {
    
    if (!store) {
        return {nullptr, nullptr, false};
    }
    
    // Get checkpoint data from store
    std::vector<uint8_t> data;
    bool existed = false;
    try {
        std::tie(data, existed) = store->Get(ctx, key);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to get checkpoint from store: ") + e.what());
    }
    
    if (!existed) {
        return {nullptr, nullptr, false};
    }
    
    // Deserialize data into RunContext and InterruptInfo
    auto [run_ctx_void, interrupt_info, error] = DeserializeCheckPoint(data);
    if (!error.empty()) {
        throw std::runtime_error(error);
    }
    
    // Convert to RunContext (assuming RunContext is just void* for now)
    auto run_ctx = std::make_shared<RunContext>();
    
    // Create ResumeInfo from InterruptInfo
    auto resume_info = std::make_shared<ResumeInfo>();
    resume_info->enable_streaming = false;
    if (interrupt_info) {
        resume_info->interrupt_info = interrupt_info;
    }
    
    return {run_ctx, resume_info, true};
}

// Save checkpoint to store
void SaveCheckPoint(
    void* ctx,
    compose::CheckPointStore* store,
    const std::string& key,
    std::shared_ptr<RunContext> run_ctx,
    std::shared_ptr<InterruptInfo> info) {
    
    if (!store) {
        throw std::runtime_error("CheckPoint store is null");
    }
    
    // Serialize run_ctx and info into bytes
    std::vector<uint8_t> data = SerializeCheckPoint(
        std::static_pointer_cast<void>(run_ctx), 
        info
    );
    
    try {
        store->Set(ctx, key, data);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save checkpoint: ") + e.what());
    }
}

// ============================================================================
// WorkflowInterruptInfo Implementation
// ============================================================================

WorkflowInterruptInfo::WorkflowInterruptInfo()
    : orig_input(nullptr),
      sequential_interrupt_index(-1),
      sequential_interrupt_info(nullptr),
      loop_iterations(0) {
}

WorkflowInterruptInfo::~WorkflowInterruptInfo() {
}

// ============================================================================
// Mock CheckPoint Store (for testing, aligns with eino mockStore)
// ============================================================================

class MockCheckPointStore : public compose::CheckPointStore {
public:
    MockCheckPointStore() : valid_(false) {}
    
    MockCheckPointStore(const std::vector<uint8_t>& data) 
        : data_(data), valid_(true) {}
    
    std::pair<std::vector<uint8_t>, bool> Get(void* ctx, const std::string& id) override {
        if (valid_) {
            return {data_, true};
        }
        return {{}, false};
    }
    
    void Set(void* ctx, const std::string& id, const std::vector<uint8_t>& data) override {
        data_ = data;
        valid_ = true;
    }

private:
    std::vector<uint8_t> data_;
    bool valid_;
};

// Factory functions
std::unique_ptr<compose::CheckPointStore> NewEmptyStore() {
    return std::make_unique<MockCheckPointStore>();
}

std::unique_ptr<compose::CheckPointStore> NewResumeStore(const std::vector<uint8_t>& data) {
    return std::make_unique<MockCheckPointStore>(data);
}

}  // namespace adk
}  // namespace eino
