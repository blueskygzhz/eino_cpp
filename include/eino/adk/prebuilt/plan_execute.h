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

#ifndef EINO_CPP_ADK_PREBUILT_PLAN_EXECUTE_H_
#define EINO_CPP_ADK_PREBUILT_PLAN_EXECUTE_H_

#include "../agent.h"
#include "../chat_model_agent.h"
#include "../executor.h"
#include "../../schema/types.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <map>

namespace eino {
namespace adk {
namespace prebuilt {

// Plan represents an execution plan with a sequence of actionable steps
class Plan {
public:
    virtual ~Plan() = default;
    
    // FirstStep returns the first step to be executed in the plan
    virtual std::string FirstStep(void* ctx) = 0;
    
    // ToJSON serializes the Plan into JSON
    virtual std::string ToJSON(void* ctx) = 0;
    
    // FromJSON deserializes JSON content into the Plan
    virtual bool FromJSON(void* ctx, const std::string& json) = 0;
};

// DefaultPlan is the default implementation of the Plan interface
// JSON Schema:
// {
//   "type": "object",
//   "properties": {
//     "steps": {
//       "type": "array",
//       "items": {"type": "string"},
//       "description": "Ordered list of actions to be taken"
//     }
//   },
//   "required": ["steps"]
// }
class DefaultPlan : public Plan {
public:
    std::string FirstStep(void* ctx) override;
    std::string ToJSON(void* ctx) override;
    bool FromJSON(void* ctx, const std::string& json) override;
    
    void AddStep(const std::string& step);
    void ClearSteps();
    const std::vector<std::string>& GetSteps() const { return steps_; }
    
private:
    std::vector<std::string> steps_;
};

// NewPlan is a function type that creates a new Plan instance
typedef std::function<std::shared_ptr<Plan>(void* ctx)> NewPlanFunc;

// Response represents the final response to the user
struct Response {
    std::string response;  // The complete response to provide to the user
};

// ExecutedStep represents a completed step and its result
struct ExecutedStep {
    std::string step;
    std::string result;
};

// ExecutionContext is the input information for the executor and replanner
struct ExecutionContext {
    std::vector<Message> user_input;
    std::shared_ptr<Plan> plan;
    std::vector<ExecutedStep> executed_steps;
};

// GenPlannerModelInputFn generates input messages for the planner
typedef std::function<std::vector<Message>(void* ctx, const std::vector<Message>& user_input)> GenPlannerModelInputFn;

// GenModelInputFn generates input messages for executor and replanner
typedef std::function<std::vector<Message>(void* ctx, const ExecutionContext& exec_ctx)> GenModelInputFn;

// PlannerConfig provides configuration for creating a planner agent
struct PlannerConfig {
    // ChatModelWithFormattedOutput is a model pre-configured to output in Plan format
    void* chat_model_with_formatted_output = nullptr;
    
    // ToolCallingChatModel is a model that supports tool calling capabilities
    void* tool_calling_chat_model = nullptr;
    
    // ToolInfo defines the schema for the Plan structure when using tool calling
    std::shared_ptr<schema::ToolInfo> tool_info = nullptr;
    
    // GenInputFn generates the input messages for the planner
    GenPlannerModelInputFn gen_input_fn = nullptr;
    
    // NewPlan creates new Plan instances for JSON deserialization
    NewPlanFunc new_plan = nullptr;
};

// Planner agent generates execution plans
class Planner : public Agent {
public:
    explicit Planner(const PlannerConfig& config);
    ~Planner() override;
    
    std::string Name(void* ctx) override;
    std::string Description(void* ctx) override;
    
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;
        
private:
    PlannerConfig config_;
    bool use_tool_call_;
};

// NewPlanner creates a new Planner agent
std::shared_ptr<Planner> NewPlanner(void* ctx, const PlannerConfig& config);

// ExecutorConfig provides configuration for creating an executor agent
struct ExecutorConfig {
    // Model is the chat model used by the executor
    void* model = nullptr;
    
    // ToolsConfig specifies the tools available to the executor
    ToolsConfig tools_config;
    
    // MaxIterations defines the upper limit of ChatModel generation cycles
    int max_iterations = 20;
    
    // GenInputFn generates the input messages for the executor
    GenModelInputFn gen_input_fn = nullptr;
};

// NewExecutor creates a new executor agent for plan execution
std::shared_ptr<Agent> NewExecutor(void* ctx, const ExecutorConfig& config);

// ReplannerConfig provides configuration for creating a replanner agent
struct ReplannerConfig {
    // ChatModel is the model that supports tool calling capabilities
    void* chat_model = nullptr;
    
    // PlanTool defines the schema for the Plan tool
    std::shared_ptr<schema::ToolInfo> plan_tool = nullptr;
    
    // RespondTool defines the schema for the response tool
    std::shared_ptr<schema::ToolInfo> respond_tool = nullptr;
    
    // GenInputFn generates the input messages for the replanner
    GenModelInputFn gen_input_fn = nullptr;
    
    // NewPlan creates new Plan instances
    NewPlanFunc new_plan = nullptr;
};

// Replanner agent evaluates progress and replans if necessary
class Replanner : public Agent {
public:
    explicit Replanner(const ReplannerConfig& config);
    ~Replanner() override;
    
    std::string Name(void* ctx) override;
    std::string Description(void* ctx) override;
    
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;
        
private:
    ReplannerConfig config_;
};

// NewReplanner creates a new Replanner agent
std::shared_ptr<Replanner> NewReplanner(void* ctx, const ReplannerConfig& config);

// Config for creating a complete plan-execute-replan workflow
struct PlanExecuteReplanConfig {
    // Planner generates the initial plan
    std::shared_ptr<Agent> planner;
    
    // Executor executes plan steps
    std::shared_ptr<Agent> executor;
    
    // Replanner evaluates and replans
    std::shared_ptr<Agent> replanner;
    
    // MaxIterations defines the maximum execute-replan loops
    int max_iterations = 10;
};

// NewPlanExecuteReplan creates a complete plan-execute-replan agent
std::shared_ptr<Agent> NewPlanExecuteReplan(void* ctx, const PlanExecuteReplanConfig& config);

// ToolInfo definitions for structured output
extern const schema::ToolInfo kPlanToolInfo;
extern const schema::ToolInfo kRespondToolInfo;

// Session keys
constexpr const char* kSessionKeyUserInput = "user_input";
constexpr const char* kSessionKeyPlan = "plan";
constexpr const char* kSessionKeyExecutedStep = "executed_step";
constexpr const char* kSessionKeyExecutedSteps = "executed_steps";

// Prompt templates
extern const std::string kPlannerPrompt;
extern const std::string kExecutorPrompt;
extern const std::string kReplannerPrompt;

}  // namespace prebuilt
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_PREBUILT_PLAN_EXECUTE_H_
