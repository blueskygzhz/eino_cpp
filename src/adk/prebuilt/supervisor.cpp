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

#include "eino/adk/prebuilt/supervisor.h"

#include "eino/adk/agent.h"

namespace eino {
namespace adk {
namespace supervisor {

std::pair<std::shared_ptr<Agent>, std::string> New(
    void* ctx,
    const Config& config) {
    
    // 对齐 eino/adk/prebuilt/supervisor/supervisor.go:32-47
    
    if (!config.supervisor) {
        return {nullptr, "supervisor agent cannot be null"};
    }
    
    std::string supervisor_name = config.supervisor->Name(ctx);
    std::vector<std::shared_ptr<Agent>> wrapped_sub_agents;
    
    // Wrap each sub-agent with DeterministicTransferTo to ensure they can
    // only transfer control back to the supervisor
    for (const auto& sub_agent : config.sub_agents) {
        if (!sub_agent) {
            return {nullptr, "sub_agent cannot be null"};
        }
        
        // Configure the sub-agent to only be able to transfer to the supervisor
        DeterministicTransferConfig transfer_config;
        transfer_config.agent = sub_agent;
        transfer_config.to_agent_names = {supervisor_name};
        
        auto wrapped = AgentWithDeterministicTransferTo(ctx, transfer_config);
        if (!wrapped) {
            return {nullptr, "failed to wrap sub-agent with DeterministicTransferTo"};
        }
        
        wrapped_sub_agents.push_back(wrapped);
    }
    
    // Set the wrapped sub-agents to the supervisor
    auto result = SetSubAgents(ctx, config.supervisor, wrapped_sub_agents);
    if (!result) {
        return {nullptr, "failed to set sub-agents to supervisor"};
    }
    
    return {result, ""};
}

} // namespace supervisor
} // namespace adk
} // namespace eino
