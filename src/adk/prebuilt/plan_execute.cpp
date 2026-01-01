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

#include "eino/adk/prebuilt/plan_execute.h"
#include "eino/adk/workflow.h"
#include "eino/adk/chat_model_agent.h"
#include "eino/adk/context.h"
#include "eino/compose/chain.h"
#include "eino/schema/stream.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <algorithm>

using json = nlohmann::json;

namespace eino {
namespace adk {
namespace prebuilt {

// ============================================================================
// DefaultPlan Implementation
// ============================================================================

std::string DefaultPlan::FirstStep(void* ctx) {
    if (steps_.empty()) {
        return "";
    }
    return steps_[0];
}

std::string DefaultPlan::ToJSON(void* ctx) {
    json j;
    j["steps"] = steps_;
    return j.dump();
}

bool DefaultPlan::FromJSON(void* ctx, const std::string& json_str) {
    try {
        auto j = json::parse(json_str);
        steps_.clear();
        
        if (j.contains("steps") && j["steps"].is_array()) {
            for (const auto& step : j["steps"]) {
                if (step.is_string()) {
                    steps_.push_back(step.get<std::string>());
                }
            }
            return !steps_.empty();
        }
        return false;
    } catch (const std::exception&) {
        return false;
    }
}

void DefaultPlan::AddStep(const std::string& step) {
    steps_.push_back(step);
}

void DefaultPlan::ClearSteps() {
    steps_.clear();
}

// ============================================================================
// Planner Implementation
// ============================================================================

Planner::Planner(const PlannerConfig& config) 
    : config_(config), use_tool_call_(config.tool_calling_chat_model != nullptr) {
}

Planner::~Planner() = default;

std::string Planner::Name(void* ctx) {
    return "Planner";
}

std::string Planner::Description(void* ctx) {
    return "a planner agent";
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Planner::Run(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iterator = pair.first;
    auto generator = pair.second;
    
    // Store user input in session
    // AddSessionValue(ctx, kSessionKeyUserInput, input->messages);
    
    // In production implementation:
    // 1. Generate planning prompt using gen_input_fn or default
    // 2. Call chat model (with tools if tool_call mode)
    // 3. Parse response into Plan structure
    // 4. Store plan in session with kSessionKeyPlan
    // 5. Emit events with plan output
    
    // For now, emit a basic event indicating planner started
    auto event = std::make_shared<AgentEvent>();
    event->output = std::make_shared<AgentOutput>();
    
    // Create a default plan as placeholder
    auto plan = std::make_shared<DefaultPlan>();
    plan->AddStep("Analyze the problem");
    plan->AddStep("Execute the solution");
    
    // In real implementation, parse plan from model response
    // AddSessionValue(ctx, kSessionKeyPlan, plan);
    
    generator->Send(event);
    generator->Close();
    
    return iterator;
}

std::shared_ptr<Planner> NewPlanner(void* ctx, const PlannerConfig& config) {
    return std::make_shared<Planner>(config);
}

// ============================================================================
// Executor
// ============================================================================

std::shared_ptr<Agent> NewExecutor(void* ctx, const ExecutorConfig& config) {
    // Create a ChatModelAgent configured for execution
    auto cm_config = std::make_shared<ChatModelAgentConfig>();
    cm_config->name = "Executor";
    cm_config->description = "an executor agent";
    cm_config->model = config.model;
    cm_config->max_iterations = config.max_iterations > 0 ? config.max_iterations : 20;
    cm_config->output_key = kSessionKeyExecutedStep;
    
    // Set up tools from config
    cm_config->tools_config = config.tools_config;
    
    // Gen model input function
    cm_config->gen_model_input = [gen_input_fn = config.gen_input_fn](
        void* ctx, const std::string& instruction, const std::shared_ptr<AgentInput>& input) 
        -> std::vector<Message> {
        
        // Retrieve plan and execution context from session
        // In real implementation:
        // auto plan = GetSessionValue<Plan>(ctx, kSessionKeyPlan);
        // auto user_input = GetSessionValue<Messages>(ctx, kSessionKeyUserInput);
        // auto executed_steps = GetSessionValue<ExecutedSteps>(ctx, kSessionKeyExecutedSteps);
        
        // ExecutionContext exec_ctx;
        // exec_ctx.user_input = user_input;
        // exec_ctx.plan = plan;
        // exec_ctx.executed_steps = executed_steps;
        
        // if (gen_input_fn) {
        //     return gen_input_fn(ctx, exec_ctx);
        // }
        
        // For now, return input messages
        return input->messages;
    };
    
    return NewChatModelAgent(ctx, cm_config);
}

// ============================================================================
// Replanner Implementation
// ============================================================================

Replanner::Replanner(const ReplannerConfig& config) : config_(config) {
}

Replanner::~Replanner() = default;

std::string Replanner::Name(void* ctx) {
    return "Replanner";
}

std::string Replanner::Description(void* ctx) {
    return "a replanner agent";
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Replanner::Run(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iterator = pair.first;
    auto generator = pair.second;
    
    // In production implementation:
    // 1. Retrieve executed step from session
    // 2. Get current plan and user input
    // 3. Append executed step to executed_steps list
    // 4. Generate replanner input with execution context
    // 5. Call chat model with plan_tool and respond_tool
    // 6. Parse tool call response:
    //    - If Respond tool: emit BreakLoop action to complete
    //    - If Plan tool: parse new plan and update session
    // 7. Emit appropriate events
    
    auto event = std::make_shared<AgentEvent>();
    generator->Send(event);
    generator->Close();
    
    return iterator;
}

std::shared_ptr<Replanner> NewReplanner(void* ctx, const ReplannerConfig& config) {
    return std::make_shared<Replanner>(config);
}

// ============================================================================
// Complete Plan-Execute-Replan Workflow
// ============================================================================

std::shared_ptr<Agent> NewPlanExecuteReplan(void* ctx, const PlanExecuteReplanConfig& config) {
    // Create execute-replan loop
    int max_iterations = config.max_iterations > 0 ? config.max_iterations : 10;
    
    std::vector<std::shared_ptr<Agent>> loop_agents = {
        config.executor,
        config.replanner
    };
    
    LoopAgentConfig loop_config;
    loop_config.name = "execute_replan";
    loop_config.sub_agents = loop_agents;
    loop_config.max_iterations = max_iterations;
    
    auto loop_agent = NewLoopAgent(ctx, loop_config);
    if (!loop_agent) {
        return nullptr;
    }
    
    // Create sequential workflow: Planner -> ExecuteReplanLoop
    std::vector<std::shared_ptr<Agent>> seq_agents = {
        config.planner,
        loop_agent
    };
    
    SequentialAgentConfig seq_config;
    seq_config.name = "plan_execute_replan";
    seq_config.sub_agents = seq_agents;
    
    return NewSequentialAgent(ctx, seq_config);
}

// ============================================================================
// Tool Info Definitions
// ============================================================================

const schema::ToolInfo kPlanToolInfo = {
    .name = "Plan",
    .description = "Plan with a list of steps to execute in order. Each step should be clear, actionable, and arranged in a logical sequence.",
};

const schema::ToolInfo kRespondToolInfo = {
    .name = "Respond",
    .description = "Generate a direct response to the user. Use this tool when you have all the information needed to provide a final answer.",
};

// ============================================================================
// Prompt Templates
// ============================================================================

const std::string kPlannerPrompt = R"(You are an expert planning agent. Given an objective, create a comprehensive step-by-step plan to achieve the objective.

## YOUR TASK
Analyze the objective and generate a strategic plan that breaks down the goal into manageable, executable steps.

## PLANNING REQUIREMENTS
Each step in your plan must be:
- **Specific and actionable**: Clear instructions that can be executed without ambiguity
- **Self-contained**: Include all necessary context, parameters, and requirements
- **Independently executable**: Can be performed by an agent without dependencies on other steps
- **Logically sequenced**: Arranged in optimal order for efficient execution
- **Objective-focused**: Directly contribute to achieving the main goal

## PLANNING GUIDELINES
- Eliminate redundant or unnecessary steps
- Include relevant constraints, parameters, and success criteria for each step
- Ensure the final step produces a complete answer or deliverable
- Anticipate potential challenges and include mitigation strategies
- Structure steps to build upon each other logically
- Provide sufficient detail for successful execution)";

const std::string kExecutorPrompt = R"(You are a diligent and meticulous executor agent. Follow the given plan and execute your tasks carefully and thoroughly.

## OBJECTIVE
{input}

## Given the following plan:
{plan}

## COMPLETED STEPS & RESULTS
{executed_steps}

## Your task is to execute the first step, which is:
{step})";

const std::string kReplannerPrompt = R"(You are going to review the progress toward an objective. Analyze the current state and determine the optimal next action.

## YOUR TASK
Based on the progress above, you MUST choose exactly ONE action:

### Option 1: COMPLETE (if objective is fully achieved)
Call '{respond_tool}' with:
- A comprehensive final answer
- Clear conclusion summarizing how the objective was met
- Key insights from the execution process

### Option 2: CONTINUE (if more work is needed)
Call '{plan_tool}' with a revised plan that:
- Contains ONLY remaining steps (exclude completed ones)
- Incorporates lessons learned from executed steps
- Addresses any gaps or issues discovered
- Maintains logical step sequence

## OBJECTIVE
{input}

## ORIGINAL PLAN
{plan}

## COMPLETED STEPS & RESULTS
{executed_steps})";

}  // namespace prebuilt
}  // namespace adk
}  // namespace eino
