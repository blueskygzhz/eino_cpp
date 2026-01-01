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
 *
 * ============================================================================
 * EINO C++ - Plan-and-Execute Agent Example
 * ============================================================================
 * 
 * 这个例子展示了如何使用 eino_cpp 创建一个完整的 Plan-Execute-Replan Agent
 * 
 * 功能演示:
 * 1. Planner - 生成执行计划
 * 2. Executor - 执行计划步骤
 * 3. Replanner - 评估进度并重新规划
 * 4. 完整的 Plan-Execute-Replan 循环
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "eino/adk/prebuilt/plan_execute.h"
#include "eino/adk/agent.h"
#include "eino/adk/types.h"
#include "eino/adk/prompts.h"
#include "eino/schema/message.h"
#include "eino/schema/tool.h"

using namespace eino::adk;
using namespace eino::adk::prebuilt;
using namespace eino::schema;

// ============================================================================
// Mock ChatModel Implementation
// ============================================================================

class MockChatModel {
public:
    std::vector<Message> Generate(void* ctx, const std::vector<Message>& messages) {
        // 简化的 mock 实现，实际应调用真实的 LLM
        std::cout << "[MockChatModel] Generating response for " << messages.size() << " messages\n";
        
        Message response;
        response.role = "assistant";
        response.content = "This is a mock response. In production, this would be a real LLM response.";
        
        return {response};
    }
    
    std::vector<ToolCall> GenerateWithTools(
        void* ctx, 
        const std::vector<Message>& messages,
        const std::vector<ToolInfo>& tools) {
        
        std::cout << "[MockChatModel] Generating with " << tools.size() << " tools\n";
        
        // Mock tool call for Plan
        if (!tools.empty() && tools[0].name == "Plan") {
            ToolCall call;
            call.id = "call_1";
            call.type = "function";
            call.function.name = "Plan";
            call.function.arguments = R"({"steps": ["Step 1: Analyze requirements", "Step 2: Design solution", "Step 3: Implement"]})";
            return {call};
        }
        
        return {};
    }
};

// ============================================================================
// Mock Tools Implementation
// ============================================================================

class SearchTool {
public:
    std::string Execute(void* ctx, const std::string& query) {
        std::cout << "[SearchTool] Searching for: " << query << "\n";
        return "Mock search results for: " + query;
    }
    
    static ToolInfo GetToolInfo() {
        ToolInfo info;
        info.name = "search";
        info.desc = "Search for information";
        
        std::map<std::string, std::shared_ptr<ParameterInfo>> params;
        auto query_param = std::make_shared<ParameterInfo>();
        query_param->type = ParameterType::String;
        query_param->desc = "The search query";
        query_param->required = true;
        params["query"] = query_param;
        
        info.params_one_of = NewParamsOneOfByParams(params);
        return info;
    }
};

class CalculatorTool {
public:
    std::string Execute(void* ctx, const std::string& expression) {
        std::cout << "[CalculatorTool] Calculating: " << expression << "\n";
        return "Result: 42 (mock)";
    }
    
    static ToolInfo GetToolInfo() {
        ToolInfo info;
        info.name = "calculator";
        info.desc = "Perform calculations";
        
        std::map<std::string, std::shared_ptr<ParameterInfo>> params;
        auto expr_param = std::make_shared<ParameterInfo>();
        expr_param->type = ParameterType::String;
        expr_param->desc = "Mathematical expression to evaluate";
        expr_param->required = true;
        params["expression"] = expr_param;
        
        info.params_one_of = NewParamsOneOfByParams(params);
        return info;
    }
};

// ============================================================================
// Helper Functions
// ============================================================================

void PrintSeparator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

void PrintPlan(const std::shared_ptr<Plan>& plan) {
    if (!plan) {
        std::cout << "No plan available\n";
        return;
    }
    
    std::cout << "Generated Plan:\n";
    std::cout << plan->ToJSON(nullptr) << "\n";
    std::cout << "First Step: " << plan->FirstStep(nullptr) << "\n";
}

void PrintExecutionContext(const ExecutionContext& ctx) {
    std::cout << "Execution Context:\n";
    std::cout << "  User Input: " << ctx.user_input.size() << " messages\n";
    std::cout << "  Executed Steps: " << ctx.executed_steps.size() << "\n";
    
    for (size_t i = 0; i < ctx.executed_steps.size(); ++i) {
        std::cout << "    " << (i+1) << ". " << ctx.executed_steps[i].step << "\n";
        std::cout << "       Result: " << ctx.executed_steps[i].result << "\n";
    }
}

