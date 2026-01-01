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

#include "../include/eino/adk/call_options.h"

namespace eino {
namespace adk {

// SessionValuesOption implementation
class SessionValuesOption : public AgentRunOption {
public:
    explicit SessionValuesOption(const std::map<std::string, void*>& values)
        : values_(values) {}

    void* GetImplSpecificOption() override {
        return &values_;
    }

private:
    std::map<std::string, void*> values_;
};

// SkipTransferMessagesOption implementation
class SkipTransferMessagesOption : public AgentRunOption {
public:
    void* GetImplSpecificOption() override {
        return nullptr;
    }
};

// CheckPointIDOption implementation
class CheckPointIDOption : public AgentRunOption {
public:
    explicit CheckPointIDOption(const std::string& id)
        : checkpoint_id_(id) {}

    void* GetImplSpecificOption() override {
        return (void*)checkpoint_id_.c_str();
    }

private:
    std::string checkpoint_id_;
};

// Factory functions
AgentRunOption* WithSessionValues(const std::map<std::string, void*>& values) {
    return new SessionValuesOption(values);
}

AgentRunOption* WithSkipTransferMessages() {
    return new SkipTransferMessagesOption();
}

AgentRunOption* WithCheckPointID(const std::string& id) {
    return new CheckPointIDOption(id);
}

// GetCommonOptions implementation
AgentRunCommonOptions GetCommonOptions(
    const AgentRunCommonOptions* base,
    const std::vector<AgentRunOption*>& opts) {
    
    AgentRunCommonOptions result;
    if (base) {
        result = *base;
    }

    for (auto opt : opts) {
        if (!opt) continue;
        // Merge options based on type
        // This is a simplified version
    }

    return result;
}

// AgentToolOptions factories
AgentToolOption WithFullChatHistoryAsInput() {
    return [](AgentToolOptions* opts) {
        opts->full_chat_history_as_input = true;
        return opts;
    };
}

AgentToolOption WithAgentInputSchema(void* schema) {
    return [schema](AgentToolOptions* opts) {
        opts->agent_input_schema = schema;
        return opts;
    };
}

}  // namespace adk
}  // namespace eino
