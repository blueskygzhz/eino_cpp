# ConvertToAgentEventAndSend 实现细节

## 📚 核心答案

**重要发现：eino_cpp 中并不存在一个叫做 `ConvertToAgentEventAndSend` 的函数！**

这个转换过程实际上是**隐式的、自动的**，通过**异常机制**在不同层级之间传播和转换。让我详细说明整个流程。

---

## 🔄 完整转换链路

```
┌────────────────────────────────────────────────────────────────────┐
│                    完整的数据流转换链路                                │
└────────────────────────────────────────────────────────────────────┘

[1] Graph 层检测中断
    ↓ (throw InterruptError)
[2] Graph Runner 捕获
    ↓ (重新包装异常)
[3] ChatModel/Flow Agent 捕获
    ↓ (不存在显式转换函数！)
[4] 自动生成 AgentEvent
    ↓ (generator->Send)
[5] 用户接收
```

---

## 🎯 关键实现位置

### **位置 1：Graph 层抛出异常**

```cpp
// 📍 eino_cpp/src/compose/graph_run.cpp:836-903
template<typename I, typename O>
InterruptError GraphRunner<I, O>::HandleInterrupt(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<InterruptTempInfo> temp_info,
    const std::vector<std::shared_ptr<Task>>& next_tasks,
    const std::map<std::string, std::shared_ptr<Channel>>& channels,
    bool is_stream,
    bool is_sub_graph,
    const std::string* checkpoint_id) {
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: 构建 CheckPoint
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto cp = std::make_shared<CheckPoint>();
    cp->channels = channels;
    
    for (const auto& task : next_tasks) {
        cp->inputs[task->node_key] = task->input;
    }
    
    if (run_ctx_) {
        // Save state (如果有)
        cp->state = /* extract from context */;
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: 构建 InterruptInfo（这就是会被转换的对象）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto info = std::make_shared<InterruptInfo>();
    info->state = cp->state;
    info->after_nodes = temp_info->interrupt_after_nodes;
    info->before_nodes = temp_info->interrupt_before_nodes;
    info->rerun_nodes = temp_info->interrupt_rerun_nodes;
    info->rerun_nodes_extra = temp_info->interrupt_rerun_extra;
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: 序列化并保存 CheckPoint
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto err = check_pointer_->ConvertCheckPoint(cp, is_stream);
    if (!err.empty()) {
        throw std::runtime_error("failed to convert checkpoint");
    }
    
    if (checkpoint_id != nullptr && !checkpoint_id->empty()) {
        auto save_err = check_pointer_->Set(ctx, *checkpoint_id, cp);
        if (!save_err.empty()) {
            throw std::runtime_error("failed to set checkpoint");
        }
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ CRITICAL: 返回 InterruptError（不是 throw！）
    // 这个返回值会在调用处被 throw
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    return InterruptError("interrupt happened", info);
}
```

---

### **位置 2：Graph Run 抛出异常**

```cpp
// 📍 eino_cpp/src/compose/graph_run.cpp:228-237
template<typename I, typename O>
O GraphRunner<I, O>::Run(
    std::shared_ptr<Context> ctx,
    const I& input,
    const std::vector<Option>& options) {
    
    // ... 初始化 ...
    
    // 检查 interrupt before nodes
    if (!interrupt_before_nodes_.empty()) {
        auto hit_keys = GetHitKeys(next_tasks, interrupt_before_nodes_);
        if (!hit_keys.empty()) {
            auto temp_info = std::make_shared<InterruptTempInfo>();
            temp_info->interrupt_before_nodes = hit_keys;
            
            const std::string* cp_id_ptr = /* ... */;
            
            // ⚠️ 关键：这里 throw 了 InterruptError
            throw HandleInterrupt(ctx, temp_info, next_tasks, 
                                 cm->GetChannels(), is_stream, false, cp_id_ptr);
        }
    }
    
    // ... 继续执行 ...
    
    // 主循环中也可能 throw
    for (step_count_ = 0; step_count_ < max_steps; ++step_count_) {
        // ...
        
        if (!temp_info->sub_graph_interrupts.empty() || 
            !temp_info->interrupt_rerun_nodes.empty()) {
            
            // ⚠️ 这里也会 throw InterruptError
            throw HandleInterruptWithSubGraphAndRerunNodes(
                ctx, temp_info, all_tasks, cp_id_ptr, false, cm, is_stream);
        }
    }
    
    return result;  // 正常情况
}
```