// ============================================================================
// Example 1: Basic Planner Usage
// ============================================================================

void Example1_BasicPlanner() {
    PrintSeparator("Example 1: Basic Planner");
    
    // Create mock chat model
    auto chat_model = std::make_shared<MockChatModel>();
    
    // Configure Planner
    PlannerConfig config;
    config.tool_calling_chat_model = chat_model.get();
    
    // Set up tool info
    config.tool_info = std::make_shared<ToolInfo>();
    config.tool_info->name = "Plan";
    config.tool_info->desc = "Generate execution plan";
    
    // Create NewPlan function
    config.new_plan = [](void* ctx) -> std::shared_ptr<Plan> {
        return std::make_shared<DefaultPlan>();
    };
    
    // Set up input generator
    config.gen_input_fn = [](void* ctx, const std::vector<Message>& user_input) {
        std::cout << "[Planner] Generating input for " << user_input.size() << " messages\n";
        
        // Add system prompt
        std::vector<Message> messages;
        Message system_msg;
        system_msg.role = "system";
        system_msg.content = prompts::kPlannerPrompt;
        messages.push_back(system_msg);
        
        // Add user input
        messages.insert(messages.end(), user_input.begin(), user_input.end());
        
        return messages;
    };
    
    // Create Planner
    auto planner = NewPlanner(nullptr, config);
    
    std::cout << "Planner created successfully!\n";
    std::cout << "Name: " << planner->Name(nullptr) << "\n";
    std::cout << "Description: " << planner->Description(nullptr) << "\n";
    
    // Create test input
    auto input = std::make_shared<AgentInput>();
    Message user_msg;
    user_msg.role = "user";
    user_msg.content = "Help me plan a birthday party for 20 people";
    input->messages.push_back(user_msg);
    
    std::cout << "\nUser Query: " << user_msg.content << "\n";
    
    // Note: In a real implementation, we would run the planner here
    // auto result_iter = planner->Run(nullptr, input, {});
    std::cout << "\n[Note] In production, the planner would generate a detailed plan\n";
}

// ============================================================================
// Example 2: Executor with Tools
// ============================================================================

void Example2_ExecutorWithTools() {
    PrintSeparator("Example 2: Executor with Tools");
    
    // Create mock tools
    auto search_tool = std::make_shared<SearchTool>();
    auto calc_tool = std::make_shared<CalculatorTool>();
    
    // Configure Executor
    ExecutorConfig config;
    config.model = new MockChatModel();
    config.max_iterations = 20;
    
    // Set up tools
    config.tools_config.tools.push_back(SearchTool::GetToolInfo());
    config.tools_config.tools.push_back(CalculatorTool::GetToolInfo());
    
    // Set up input generator
    config.gen_input_fn = [](void* ctx, const ExecutionContext& exec_ctx) {
        std::cout << "[Executor] Generating input for execution\n";
        
        std::vector<Message> messages;
        
        // Add system message
        Message system_msg;
        system_msg.role = "system";
        system_msg.content = prompts::kExecutorPrompt;
        messages.push_back(system_msg);
        
        // Add execution context
        Message context_msg;
        context_msg.role = "user";
        context_msg.content = "Executing step: " + exec_ctx.plan->FirstStep(ctx);
        messages.push_back(context_msg);
        
        return messages;
    };
    
    // Create Executor
    auto executor = NewExecutor(nullptr, config);
    
    std::cout << "Executor created successfully!\n";
    std::cout << "Available tools: " << config.tools_config.tools.size() << "\n";
    std::cout << "Max iterations: " << config.max_iterations << "\n";
    
    // Simulate execution context
    ExecutionContext exec_ctx;
    Message user_msg;
    user_msg.role = "user";
    user_msg.content = "Find information about AI agents";
    exec_ctx.user_input.push_back(user_msg);
    
    auto plan = std::make_shared<DefaultPlan>();
    plan->AddStep("Search for AI agent information");
    plan->AddStep("Summarize findings");
    exec_ctx.plan = plan;
    
    PrintPlan(plan);
    
    std::cout << "\n[Note] In production, executor would use tools to complete the step\n";
}

// ============================================================================
// Example 3: Replanner
// ============================================================================

