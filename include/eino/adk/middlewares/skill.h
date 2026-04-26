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

#ifndef EINO_CPP_ADK_MIDDLEWARES_SKILL_H_
#define EINO_CPP_ADK_MIDDLEWARES_SKILL_H_

// Skill middleware for loading and executing skills defined in SKILL.md files.
// Aligned with Go: adk/middlewares/skill/skill.go
//
// Skills can run in different modes based on their frontmatter configuration:
//   - Inline mode (default): Skill content is returned directly as tool result
//   - Fork mode (context: fork): Forks a new agent with a clean context
//   - Fork with context mode (context: fork_with_context): Forks a new agent with history

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "eino/adk/handler.h"
#include "eino/adk/interface.h"
#include "eino/components/model.h"
#include "eino/components/tool.h"
#include "eino/schema/types.h"

namespace eino {
namespace adk {
namespace middlewares {
namespace skill {

// ContextMode defines the execution mode of a skill.
// Aligned with Go: skill.ContextMode
enum class ContextMode {
    kInline,            // Default: content returned directly
    kFork,              // Fork a new agent without parent history
    kForkWithContext,    // Fork a new agent with parent history
};

// FrontMatter defines the YAML frontmatter schema parsed from a SKILL.md file.
// Aligned with Go: skill.FrontMatter
struct FrontMatter {
    std::string name;
    std::string description;
    ContextMode context = ContextMode::kInline;
    std::string agent;
    std::string model;
};

// Skill represents a skill loaded from a backend.
// Aligned with Go: skill.Skill
struct Skill {
    FrontMatter front_matter;
    // Content is the markdown body after the frontmatter.
    std::string content;
    // BaseDirectory is the absolute directory path where the SKILL.md file is located.
    std::string base_directory;
};

// Backend loads skills and provides metadata for tool description rendering.
// Aligned with Go: skill.Backend
class SkillBackend {
public:
    virtual ~SkillBackend() = default;
    virtual std::pair<std::vector<FrontMatter>, std::string> List() = 0;
    virtual std::pair<Skill, std::string> Get(const std::string& name) = 0;
};

// AgentHubOptions contains options passed to AgentHub.Get.
// Aligned with Go: skill.AgentHubOptions
struct AgentHubOptions {
    std::shared_ptr<components::ToolCallingChatModel> model;
};

// AgentHub provides agent instances for context mode execution.
// Aligned with Go: skill.AgentHub
class AgentHub {
public:
    virtual ~AgentHub() = default;
    virtual std::pair<std::shared_ptr<Agent>, std::string> Get(
        const std::string& name,
        const AgentHubOptions& opts) = 0;
};

// ModelHub resolves model instances by name.
// Aligned with Go: skill.ModelHub
class ModelHub {
public:
    virtual ~ModelHub() = default;
    virtual std::pair<std::shared_ptr<components::ToolCallingChatModel>, std::string> Get(
        const std::string& name) = 0;
};

// SystemPromptFunc returns a custom system prompt.
// Aligned with Go: skill.SystemPromptFunc
using SystemPromptFunc = std::function<std::string(const std::string& tool_name)>;

// ToolDescriptionFunc returns a custom tool description.
// Aligned with Go: skill.ToolDescriptionFunc
using ToolDescriptionFunc = std::function<std::string(const std::vector<FrontMatter>& skills)>;

// SubAgentInput contains the context for building the sub-agent's initial messages.
// Aligned with Go: skill.SubAgentInput
struct SubAgentInput {
    Skill skill;
    ContextMode mode;
    std::string raw_arguments;
    std::string skill_content;
    std::vector<schema::Message*> history;
    std::string tool_call_id;
};

// SubAgentOutput contains the sub-agent's execution results.
// Aligned with Go: skill.SubAgentOutput
struct SubAgentOutput {
    Skill skill;
    ContextMode mode;
    std::string raw_arguments;
    std::vector<schema::Message*> messages;
    std::vector<std::string> results;
};

// BuildForkMessagesFunc customizes the messages passed to the forked sub-agent.
using BuildForkMessagesFunc = std::function<std::pair<std::vector<schema::Message*>, std::string>(
    const SubAgentInput& input)>;

// FormatForkResultFunc customizes the final text returned from the forked sub-agent.
using FormatForkResultFunc = std::function<std::pair<std::string, std::string>(
    const SubAgentOutput& output)>;

// Config is the configuration for the skill middleware.
// Aligned with Go: skill.Config
struct Config {
    // Backend is the backend for retrieving skills. Required.
    std::shared_ptr<SkillBackend> backend;

    // SkillToolName is the custom name for the skill tool. Default: "skill".
    std::string skill_tool_name = "skill";

    // AgentHub provides agent instances for fork mode execution.
    std::shared_ptr<AgentHub> agent_hub;

    // ModelHub provides model instances for skills with model specification.
    std::shared_ptr<ModelHub> model_hub;

    // CustomSystemPrompt allows customizing the system prompt. Optional.
    SystemPromptFunc custom_system_prompt;

    // CustomToolDescription allows customizing the tool description. Optional.
    ToolDescriptionFunc custom_tool_description;

    // BuildForkMessages customizes messages for forked sub-agent. Optional.
    BuildForkMessagesFunc build_fork_messages;

    // FormatForkResult customizes the final text from forked sub-agent. Optional.
    FormatForkResultFunc format_fork_result;
};

// NewMiddleware creates a new skill middleware handler for ChatModelAgent.
// Aligned with Go: skill.NewMiddleware()
std::pair<std::shared_ptr<ChatModelAgentMiddleware>, std::string> NewMiddleware(
    const Config& config);

}  // namespace skill
}  // namespace middlewares
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_MIDDLEWARES_SKILL_H_