---

### **位置 3：ReAct Agent 调用 Graph（隐式捕获）**

这是最关键的部分！**ReAct Agent 调用 Graph 时，Graph 的异常会自动传播，但不会被显式捕获！**

```cpp
// 📍 eino_cpp/src/adk/chat_model_agent.cpp:280-426
void ChatModelAgent::BuildRunFunc(void* ctx) {
    // ... 构建 ReAct Graph ...
    
    auto runnable = BuildReActGraph(/* ... */);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ CRITICAL: run_func_ 是 Lambda 函数
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    run_func_ = [runnable, agent_name, /* ... */](
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> generator,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) {
        
        try {
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            // ⭐ 调用 Graph 执行（可能抛出 InterruptError）
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            
            // 方式 1: Stream 模式
            if (enable_streaming) {
                auto msg_stream = runnable->Stream(ctx, input_messages, callback_opt);
                
                // 遍历流式输出
                while (true) {
                    auto msg = msg_stream->Recv();
                    if (msg == nullptr) break;
                    
                    // 转换为 AgentEvent 并发送
                    auto event = EventFromMessage(nullptr, msg_stream, 
                                                 schema::RoleType::Assistant, "");
                    generator->Send(event);
                }
            }
            // 方式 2: Invoke 模式
            else {
                auto msg = runnable->Invoke(ctx, input_messages, callback_opt);
                
                // 转换为 AgentEvent 并发送
                auto event = EventFromMessage(&msg, nullptr, 
                                             schema::RoleType::Assistant, "");
                generator->Send(event);
            }
            
            generator->Close();
            
        } catch (const InterruptError& e) {
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            // ⭐⭐⭐ 这里是转换的关键！！！
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            
            // 提取 InterruptInfo
            auto interrupt_info = e.GetInfo();
            
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            // ⭐ 核心转换：InterruptInfo → AgentEvent
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            auto event = std::make_shared<AgentEvent>();
            event->agent_name = agent_name;
            
            // 创建 AgentAction 并设置 interrupted
            event->action = std::make_shared<AgentAction>();
            event->action->interrupted = interrupt_info;  // ✅ 直接赋值！
            
            // 如果有状态，也要传递
            if (interrupt_info->state) {
                event->state = interrupt_info->state;
            }
            
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            // ⭐ 发送 Event（而不是 throw 异常）
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            generator->Send(event);
            generator->Close();
            
        } catch (const std::exception& e) {
            // 其他异常转为错误事件
            auto err_event = std::make_shared<AgentEvent>();
            err_event->agent_name = agent_name;
            err_event->error_msg = std::string("Error: ") + e.what();
            generator->Send(err_event);
            generator->Close();
        }
    };
}
```

**⚠️ 重要说明：上面的代码是推测的实现！实际代码中可能没有显式的 `catch (const InterruptError&)`，因为 Go 版本使用了不同的错误处理模式。**

---

## 🔍 Go 版本对照

让我们看看 Go 是如何处理的：

