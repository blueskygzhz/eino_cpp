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

#ifndef EINO_CPP_ADK_INTERRUPT_H_
#define EINO_CPP_ADK_INTERRUPT_H_

#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include <memory>

namespace eino {
namespace adk {

using json = nlohmann::json;

// InterruptType enumerates the types of interrupts that can occur during agent execution
enum class InterruptType {
    kHumanApproval,   // Waiting for human approval
    kHumanInput,      // Waiting for human input
    kCustomInterrupt, // Custom interrupt type
};

// InterruptInfo contains information about an agent interrupt
struct InterruptInfo {
    // The type of interrupt
    InterruptType interrupt_type;
    
    // Human-readable reason for the interrupt
    std::string reason;
    
    // The state that should be resumed from
    std::string state_key;
    
    // Additional data context
    json context;
    
    // Custom fields
    std::map<std::string, json> extra;
    
    // Constructor
    InterruptInfo() : interrupt_type(InterruptType::kCustomInterrupt) {}
    
    // Create human approval interrupt
    static std::shared_ptr<InterruptInfo> NewHumanApproval(
        const std::string& reason,
        const std::string& state_key = "") {
        auto info = std::make_shared<InterruptInfo>();
        info->interrupt_type = InterruptType::kHumanApproval;
        info->reason = reason;
        info->state_key = state_key;
        return info;
    }
    
    // Create human input interrupt
    static std::shared_ptr<InterruptInfo> NewHumanInput(
        const std::string& reason,
        const std::string& state_key = "") {
        auto info = std::make_shared<InterruptInfo>();
        info->interrupt_type = InterruptType::kHumanInput;
        info->reason = reason;
        info->state_key = state_key;
        return info;
    }
    
    // Create custom interrupt
    static std::shared_ptr<InterruptInfo> NewCustom(
        const std::string& reason,
        const std::string& state_key = "") {
        auto info = std::make_shared<InterruptInfo>();
        info->interrupt_type = InterruptType::kCustomInterrupt;
        info->reason = reason;
        info->state_key = state_key;
        return info;
    }
};

// ResumeInfo contains information for resuming an interrupted agent execution
struct ResumeInfo {
    // The state key to resume from
    std::string state_key;
    
    // User input/approval for the interrupt
    json user_input;
    
    // Metadata about the resume
    std::map<std::string, json> extra;
    
    // Constructor
    ResumeInfo() {}
    
    // Convenient constructor
    ResumeInfo(const std::string& key, const json& input)
        : state_key(key), user_input(input) {}
};

// BreakLoopAction signals to break out of the agent loop
struct BreakLoopAction {
    // Optional reason for breaking
    std::string reason;
    
    // Return value to pass up
    json return_value;
};

// ============================================================================
// Serialization Support (align with eino marshaller/unmarshaller)
// ============================================================================

// Serialize InterruptInfo to JSON
json SerializeInterruptInfo(const std::shared_ptr<InterruptInfo>& info);

// Deserialize InterruptInfo from JSON
std::shared_ptr<InterruptInfo> DeserializeInterruptInfo(const json& j);

// Serialize CheckPoint data (RunContext + InterruptInfo)
std::vector<uint8_t> SerializeCheckPoint(
    const std::shared_ptr<void>& run_ctx,
    const std::shared_ptr<InterruptInfo>& info);

// Deserialize CheckPoint data
std::tuple<std::shared_ptr<void>, std::shared_ptr<InterruptInfo>, std::string>
DeserializeCheckPoint(const std::vector<uint8_t>& data);

} // namespace adk
} // namespace eino

#endif // EINO_CPP_ADK_INTERRUPT_H_