void Example3_Replanner() {
    PrintSeparator("Example 3: Replanner");
    
    // Configure Replanner
    ReplannerConfig config;
    config.chat_model = new MockChatModel();
    
    // Set up tool info for Plan
    config.plan_tool = std::make_shared<ToolInfo>();
    config.plan_tool->name = "Plan";
    config.plan_tool->desc = "Replan remaining steps";
    
    // Set up tool info for Respond
    config.respond_tool = std::make_shared<ToolInfo>();
    config.respond_tool->name = "Respond";
    config.respond_tool->desc = "Provide final response";
    
    // Create NewPlan function
    config.new_plan = [](void* ctx) -> std::shared_ptr<Plan> {
        return std::make_shared<DefaultPlan>();
    };
    
    // Set up input generator
    config.gen_input_fn = [](void* ctx, const ExecutionContext& exec_ctx) {
        std::cout << "[Replanner] Generating input for replanning\n";
        
        std::vector<Message> messages;
        
        // Add system message
        Message system_msg;
        system_msg.role = "system";
        system_msg.content = prompts::kReplannerPrompt;
        messages.push_back(system_msg);
        
        // Add execution context
        Message context_msg;
        context_msg.role = "user";
        context_msg.content = "Completed " + 
            std::to_string(exec_ctx.executed_steps.size()) + " steps";
        messages.push_back(context_msg);
        
        return messages;
    };
    
    // Create Replanner
    auto replanner = NewReplanner(nullptr, config);
    
    std::cout << "Replanner created successfully!\n";
    std::cout << "Name: " << replanner->Name(nullptr) << "\n";
    std::cout << "Description: " << replanner->Description(nullptr) << "\n";
    
    // Simulate execution context
    ExecutionContext exec_ctx;
    Message user_msg;
    user_msg.role = "user";
    user_msg.content = "Research and summarize AI trends";
    exec_ctx.user_input.push_back(user_msg);
    
    auto plan = std::make_shared<DefaultPlan>();
    plan->AddStep("Research AI trends");
    plan->AddStep("Analyze findings");
    plan->AddStep("Create summary");
    exec_ctx.plan = plan;
    
    // Add executed steps
    ExecutedStep step1;
    step1.step = "Research AI trends";
    step1.result = "Found 10 major AI trends in 2024";
    exec_ctx.executed_steps.push_back(step1);
    
    PrintExecutionContext(exec_ctx);
    
    std::cout << "\n[Note] Replanner would decide to continue or provide final response\n";
}

// ============================================================================
// Example 4: Complete Plan-Execute-Replan Workflow
// ============================================================================

void Example4_CompletePlanExecuteReplan() {
    PrintSeparator("Example 4: Complete Plan-Execute-Replan Workflow");
    
    // Step 1: Create Planner
    PlannerConfig planner_config;
    planner_config.tool_calling_chat_model = new MockChatModel();
    planner_config.tool_info = std::make_shared<ToolInfo>();
    planner_config.new_plan = [](void* ctx) { return std::make_shared<DefaultPlan>(); };
    planner_config.gen_input_fn = [](void* ctx, const std::vector<Message>& input) {
        return input;  // Simplified
    };
    
    auto planner = NewPlanner(nullptr, planner_config);
    std::cout << "✓ Planner created\n";
    
    // Step 2: Create Executor
    ExecutorConfig executor_config;
    executor_config.model = new MockChatModel();
    executor_config.max_iterations = 20;
    executor_config.tools_config.tools.push_back(SearchTool::GetToolInfo());
    executor_config.tools_config.tools.push_back(CalculatorTool::GetToolInfo());
    executor_config.gen_input_fn = [](void* ctx, const ExecutionContext& exec_ctx) {
        return std::vector<Message>{};  // Simplified
    };
    
    auto executor = NewExecutor(nullptr, executor_config);
    std::cout << "✓ Executor created (with " << executor_config.tools_config.tools.size() << " tools)\n";
    
    // Step 3: Create Replanner
    ReplannerConfig replanner_config;
    replanner_config.chat_model = new MockChatModel();
    replanner_config.plan_tool = std::make_shared<ToolInfo>();
    replanner_config.respond_tool = std::make_shared<ToolInfo>();
    replanner_config.new_plan = [](void* ctx) { return std::make_shared<DefaultPlan>(); };
    replanner_config.gen_input_fn = [](void* ctx, const ExecutionContext& exec_ctx) {
        return std::vector<Message>{};  // Simplified
    };
    
    auto replanner = NewReplanner(nullptr, replanner_config);
    std::cout << "✓ Replanner created\n";
    
    // Step 4: Create complete Plan-Execute-Replan workflow
    PlanExecuteReplanConfig workflow_config;
    workflow_config.planner = planner;
    workflow_config.executor = executor;
    workflow_config.replanner = replanner;
    workflow_config.max_iterations = 10;
    
    auto workflow = NewPlanExecuteReplan(nullptr, workflow_config);
    
    std::cout << "\n✓ Complete Plan-Execute-Replan workflow created!\n";
    std::cout << "  Max iterations: " << workflow_config.max_iterations << "\n";
    
    // Step 5: Simulate workflow execution
    std::cout << "\n--- Workflow Simulation ---\n\n";
    
    auto input = std::make_shared<AgentInput>();
    Message user_msg;
    user_msg.role = "user";
    user_msg.content = "Plan and execute a market research project for a new product";
    input->messages.push_back(user_msg);
    
    std::cout << "User Query: " << user_msg.content << "\n\n";
    
    std::cout << "Expected Workflow:\n";
    std::cout << "1. Planner generates initial plan\n";
    std::cout << "2. Executor executes first step using available tools\n";
    std::cout << "3. Replanner evaluates progress:\n";
    std::cout << "   - If complete: Generate final response\n";
    std::cout << "   - If not: Replan remaining steps\n";
    std::cout << "4. Repeat steps 2-3 until complete or max iterations\n";
    
    std::cout << "\n[Note] In production, this would execute the full workflow\n";
}