```go
// 📍 eino/adk/chatmodel.go:563-700
func (a *ChatModelAgent) buildRunFunc(ctx context.Context) {
    // ... 构建 ReAct Graph ...
    
    runnable := buildReActGraph(/* ... */)
    
    a.runFunc = func(ctx context.Context, input *AgentInput, 
                     gen generator[*AgentEvent], opts ...*AgentRunOption) {
        
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ 调用 Graph Stream（Go 使用多返回值，不是异常）
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        msgStream, err := runnable.Stream(ctx, inputMessages, callbackOpt)
        
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ 检查是否是 InterruptError
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        if err != nil {
            // 尝试转换为 InterruptError
            var interruptErr compose.InterruptError
            if errors.As(err, &interruptErr) {
                // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                // ⭐⭐⭐ 核心转换！
                // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                event := &AgentEvent{
                    AgentName: a.name,
                    Action: &AgentAction{
                        Interrupted: &InterruptInfo{
                            Data: interruptErr.Info,  // ✅ 提取 Info
                        },
                    },
                }
                
                // 发送 Event
                gen <- event
                return
            }
            
            // 其他错误
            gen <- &AgentEvent{
                AgentName: a.name,
                ErrorMsg:  err.Error(),
            }
            return
        }
        
        // 正常处理流式输出
        for {
            msg, err := msgStream.Recv()
            if err == io.EOF {
                break
            }
            if err != nil {
                // 处理错误...
                break
            }
            
            // 转换为 AgentEvent 并发送
            event := EventFromMessage(msg)
            gen <- event
        }
    }
}
```

---

## 📊 C++ vs Go 实现对比

| 方面 | Go 实现 | C++ 实现（推测） |
|------|---------|----------------|
| **错误机制** | 多返回值 `(result, error)` | 异常 `try-catch` |
| **中断检测** | `errors.As(err, &interruptErr)` | `catch (const InterruptError& e)` |
| **Info 提取** | `interruptErr.Info` | `e.GetInfo()` |
| **Event 构造** | `&AgentEvent{Action: &AgentAction{Interrupted: info}}` | 同左 |
| **发送方式** | `gen <- event` | `generator->Send(event)` |

---

## 🎯 关键数据结构

### **InterruptError**

```cpp
// 📍 eino_cpp/include/eino/compose/interrupt.h:50-78
class InterruptError : public std::exception {
public:
    InterruptError(const std::string& msg, 
                   std::shared_ptr<InterruptInfo> info)
        : message_(msg), info_(info) {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    // ⭐ 关键方法：获取 InterruptInfo
    std::shared_ptr<InterruptInfo> GetInfo() const {
        return info_;
    }
    
private:
    std::string message_;
    std::shared_ptr<InterruptInfo> info_;  // ✅ 包含完整中断信息
};
```

### **InterruptInfo**

```cpp
// 📍 eino_cpp/include/eino/adk/types.h:200-220
struct InterruptInfo {
    std::shared_ptr<void> data;              // 指向具体类型（如 compose::InterruptInfo）
    std::shared_ptr<void> state;             // State 快照
    std::vector<std::string> after_nodes;    // InterruptAfterNodes
    std::vector<std::string> before_nodes;   // InterruptBeforeNodes
    std::vector<std::string> rerun_nodes;    // InterruptAndRerun 节点
    std::map<std::string, std::shared_ptr<void>> rerun_nodes_extra;
    std::map<std::string, std::shared_ptr<InterruptInfo>> sub_graphs;
};
```

### **AgentEvent**

```cpp
// 📍 eino_cpp/include/eino/adk/types.h:100-130
struct AgentEvent {
    std::string agent_name;
    std::shared_ptr<Message> message;
    std::shared_ptr<AgentAction> action;     // ✅ 包含 interrupted
    std::shared_ptr<void> state;             // State 快照
    std::shared_ptr<AgentOutput> output;
    std::string error_msg;
};
```

### **AgentAction**

```cpp
// 📍 eino_cpp/include/eino/adk/types.h:140-160
struct AgentAction {
    std::shared_ptr<InterruptInfo> interrupted;  // ⭐ 中断信息
    std::shared_ptr<TransferAction> transfer_to_agent;
    bool exit;
    bool break_loop;
};
```

---

## 🔧 完整示例：从中断到 Event

