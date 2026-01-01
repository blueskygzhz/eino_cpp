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

#ifndef EINO_CPP_ADK_CHAT_MODEL_AGENT_H_
#define EINO_CPP_ADK_CHAT_MODEL_AGENT_H_

#include "agent.h"
#include "../components/component.h"
#include "../schema/types.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <map>

namespace eino {
namespace adk {

// ToolsConfig configures tools for the agent
struct ToolsConfig {
    std::vector<void*> tools;  // BaseTool pointers
    std::map<std::string, bool> return_directly;  // tool names that trigger return
    std::vector<void*> tool_call_middlewares;  // ToolMiddleware pointers
};

// GenModelInput is a function that transforms agent instructions and input
typedef std::function<std::vector<Message>(void* ctx, const std::string& instruction,
                                          const std::shared_ptr<AgentInput>& input)>
    GenModelInput;

// ChatModelAgentConfig configures a ChatModelAgent
struct ChatModelAgentConfig {
    std::string name;
    std::string description;
    std::string instruction;

    void* model = nullptr;  // ToolCallingChatModel pointer
    ToolsConfig tools_config;

    GenModelInput gen_model_input = nullptr;
    void* exit_tool = nullptr;  // BaseTool pointer

    std::string output_key;
    int32_t max_iterations = 20;

    std::vector<AgentMiddleware> middlewares;
};

// ChatModelAgent is an agent implementation based on chat model with tools
// ARCHITECTURE: Built on Compose framework
// - Internally uses Compose Chain/Graph for: Model call → Tool invocation loop
// - State managed through Compose State for message history and context
// - Tool calling handled via Compose graph edges/branching
class ChatModelAgent : public ResumableAgent, public OnSubAgents {
public:
    ChatModelAgent(void* ctx, const std::shared_ptr<ChatModelAgentConfig>& config);
    ~ChatModelAgent() override;

    std::string Name(void* ctx) override;
    std::string Description(void* ctx) override;

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Resume(
        void* ctx,
        const std::shared_ptr<ResumeInfo>& info,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

    void OnSetSubAgents(void* ctx, const std::vector<std::shared_ptr<Agent>>& agents) override;
    void OnSetAsSubAgent(void* ctx, const std::shared_ptr<Agent>& parent) override;
    void OnDisallowTransferToParent(void* ctx) override;

private:
    std::string name_;
    std::string description_;
    std::string instruction_;

    void* model_ = nullptr;
    ToolsConfig tools_config_;
    GenModelInput gen_model_input_;
    void* exit_tool_ = nullptr;

    std::string output_key_;
    int32_t max_iterations_;

    std::vector<std::shared_ptr<Agent>> sub_agents_;
    std::shared_ptr<Agent> parent_agent_;
    bool disallow_transfer_to_parent_ = false;

    std::vector<std::function<void(void*, ChatModelAgentState*)>> before_chat_models_;
    std::vector<std::function<void(void*, ChatModelAgentState*)>> after_chat_models_;

    std::atomic<bool> frozen_{false};
    std::atomic<bool> run_func_built_{false};
    std::mutex build_mutex_;
    
    // runFunc is the actual execution function built by BuildRunFunc
    // Aligns with Go's runFunc type
    using RunFunc = std::function<void(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>>& generator,
        const std::vector<std::shared_ptr<AgentRunOption>>& options)>;
    
    RunFunc run_func_;
    
    // BuildRunFunc builds the execution function (LazyBuild)
    // Aligns with eino adk ChatModelAgent.buildRunFunc
    void BuildRunFunc(void* ctx);
};

// NewChatModelAgent creates a new ChatModelAgent
std::shared_ptr<ChatModelAgent> NewChatModelAgent(
    void* ctx,
    const std::shared_ptr<ChatModelAgentConfig>& config);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_CHAT_MODEL_AGENT_H_
