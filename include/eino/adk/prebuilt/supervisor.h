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

#ifndef EINO_CPP_ADK_PREBUILT_SUPERVISOR_H_
#define EINO_CPP_ADK_PREBUILT_SUPERVISOR_H_

#include <memory>
#include <vector>
#include <string>

#include "eino/adk/agent.h"

namespace eino {
namespace adk {
namespace supervisor {

// Config provides configuration for creating a supervisor-based multi-agent system.
//
// 对齐 eino/adk/prebuilt/supervisor/supervisor.go:22-30
//
// In the supervisor pattern, a designated supervisor agent coordinates multiple sub-agents.
// The supervisor can delegate tasks to sub-agents and receive their responses, while
// sub-agents can only communicate with the supervisor (not with each other directly).
// This hierarchical structure enables complex problem-solving through coordinated agent interactions.
struct Config {
    // Supervisor specifies the agent that will act as the supervisor,
    // coordinating and managing the sub-agents.
    std::shared_ptr<Agent> supervisor;
    
    // SubAgents specifies the list of agents that will be supervised and
    // coordinated by the supervisor agent.
    std::vector<std::shared_ptr<Agent>> sub_agents;
};

// New creates a supervisor-based multi-agent system with the given configuration.
//
// 对齐 eino/adk/prebuilt/supervisor/supervisor.go:32-47
//
// The function sets up a hierarchical structure where:
// 1. Each sub-agent is wrapped with DeterministicTransferTo to ensure they can
//    only transfer control back to the supervisor (not to other sub-agents)
// 2. The supervisor is configured with all wrapped sub-agents via SetSubAgents
//
// This pattern is useful for scenarios like:
// - Multi-expert systems where a coordinator delegates to specialists
// - Workflow orchestration with a central controller
// - Hierarchical task decomposition and delegation
//
// @param ctx: Context
// @param config: Supervisor configuration
// @return: Pair of (configured supervisor agent, error message)
std::pair<std::shared_ptr<Agent>, std::string> New(
    void* ctx,
    const Config& config);

} // namespace supervisor
} // namespace adk
} // namespace eino

#endif // EINO_CPP_ADK_PREBUILT_SUPERVISOR_H_