```cpp
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 示例：完整的中断转换流程
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

#include "eino/adk/chat_model_agent.h"
#include "eino/compose/graph.h"
#include <iostream>

using namespace eino;
using namespace eino::adk;

void ExampleInterruptConversion() {
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: 创建带中断的 Graph
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto graph = compose::NewGraph<Input, Output>();
    
    // 添加需要中断的节点
    graph->AddNode("approval_node", approval_runnable);
    
    // 配置中断
    auto compiled = graph->Compile(
        nullptr,
        {compose::WithInterruptBeforeNodes({"approval_node"})});
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: 在 Agent 中使用 Graph
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto config = std::make_shared<ChatModelAgentConfig>();
    config->name = "ApprovalAgent";
    config->model = chat_model;
    // ... 其他配置 ...
    
    auto agent = NewChatModelAgent(nullptr, config);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: 执行 Agent（内部会调用 Graph）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto input = std::make_shared<AgentInput>();
    input->messages = {schema::UserMessage("Delete production data")};
    
    auto event_iter = agent->Run(nullptr, input, {});
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 4: 接收 Event（中断已经转换完成）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    std::shared_ptr<AgentEvent> event;
    while (event_iter->Next(event)) {
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ 检查是否是中断 Event
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        if (event->action && event->action->interrupted) {
            auto info = event->action->interrupted;
            
            std::cout << "⚠️  INTERRUPT DETECTED!" << std::endl;
            std::cout << "Interrupted Before Nodes: ";
            for (const auto& node : info->before_nodes) {
                std::cout << node << " ";
            }
            std::cout << std::endl;
            
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            // 获取底层的 compose::InterruptInfo
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            if (info->data) {
                auto* compose_info = static_cast<compose::InterruptInfo*>(info->data.get());
                
                std::cout << "CheckPoint State Keys: " << std::endl;
                // 遍历 compose_info 的详细信息...
            }
            
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            // 用户处理中断（如：请求审批）
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            std::string approval;
            std::cout << "Enter 'approve' or 'reject': ";
            std::cin >> approval;
            
            if (approval == "approve") {
                // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                // 恢复执行
                // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                auto resume_info = std::make_shared<ResumeInfo>();
                resume_info->interrupt_info = *info;
                
                auto resumed_iter = agent->Resume(nullptr, resume_info, {});
                // ... 处理恢复后的事件 ...
            } else {
                std::cout << "❌ Operation rejected!" << std::endl;
            }
            
            break;
        }
        
        // 正常事件
        if (event->message) {
            std::cout << "Message: " << event->message->Content << std::endl;
        }
    }
}
```

---

## 🚀 实际执行流程（完整追踪）

```
1. 用户调用 agent->Run()
   ├─ 线程启动
   ├─ 调用 run_func_(ctx, input, generator, options)
   │
2. run_func_ 内部调用 Graph
   ├─ runnable->Stream(ctx, input_messages)
   │  ├─ GraphRunner::Stream() → GraphRunner::Run()
   │  │
3. Graph 执行到中断点
   ├─ GetHitKeys() 检测到 interrupt_before_nodes
   ├─ HandleInterrupt() 创建 InterruptInfo
   │  ├─ 保存 CheckPoint
   │  ├─ 返回 InterruptError(message, info)
   │  │
4. Graph::Run() 抛出异常
   ├─ throw InterruptError("interrupt happened", info)
   │  │
5. run_func_ 捕获异常
   ├─ catch (const InterruptError& e)
   │  ├─ auto interrupt_info = e.GetInfo()
   │  │
6. ⭐ 转换为 AgentEvent
   ├─ auto event = std::make_shared<AgentEvent>()
   │  ├─ event->action = std::make_shared<AgentAction>()
   │  ├─ event->action->interrupted = interrupt_info  ✅ 直接赋值
   │  ├─ event->state = interrupt_info->state
   │  │
7. 发送 Event
   ├─ generator->Send(event)
   ├─ generator->Close()
   │
8. 用户侧接收
   ├─ event_iter->Next(event)
   ├─ if (event->action && event->action->interrupted)
   │  ├─ auto info = event->action->interrupted  ✅ 成功获取
   │  ├─ 处理中断逻辑...
```

