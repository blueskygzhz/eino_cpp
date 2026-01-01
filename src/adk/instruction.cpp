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

// Instruction Generation for Agent Transfer
// ==========================================
// Aligns with eino/adk/instruction.go
//
// Generates instructions for agents on how to transfer execution
// to other agents in a multi-agent system.

#include "eino/adk/interface.h"
#include "eino/adk/agent.h"
#include <sstream>
#include <vector>

namespace eino {
namespace adk {

// Transfer instruction template (from eino/adk/instruction.go)
const char* kTransferToAgentInstruction = R"(Available other agents: %s

Decision rule:
- If you're best suited for the question according to your description: ANSWER
- If another agent is better according its description: CALL '%s' function with their agent name

When transferring: OUTPUT ONLY THE FUNCTION CALL)";

// ============================================================================
// GenTransferToAgentInstruction
// Aligns with: genTransferToAgentInstruction in eino/adk/instruction.go
// ============================================================================

std::string GenTransferToAgentInstruction(
    void* ctx,
    const std::vector<std::shared_ptr<Agent>>& agents) {
    
    // Build agent list string
    std::ostringstream agent_list;
    for (const auto& agent : agents) {
        agent_list << "\n- Agent name: " << agent->Name(ctx)
                   << "\n  Agent description: " << agent->Description(ctx);
    }
    
    // Format instruction with agent list and transfer tool name
    char buffer[4096];
    snprintf(buffer, sizeof(buffer), kTransferToAgentInstruction,
             agent_list.str().c_str(),
             GetTransferToAgentToolName().c_str());
    
    return std::string(buffer);
}

}  // namespace adk
}  // namespace eino
