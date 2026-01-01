# 🔄 InterruptInfo 到 AgentEvent 的转换详解

> **完整的中断信息转换和传播机制**

---

## 📋 目录

1. [核心概念](#核心概念)
2. [转换流程](#转换流程)
3. [代码实现详解](#代码实现详解)
4. [不同场景的转换](#不同场景的转换)
5. [完整示例](#完整示例)

---

## 核心概念

### 🎯 数据结构关系

```
InterruptInfo                      AgentAction                    AgentEvent
    │                                  │                              │
    │  (包含中断元数据)                  │  (包含 InterruptInfo)         │  (包含 AgentAction)
    │                                  │                              │
    ↓                                  ↓                              ↓
┌─────────────────┐           ┌────────────────────┐        ┌──────────────────┐
│ InterruptInfo   │──────────→│  AgentAction       │───────→│   AgentEvent     │
│                 │  填充到    │                    │ 封装到 │                  │
│ - data          │           │ - interrupted ─────┼───────→│ - action         │
│ - type          │           │ - exit             │        │ - agent_name     │
│ - reason        │           │ - transfer         │        │ - run_path       │
│ - state_key     │           │ - break_loop       │        │ - output         │
└─────────────────┘           └────────────────────┘        └──────────────────┘
```

### 🔑 关键步骤

1. **创建 InterruptInfo** - 在检测到中断条件时
2. **填充到 AgentAction** - 设置 `action->interrupted`
3. **封装到 AgentEvent** - 创建完整的事件对象
4. **发送 Event** - 通过 Generator 发送给调用者

---

## 转换流程

### 📊 完整流程图

```
┌─────────────────────────────────────────────────────────────────────┐
│                     中断检测阶段                                      │
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                           ↓
         ┌─────────────────────────────────┐
         │ 1. 检测到中断条件                 │
         │    - InterruptBeforeNodes       │
         │    - InterruptAfterNodes        │
         │    - InterruptAndRerun Error    │
         └──────────────┬──────────────────┘
                        │
                        ↓
         ┌─────────────────────────────────┐
         │ 2. 创建 InterruptInfo            │
         │    auto info = make_shared<>()   │
         │    info->data = ...              │
         │    info->reason = ...            │
         └──────────────┬──────────────────┘
                        │
                        ↓
┌─────────────────────────────────────────────────────────────────────┐
│                     转换为 AgentAction                               │
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                           ↓
         ┌─────────────────────────────────┐
         │ 3. 创建 AgentAction              │
         │    auto action = make_shared<>() │
         │    action->interrupted = info    │
         │    action->exit = false          │
         └──────────────┬──────────────────┘
                        │
                        ↓
┌─────────────────────────────────────────────────────────────────────┐
│                     封装为 AgentEvent                                │
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                           ↓
         ┌─────────────────────────────────┐
         │ 4. 创建 AgentEvent               │
         │    auto event = make_shared<>()  │
         │    event->action = action        │
         │    event->agent_name = ...       │
         │    event->run_path = ...         │
         └──────────────┬──────────────────┘
                        │
                        ↓
         ┌─────────────────────────────────┐
         │ 5. 发送 Event                    │
         │    generator->Send(event)        │
         └─────────────────────────────────┘
```

---

## 代码实现详解

### 1️⃣ **基础转换 - 简单场景**

#### **场景：Agent 内部检测到中断**

**Go 实现**（参考）：

```go
// eino/adk/workflow.go:250-269
func wrapWorkflowInterrupt(e *AgentEvent, origInput *AgentInput, seqIdx int, iterations int) *AgentEvent {
    // 创建新的 AgentEvent
    newEvent := &AgentEvent{
        AgentName: e.AgentName,
        RunPath:   e.RunPath,
        Output:    e.Output,
        
        // 🔑 创建新的 AgentAction，包含 InterruptInfo
        Action: &AgentAction{
            Exit:             e.Action.Exit,
            Interrupted:      &InterruptInfo{Data: e.Action.Interrupted.Data},  // 包装原始 InterruptInfo
            TransferToAgent:  e.Action.TransferToAgent,
            CustomizedAction: e.Action.CustomizedAction,
        },
        Err: e.Err,
    }
    
    // 263-268: 将原始中断信息包装到 WorkflowInterruptInfo
    newEvent.Action.Interrupted.Data = &WorkflowInterruptInfo{
        OrigInput:                origInput,
        SequentialInterruptIndex: seqIdx,
        SequentialInterruptInfo:  e.Action.Interrupted,  // 保留原始 InterruptInfo
        LoopIterations:           iterations,
    }
    
    return newEvent
}
```

**C++ 等效实现**：

```cpp
// eino_cpp/src/adk/workflow.cpp:81-94
// 检测到子 Agent 中断
if (last_event && last_event->action) {
    if (last_event->action->interrupted) {
        // 🔑 步骤 1: 创建 WorkflowInterruptInfo 包装器
        auto workflow_interrupt = std::make_shared<WorkflowInterruptInfo>();
        workflow_interrupt->orig_input = input;
        workflow_interrupt->sequential_interrupt_index = i;
        workflow_interrupt->sequential_interrupt_info = last_event->action->interrupted;  // 保留原始
        workflow_interrupt->loop_iterations = iterations;

        // 🔑 步骤 2: 创建新的 AgentEvent
        auto wrapped_event = std::make_shared<AgentEvent>(*last_event);  // 拷贝构造
        
        // 🔑 步骤 3: 创建新的 InterruptInfo，包装 WorkflowInterruptInfo
        wrapped_event->action->interrupted = std::make_shared<InterruptInfo>();
        wrapped_event->action->interrupted->data = workflow_interrupt.get();
        
        // 🔑 步骤 4: 发送 Event
        gen->Send(wrapped_event);
        return {true, true};  // exit=true, interrupted=true
    }
}
```

---

### 2️⃣ **并行工作流中的转换**

#### **场景：多个子 Agent 同时中断**

**Go 实现**：

```go
// eino/adk/workflow.go:398-412
if len(interruptMap) > 0 {
    replaceInterruptRunCtx(ctx, getRunCtx(ctx))
    
    // 🔑 创建新的 AgentEvent
    generator.Send(&AgentEvent{
        AgentName: a.Name(ctx),
        RunPath:   getRunCtx(ctx).RunPath,
        
        // 🔑 创建 AgentAction 包含所有子 Agent 的中断信息
        Action: &AgentAction{
            Interrupted: &InterruptInfo{
                Data: &WorkflowInterruptInfo{
                    OrigInput:             input,
                    ParallelInterruptInfo: interruptMap,  // map[int]*InterruptInfo
                },
            },
        },
    })
}
```

**C++ 等效实现**：

```cpp
// eino_cpp/src/adk/workflow.cpp:161-172
if (!interrupt_events.empty()) {
    // 步骤 1: 创建 WorkflowInterruptInfo
    auto workflow_interrupt = std::make_shared<WorkflowInterruptInfo>();
    workflow_interrupt->orig_input = input;
    
    // 步骤 2: 收集所有子 Agent 的中断信息
    for (size_t i = 0; i < interrupt_events.size(); ++i) {
        workflow_interrupt->parallel_interrupt_info[i] = 
            interrupt_events[i]->action->interrupted;
    }
    
    // 步骤 3: 创建包装的 AgentEvent
    auto wrapped_event = std::make_shared<AgentEvent>();
    wrapped_event->action = std::make_shared<AgentAction>();
    wrapped_event->action->interrupted = std::make_shared<InterruptInfo>();
    wrapped_event->action->interrupted->data = workflow_interrupt.get();
    
    // 步骤 4: 发送
    gen->Send(wrapped_event);
}
```

---

### 3️⃣ **Runner 中的转换与 CheckPoint 保存**

#### **场景：Runner 检测到中断并保存 CheckPoint**

**C++ 实现**：

```cpp
// eino_cpp/src/adk/runner.cpp:266-286
// 检测中断
if (event && event->action && 
    (event->action->interrupted || event->action->break_loop)) {
    interrupt_event = event;  // 🔑 保存中断事件
}

gen->Send(event);  // 🔑 立即转发事件给用户

// ...循环结束后...

// 保存 CheckPoint（如果有中断）
if (interrupt_event && checkpoint_store_ && !checkpoint_id.empty()) {
    try {
        nlohmann::json checkpoint_json;
        
        // 保存中断状态
        checkpoint_json["interrupted"] = true;
        checkpoint_json["interrupt_reason"] = interrupt_event->action->interrupted 
            ? "interrupted" : "break_loop";
        
        // 保存其他状态...
        checkpoint_json["messages"] = accumulated_messages;
        checkpoint_json["session_state"] = session_state;
        checkpoint_json["agent_state"] = *interrupt_event->state;
        checkpoint_json["timestamp"] = current_time();
        
        // 序列化保存
        std::string serialized = checkpoint_json.dump();
        checkpoint_store_->Save(checkpoint_id, serialized);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to save checkpoint: " << e.what() << std::endl;
    }
}
```

**关键点**：
1. ✅ **先转发 Event** - 用户立即收到中断通知
2. ✅ **后保存 CheckPoint** - 不阻塞事件流
3. ✅ **从 Event 提取信息** - 使用 `interrupt_event->action->interrupted`

---

### 4️⃣ **从 Graph 传播中断**

#### **场景：Graph 执行器检测到节点中断**

**伪代码流程**：

```cpp
// Graph Executor 内部
void GraphExecutor::handleInterrupt(
    void* ctx,
    InterruptTempInfo* temp_info,
    std::vector<Task*> next_tasks,
    std::map<std::string, Channel> channels,
    bool is_stream) {
    
    // ========== 步骤 1: 创建 CheckPoint ==========
    auto checkpoint = std::make_shared<CheckPoint>();
    checkpoint->channels = channels;
    checkpoint->state = getCurrentState(ctx);
    
    for (auto task : next_tasks) {
        checkpoint->inputs[task->node_key] = task->input;
    }
    
    // ========== 步骤 2: 创建 Compose InterruptInfo ==========
    auto compose_interrupt_info = std::make_shared<compose::InterruptInfo>();
    compose_interrupt_info->State = checkpoint->state;
    compose_interrupt_info->BeforeNodes = temp_info->interrupt_before_nodes;
    compose_interrupt_info->AfterNodes = temp_info->interrupt_after_nodes;
    compose_interrupt_info->RerunNodes = temp_info->interrupt_rerun_nodes;
    compose_interrupt_info->RerunNodesExtra = temp_info->interrupt_rerun_extra;
    
    // ========== 步骤 3: 转换为 ADK InterruptInfo ==========
    auto adk_interrupt_info = std::make_shared<adk::InterruptInfo>();
    adk_interrupt_info->data = compose_interrupt_info;  // 🔑 包装 Compose InterruptInfo
    
    // ========== 步骤 4: 创建 AgentAction ==========
    auto action = std::make_shared<adk::AgentAction>();
    action->interrupted = adk_interrupt_info;  // 🔑 填充 interrupted 字段
    action->exit = false;
    
    // ========== 步骤 5: 创建 AgentEvent ==========
    auto event = std::make_shared<adk::AgentEvent>();
    event->agent_name = getCurrentAgentName(ctx);
    event->run_path = getRunPath(ctx);
    event->action = action;  // 🔑 包含中断信息的 action
    
    // ========== 步骤 6: 保存 CheckPoint ==========
    saveCheckPoint(ctx, checkpoint_store, checkpoint_id, checkpoint, compose_interrupt_info);
    
    // ========== 步骤 7: 抛出中断异常或返回 Event ==========
    throw InterruptError(event);  // 或者通过 generator 发送
}
```

---

## 不同场景的转换

### 📌 场景 1：ChatModel Agent 中断

```cpp
// ChatModelAgent 执行过程中检测到需要人工审批

// 步骤 1: 检测条件
if (tool_call.name == "sensitive_operation") {
    
    // 步骤 2: 创建 InterruptInfo
    auto interrupt_info = std::make_shared<InterruptInfo>();
    interrupt_info->interrupt_type = InterruptType::kHumanApproval;
    interrupt_info->reason = "Sensitive operation requires approval";
    interrupt_info->state_key = "approval_checkpoint_1";
    interrupt_info->context = {
        {"operation", "delete_database"},
        {"target", "production"},
    };
    
    // 步骤 3: 创建 AgentAction
    auto action = std::make_shared<AgentAction>();
    action->interrupted = interrupt_info;
    action->exit = false;
    
    // 步骤 4: 创建 AgentEvent
    auto event = std::make_shared<AgentEvent>();
    event->agent_name = agent_name_;
    event->run_path = getRunPath(ctx);
    event->action = action;
    
    // 步骤 5: 发送 Event
    generator->Send(event);
    
    // 步骤 6: 结束执行（等待恢复）
    return;
}
```

**用户侧接收**：

```cpp
auto events = agent->Run(ctx, input);

for (auto event : events) {
    if (event->action && event->action->interrupted) {
        // ✅ 成功接收到 InterruptInfo
        auto info = event->action->interrupted;
        
        std::cout << "中断类型: " << static_cast<int>(info->interrupt_type) << std::endl;
        std::cout << "原因: " << info->reason << std::endl;
        std::cout << "状态键: " << info->state_key << std::endl;
        
        // 处理审批逻辑...
    }
}
```

---

### 📌 场景 2：Sequential Workflow 中断

```cpp
// Sequential Agent 执行第 3 个子 Agent 时中断

// 子 Agent 发送的原始事件
AgentEvent original_event {
    agent_name: "SubAgent3",
    run_path: {"Workflow", "SubAgent1", "SubAgent2", "SubAgent3"},
    action: {
        interrupted: {
            data: nullptr,  // 简单中断
            reason: "Waiting for user input",
        }
    }
}

// Sequential Agent 包装后发送的事件
AgentEvent wrapped_event {
    agent_name: "SequentialWorkflow",
    run_path: {"Workflow"},
    action: {
        interrupted: {
            data: WorkflowInterruptInfo {
                orig_input: original_workflow_input,
                sequential_interrupt_index: 2,  // 第 3 个 Agent (0-based)
                sequential_interrupt_info: original_event.action.interrupted,  // 保留原始中断
                loop_iterations: 0,
            }
        }
    }
}
```

**转换代码**：

```cpp
// eino_cpp/src/adk/workflow.cpp:81-94
if (last_event->action->interrupted) {
    auto workflow_interrupt = std::make_shared<WorkflowInterruptInfo>();
    workflow_interrupt->orig_input = input;
    workflow_interrupt->sequential_interrupt_index = i;  // 当前索引
    workflow_interrupt->sequential_interrupt_info = last_event->action->interrupted;
    workflow_interrupt->loop_iterations = iterations;

    auto wrapped_event = std::make_shared<AgentEvent>(*last_event);
    wrapped_event->action->interrupted = std::make_shared<InterruptInfo>();
    wrapped_event->action->interrupted->data = workflow_interrupt.get();  // 🔑 包装
    
    gen->Send(wrapped_event);
    return {true, true};
}
```

---

### 📌 场景 3：Tool Node InterruptAndRerun

```cpp
// Tool 执行失败，需要重新运行

// 工具内部抛出异常
void WebSearchTool::Invoke(...) {
    if (network_error) {
        // 创建 RerunExtra
        auto rerun_extra = std::make_shared<ToolsInterruptAndRerunExtra>();
        rerun_extra->executed_tools = {"tool1", "tool2"};  // 已执行的工具
        
        // 抛出 InterruptAndRerun 错误
        throw InterruptAndRerunError(rerun_extra);
    }
}

// ToolNode 捕获错误并转换
try {
    tool->Invoke(ctx, input);
} catch (const InterruptAndRerunError& e) {
    // 步骤 1: 创建 Compose InterruptInfo
    auto compose_info = std::make_shared<compose::InterruptInfo>();
    compose_info->RerunNodes = {current_node_key};
    compose_info->RerunNodesExtra[current_node_key] = e.GetExtra();
    
    // 步骤 2: 转换为 ADK InterruptInfo
    auto adk_info = std::make_shared<adk::InterruptInfo>();
    adk_info->data = compose_info;
    
    // 步骤 3: 创建 AgentEvent
    auto event = std::make_shared<AgentEvent>();
    event->action = std::make_shared<AgentAction>();
    event->action->interrupted = adk_info;
    
    // 步骤 4: 发送
    generator->Send(event);
}
```

---

## 完整示例

### 示例：端到端中断转换

```cpp
#include "eino/adk/chat_model_agent.h"
#include "eino/adk/workflow.h"
#include "eino/adk/interrupt.h"
#include <iostream>

using namespace eino::adk;

// ============================================================================
// 自定义 Agent：需要人工审批
// ============================================================================

class ApprovalAgent : public Agent {
public:
    ApprovalAgent(const std::string& name) : name_(name) {}
    
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        std::shared_ptr<AgentInput> input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override {
        
        auto gen = std::make_shared<AsyncGenerator<std::shared_ptr<AgentEvent>>>();
        
        std::thread([this, ctx, input, gen]() {
            // ========== 步骤 1: 模拟业务逻辑 ==========
            std::cout << "[" << name_ << "] Processing request..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // ========== 步骤 2: 检测到需要审批 ==========
            bool needs_approval = CheckIfNeedsApproval(input);
            
            if (needs_approval) {
                std::cout << "[" << name_ << "] ⚠️  Need human approval!" << std::endl;
                
                // ========== 步骤 3: 创建 InterruptInfo ==========
                auto interrupt_info = InterruptInfo::NewHumanApproval(
                    "Sensitive operation requires manager approval",
                    "approval_state_001"
                );
                
                // 添加上下文
                interrupt_info->context = {
                    {"operation", "delete_production_data"},
                    {"affected_records", 10000},
                    {"requestor", "user_123"},
                };
                
                // ========== 步骤 4: 创建 AgentAction ==========
                auto action = std::make_shared<AgentAction>();
                action->interrupted = interrupt_info;  // 🔑 填充中断信息
                action->exit = false;
                
                // ========== 步骤 5: 创建 AgentEvent ==========
                auto event = std::make_shared<AgentEvent>();
                event->agent_name = name_;
                event->run_path = {RunStep{name_}};
                event->action = action;  // 🔑 包含 AgentAction
                
                // ========== 步骤 6: 发送 Event ==========
                gen->Send(event);
                
                std::cout << "[" << name_ << "] ✅ Interrupt event sent!" << std::endl;
            } else {
                // 正常完成
                auto event = std::make_shared<AgentEvent>();
                event->agent_name = name_;
                event->action = std::make_shared<AgentAction>();
                event->action->exit = true;
                
                gen->Send(event);
            }
            
            gen->Close();
        }).detach();
        
        return gen->GetIterator();
    }
    
private:
    bool CheckIfNeedsApproval(std::shared_ptr<AgentInput> input) {
        // 检查是否包含敏感操作关键词
        for (const auto& msg : input->messages) {
            if (msg->content.find("delete") != std::string::npos ||
                msg->content.find("production") != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    std::string name_;
};

// ============================================================================
// 测试：捕获和处理中断
// ============================================================================

int main() {
    std::cout << "\n========== InterruptInfo 到 AgentEvent 转换示例 ==========\n" << std::endl;
    
    auto ctx = CreateContext();
    
    // ========== 1. 创建 Agent ==========
    auto agent = std::make_shared<ApprovalAgent>("ApprovalAgent");
    
    // ========== 2. 准备输入（包含敏感操作）==========
    auto input = std::make_shared<AgentInput>();
    input->messages = {
        CreateMessage(schema::RoleType::User, 
                     "Please delete all production data from the database")
    };
    
    // ========== 3. 运行 Agent ==========
    std::cout << "🚀 Running agent...\n" << std::endl;
    
    auto events = agent->Run(ctx, input, {});
    
    // ========== 4. 捕获并解析 AgentEvent ==========
    std::shared_ptr<AgentEvent> interrupt_event;
    
    while (true) {
        std::shared_ptr<AgentEvent> event;
        if (!events->Next(event)) {
            break;
        }
        
        std::cout << "📨 Received event from: " << event->agent_name << std::endl;
        
        // ========== 5. 检查是否包含中断 ==========
        if (event->action && event->action->interrupted) {
            interrupt_event = event;
            
            std::cout << "\n⚠️  ==================== INTERRUPT DETECTED ====================" << std::endl;
            
            auto info = event->action->interrupted;
            
            // ========== 6. 提取 InterruptInfo 信息 ==========
            std::cout << "中断类型: " << static_cast<int>(info->interrupt_type) << std::endl;
            std::cout << "原因: " << info->reason << std::endl;
            std::cout << "状态键: " << info->state_key << std::endl;
            
            std::cout << "\n上下文信息:" << std::endl;
            if (info->context.contains("operation")) {
                std::cout << "  - Operation: " << info->context["operation"] << std::endl;
            }
            if (info->context.contains("affected_records")) {
                std::cout << "  - Affected Records: " << info->context["affected_records"] << std::endl;
            }
            if (info->context.contains("requestor")) {
                std::cout << "  - Requestor: " << info->context["requestor"] << std::endl;
            }
            
            std::cout << "===============================================================\n" << std::endl;
            
            break;  // 中断处理
        }
    }
    
    // ========== 7. 模拟人工审批流程 ==========
    if (interrupt_event) {
        std::cout << "等待管理员审批..." << std::endl;
        std::cout << "输入 'approve' 批准，'reject' 拒绝: ";
        
        std::string decision;
        std::cin >> decision;
        
        if (decision == "approve") {
            std::cout << "\n✅ 审批通过！继续执行..." << std::endl;
            
            // ========== 8. 准备 ResumeInfo ==========
            auto resume_info = std::make_shared<ResumeInfo>();
            resume_info->interrupt_info = interrupt_event->action->interrupted;
            resume_info->interrupt_info->extra["approval_decision"] = decision;
            resume_info->interrupt_info->extra["approver"] = "manager_456";
            resume_info->interrupt_info->extra["approval_time"] = getCurrentTimestamp();
            
            // ========== 9. 恢复执行 ==========
            // auto resume_events = agent->Resume(ctx, resume_info, {});
            // ... 处理恢复后的事件 ...
            
        } else {
            std::cout << "\n❌ 审批被拒绝！操作取消。" << std::endl;
        }
    } else {
        std::cout << "\n✅ Agent 完成执行，无中断。" << std::endl;
    }
    
    std::cout << "\n========== 示例结束 ==========\n" << std::endl;
    
    return 0;
}

// ============================================================================
// 辅助函数
// ============================================================================

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    return std::ctime(&time_t);
}

schema::Message* CreateMessage(schema::RoleType role, const std::string& content) {
    auto msg = new schema::Message();
    msg->role = role;
    msg->content = content;
    return msg;
}

void* CreateContext() {
    // 创建执行上下文
    return nullptr;  // 简化示例
}
```

**预期输出**：

```
========== InterruptInfo 到 AgentEvent 转换示例 ==========

🚀 Running agent...

[ApprovalAgent] Processing request...
[ApprovalAgent] ⚠️  Need human approval!
[ApprovalAgent] ✅ Interrupt event sent!
📨 Received event from: ApprovalAgent

⚠️  ==================== INTERRUPT DETECTED ====================
中断类型: 0
原因: Sensitive operation requires manager approval
状态键: approval_state_001

上下文信息:
  - Operation: delete_production_data
  - Affected Records: 10000
  - Requestor: user_123
===============================================================

等待管理员审批...
输入 'approve' 批准，'reject' 拒绝: approve

✅ 审批通过！继续执行...

========== 示例结束 ==========
```

---

## 关键转换点总结

### 🔄 转换层级

```
Layer 1: 业务逻辑层
    ↓
    创建 InterruptInfo
    {
        interrupt_type: kHumanApproval,
        reason: "...",
        state_key: "...",
        context: {...},
    }
    
Layer 2: Action 封装层
    ↓
    填充到 AgentAction
    {
        interrupted: InterruptInfo*,
        exit: false,
        ...
    }
    
Layer 3: Event 封装层
    ↓
    包装为 AgentEvent
    {
        agent_name: "...",
        run_path: [...],
        action: AgentAction*,
        ...
    }
    
Layer 4: 传播层
    ↓
    发送给调用者
    generator->Send(event)
    
Layer 5: 用户处理层
    ↓
    解析 InterruptInfo
    if (event->action->interrupted) {
        auto info = event->action->interrupted;
        // 处理中断...
    }
```

---

## 不同上下文的 data 字段

### 📦 InterruptInfo.data 的类型变化

| 场景 | data 类型 | 说明 |
|------|----------|------|
| **简单中断** | `nullptr` 或简单数据 | Agent 内部直接中断 |
| **Workflow Sequential** | `WorkflowInterruptInfo*` | 包含子 Agent 索引和原始中断 |
| **Workflow Parallel** | `WorkflowInterruptInfo*` | 包含多个子 Agent 的中断 map |
| **Workflow Loop** | `WorkflowInterruptInfo*` | 包含当前迭代次数 |
| **Graph Interrupt** | `compose::InterruptInfo*` | 包含节点、状态、通道信息 |
| **Tool Rerun** | `ToolsInterruptAndRerunExtra*` | 包含已执行工具列表 |

---

## 最佳实践

### ✅ DO

1. **保留原始中断信息**
   ```cpp
   // Workflow 包装时保留原始中断
   workflow_interrupt->sequential_interrupt_info = original_interrupt;
   ```

2. **添加足够的上下文**
   ```cpp
   interrupt_info->context = {
       {"timestamp", getCurrentTime()},
       {"user_id", current_user},
       {"operation", operation_type},
   };
   ```

3. **及时转发 Event**
   ```cpp
   gen->Send(event);  // 立即发送，不要阻塞
   ```

4. **检查 nullptr**
   ```cpp
   if (event->action && event->action->interrupted) {
       // 安全访问
   }
   ```

---

### ❌ DON'T

1. **不要丢失原始信息**
   ```cpp
   // ❌ 错误：覆盖了原始中断
   event->action->interrupted = new_interrupt;
   
   // ✅ 正确：包装原始中断
   wrapper->original_interrupt = event->action->interrupted;
   event->action->interrupted->data = wrapper;
   ```

2. **不要假设 data 类型**
   ```cpp
   // ❌ 错误：直接转换
   auto workflow_info = static_cast<WorkflowInterruptInfo*>(info->data);
   
   // ✅ 正确：检查类型
   if (auto workflow_info = dynamic_cast<WorkflowInterruptInfo*>(info->data)) {
       // 使用 workflow_info
   }
   ```

---

## 总结

### 🎯 核心流程

```
InterruptInfo → AgentAction.interrupted → AgentEvent.action → User
     ↑                    ↑                      ↑              ↓
  创建元信息          封装到 Action          发送 Event      处理中断
```

### 🔑 关键代码位置

**C++ 实现**：
- `eino_cpp/src/adk/workflow.cpp:81-94` - Sequential 包装
- `eino_cpp/src/adk/workflow.cpp:161-172` - Parallel 包装
- `eino_cpp/src/adk/runner.cpp:266-286` - Runner 保存 CheckPoint

**Go 参考**：
- `eino/adk/workflow.go:250-269` - wrapWorkflowInterrupt
- `eino/adk/workflow.go:398-412` - Parallel 中断
- `eino/compose/graph_run.go:465-499` - Graph handleInterrupt

---

**现在你已经完全理解了 InterruptInfo 到 AgentEvent 的转换机制！** 🎉