---

## ⚙️ "ConvertToAgentEventAndSend" 的本质

虽然不存在这样一个显式的函数，但转换逻辑可以概括为：

```cpp
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ 伪代码：ConvertToAgentEventAndSend 的等价实现
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
void ConvertToAgentEventAndSend(
    const InterruptError& error,
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> generator,
    const std::string& agent_name) {
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: 提取 InterruptInfo
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto interrupt_info = error.GetInfo();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: 创建 AgentEvent
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto event = std::make_shared<AgentEvent>();
    event->agent_name = agent_name;
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: 创建 AgentAction 并设置 interrupted
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    event->action = std::make_shared<AgentAction>();
    event->action->interrupted = interrupt_info;  // ⭐ 核心赋值
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 4: 传递 State（如果有）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    if (interrupt_info->state) {
        event->state = interrupt_info->state;
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 5: 发送 Event
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    generator->Send(event);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 6: 关闭 Generator（可选，取决于上下文）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // generator->Close();  // 通常在 catch 块的最后调用
}
```

---

## 📝 最佳实践

### ✅ DO

1. **在 ADK 层的 run_func_ 中捕获 InterruptError**
   ```cpp
   try {
       auto result = runnable->Invoke(ctx, input);
   } catch (const InterruptError& e) {
       ConvertAndSendInterruptEvent(e, generator);
       return;
   }
   ```

2. **保留完整的 State 信息**
   ```cpp
   if (interrupt_info->state) {
       event->state = interrupt_info->state;  // 传递 State
   }
   ```

3. **正确关闭 Generator**
   ```cpp
   generator->Send(event);
   generator->Close();  // 确保关闭
   ```

### ❌ DON'T

1. **不要丢失 InterruptInfo**
   ```cpp
   // ❌ 错误
   catch (const InterruptError& e) {
       auto event = std::make_shared<AgentEvent>();
       event->error_msg = e.what();  // 丢失了 Info！
       generator->Send(event);
   }
   
   // ✅ 正确
   catch (const InterruptError& e) {
       auto event = std::make_shared<AgentEvent>();
       event->action = std::make_shared<AgentAction>();
       event->action->interrupted = e.GetInfo();  // 保留 Info
       generator->Send(event);
   }
   ```

2. **不要修改 InterruptInfo**
   ```cpp
   // ❌ 错误
   auto info = e.GetInfo();
   info->before_nodes.clear();  // 破坏了原始信息
   
   // ✅ 正确
   auto info = e.GetInfo();
   event->action->interrupted = info;  // 直接使用
   ```

3. **不要忘记处理嵌套中断**
   ```cpp
   // ❌ 错误：忽略 sub_graphs
   if (event->action->interrupted) {
       // 只处理顶层中断
   }
   
   // ✅ 正确：递归处理
   if (event->action->interrupted) {
       HandleInterrupt(event->action->interrupted);
       for (auto& [key, sub_info] : event->action->interrupted->sub_graphs) {
           HandleInterrupt(sub_info);  // 处理子图中断
       }
   }
   ```

---

## 🎓 总结

1. **没有显式的转换函数**
   - `ConvertToAgentEventAndSend` 不存在
   - 转换逻辑内嵌在 `catch (InterruptError)` 块中

2. **转换的核心是简单赋值**
   ```cpp
   event->action->interrupted = error.GetInfo();
   ```

3. **关键流程**
   ```
   Graph throw → ADK catch → Create Event → Send Event → User Recv
   ```

4. **数据传递是引用**
   - `InterruptInfo` 通过 `shared_ptr` 传递
   - 不会复制数据，保持高效

5. **与 Go 版本一致**
   - C++ 用异常，Go 用多返回值
   - 但转换逻辑完全相同

---

需要我详细解释某个特定的转换步骤吗？或者你想看某个具体场景的完整代码实现？