// ============================================================================
// Example 5: DefaultPlan Usage
// ============================================================================

void Example5_DefaultPlanUsage() {
    PrintSeparator("Example 5: DefaultPlan Usage");
    
    // Create a new plan
    auto plan = std::make_shared<DefaultPlan>();
    
    // Add steps
    plan->AddStep("Identify target market segments");
    plan->AddStep("Conduct competitor analysis");
    plan->AddStep("Survey potential customers");
    plan->AddStep("Analyze survey results");
    plan->AddStep("Generate market report");
    
    std::cout << "Plan created with " << plan->GetSteps().size() << " steps\n\n";
    
    // Display plan
    std::cout << "Steps:\n";
    for (size_t i = 0; i < plan->GetSteps().size(); ++i) {
        std::cout << "  " << (i+1) << ". " << plan->GetSteps()[i] << "\n";
    }
    
    std::cout << "\nFirst Step: " << plan->FirstStep(nullptr) << "\n";
    
    // Serialize to JSON
    std::string json = plan->ToJSON(nullptr);
    std::cout << "\nJSON Representation:\n" << json << "\n";
    
    // Deserialize from JSON
    auto plan2 = std::make_shared<DefaultPlan>();
    bool success = plan2->FromJSON(nullptr, json);
    
    std::cout << "\nDeserialization: " << (success ? "SUCCESS" : "FAILED") << "\n";
    std::cout << "Recovered steps: " << plan2->GetSteps().size() << "\n";
}

// ============================================================================
// Main Function
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════════╗
║                                                                   ║
║        EINO C++ - Plan-and-Execute Agent Example                 ║
║                                                                   ║
║  Demonstrates the Plan-Execute-Replan pattern for complex        ║
║  task decomposition and execution.                               ║
║                                                                   ║
╚═══════════════════════════════════════════════════════════════════╝
)" << std::endl;

    try {
        // Run all examples
        Example1_BasicPlanner();
        Example2_ExecutorWithTools();
        Example3_Replanner();
        Example4_CompletePlanExecuteReplan();
        Example5_DefaultPlanUsage();
        
        PrintSeparator("Summary");
        std::cout << "All examples completed successfully!\n\n";
        std::cout << "Key Concepts Demonstrated:\n";
        std::cout << "  ✓ Planner - Breaks down complex objectives into steps\n";
        std::cout << "  ✓ Executor - Executes individual steps using tools\n";
        std::cout << "  ✓ Replanner - Evaluates progress and adapts the plan\n";
        std::cout << "  ✓ DefaultPlan - JSON serialization for plan persistence\n";
        std::cout << "  ✓ Complete Workflow - Orchestrates all components\n\n";
        
        std::cout << "Next Steps:\n";
        std::cout << "  1. Replace MockChatModel with real LLM integration\n";
        std::cout << "  2. Implement actual tool executors\n";
        std::cout << "  3. Add error handling and retries\n";
        std::cout << "  4. Integrate with checkpoint system for persistence\n";
        std::cout << "  5. Add callbacks for monitoring and logging\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
