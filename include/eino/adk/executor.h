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

#ifndef EINO_CPP_ADK_EXECUTOR_H_
#define EINO_CPP_ADK_EXECUTOR_H_

#include "agent.h"
#include "types.h"
#include "../schema/types.h"
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>

namespace eino {
namespace adk {

// ============================================================================
// Tool Interface - Aligned with eino tool.BaseTool
// ============================================================================

// Tool defines the interface for tools that can be used by executor
// Aligned with eino's tool.BaseTool
class Tool {
public:
    virtual ~Tool() = default;
    
    // Info returns the tool information including name, description, and parameters
    // Aligned with eino tool.BaseTool.Info(ctx context.Context)
    virtual std::shared_ptr<schema::ToolInfo> Info(void* ctx) = 0;
    
    // Run executes the tool with given JSON arguments
    // Aligned with eino tool.BaseTool.Run(ctx context.Context, argumentsInJSON string)
    virtual std::string Run(void* ctx, const std::string& arguments_json) = 0;
    
    // StreamableRun executes the tool with streaming support
    // Aligned with eino tool.StreamableTool.StreamableRun
    virtual void StreamableRun(void* ctx, const std::string& arguments_json,
                               std::function<void(const std::string&)> on_chunk) {
        // Default implementation: non-streaming fallback
        std::string result = Run(ctx, arguments_json);
        if (!result.empty()) {
            on_chunk(result);
        }
    }
};

// ============================================================================
// ToolsConfig - Aligned with eino adk.ToolsConfig
// ============================================================================

// ToolsConfig represents the configuration for tools available to the executor
// Aligned with eino's adk.ToolsConfig
struct ToolsConfig {
    // Tools is the list of available tools
    std::vector<std::shared_ptr<Tool>> tools;
    
    // ReturnDirect indicates whether to return tool output directly without further processing
    // Maps tool name to whether it should return directly
    std::map<std::string, bool> return_direct;
};

// ============================================================================
// ExecutionContext - Aligned with eino prebuilt.ExecutionContext
// ============================================================================

// ExecutionContext provides context information during execution
// Aligned with eino's prebuilt.ExecutionContext
struct ExecutionContext {
    // UserInput contains the original user input messages
    std::vector<Message> user_input;
    
    // Plan contains the current execution plan as a string
    // In eino this is Plan interface, but we simplify to string for C++
    std::string plan;
    
    // ExecutedSteps tracks previously executed steps and their results
    // Each pair is (step_description, result)
    std::vector<std::pair<std::string, std::string>> executed_steps;
};

// ============================================================================
// GenExecutorInputFn - Aligned with eino prebuilt.GenModelInputFn
// ============================================================================

// GenExecutorInputFn is a function type that generates input messages for executor
// Aligned with eino's prebuilt.GenModelInputFn
typedef std::function<std::vector<Message>(void* ctx, const ExecutionContext& exec_ctx)> 
    GenExecutorInputFn;

// ============================================================================
// ExecutorConfig - Aligned with eino prebuilt.ExecutorConfig
// ============================================================================

// ExecutorConfig provides configuration for creating an Executor agent
// Aligned with eino's prebuilt.ExecutorConfig
struct ExecutorConfig {
    // Name identifier for the executor
    std::string name = "Executor";
    
    // Description of the executor
    std::string description = "an executor agent";
    
    // Model is the chat model used by the executor (typically ToolCallingChatModel)
    // Aligned with eino: Model model.ToolCallingChatModel
    void* model = nullptr;
    
    // ToolsConfig specifies the tools available to the executor
    // Aligned with eino: ToolsConfig adk.ToolsConfig
    ToolsConfig tools_config;
    
    // MaxIterations defines the upper limit of ChatModel generation cycles
    // The agent will terminate with an error if this limit is exceeded
    // Aligned with eino: MaxIterations int (default 20)
    int max_iterations = 20;
    
    // GenInputFn generates the input messages for the executor
    // Optional. If not provided, default generator will be used
    // Aligned with eino: GenInputFn GenModelInputFn
    GenExecutorInputFn gen_input_fn = nullptr;
};

// ============================================================================
// Executor Agent - Aligned with eino prebuilt.Executor pattern
// ============================================================================

// Executor is an agent that executes individual steps from a plan with tool support
// Aligned with eino's pattern:
// - In eino, NewExecutor returns an Agent created via adk.NewChatModelAgent
// - Here, Executor itself is an Agent that wraps a ChatModelAgent internally
class Executor : public Agent {
public:
    explicit Executor(const ExecutorConfig& config);
    ~Executor() override;
    
    // Name returns the executor's name
    std::string Name(void* ctx) override;
    
    // Description returns the executor's description
    std::string Description(void* ctx) override;
    
    // Run executes the executor with given input
    // Returns an async iterator for streaming results
    // Aligned with eino's Agent.Run()
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;
    
    // GetToolsConfig returns the tools configuration
    const ToolsConfig& GetToolsConfig() const { return config_.tools_config; }
    
    // SetToolsConfig updates the tools configuration
    void SetToolsConfig(const ToolsConfig& tools_config) { 
        config_.tools_config = tools_config; 
    }
    
    // AddTool adds a new tool to the executor
    void AddTool(const std::shared_ptr<Tool>& tool);
    
    // RemoveTool removes a tool by name
    void RemoveTool(const std::string& tool_name);
    
    // GetTool retrieves a tool by name
    std::shared_ptr<Tool> GetTool(const std::string& tool_name);

private:
    ExecutorConfig config_;
};

// ============================================================================
// Factory Function - Aligned with eino prebuilt.NewExecutor
// ============================================================================

// NewExecutor creates a new Executor agent
// Aligned with eino's prebuilt.NewExecutor(ctx context.Context, cfg *ExecutorConfig)
std::shared_ptr<Executor> NewExecutor(void* ctx, const ExecutorConfig& config);

// ============================================================================
// Session Keys - Aligned with eino prebuilt plan_execute session keys
// ============================================================================

// Session keys for storing execution context in session
// These allow multi-phase coordination between Planner, Executor, and Replanner
// Aligned with eino's session key constants

// kExecutorSessionKeyUserInput stores the original user input messages
// Type: std::vector<Message>
constexpr const char* kExecutorSessionKeyUserInput = "executor_user_input";

// kExecutorSessionKeyPlan stores the current execution plan
// Type: std::string (formatted plan)
constexpr const char* kExecutorSessionKeyPlan = "executor_plan";

// kExecutorSessionKeyExecutedSteps stores completed steps and their results
// Type: std::vector<std::pair<std::string, std::string>>
constexpr const char* kExecutorSessionKeyExecutedSteps = "executor_executed_steps";

// kExecutorSessionKeyCurrentStep stores the step currently being executed
// Type: std::string
constexpr const char* kExecutorSessionKeyCurrentStep = "executor_current_step";

// kExecutorSessionKeyResult stores the result of step execution
// Type: std::string
constexpr const char* kExecutorSessionKeyResult = "executor_result";

// ============================================================================
// Helper Functions - Session Management
// ============================================================================

// GetSessionValue retrieves a value from the execution context/session
// Returns nullptr if key not found
// Aligned with eino's adk.GetSessionValue(ctx context.Context, key string)
void* GetSessionValue(void* ctx, const std::string& key);

// AddSessionValue adds a value to the execution context/session
// Aligned with eino's adk.AddSessionValue(ctx context.Context, key string, value any)
void AddSessionValue(void* ctx, const std::string& key, void* value);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_EXECUTOR_H_
