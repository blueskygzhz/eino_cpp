# 🎯 Plan and Execute 超详细示例

这是一个完整的 Plan-Execute-Replan 模式的深度剖析，展示每一步的代码执行细节。

---

## 📖 目录

1. [整体架构](#整体架构)
2. [完整代码示例](#完整代码示例)
3. [逐步执行详解](#逐步执行详解)
4. [调用时序图](#调用时序图)
5. [数据流分析](#数据流分析)

---

## 🏗️ 整体架构

```
Plan-Execute-Replan Agent
│
├─> Planner (规划器)
│   └─> 输入：用户目标
│   └─> 输出：执行计划 (Plan)
│
├─> Execute-Replan Loop (执行-重规划循环)
│   │
│   ├─> Executor (执行器)
│   │   └─> 输入：计划 + 已执行步骤
│   │   └─> 输出：当前步骤执行结果
│   │   └─> 工具：search, calculator, etc.
│   │
│   └─> Replanner (重规划器)
│       └─> 输入：计划 + 所有已执行步骤
│       └─> 输出：
│           ├─> 继续：更新的计划
│           └─> 完成：最终响应 (BreakLoop)
│
└─> 输出：最终结果
```

---

## 💻 完整代码示例

### 场景：市场调研任务

**用户目标**：
```
"为一款新的AI写作工具进行市场调研，分析竞争对手，并给出定价建议"
```

### Step 1: 初始化工具

```cpp
#include "eino/adk/prebuilt/plan_execute.h"
#include "eino/adk/agent.h"
#include "eino/components/model.h"
#include "eino/schema/message.h"

using namespace eino::adk;
using namespace eino::adk::prebuilt;
using namespace eino::schema;

// ============================================================================
// 工具定义
// ============================================================================

class WebSearchTool : public Tool {
public:
    std::string Invoke(void* ctx, const std::string& args_json) override {
        // 解析参数
        auto j = json::parse(args_json);
        std::string query = j["query"];
        
        std::cout << "🔍 [WebSearchTool] Searching for: " << query << "\n";
        
        // 模拟搜索结果
        return R"({
            "results": [
                "Competitor A: Jasper AI - $99/month, 50k words",
                "Competitor B: Copy.ai - $49/month, unlimited words",
                "Competitor C: Writesonic - $19/month, 50k words"
            ],
            "market_size": "AI writing market valued at $800M in 2024",
            "growth_rate": "35% CAGR expected through 2028"
        })";
    }
    
    ToolInfo Info(void* ctx) override {
        ToolInfo info;
        info.name = "web_search";
        info.desc = "Search the web for information";
        
        // 定义参数
        std::map<std::string, std::shared_ptr<ParameterInfo>> params;
        auto query_param = std::make_shared<ParameterInfo>();
        query_param->type = ParameterType::String;
        query_param->desc = "Search query";
        query_param->required = true;
        params["query"] = query_param;
        
        info.params_one_of = NewParamsOneOfByParams(params);
        return info;
    }
};

class DataAnalysisTool : public Tool {
public:
    std::string Invoke(void* ctx, const std::string& args_json) override {
        auto j = json::parse(args_json);
        std::string data = j["data"];
        std::string analysis_type = j.value("type", "summary");
        
        std::cout << "📊 [DataAnalysisTool] Analyzing data with type: " 
                  << analysis_type << "\n";
        
        return R"({
            "analysis": {
                "average_price": "$55.67",
                "price_range": "$19 - $99",
                "most_common_tier": "$49/month",
                "value_proposition": "Unlimited words provide better value"
            },
            "recommendation": "Price at $39-59 range for competitive positioning"
        })";
    }
    
    ToolInfo Info(void* ctx) override {
        ToolInfo info;
        info.name = "analyze_data";
        info.desc = "Analyze data and generate insights";
        
        std::map<std::string, std::shared_ptr<ParameterInfo>> params;
        
        auto data_param = std::make_shared<ParameterInfo>();
        data_param->type = ParameterType::String;
        data_param->desc = "Data to analyze";
        data_param->required = true;
        params["data"] = data_param;
        
        auto type_param = std::make_shared<ParameterInfo>();
        type_param->type = ParameterType::String;
        type_param->desc = "Type of analysis: summary, comparison, trends";
        type_param->required = false;
        params["type"] = type_param;
        
        info.params_one_of = NewParamsOneOfByParams(params);
        return info;
    }
};

// ============================================================================
// Step 2: 创建 Planner
// ============================================================================

std::shared_ptr<Agent> CreatePlanner(components::ChatModel* chat_model) {
    std::cout << "\n🏗️  [CreatePlanner] 开始创建规划器...\n";
    
    PlannerConfig config;
    
    // 1. 设置大模型
    config.tool_calling_chat_model = chat_model;
    std::cout << "  ✓ Chat model configured\n";
    
    // 2. 配置 Plan 工具信息
    config.tool_info = std::make_shared<ToolInfo>();
    config.tool_info->name = "Plan";
    config.tool_info->desc = "Generate a step-by-step execution plan";
    
    // Plan 工具的参数定义
    std::map<std::string, std::shared_ptr<ParameterInfo>> params;
    auto steps_param = std::make_shared<ParameterInfo>();
    steps_param->type = ParameterType::Array;
    steps_param->desc = "List of execution steps in order";
    steps_param->required = true;
    params["steps"] = steps_param;
    
    config.tool_info->params_one_of = NewParamsOneOfByParams(params);
    std::cout << "  ✓ Plan tool configured\n";
    
    // 3. 设置 Plan 创建函数
    config.new_plan = [](void* ctx) -> std::shared_ptr<Plan> {
        std::cout << "    [new_plan] Creating new DefaultPlan instance\n";
        return std::make_shared<DefaultPlan>();
    };
    
    // 4. 设置输入生成函数
    config.gen_input_fn = [](void* ctx, const std::vector<Message>& user_input) 
        -> std::vector<Message> {
        
        std::cout << "    [gen_input_fn] Generating planner input\n";
        std::cout << "    - User messages: " << user_input.size() << "\n";
        
        std::vector<Message> messages;
        
        // 添加 System Prompt
        Message sys_msg;
        sys_msg.role = RoleType::System;
        sys_msg.content = prompts::kPlannerPrompt;
        messages.push_back(sys_msg);
        
        std::cout << "    - Added system prompt (" 
                  << sys_msg.content.substr(0, 50) << "...)\n";
        
        // 添加用户输入
        messages.insert(messages.end(), user_input.begin(), user_input.end());
        
        std::cout << "    - Total messages to model: " << messages.size() << "\n";
        return messages;
    };
    
    // 5. 创建 Planner 实例
    auto planner = NewPlanner(nullptr, config);
    std::cout << "✅ Planner created successfully!\n";
    std::cout << "   Name: " << planner->Name(nullptr) << "\n";
    std::cout << "   Description: " << planner->Description(nullptr) << "\n\n";
    
    return planner;
}

// ============================================================================
// Step 3: 创建 Executor
// ============================================================================

std::shared_ptr<Agent> CreateExecutor(
    components::ChatModel* chat_model,
    const std::vector<std::shared_ptr<Tool>>& tools) {
    
    std::cout << "🛠️  [CreateExecutor] 开始创建执行器...\n";
    
    ExecutorConfig config;
    
    // 1. 设置大模型
    config.model = chat_model;
    config.max_iterations = 20;
    std::cout << "  ✓ Chat model configured (max_iterations: " 
              << config.max_iterations << ")\n";
    
    // 2. 添加工具
    for (const auto& tool : tools) {
        auto tool_info = tool->Info(nullptr);
        config.tools_config.tools.push_back(tool_info);
        std::cout << "  ✓ Added tool: " << tool_info.name << "\n";
    }
    
    // 3. 设置输入生成函数
    config.gen_input_fn = [](void* ctx, const ExecutionContext& exec_ctx) 
        -> std::vector<Message> {
        
        std::cout << "    [gen_input_fn] Generating executor input\n";
        
        std::vector<Message> messages;
        
        // 3.1 添加 System Prompt
        Message sys_msg;
        sys_msg.role = RoleType::System;
        sys_msg.content = prompts::kExecutorPrompt;
        messages.push_back(sys_msg);
        
        // 3.2 添加用户原始输入
        std::cout << "    - User input messages: " << exec_ctx.user_input.size() << "\n";
        messages.insert(messages.end(), 
                       exec_ctx.user_input.begin(), 
                       exec_ctx.user_input.end());
        
        // 3.3 添加计划信息
        if (exec_ctx.plan) {
            Message plan_msg;
            plan_msg.role = RoleType::System;
            plan_msg.content = "## Current Plan:\n" + exec_ctx.plan->ToJSON(ctx);
            messages.push_back(plan_msg);
            std::cout << "    - Added plan with " 
                      << exec_ctx.plan->GetSteps().size() << " steps\n";
        }
        
        // 3.4 添加已执行步骤的历史
        if (!exec_ctx.executed_steps.empty()) {
            std::stringstream ss;
            ss << "## Previously Executed Steps:\n";
            for (size_t i = 0; i < exec_ctx.executed_steps.size(); ++i) {
                const auto& step = exec_ctx.executed_steps[i];
                ss << (i+1) << ". **" << step.step << "**\n"
                   << "   Result: " << step.result << "\n\n";
            }
            
            Message history_msg;
            history_msg.role = RoleType::System;
            history_msg.content = ss.str();
            messages.push_back(history_msg);
            
            std::cout << "    - Added execution history (" 
                      << exec_ctx.executed_steps.size() << " steps)\n";
        }
        
        // 3.5 添加当前要执行的步骤
        if (exec_ctx.plan) {
            std::string current_step = exec_ctx.plan->FirstStep(ctx);
            Message step_msg;
            step_msg.role = RoleType::User;
            step_msg.content = "Now execute this step:\n**" + current_step + "**";
            messages.push_back(step_msg);
            
            std::cout << "    - Current step to execute: " << current_step << "\n";
        }
        
        std::cout << "    - Total messages: " << messages.size() << "\n";
        return messages;
    };
    
    // 4. 创建 Executor (实际是一个配置好的 ChatModelAgent)
    auto executor = NewExecutor(nullptr, config);
    std::cout << "✅ Executor created successfully!\n";
    std::cout << "   Available tools: " << config.tools_config.tools.size() << "\n\n";
    
    return executor;
}

// ============================================================================
// Step 4: 创建 Replanner
// ============================================================================

std::shared_ptr<Agent> CreateReplanner(components::ChatModel* chat_model) {
    std::cout << "🔄 [CreateReplanner] 开始创建重规划器...\n";
    
    ReplannerConfig config;
    
    // 1. 设置大模型
    config.chat_model = chat_model;
    std::cout << "  ✓ Chat model configured\n";
    
    // 2. 配置 Plan 工具（用于生成新计划）
    config.plan_tool = std::make_shared<ToolInfo>();
    config.plan_tool->name = "Plan";
    config.plan_tool->desc = "Generate updated plan with remaining steps";
    
    std::map<std::string, std::shared_ptr<ParameterInfo>> plan_params;
    auto steps_param = std::make_shared<ParameterInfo>();
    steps_param->type = ParameterType::Array;
    steps_param->desc = "Updated list of remaining steps";
    steps_param->required = true;
    plan_params["steps"] = steps_param;
    
    config.plan_tool->params_one_of = NewParamsOneOfByParams(plan_params);
    std::cout << "  ✓ Plan tool configured\n";
    
    // 3. 配置 Respond 工具（用于提供最终答案）
    config.respond_tool = std::make_shared<ToolInfo>();
    config.respond_tool->name = "Respond";
    config.respond_tool->desc = "Provide final answer to user";
    
    std::map<std::string, std::shared_ptr<ParameterInfo>> respond_params;
    auto answer_param = std::make_shared<ParameterInfo>();
    answer_param->type = ParameterType::String;
    answer_param->desc = "Final comprehensive answer";
    answer_param->required = true;
    respond_params["answer"] = answer_param;
    
    config.respond_tool->params_one_of = NewParamsOneOfByParams(respond_params);
    std::cout << "  ✓ Respond tool configured\n";
    
    // 4. 设置 Plan 创建函数
    config.new_plan = [](void* ctx) -> std::shared_ptr<Plan> {
        std::cout << "    [new_plan] Creating new plan for replanning\n";
        return std::make_shared<DefaultPlan>();
    };
    
    // 5. 设置输入生成函数
    config.gen_input_fn = [](void* ctx, const ExecutionContext& exec_ctx) 
        -> std::vector<Message> {
        
        std::cout << "    [gen_input_fn] Generating replanner input\n";
        
        std::vector<Message> messages;
        
        // 5.1 System Prompt
        Message sys_msg;
        sys_msg.role = RoleType::System;
        sys_msg.content = prompts::kReplannerPrompt;
        messages.push_back(sys_msg);
        
        // 5.2 用户目标
        messages.insert(messages.end(), 
                       exec_ctx.user_input.begin(), 
                       exec_ctx.user_input.end());
        
        // 5.3 原始计划
        if (exec_ctx.plan) {
            Message plan_msg;
            plan_msg.role = RoleType::System;
            plan_msg.content = "## Original Plan:\n" + exec_ctx.plan->ToJSON(ctx);
            messages.push_back(plan_msg);
        }
        
        // 5.4 所有已执行步骤及结果
        if (!exec_ctx.executed_steps.empty()) {
            std::stringstream ss;
            ss << "## Execution Progress (" 
               << exec_ctx.executed_steps.size() << " steps completed):\n\n";
            
            for (size_t i = 0; i < exec_ctx.executed_steps.size(); ++i) {
                const auto& step = exec_ctx.executed_steps[i];
                ss << "### Step " << (i+1) << ": " << step.step << "\n"
                   << "**Result:**\n" << step.result << "\n\n";
            }
            
            Message progress_msg;
            progress_msg.role = RoleType::System;
            progress_msg.content = ss.str();
            messages.push_back(progress_msg);
            
            std::cout << "    - Added " << exec_ctx.executed_steps.size() 
                      << " executed steps\n";
        }
        
        // 5.5 决策提示
        Message decision_msg;
        decision_msg.role = RoleType::User;
        decision_msg.content = R"(
Based on the progress above, choose ONE action:

1. If the objective is FULLY ACHIEVED:
   Call 'Respond' tool with a comprehensive final answer

2. If MORE WORK is needed:
   Call 'Plan' tool with ONLY the remaining steps
)";
        messages.push_back(decision_msg);
        
        std::cout << "    - Total messages: " << messages.size() << "\n";
        return messages;
    };
    
    // 6. 创建 Replanner 实例
    auto replanner = NewReplanner(nullptr, config);
    std::cout << "✅ Replanner created successfully!\n\n";
    
    return replanner;
}

// ============================================================================
// Step 5: 组装完整的 Plan-Execute-Replan 工作流
// ============================================================================

std::shared_ptr<Agent> CreatePlanExecuteReplanWorkflow(
    std::shared_ptr<Agent> planner,
    std::shared_ptr<Agent> executor,
    std::shared_ptr<Agent> replanner) {
    
    std::cout << "🎯 [CreateWorkflow] 组装 Plan-Execute-Replan 工作流...\n";
    
    PlanExecuteReplanConfig config;
    config.planner = planner;
    config.executor = executor;
    config.replanner = replanner;
    config.max_iterations = 10;  // 最多执行10轮
    
    std::cout << "  ✓ Planner: " << planner->Name(nullptr) << "\n";
    std::cout << "  ✓ Executor: " << executor->Name(nullptr) << "\n";
    std::cout << "  ✓ Replanner: " << replanner->Name(nullptr) << "\n";
    std::cout << "  ✓ Max iterations: " << config.max_iterations << "\n";
    
    // 创建工作流
    // 内部结构:
    // SequentialAgent [
    //     Planner,
    //     LoopAgent [
    //         Executor,
    //         Replanner
    //     ]
    // ]
    auto workflow = NewPlanExecuteReplan(nullptr, config);
    
    std::cout << "\n✅ Complete workflow created!\n";
    std::cout << "   Workflow name: " << workflow->Name(nullptr) << "\n\n";
    
    return workflow;
}

// ============================================================================
// Step 6: 执行工作流
// ============================================================================

void ExecuteWorkflow(std::shared_ptr<Agent> workflow) {
    std::cout << "🚀 [ExecuteWorkflow] 开始执行工作流...\n\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << "USER OBJECTIVE\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << "为一款新的AI写作工具进行市场调研，分析竞争对手，并给出定价建议\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    // 6.1 准备输入
    auto input = std::make_shared<AgentInput>();
    
    Message user_msg;
    user_msg.role = RoleType::User;
    user_msg.content = "为一款新的AI写作工具进行市场调研，分析竞争对手，并给出定价建议";
    input->messages.push_back(user_msg);
    input->enable_streaming = false;
    
    std::cout << "📥 Input prepared:\n";
    std::cout << "   - Message count: " << input->messages.size() << "\n";
    std::cout << "   - Streaming: " << (input->enable_streaming ? "Yes" : "No") << "\n\n";
    
    // 6.2 执行工作流
    std::cout << "⚙️  Executing workflow...\n\n";
    
    auto events = workflow->Run(nullptr, input, {});
    
    // 6.3 消费事件
    int event_count = 0;
    std::cout << "📊 Processing events:\n";
    std::cout << std::string(70, '-') << "\n";
    
    for (auto event_iter = events->Begin(); 
         event_iter != events->End(); 
         ++event_iter) {
        
        auto event = *event_iter;
        ++event_count;
        
        std::cout << "\n[Event #" << event_count << "]\n";
        
        if (!event->agent_name.empty()) {
            std::cout << "  Agent: " << event->agent_name << "\n";
        }
        
        if (event->output) {
            if (event->output->message) {
                std::cout << "  Message: " << event->output->message->content << "\n";
            }
            if (event->output->tool_calls && !event->output->tool_calls->empty()) {
                std::cout << "  Tool Calls: " << event->output->tool_calls->size() << "\n";
                for (const auto& tc : *event->output->tool_calls) {
                    std::cout << "    - " << tc.function.name 
                              << "(" << tc.function.arguments << ")\n";
                }
            }
        }
        
        if (!event->error_msg.empty()) {
            std::cout << "  ❌ Error: " << event->error_msg << "\n";
        }
        
        if (event->action) {
            std::cout << "  Action: " << event->action->type << "\n";
            if (event->action->type == AgentActionType::BreakLoop) {
                std::cout << "  🎉 Workflow completed!\n";
            }
        }
    }
    
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "✅ Workflow execution finished!\n";
    std::cout << "   Total events: " << event_count << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

// ============================================================================
// Main Function - 完整示例
// ============================================================================

int main() {
    std::cout << R"(
╔════════════════════════════════════════════════════════════════════╗
║                                                                    ║
║         EINO C++ Plan-Execute-Replan Detailed Example             ║
║                                                                    ║
║  场景：AI写作工具市场调研                                         ║
║                                                                    ║
╚════════════════════════════════════════════════════════════════════╝
)" << "\n\n";

    try {
        // 假设已有 ChatModel 实例
        components::ChatModel* chat_model = nullptr;  // 实际使用时替换为真实模型
        
        // Step 1: 创建工具
        std::cout << "━━━ STEP 1: 创建工具 ━━━\n";
        auto search_tool = std::make_shared<WebSearchTool>();
        auto analysis_tool = std::make_shared<DataAnalysisTool>();
        std::vector<std::shared_ptr<Tool>> tools = {search_tool, analysis_tool};
        std::cout << "✅ Created " << tools.size() << " tools\n\n";
        
        // Step 2: 创建 Planner
        std::cout << "━━━ STEP 2: 创建 Planner ━━━\n";
        auto planner = CreatePlanner(chat_model);
        
        // Step 3: 创建 Executor
        std::cout << "━━━ STEP 3: 创建 Executor ━━━\n";
        auto executor = CreateExecutor(chat_model, tools);
        
        // Step 4: 创建 Replanner
        std::cout << "━━━ STEP 4: 创建 Replanner ━━━\n";
        auto replanner = CreateReplanner(chat_model);
        
        // Step 5: 组装工作流
        std::cout << "━━━ STEP 5: 组装工作流 ━━━\n";
        auto workflow = CreatePlanExecuteReplanWorkflow(planner, executor, replanner);
        
        // Step 6: 执行工作流
        std::cout << "━━━ STEP 6: 执行工作流 ━━━\n";
        ExecuteWorkflow(workflow);
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
```

---

## 🔍 逐步执行详解

### Phase 1: Planner 执行

#### 1.1 输入准备
```cpp
// 用户输入
Message user_msg;
user_msg.role = RoleType::User;
user_msg.content = "为一款新的AI写作工具进行市场调研...";

// Planner::Run() 被调用
auto events = planner->Run(ctx, input, {});
```

#### 1.2 生成 Planning Prompt
```cpp
// config.gen_input_fn 被调用
std::vector<Message> planning_messages = {
    {
        role: System,
        content: "You are an expert planning agent. Given an objective..."
    },
    {
        role: User,
        content: "为一款新的AI写作工具进行市场调研，分析竞争对手，并给出定价建议"
    }
};
```

#### 1.3 调用大模型生成计划
```cpp
// 大模型收到消息，返回 tool_call
ToolCall plan_call = {
    id: "call_123",
    type: "function",
    function: {
        name: "Plan",
        arguments: {
            "steps": [
                "搜索AI写作工具市场的竞争对手信息",
                "分析收集到的竞争对手数据，提取价格、功能等关键信息",
                "基于分析结果，生成定价建议报告"
            ]
        }
    }
};
```

#### 1.4 解析并存储 Plan
```cpp
// 解析 tool_call.arguments
auto plan = config.new_plan(ctx);  // 创建 DefaultPlan
for (const auto& step : parsed_steps) {
    plan->AddStep(step);
}

// 存储到 Session
AddSessionValue(ctx, kSessionKeyPlan, plan);

// 同时存储用户输入
AddSessionValue(ctx, kSessionKeyUserInput, input->messages);

// 初始化已执行步骤列表
std::vector<ExecutedStep> executed_steps;
AddSessionValue(ctx, kSessionKeyExecutedSteps, executed_steps);
```

#### 1.5 发送 Planner Event
```cpp
auto event = std::make_shared<AgentEvent>();
event->agent_name = "Planner";
event->output = std::make_shared<AgentOutput>();
event->output->message = std::make_shared<Message>();
event->output->message->role = RoleType::Assistant;
event->output->message->content = "Generated plan with 3 steps";

generator->Send(event);
generator->Close();
```

---

### Phase 2: Execute-Replan Loop - 第1轮

#### 2.1 Executor 执行第1步

##### 2.1.1 从 Session 读取数据
```cpp
// Executor::Run() 被调用
auto plan = GetSessionValue<Plan>(ctx, kSessionKeyPlan);
// plan->steps = [
//     "搜索AI写作工具市场的竞争对手信息",
//     "分析收集到的竞争对手数据，提取价格、功能等关键信息",
//     "基于分析结果，生成定价建议报告"
// ]

auto user_input = GetSessionValue<Messages>(ctx, kSessionKeyUserInput);
auto executed_steps = GetSessionValue<ExecutedSteps>(ctx, kSessionKeyExecutedSteps);
// executed_steps = []  (首轮为空)
```

##### 2.1.2 构建 ExecutionContext
```cpp
ExecutionContext exec_ctx;
exec_ctx.user_input = user_input;
exec_ctx.plan = plan;
exec_ctx.executed_steps = executed_steps;
```

##### 2.1.3 生成 Executor Input
```cpp
// config.gen_input_fn(ctx, exec_ctx) 被调用
std::vector<Message> executor_messages = {
    {
        role: System,
        content: "You are a diligent executor agent..."
    },
    {
        role: User,
        content: "为一款新的AI写作工具进行市场调研..."  // 用户原始输入
    },
    {
        role: System,
        content: "## Current Plan:\n{\"steps\": [...]}"  // 当前计划
    },
    // 没有已执行步骤（首轮）
    {
        role: User,
        content: "Now execute this step:\n**搜索AI写作工具市场的竞争对手信息**"
    }
};
```

##### 2.1.4 ChatModel 生成工具调用
```cpp
// Executor 内部是 ChatModelAgent + ReAct
// 大模型决定使用 web_search 工具

ToolCall search_call = {
    id: "call_456",
    type: "function",
    function: {
        name: "web_search",
        arguments: {
            "query": "AI writing tools competitors pricing 2024"
        }
    }
};
```

##### 2.1.5 执行工具
```cpp
// ReAct 框架执行工具
auto search_tool = FindTool("web_search", tools);
std::string tool_result = search_tool->Invoke(ctx, search_call.function.arguments);

// tool_result = {
//     "results": [
//         "Competitor A: Jasper AI - $99/month, 50k words",
//         "Competitor B: Copy.ai - $49/month, unlimited words",
//         "Competitor C: Writesonic - $19/month, 50k words"
//     ],
//     ...
// }
```

##### 2.1.6 模型生成最终响应
```cpp
// 工具结果返回给模型，模型生成总结
Message executor_response;
executor_response.role = RoleType::Assistant;
executor_response.content = R"(
I have searched for AI writing tool competitors. Found 3 main competitors:
- Jasper AI: $99/month (50k words)
- Copy.ai: $49/month (unlimited words)  
- Writesonic: $19/month (50k words)

Market size is $800M with 35% CAGR growth expected.
)";
```

##### 2.1.7 存储执行结果到 Session
```cpp
// 保存到 kSessionKeyExecutedStep
ExecutedStep step1;
step1.step = "搜索AI写作工具市场的竞争对手信息";
step1.result = executor_response.content;

AddSessionValue(ctx, kSessionKeyExecutedStep, step1);
```

##### 2.1.8 发送 Executor Event
```cpp
auto event = std::make_shared<AgentEvent>();
event->agent_name = "Executor";
event->output = std::make_shared<AgentOutput>();
event->output->message = executor_response;

generator->Send(event);
```

---

#### 2.2 Replanner 评估进度

##### 2.2.1 从 Session 读取最新状态
```cpp
// Replanner::Run() 被调用
auto plan = GetSessionValue<Plan>(ctx, kSessionKeyPlan);
auto user_input = GetSessionValue<Messages>(ctx, kSessionKeyUserInput);
auto executed_steps = GetSessionValue<ExecutedSteps>(ctx, kSessionKeyExecutedSteps);

// 读取刚刚执行的步骤
auto last_step = GetSessionValue<ExecutedStep>(ctx, kSessionKeyExecutedStep);

// 追加到历史
executed_steps.push_back(last_step);

// 更新 Session
AddSessionValue(ctx, kSessionKeyExecutedSteps, executed_steps);
```

##### 2.2.2 构建 Replanner Input
```cpp
ExecutionContext replan_ctx;
replan_ctx.user_input = user_input;
replan_ctx.plan = plan;
replan_ctx.executed_steps = executed_steps;  // 现在有1个已执行步骤

std::vector<Message> replanner_messages = {
    {
        role: System,
        content: "You are going to review the progress..."
    },
    {
        role: User,
        content: "为一款新的AI写作工具进行市场调研..."
    },
    {
        role: System,
        content: "## Original Plan:\n{\"steps\": [...]}"
    },
    {
        role: System,
        content: "## Execution Progress (1 steps completed):\n\n" +
                 "### Step 1: 搜索AI写作工具市场的竞争对手信息\n" +
                 "**Result:**\nI have searched for AI writing tool competitors..."
    },
    {
        role: User,
        content: "Based on the progress, choose ONE action:\n" +
                 "1. Call 'Respond' if objective is achieved\n" +
                 "2. Call 'Plan' with remaining steps"
    }
};
```

##### 2.2.3 ChatModel 决策
```cpp
// 大模型判断：目标未完成，需要继续执行
// 调用 Plan 工具，更新计划

ToolCall replan_call = {
    id: "call_789",
    type: "function",
    function: {
        name: "Plan",
        arguments: {
            "steps": [
                "分析收集到的竞争对手数据，提取价格、功能等关键信息",
                "基于分析结果，生成定价建议报告"
            ]
        }
    }
};
```

##### 2.2.4 更新 Plan
```cpp
// 解析新计划
auto new_plan = config.new_plan(ctx);
for (const auto& step : parsed_steps) {
    new_plan->AddStep(step);
}

// 更新 Session
AddSessionValue(ctx, kSessionKeyPlan, new_plan);
```

##### 2.2.5 发送 Replanner Event（继续循环）
```cpp
auto event = std::make_shared<AgentEvent>();
event->agent_name = "Replanner";
event->output = std::make_shared<AgentOutput>();
event->output->message = std::make_shared<Message>();
event->output->message->content = "Updated plan with 2 remaining steps";

// 注意：没有 BreakLoop action，所以循环继续

generator->Send(event);
```

---

### Phase 3: Execute-Replan Loop - 第2轮

#### 3.1 Executor 执行第2步

```cpp
// 流程类似第1轮，但现在：
// executed_steps = [step1]
// plan->steps = [
//     "分析收集到的竞争对手数据，提取价格、功能等关键信息",
//     "基于分析结果，生成定价建议报告"
// ]

// 大模型使用 analyze_data 工具
ToolCall analysis_call = {
    function: {
        name: "analyze_data",
        arguments: {
            "data": "Jasper: $99/month, Copy.ai: $49/month, Writesonic: $19/month",
            "type": "comparison"
        }
    }
};

// 工具返回
// {
//     "analysis": {
//         "average_price": "$55.67",
//         "price_range": "$19 - $99",
//         "recommendation": "Price at $39-59 range for competitive positioning"
//     }
// }

// 执行器总结
step2.step = "分析收集到的竞争对手数据，提取价格、功能等关键信息";
step2.result = "Analysis complete: Average price $55.67, range $19-99. " +
               "Recommendation: $39-59 for competitive positioning.";
```

#### 3.2 Replanner 再次评估

```cpp
// 现在 executed_steps = [step1, step2]
// 原始计划还剩1步："基于分析结果，生成定价建议报告"

// Replanner 可能决定：
// 选项1：继续执行（调用 Plan 工具更新剩余步骤）
// 选项2：已有足够信息，直接响应（调用 Respond 工具）
```

---

### Phase 4: 完成工作流

#### 4.1 Replanner 调用 Respond 工具

```cpp
// 第3轮 Replanner 判断已有足够信息，调用 Respond

ToolCall respond_call = {
    function: {
        name: "Respond",
        arguments: {
            "answer": "Based on comprehensive market research:\n\n" +
                     "## Competitor Analysis\n" +
                     "- Jasper AI: $99/month (50k words)\n" +
                     "- Copy.ai: $49/month (unlimited)\n" +
                     "- Writesonic: $19/month (50k words)\n\n" +
                     "## Market Insights\n" +
                     "- Market size: $800M (2024)\n" +
                     "- Growth rate: 35% CAGR\n" +
                     "- Average price: $55.67\n\n" +
                     "## Pricing Recommendation\n" +
                     "**Suggested price: $39-49/month**\n\n" +
                     "Rationale:\n" +
                     "- Competitive with Copy.ai\n" +
                     "- Higher value than Writesonic\n" +
                     "- More accessible than Jasper AI\n" +
                     "- Allows for market penetration while maintaining margins"
        }
    }
};
```

#### 4.2 触发 BreakLoop

```cpp
// Replanner 检测到 Respond 工具调用
// 生成 BreakLoop action

auto event = std::make_shared<AgentEvent>();
event->agent_name = "Replanner";
event->output = std::make_shared<AgentOutput>();
event->output->message = respond_call.function.arguments["answer"];

// 🔑 关键：设置 BreakLoop action
event->action = std::make_shared<AgentAction>();
event->action->type = AgentActionType::BreakLoop;

generator->Send(event);
```

#### 4.3 LoopAgent 检测到 BreakLoop

```cpp
// LoopAgent::Run() 中
for (int i = 0; i < max_iterations; ++i) {
    // 执行 sub_agents (Executor -> Replanner)
    
    // 检查事件
    for (auto event : sub_events) {
        if (event->action && event->action->type == AgentActionType::BreakLoop) {
            // 🎉 检测到终止信号
            std::cout << "Loop terminated by BreakLoop action\n";
            
            // 转发最终事件
            generator->Send(event);
            generator->Close();
            return;  // 退出循环
        }
    }
}
```

#### 4.4 SequentialAgent 完成

```cpp
// SequentialAgent 顺序执行：
// 1. Planner ✅
// 2. LoopAgent (Executor + Replanner) ✅ (BreakLoop退出)

// 所有 sub-agents 完成，发送最终事件
generator->Close();
```

---

## 📊 调用时序图

```
User
  │
  ├─> workflow->Run(input)
  │       │
  │       ├─> SequentialAgent::Run()
  │       │       │
  │       │       ├─> [Phase 1] Planner::Run()
  │       │       │       │
  │       │       │       ├─> gen_input_fn() → planning_messages
  │       │       │       ├─> ChatModel::Generate(planning_messages)
  │       │       │       │       └─> ToolCall: Plan(steps: [...])
  │       │       │       ├─> Parse tool_call → DefaultPlan
  │       │       │       ├─> AddSessionValue(kSessionKeyPlan, plan)
  │       │       │       └─> Send Event: "Plan generated"
  │       │       │
  │       │       └─> [Phase 2] LoopAgent::Run(max_iterations=10)
  │       │               │
  │       │               ├─> [Iteration 1]
  │       │               │   │
  │       │               │   ├─> Executor::Run()
  │       │               │   │   │
  │       │               │   │   ├─> GetSessionValue(kSessionKeyPlan)
  │       │               │   │   ├─> gen_input_fn(exec_ctx) → executor_messages
  │       │               │   │   ├─> ChatModelAgent::Run(executor_messages)
  │       │               │   │   │   │
  │       │               │   │   │   ├─> ReAct Loop:
  │       │               │   │   │   │   ├─> ChatModel::Generate()
  │       │               │   │   │   │   │   └─> ToolCall: web_search(query)
  │       │               │   │   │   │   ├─> Tool::Invoke() → tool_result
  │       │               │   │   │   │   ├─> ChatModel::Generate(with tool_result)
  │       │               │   │   │   │   └─> Final Message
  │       │               │   │   │   │
  │       │               │   │   │   └─> Send Event
  │       │               │   │   │
  │       │               │   │   └─> AddSessionValue(kSessionKeyExecutedStep, step1)
  │       │               │   │
  │       │               │   └─> Replanner::Run()
  │       │               │       │
  │       │               │       ├─> GetSessionValue(executed_steps) + last_step
  │       │               │       ├─> gen_input_fn(replan_ctx) → replanner_messages
  │       │               │       ├─> ChatModel::Generate(replanner_messages)
  │       │               │       │   └─> ToolCall: Plan(remaining_steps)
  │       │               │       ├─> Parse → new_plan
  │       │               │       ├─> AddSessionValue(kSessionKeyPlan, new_plan)
  │       │               │       └─> Send Event (no BreakLoop)
  │       │               │
  │       │               ├─> [Iteration 2]
  │       │               │   └─> (类似 Iteration 1)
  │       │               │
  │       │               ├─> [Iteration 3]
  │       │               │   ├─> Executor::Run() → step3
  │       │               │   └─> Replanner::Run()
  │       │               │       │
  │       │               │       ├─> ChatModel::Generate()
  │       │               │       │   └─> ToolCall: Respond(answer: "...")
  │       │               │       │
  │       │               │       └─> Send Event with BreakLoop ⚡
  │       │               │
  │       │               └─> Detect BreakLoop → Exit Loop
  │       │
  │       └─> Close & Return
  │
  └─> User receives final answer
```

---

## 📈 数据流分析

### Session 状态演变

#### 初始状态
```json
{
  "kSessionKeyUserInput": [],
  "kSessionKeyPlan": null,
  "kSessionKeyExecutedSteps": [],
  "kSessionKeyExecutedStep": null
}
```

#### After Planner
```json
{
  "kSessionKeyUserInput": [
    {"role": "user", "content": "为一款新的AI写作工具..."}
  ],
  "kSessionKeyPlan": {
    "steps": [
      "搜索AI写作工具市场的竞争对手信息",
      "分析收集到的竞争对手数据，提取价格、功能等关键信息",
      "基于分析结果，生成定价建议报告"
    ]
  },
  "kSessionKeyExecutedSteps": []
}
```

#### After Executor (Round 1)
```json
{
  ...,
  "kSessionKeyExecutedStep": {
    "step": "搜索AI写作工具市场的竞争对手信息",
    "result": "I have searched for AI writing tool competitors..."
  }
}
```

#### After Replanner (Round 1)
```json
{
  ...,
  "kSessionKeyPlan": {
    "steps": [
      "分析收集到的竞争对手数据，提取价格、功能等关键信息",
      "基于分析结果，生成定价建议报告"
    ]
  },
  "kSessionKeyExecutedSteps": [
    {
      "step": "搜索AI写作工具市场的竞争对手信息",
      "result": "..."
    }
  ]
}
```

#### After Completion
```json
{
  ...,
  "kSessionKeyExecutedSteps": [
    {"step": "搜索AI写作工具市场的竞争对手信息", "result": "..."},
    {"step": "分析收集到的竞争对手数据...", "result": "..."},
    {"step": "基于分析结果，生成定价建议报告", "result": "..."}
  ],
  "__workflow_complete": true,
  "__final_answer": "Based on comprehensive market research..."
}
```

---

## 🎓 关键要点总结

### 1. **三阶段工作流**
- **Planning** → **Execution** → **Replanning**
- 动态调整，而非静态执行

### 2. **Session 驱动**
- 所有状态存储在 Session 中
- Agent 间通过 Session 共享数据

### 3. **工具链协作**
- Executor 使用工具完成任务
- Replanner 决策是否继续或完成

### 4. **BreakLoop 机制**
- Replanner 通过 Respond 工具触发
- LoopAgent 检测并退出循环

### 5. **可扩展性**
- 可添加更多工具
- 可自定义 gen_input_fn
- 可调整 max_iterations

---

## 📝 完整执行日志示例

```
🏗️  [CreatePlanner] 开始创建规划器...
  ✓ Chat model configured
  ✓ Plan tool configured
✅ Planner created successfully!

🛠️  [CreateExecutor] 开始创建执行器...
  ✓ Chat model configured (max_iterations: 20)
  ✓ Added tool: web_search
  ✓ Added tool: analyze_data
✅ Executor created successfully!

🔄 [CreateReplanner] 开始创建重规划器...
  ✓ Chat model configured
  ✓ Plan tool configured
  ✓ Respond tool configured
✅ Replanner created successfully!

🎯 [CreateWorkflow] 组装 Plan-Execute-Replan 工作流...
  ✓ Planner: Planner
  ✓ Executor: Executor
  ✓ Replanner: Replanner
  ✓ Max iterations: 10
✅ Complete workflow created!

🚀 [ExecuteWorkflow] 开始执行工作流...

[Event #1] Planner
  Generated plan with 3 steps:
  1. 搜索AI写作工具市场的竞争对手信息
  2. 分析收集到的竞争对手数据，提取价格、功能等关键信息
  3. 基于分析结果，生成定价建议报告

[Event #2] Executor (Round 1)
  Tool Call: web_search("AI writing tools competitors pricing 2024")
  🔍 Found: Jasper AI ($99), Copy.ai ($49), Writesonic ($19)

[Event #3] Replanner (Round 1)
  Decision: Continue
  Updated plan with 2 remaining steps

[Event #4] Executor (Round 2)
  Tool Call: analyze_data(...)
  📊 Analysis: Average $55.67, range $19-99

[Event #5] Replanner (Round 2)
  Decision: Continue
  Updated plan with 1 remaining step

[Event #6] Executor (Round 3)
  Generating pricing recommendation report...

[Event #7] Replanner (Round 3)
  Decision: Complete ✅
  Tool Call: Respond(final_answer)
  🎉 Workflow completed!

Final Answer:
╔════════════════════════════════════════╗
║   AI Writing Tool Pricing Strategy     ║
╚════════════════════════════════════════╝

Recommendation: $39-49/month

Rationale:
- Competitive positioning
- Market penetration potential
- Value-based pricing
- Sustainable margins

✅ Task completed successfully!
```

---

这个示例展示了 Plan-Execute-Replan 模式的完整实现，包括每一步的代码细节和执行流程。你可以基于这个模板构建自己的复杂 Agent 工作流！
