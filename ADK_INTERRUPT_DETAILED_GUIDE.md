# 🔄 eino_cpp ADK Interrupt 交互链路详细指南

> **完整的中断机制实现与执行流程解析**

---

## 📋 目录

1. [核心概念](#核心概念)
2. [数据结构详解](#数据结构详解)
3. [完整交互链路](#完整交互链路)
4. [代码执行流程](#代码执行流程)
5. [使用示例](#使用示例)
6. [最佳实践](#最佳实践)

---

## 核心概念

### 🎯 什么是 Interrupt？

**Interrupt（中断）** 是 eino ADK 提供的一种**检查点机制**，允许：

1. **暂停执行**：在关键节点暂停 Agent/Graph 执行
2. **保存状态**：序列化整个执行上下文（CheckPoint）
3. **恢复执行**：从中断点恢复继续运行
4. **人机协作**：等待人工审批、输入或决策

### 🔑 三种中断类型

```cpp
enum class InterruptType {
    kHumanApproval,   // 等待人工审批（如敏感操作）
    kHumanInput,      // 等待人工输入（如补充信息）
    kCustomInterrupt, // 自定义中断（如超时、资源限制）
};
```

---

## 数据结构详解

### 1️⃣ **InterruptInfo** - 中断元信息

```cpp
// eino_cpp/include/eino/adk/types.h:84-87
struct InterruptInfo {
    // 序列化的中断数据（类型取决于来源）
    std::shared_ptr<void> data;
};
```

**对应 Go 实现**：
```go
// eino/adk/interrupt.go:34-36
type InterruptInfo struct {
    Data any  // 可存储任意类型数据
}
```

**扩展版本**（在 `interrupt.h` 中）：
```cpp
// eino_cpp/include/eino/adk/interrupt.h:38-89
struct InterruptInfo {
    InterruptType interrupt_type;      // 中断类型
    std::string reason;                // 中断原因
    std::string state_key;             // 状态恢复键
    json context;                      // 额外上下文
    std::map<std::string, json> extra; // 自定义字段
    
    // 工厂方法
    static std::shared_ptr<InterruptInfo> NewHumanApproval(
        const std::string& reason,
        const std::string& state_key = "");
        
    static std::shared_ptr<InterruptInfo> NewHumanInput(...);
    static std::shared_ptr<InterruptInfo> NewCustom(...);
};
```

---

### 2️⃣ **AgentAction** - Agent 执行动作

```cpp
// eino_cpp/include/eino/adk/types.h:91-106
struct AgentAction {
    bool exit = false;                              // 退出信号
    std::shared_ptr<InterruptInfo> interrupted;     // 🔑 中断信息
    std::shared_ptr<TransferToAgentAction> transfer_to_agent;  // 转移控制
    std::shared_ptr<void> break_loop;               // 跳出循环
    std::shared_ptr<void> customized_action;        // 自定义动作
};
```

**中断触发判断**：
```cpp
if (event->action && event->action->interrupted) {
    // ✅ 发生了中断！
    auto interrupt_info = event->action->interrupted;
    // 保存 checkpoint 并通知用户
}
```

---

### 3️⃣ **ResumeInfo** - 恢复执行信息

```cpp
// eino_cpp/include/eino/adk/types.h:159-165
struct ResumeInfo {
    bool enable_streaming = false;              // 是否启用流式
    std::shared_ptr<InterruptInfo> interrupt_info;  // 中断点信息
};
```

**对应 Go**：
```go
// eino/adk/interrupt.go:29-32
type ResumeInfo struct {
    EnableStreaming bool
    *InterruptInfo
}
```

---

### 4️⃣ **WorkflowInterruptInfo** - 工作流中断信息

```cpp
// eino_cpp/include/eino/adk/types.h:194-207
struct WorkflowInterruptInfo {
    // 原始输入
    std::shared_ptr<AgentInput> orig_input;
    
    // 顺序工作流：中断的子 Agent 索引
    int sequential_interrupt_index = -1;
    std::shared_ptr<InterruptInfo> sequential_interrupt_info;
    
    // 循环工作流：当前迭代次数
    int loop_iterations = 0;
    
    // 并行工作流：各子 Agent 的中断信息
    std::map<int, std::shared_ptr<InterruptInfo>> parallel_interrupt_info;
};
```

---

### 5️⃣ **CheckPoint** - 检查点存储

```cpp
// eino_cpp/include/eino/compose/checkpoint.h
struct CheckPoint {
    // 当前状态（State）
    std::shared_ptr<void> state;
    
    // 各节点的输入数据
    std::map<std::string, std::shared_ptr<void>> inputs;
    
    // 各通道的数据
    std::map<std::string, channel> channels;
    
    // 需要重新运行的节点
    std::vector<std::string> rerun_nodes;
    
    // 需要跳过 PreHandler 的节点
    std::map<std::string, bool> skip_pre_handler;
};
```

---

## 完整交互链路

### 📊 总览架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                       User Application                          │
│  (调用 Agent 并处理中断/恢复)                                    │
└────────────┬─────────────────────────────────────┬──────────────┘
             │                                     │
             │ 1. agent->Run(ctx, input)           │ 6. agent->Resume(ctx, resume_info)
             ↓                                     ↑
┌─────────────────────────────────────────────────────────────────┐
│                        ADK Agent                                │
│  (ChatModelAgent / SequentialAgent / LoopAgent)                 │
└────────────┬─────────────────────────────────────┬──────────────┘
             │                                     │
             │ 2. 执行 Graph/Workflow              │ 5. 从 CheckPoint 恢复
             ↓                                     ↑
┌─────────────────────────────────────────────────────────────────┐
│                      Compose Graph                              │
│  (Graph Executor - graph_run.go)                                │
│   - 检测 InterruptBeforeNodes / InterruptAfterNodes              │
│   - 检测 InterruptAndRerun 错误                                  │
└────────────┬─────────────────────────────────────┬──────────────┘
             │                                     │
             │ 3. 触发中断                          │ 4. 保存 CheckPoint
             ↓                                     ↓
┌─────────────────────────────┐    ┌──────────────────────────────┐
│     InterruptInfo           │    │   CheckPointStore            │
│  - State                    │    │  - Get(ctx, id)              │
│  - BeforeNodes              │    │  - Set(ctx, id, data)        │
│  - AfterNodes               │────│                              │
│  - RerunNodes               │    │  (序列化/反序列化)            │
└─────────────────────────────┘    └──────────────────────────────┘
```

---

## 代码执行流程

### 🔄 完整执行周期

#### **阶段 1：初始执行（Run）**

```
User Code
    │
    ├─> agent->Run(ctx, input)
    │       │
    │       └─> 内部调用 runner->run(ctx, input, options)
    │               │
    │               ├─> 初始化 RunContext
    │               │   └─> run_ctx = {root_input, run_path, session_values}
    │               │
    │               └─> 执行 Graph/Workflow
    │                       │
    │                       ├─> executeNode(node1)
    │                       ├─> executeNode(node2)
    │                       └─> executeNode(node3) ❌ 触发中断！
    │                               │
    │                               └─> 检测到中断条件：
    │                                   - node3 在 interruptBeforeNodes 中
    │                                   - 或者 node 返回 InterruptAndRerun 错误
    │
    └─> 返回中断 Event：
            event->action->interrupted = interrupt_info
```

#### **阶段 2：中断检测（Graph Run）**

**Go 代码路径**：`eino/compose/graph_run.go:215-226`

```go
// 215: 检查 interruptBeforeNodes
if keys := getHitKey(nextTasks, r.interruptBeforeNodes); len(keys) > 0 {
    tempInfo := newInterruptTempInfo()
    tempInfo.interruptBeforeNodes = append(tempInfo.interruptBeforeNodes, keys...)
    
    // 218: 调用 handleInterrupt
    return nil, r.handleInterrupt(ctx,
        tempInfo,
        nextTasks,
        cm.channels,
        isStream,
        isSubGraph,
        writeToCheckPointID,
    )
}
```

**C++ 等效逻辑**（在 Graph 执行器中）：

```cpp
// 伪代码：graph_executor.cpp
std::vector<Task*> next_tasks = calculateNextTasks(completed_tasks);

// 检查是否命中 interruptBeforeNodes
std::vector<std::string> hit_keys = getHitKeys(next_tasks, interrupt_before_nodes_);
if (!hit_keys.empty()) {
    // 🚨 触发中断！
    auto temp_info = std::make_shared<InterruptTempInfo>();
    temp_info->interrupt_before_nodes = hit_keys;
    
    return handleInterrupt(ctx, temp_info, next_tasks, channels, is_stream);
}
```

#### **阶段 3：保存 CheckPoint**

**Go 代码**：`eino/compose/graph_run.go:465-499`

```go
func (r *runner) handleInterrupt(
    ctx context.Context,
    tempInfo *interruptTempInfo,
    nextTasks []*task,
    channels map[string]channel,
    isStream bool,
    isSubGraph bool,
    checkPointID *string,
) error {
    // 474: 创建 CheckPoint
    cp := &checkpoint{
        Channels:       channels,          // 保存所有通道数据
        Inputs:         make(map[string]any),
        SkipPreHandler: map[string]bool{},
    }
    
    // 479-484: 保存当前 State
    if r.runCtx != nil {
        if state, ok := ctx.Value(stateKey{}).(*internalState); ok {
            cp.State = state.state
        }
    }
    
    // 485-492: 构造 InterruptInfo
    intInfo := &InterruptInfo{
        State:           cp.State,
        AfterNodes:      tempInfo.interruptAfterNodes,
        BeforeNodes:     tempInfo.interruptBeforeNodes,
        RerunNodes:      tempInfo.interruptRerunNodes,
        RerunNodesExtra: tempInfo.interruptRerunExtra,
        SubGraphs:       make(map[string]*InterruptInfo),
    }
    
    // 493-495: 保存下一步的输入
    for _, t := range nextTasks {
        cp.Inputs[t.nodeKey] = t.input
    }
    
    // 496: 转换并持久化 CheckPoint
    err := r.checkPointer.convertCheckPoint(cp, isStream)
    // ...
}
```

**C++ 等效实现**：

```cpp
// eino_cpp/src/adk/interrupt.cpp:196-219
void SaveCheckPoint(
    void* ctx,
    compose::CheckPointStore* store,
    const std::string& key,
    std::shared_ptr<RunContext> run_ctx,
    std::shared_ptr<InterruptInfo> info) {
    
    if (!store) {
        throw std::runtime_error("CheckPoint store is null");
    }
    
    // 序列化 run_ctx 和 info 为字节
    std::vector<uint8_t> data = SerializeCheckPoint(
        std::static_pointer_cast<void>(run_ctx), 
        info
    );
    
    try {
        store->Set(ctx, key, data);  // 持久化到存储
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to save checkpoint: " + std::string(e.what()));
    }
}
```

#### **阶段 4：序列化**

**Go 实现**：`eino/adk/interrupt.go:82-98`

```go
func saveCheckPoint(
    ctx context.Context,
    store compose.CheckPointStore,
    key string,
    runCtx *runContext,
    info *InterruptInfo,
) error {
    buf := &bytes.Buffer{}
    
    // 使用 gob 编码序列化
    err := gob.NewEncoder(buf).Encode(&serialization{
        RunCtx: runCtx,
        Info:   info,
    })
    if err != nil {
        return fmt.Errorf("failed to encode checkpoint: %w", err)
    }
    
    return store.Set(ctx, key, buf.Bytes())
}
```

**C++ 实现**：`eino_cpp/src/adk/interrupt.cpp:82-101`

```cpp
std::vector<uint8_t> SerializeCheckPoint(
    const std::shared_ptr<void>& run_ctx,
    const std::shared_ptr<InterruptInfo>& info) {
    
    json j;
    
    // 序列化 interrupt info
    if (info) {
        j["interrupt_info"] = SerializeInterruptInfo(info);
    }
    
    // 序列化 run context（占位符，实际需要自定义）
    j["run_ctx"] = json::object();
    
    // 转换为字节
    std::string json_str = j.dump();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

// 辅助函数：序列化 InterruptInfo
json SerializeInterruptInfo(const std::shared_ptr<InterruptInfo>& info) {
    if (!info) return json::object();
    
    json j;
    j["interrupt_type"] = static_cast<int>(info->interrupt_type);
    j["reason"] = info->reason;
    j["state_key"] = info->state_key;
    j["context"] = info->context;
    j["extra"] = json(info->extra);
    
    return j;
}
```

---

#### **阶段 5：用户处理中断**

```cpp
// 用户代码示例
auto events = agent->Run(ctx, input);

for (auto event : events) {
    if (event->action && event->action->interrupted) {
        // ✅ 检测到中断！
        auto interrupt_info = event->action->interrupted;
        
        std::cout << "⚠️  Execution interrupted!" << std::endl;
        std::cout << "Reason: " << interrupt_info->reason << std::endl;
        
        // 保存 checkpoint ID 供后续恢复
        std::string checkpoint_id = event->checkpoint_id;  // 假设有此字段
        
        // 等待人工决策/输入...
        // （用户可以稍后调用 Resume）
        
        return;  // 暂停处理
    }
}
```

---

#### **阶段 6：恢复执行（Resume）**

**用户调用**：

```cpp
// 用户稍后恢复
auto resume_info = std::make_shared<ResumeInfo>();
resume_info->enable_streaming = true;
resume_info->interrupt_info = saved_interrupt_info;

auto events = agent->Resume(ctx, resume_info);
// 继续处理 events...
```

**内部实现**：

**Go 代码**：`eino/adk/interrupt.go:55-80`

```go
func getCheckPoint(
    ctx context.Context,
    store compose.CheckPointStore,
    key string,
) (*runContext, *ResumeInfo, bool, error) {
    // 60: 从存储读取
    data, existed, err := store.Get(ctx, key)
    if err != nil {
        return nil, nil, false, fmt.Errorf("failed to get checkpoint: %w", err)
    }
    if !existed {
        return nil, nil, false, nil
    }
    
    // 67-71: 反序列化
    s := &serialization{}
    err = gob.NewDecoder(bytes.NewReader(data)).Decode(s)
    if err != nil {
        return nil, nil, false, fmt.Errorf("failed to decode: %w", err)
    }
    
    // 72-79: 构造 ResumeInfo
    enableStreaming := false
    if s.RunCtx.RootInput != nil {
        enableStreaming = s.RunCtx.RootInput.EnableStreaming
    }
    return s.RunCtx, &ResumeInfo{
        EnableStreaming: enableStreaming,
        InterruptInfo:   s.Info,
    }, true, nil
}
```

**C++ 实现**：`eino_cpp/src/adk/interrupt.cpp:154-194`

```cpp
std::tuple<std::shared_ptr<RunContext>, std::shared_ptr<ResumeInfo>, bool> GetCheckPoint(
    void* ctx,
    compose::CheckPointStore* store,
    const std::string& key) {
    
    if (!store) {
        return {nullptr, nullptr, false};
    }
    
    // 164-175: 从存储读取
    std::vector<uint8_t> data;
    bool existed = false;
    try {
        std::tie(data, existed) = store->Get(ctx, key);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to get checkpoint: " + std::string(e.what()));
    }
    
    if (!existed) {
        return {nullptr, nullptr, false};
    }
    
    // 177-181: 反序列化
    auto [run_ctx_void, interrupt_info, error] = DeserializeCheckPoint(data);
    if (!error.empty()) {
        throw std::runtime_error(error);
    }
    
    // 183-193: 构造 RunContext 和 ResumeInfo
    auto run_ctx = std::make_shared<RunContext>();
    
    auto resume_info = std::make_shared<ResumeInfo>();
    resume_info->enable_streaming = false;
    if (interrupt_info) {
        resume_info->interrupt_info = interrupt_info;
    }
    
    return {run_ctx, resume_info, true};
}
```

**恢复后继续执行**：

**Go 代码**：`eino/compose/graph_run.go:366-399`

```go
func (r *runner) restoreFromCheckPoint(
    ctx context.Context,
    path NodePath,
    sm StateModifier,
    cp *checkpoint,
    isStream bool,
    cm *channelManager,
    optMap map[string][]any,
) (context.Context, []*task, error) {
    // 375: 恢复 checkpoint
    err := r.checkPointer.restoreCheckPoint(cp, isStream)
    if err != nil {
        return ctx, nil, newGraphRunError(fmt.Errorf("restore checkpoint fail: %w", err))
    }

    // 380: 恢复 channels
    err = cm.loadChannels(cp.Channels)
    if err != nil {
        return ctx, nil, newGraphRunError(err)
    }
    
    // 384-388: 恢复 State
    if sm != nil && cp.State != nil {
        err = sm(ctx, path, cp.State)
        if err != nil {
            return ctx, nil, newGraphRunError(fmt.Errorf("state modifier fail: %w", err))
        }
    }
    if cp.State != nil {
        ctx = context.WithValue(ctx, stateKey{}, &internalState{state: cp.State})
    }

    // 394: 恢复 Tasks
    nextTasks, err := r.restoreTasks(ctx, cp.Inputs, cp.SkipPreHandler, 
        cp.ToolsNodeExecutedTools, cp.RerunNodes, isStream, optMap)
    if err != nil {
        return ctx, nil, newGraphRunError(fmt.Errorf("restore tasks fail: %w", err))
    }
    return ctx, nextTasks, nil
}
```

---

## 使用示例

### 示例 1：人工审批工作流

```cpp
#include "eino/adk/chat_model_agent.h"
#include "eino/adk/types.h"
#include "eino/compose/checkpoint.h"
#include <iostream>

using namespace eino::adk;

int main() {
    // ========== 初始设置 ==========
    
    auto ctx = CreateContext();
    auto chat_model = CreateMockChatModel();  // 假设的模型
    
    // 创建 Agent
    auto agent_config = std::make_shared<ChatModelAgentConfig>();
    agent_config->name = "ApprovalAgent";
    agent_config->model = chat_model;
    agent_config->output_key = "approval_decision";
    
    auto agent = NewChatModelAgent(ctx, agent_config);
    
    // 创建 CheckPoint Store
    auto checkpoint_store = compose::NewMemoryCheckPointStore();
    std::string checkpoint_id = "approval_workflow_001";
    
    // ========== 初次执行 ==========
    
    auto input = std::make_shared<AgentInput>();
    input->messages = {
        CreateMessage(schema::RoleType::User, "删除生产数据库 users 表？")
    };
    input->enable_streaming = false;
    
    std::cout << "🚀 Starting approval workflow..." << std::endl;
    
    auto events = agent->Run(ctx, input, {
        WithCheckPointID(checkpoint_id),
        WithCheckPointStore(checkpoint_store),
    });
    
    bool interrupted = false;
    std::shared_ptr<InterruptInfo> saved_interrupt_info;
    
    for (auto event : events) {
        if (event->HasError()) {
            std::cerr << "❌ Error: " << event->error_msg << std::endl;
            return 1;
        }
        
        // ✅ 检测中断
        if (event->action && event->action->interrupted) {
            interrupted = true;
            saved_interrupt_info = event->action->interrupted;
            
            std::cout << "\n⚠️  ==================== INTERRUPT ====================" << std::endl;
            std::cout << "类型: " << static_cast<int>(saved_interrupt_info->interrupt_type) << std::endl;
            std::cout << "原因: " << saved_interrupt_info->reason << std::endl;
            std::cout << "状态键: " << saved_interrupt_info->state_key << std::endl;
            std::cout << "===================================================\n" << std::endl;
            
            // 保存 checkpoint 已自动完成
            break;
        }
    }
    
    if (!interrupted) {
        std::cout << "✅ Workflow completed without interruption" << std::endl;
        return 0;
    }
    
    // ========== 模拟人工审批 ==========
    
    std::cout << "等待管理员审批..." << std::endl;
    std::cout << "输入 'approve' 批准，'reject' 拒绝: ";
    
    std::string decision;
    std::cin >> decision;
    
    // ========== 恢复执行 ==========
    
    std::cout << "\n🔄 Resuming workflow with decision: " << decision << std::endl;
    
    // 加载 checkpoint
    auto [run_ctx, resume_info, existed] = GetCheckPoint(ctx, checkpoint_store.get(), checkpoint_id);
    if (!existed) {
        std::cerr << "❌ CheckPoint not found!" << std::endl;
        return 1;
    }
    
    // 注入人工决策
    resume_info->interrupt_info->context["user_decision"] = decision;
    
    // 恢复执行
    auto resume_events = agent->Resume(ctx, resume_info, {
        WithCheckPointStore(checkpoint_store),
    });
    
    for (auto event : resume_events) {
        if (event->HasError()) {
            std::cerr << "❌ Resume Error: " << event->error_msg << std::endl;
            return 1;
        }
        
        if (event->output && event->output->message_output) {
            auto msg = event->output->message_output->message;
            std::cout << "✅ Final Output: " << msg->content << std::endl;
        }
    }
    
    std::cout << "\n🎉 Workflow completed successfully!" << std::endl;
    return 0;
}
```

**预期输出**：

```
🚀 Starting approval workflow...

⚠️  ==================== INTERRUPT ====================
类型: 0
原因: Waiting for human approval on sensitive operation
状态键: approval_checkpoint_1
===================================================

等待管理员审批...
输入 'approve' 批准，'reject' 拒绝: approve

🔄 Resuming workflow with decision: approve
✅ Final Output: Operation approved. Proceeding with database deletion...

🎉 Workflow completed successfully!
```

---

### 示例 2：Graph 节点中断

```cpp
#include "eino/compose/graph.h"
#include "eino/compose/interrupt.h"

using namespace eino::compose;

// 定义节点函数
std::shared_ptr<void> StepA(void* ctx, std::shared_ptr<void> input) {
    std::cout << "✓ Step A: Data collection" << std::endl;
    return std::make_shared<std::string>("data_from_A");
}

std::shared_ptr<void> StepB(void* ctx, std::shared_ptr<void> input) {
    std::cout << "✓ Step B: Data processing" << std::endl;
    return std::make_shared<std::string>("processed_data");
}

std::shared_ptr<void> StepC(void* ctx, std::shared_ptr<void> input) {
    std::cout << "✓ Step C: Critical operation (needs approval)" << std::endl;
    return std::make_shared<std::string>("critical_result");
}

int main() {
    auto ctx = CreateContext();
    
    // ========== 构建 Graph ==========
    
    auto graph_builder = NewGraphBuilder();
    
    graph_builder->AddNode("step_a", StepA);
    graph_builder->AddNode("step_b", StepB);
    graph_builder->AddNode("step_c", StepC);  // 🔑 关键节点
    
    graph_builder->AddEdge("step_a", "step_b");
    graph_builder->AddEdge("step_b", "step_c");
    
    // ========== 配置中断 ==========
    
    auto graph = graph_builder->Compile({
        WithInterruptBeforeNodes({"step_c"}),  // 🚨 在 step_c 前中断
    });
    
    auto checkpoint_store = NewMemoryCheckPointStore();
    
    // ========== 执行 Graph ==========
    
    auto input = std::make_shared<std::string>("initial_input");
    
    try {
        auto result = graph->Invoke(ctx, input, {
            WithCheckPointStore(checkpoint_store),
            WithCheckPointID("graph_checkpoint_001"),
        });
        
        std::cout << "✅ Graph completed without interrupt" << std::endl;
        
    } catch (const InterruptError& e) {
        // ✅ 捕获中断错误
        std::cout << "\n⚠️  Graph interrupted!" << std::endl;
        std::cout << "Info: " << e.what() << std::endl;
        
        auto interrupt_info = e.GetInterruptInfo();
        std::cout << "Before Nodes: ";
        for (const auto& node : interrupt_info->BeforeNodes) {
            std::cout << node << " ";
        }
        std::cout << std::endl;
        
        // ========== 模拟人工审批 ==========
        
        std::cout << "\n等待审批..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "✅ 审批通过！" << std::endl;
        
        // ========== 恢复执行 ==========
        
        auto checkpoint_data = checkpoint_store->Get(ctx, "graph_checkpoint_001");
        
        auto resume_result = graph->Resume(ctx, checkpoint_data.first, {
            WithCheckPointStore(checkpoint_store),
        });
        
        std::cout << "\n✅ Graph resumed and completed successfully!" << std::endl;
    }
    
    return 0;
}
```

**预期输出**：

```
✓ Step A: Data collection
✓ Step B: Data processing

⚠️  Graph interrupted!
Info: interrupt happened, before nodes: [step_c]
Before Nodes: step_c 

等待审批...
✅ 审批通过！

✓ Step C: Critical operation (needs approval)

✅ Graph resumed and completed successfully!
```

---

### 示例 3：InterruptAndRerun 机制

**场景**：工具调用失败后，需要重新运行节点。

```cpp
// 工具节点实现
class WebSearchTool : public Tool {
public:
    std::shared_ptr<void> Invoke(void* ctx, std::shared_ptr<void> input) override {
        // 模拟网络错误
        if (should_retry_) {
            std::cout << "⚠️  Network error, requesting rerun..." << std::endl;
            
            // 🔑 抛出 InterruptAndRerun 错误
            throw InterruptAndRerunError("Network timeout", {
                {"retry_count", retry_count_++},
                {"last_error", "Connection timeout after 5s"},
            });
        }
        
        std::cout << "✅ Search succeeded on retry #" << retry_count_ << std::endl;
        return std::make_shared<std::string>("Search results...");
    }
    
private:
    int retry_count_ = 0;
    bool should_retry_ = true;
};

// Graph 执行逻辑
void runGraphWithRetry() {
    auto ctx = CreateContext();
    auto graph = buildSearchGraph();  // 包含 WebSearchTool
    
    auto checkpoint_store = NewMemoryCheckPointStore();
    
    try {
        auto result = graph->Invoke(ctx, input, {
            WithCheckPointStore(checkpoint_store),
        });
    } catch (const InterruptError& e) {
        auto interrupt_info = e.GetInterruptInfo();
        
        std::cout << "Rerun Nodes: ";
        for (const auto& node : interrupt_info->RerunNodes) {
            std::cout << node << " ";
        }
        std::cout << std::endl;
        
        // 自动恢复并重试
        auto result = graph->Resume(ctx, checkpoint_data);
        std::cout << "✅ Succeeded after retry!" << std::endl;
    }
}
```

---

## 最佳实践

### ✅ DO

1. **明确中断点**
   ```cpp
   // 在敏感操作前设置中断
   WithInterruptBeforeNodes({"delete_data", "send_email", "deploy_prod"})
   ```

2. **保存详细上下文**
   ```cpp
   interrupt_info->context = {
       {"operation", "database_delete"},
       {"affected_rows", 1000},
       {"timestamp", current_time()},
   };
   ```

3. **使用 CheckPointStore**
   ```cpp
   // 生产环境使用持久化存储
   auto store = NewRedisCheckPointStore(redis_config);
   // 或 NewPostgresCheckPointStore(pg_config);
   ```

4. **异常处理**
   ```cpp
   try {
       agent->Run(ctx, input);
   } catch (const InterruptError& e) {
       // 捕获并处理中断
       saveInterruptInfoToDatabase(e.GetInterruptInfo());
   }
   ```

---

### ❌ DON'T

1. **不要在循环内设置过多中断点**
   ```cpp
   // ❌ 错误：每次迭代都中断
   for (int i = 0; i < 1000; i++) {
       WithInterruptBeforeNodes({"process_item_" + std::to_string(i)})
   }
   
   // ✅ 正确：关键节点中断
   WithInterruptBeforeNodes({"validate_batch", "commit_changes"})
   ```

2. **不要忽略序列化限制**
   ```cpp
   // ❌ 错误：无法序列化的类型
   interrupt_info->extra["callback"] = lambda_function;  // Lambda 无法序列化
   
   // ✅ 正确：使用可序列化数据
   interrupt_info->extra["callback_id"] = "process_callback_001";
   ```

3. **不要假设恢复会立即发生**
   ```cpp
   // ❌ 错误：依赖时序
   auto start_time = std::chrono::now();
   // ... 中断 ...
   // Resume 时 start_time 已失效
   
   // ✅ 正确：使用绝对时间戳
   interrupt_info->extra["start_timestamp"] = getCurrentTimestamp();
   ```

---

## 高级主题

### 🔄 嵌套 Graph 中断

```cpp
// 父 Graph
auto parent_graph = parent_builder->Compile({
    WithInterruptBeforeNodes({"critical_subgraph"}),
});

// 子 Graph
auto child_graph = child_builder->Compile({
    WithInterruptBeforeNodes({"sensitive_operation"}),
});

// 执行时：
// 1. 父 Graph 在 critical_subgraph 前中断
// 2. 恢复后，子 Graph 在 sensitive_operation 前中断
// 3. InterruptInfo 中包含 SubGraphs 字段记录层级
```

**数据结构**：

```cpp
InterruptInfo {
    BeforeNodes: ["critical_subgraph"],
    SubGraphs: {
        "critical_subgraph": InterruptInfo {
            BeforeNodes: ["sensitive_operation"],
            State: {...},
        }
    }
}
```

---

### 🔁 Workflow 中断恢复

**Sequential Agent**：

```cpp
WorkflowInterruptInfo {
    orig_input: original_agent_input,
    sequential_interrupt_index: 2,  // 第3个子 Agent 中断
    sequential_interrupt_info: sub_agent_interrupt,
}
```

**Loop Agent**：

```cpp
WorkflowInterruptInfo {
    orig_input: original_agent_input,
    loop_iterations: 5,  // 第5次迭代时中断
}
```

**Parallel Agent**：

```cpp
WorkflowInterruptInfo {
    orig_input: original_agent_input,
    parallel_interrupt_info: {
        0: interrupt_from_agent_0,
        2: interrupt_from_agent_2,
        // Agent 1 和 3 未中断
    }
}
```

---

## 总结

### 🎯 核心要点

| 概念 | 说明 |
|------|------|
| **InterruptInfo** | 中断元信息（类型、原因、状态键） |
| **AgentAction.interrupted** | Agent 中断信号 |
| **CheckPoint** | 执行状态快照（State + Inputs + Channels） |
| **ResumeInfo** | 恢复所需信息（流式标志 + 中断信息） |
| **CheckPointStore** | 持久化存储接口（Get/Set） |

### 🔄 完整生命周期

```
Run → Detect Interrupt → Save CheckPoint → User Handles → Resume → Continue
  ↓         ↓                   ↓                ↓            ↓         ↓
Input   Condition         Serialize         Decision    Restore   Complete
```

### 📚 相关文件

**C++ 核心文件**：
- `eino_cpp/include/eino/adk/types.h` - 数据结构定义
- `eino_cpp/include/eino/adk/interrupt.h` - 中断接口
- `eino_cpp/src/adk/interrupt.cpp` - 序列化/存储实现
- `eino_cpp/include/eino/compose/checkpoint.h` - CheckPoint 定义

**Go 参考实现**：
- `eino/adk/interrupt.go` - ADK 中断逻辑
- `eino/compose/interrupt.go` - Compose 中断类型
- `eino/compose/graph_run.go` - Graph 执行中断检测
- `eino/adk/workflow.go` - Workflow 中断处理

---

**现在你已经完全掌握了 eino_cpp ADK Interrupt 的完整链路！** 🎉

需要进一步了解某个特定场景或实现细节吗？
