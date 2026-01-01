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

#ifndef EINO_CPP_ADK_CALL_OPTIONS_H_
#define EINO_CPP_ADK_CALL_OPTIONS_H_

#include "types.h"
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace eino {
namespace adk {

// AgentRunOption represents call options for agent execution
class AgentRunOption {
public:
    AgentRunOption() = default;
    virtual ~AgentRunOption() = default;

    std::vector<std::string> agent_names;

    AgentRunOption& DesignateAgent(const std::vector<std::string>& names) {
        agent_names = names;
        return *this;
    }

    virtual void* GetImplSpecificOption() = 0;
};

// Common options structure
struct AgentRunCommonOptions {
    std::map<std::string, void*> session_values;
    std::string* checkpoint_id = nullptr;
    bool skip_transfer_messages = false;
};

// WithSessionValues sets session values for agent execution
AgentRunOption* WithSessionValues(const std::map<std::string, void*>& values);

// WithSkipTransferMessages skips transfer messages
AgentRunOption* WithSkipTransferMessages();

// WithCheckPointID sets the checkpoint ID
AgentRunOption* WithCheckPointID(const std::string& id);

// GetCommonOptions extracts common options from option list
AgentRunCommonOptions GetCommonOptions(
    const AgentRunCommonOptions* base,
    const std::vector<AgentRunOption*>& opts);

// AgentToolOptions for tool configuration
struct AgentToolOptions {
    bool full_chat_history_as_input = false;
    void* agent_input_schema = nullptr;  // ParamsOneOf pointer
};

typedef std::function<AgentToolOptions*(AgentToolOptions*)> AgentToolOption;

// WithFullChatHistoryAsInput enables full chat history as tool input
AgentToolOption WithFullChatHistoryAsInput();

// WithAgentInputSchema sets agent input schema
AgentToolOption WithAgentInputSchema(void* schema);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_CALL_OPTIONS_H_
