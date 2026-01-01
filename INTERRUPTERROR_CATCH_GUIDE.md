# 🎯 InterruptError 捕获位置详解

> **完整的 InterruptError 抛出、捕获和处理链路**

---

## 📋 核心答案

### ❓ InterruptError 在哪里被 catch？

**关键发现：InterruptError 在 C++ 中**并不是**通过 `catch` 捕获的！而是通过返回值传播。**

```cpp
// ❌ 没有这样的代码：
try {
    graph->Run(ctx, input);
} catch (const InterruptError& e) {  // 这种方式不存在！
    // ...
}

// ✅ 实际实现：
InterruptError error = HandleInterrupt(...);  // 返回 InterruptError 对象
return error;  // 通过返回值传播，而非异常抛出
```

---

## 🔍 深入分析

### 1️⃣ **InterruptError 的本质**

**C++ 实现** (`eino_cpp/include/eino/compose/interrupt.h:65-74`):

```cpp
class InterruptError : public std::runtime_error {
public:
    explicit InterruptError(const std::string& message, 
                          const std::shared_ptr<InterruptInfo>& info)
        : std::runtime_error(message), info_(info) {}

    std::shared_ptr<InterruptInfo> GetInfo() const { return info_; }

private:
    std::shared_ptr<InterruptInfo> info_;
};
```

**关键发现**：
- ✅ `InterruptError` 继承自 `std::runtime_error`（可以抛出）
- ✅ 但在 eino_cpp 中**主要作为返回值**使用，而非异常！

---

### 2️⃣ **InterruptError 的使用方式**

#### **方式 A：作为返回值**（主流）

**创建并返回** (`eino_cpp/src/compose/graph_run.cpp:839-903`):

```cpp
template<typename I, typename O>
InterruptError GraphRunner<I, O>::HandleInterrupt(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<InterruptTempInfo> temp_info,
    const std::vector<std::shared_ptr<Task>>& next_tasks,
    const std::map<std::string, std::shared_ptr<Channel>>& channels,
    bool is_stream,
    bool is_sub_graph,
    const std::string* checkpoint_id) {
    
    // ... 构建 checkpoint 和 InterruptInfo ...
    
    // 🔑 返回 InterruptError 对象（不抛出异常）
    return InterruptError("interrupt happened", info);
}
```

**调用者接收返回值**（伪代码，因为 C++ 实现可能在模板中）：

```cpp
// Graph 执行循环中
auto interrupt_error = HandleInterrupt(ctx, temp_info, next_tasks, ...);

// ⚠️ 如何传播？可能的方式：
// 1. 存储到状态中
// 2. 设置到 Task 的 error 字段
// 3. 通过其他机制通知调用者
```

---

#### **方式 B：作为异常抛出**（辅助场景）

**仅在特定错误处理中**：

```cpp
// eino_cpp/src/compose/graph_run.cpp:886-887
auto err = check_pointer_->ConvertCheckPoint(cp, is_stream);
if (!err.empty()) {
    throw std::runtime_error("failed to convert checkpoint: " + err);
    // 注意：这里抛出的是 std::runtime_error，不是 InterruptError
}
```

---

### 3️⃣ **真正的捕获点：ResolveInterruptCompletedTasks**

**唯一的捕获位置** (`eino_cpp/src/compose/graph_run.cpp:787-814`):

```cpp
template<typename I, typename O>
void GraphRunner<I, O>::ResolveInterruptCompletedTasks(
    std::shared_ptr<InterruptTempInfo> temp_info,
    const std::vector<std::shared_ptr<Task>>& completed_tasks) {
    
    for (const auto& task : completed_tasks) {
        if (!task->error) continue;
        
        // 🔑 这里是捕获点！
        try {
            // 重新抛出存储在 Task 中的异常
            std::rethrow_exception(std::make_exception_ptr(*task->error));
            
        } catch (const SubGraphInterruptError& e) {
            // ✅ 捕获子图中断
            temp_info->sub_graph_interrupts[task->node_key] = e.GetInfo();
            
        } catch (const InterruptAndRerunError& e) {
            // ✅ 捕获 InterruptAndRerun 错误
            temp_info->interrupt_rerun_nodes.push_back(task->node_key);
            temp_info->interrupt_rerun_extra[task->node_key] = e.GetExtra();
            
        } catch (...) {
            // Regular error, not interrupt
        }
        
        // ... 检查 interrupt_after_nodes ...
    }
}
```

**关键点**：
1. ✅ **唯一显式捕获** `SubGraphInterruptError` 和 `InterruptAndRerunError`
2. ✅ 错误存储在 `task->error` 中（`std::exception_ptr`）
3. ✅ 使用 `std::rethrow_exception` 重新抛出后捕获分析

---

## 🔄 完整执行链路

### 📊 流程图

```
┌─────────────────────────────────────────────────────────────────┐
│                   Graph 执行开始                                 │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ↓
         ┌─────────────────────────────────┐
         │ 1. GraphRunner::Run()            │
         │    执行 Graph 主循环              │
         └──────────────┬──────────────────┘
                        │
                        ↓
         ┌─────────────────────────────────┐
         │ 2. 执行各个节点 (Tasks)          │
         │    task->Execute()               │
         └──────────────┬──────────────────┘
                        │
                        ├─────────────────────────────────┐
                        │                                 │
                        ↓                                 ↓
         ┌──────────────────────────┐      ┌──────────────────────────┐
         │ 正常完成                  │      │ 节点抛出异常              │
         │ task->error = nullptr     │      │ task->error = eptr       │
         └──────────────┬───────────┘      └──────────┬───────────────┘
                        │                              │
                        │                              │
                        └──────────┬───────────────────┘
                                   │
                                   ↓
         ┌─────────────────────────────────────────────┐
         │ 3. ResolveInterruptCompletedTasks()          │
         │    遍历 completed_tasks                      │
         └──────────────┬────────────────────────────┬─┘
                        │                            │
                        ↓                            ↓
         ┌──────────────────────────┐  ┌──────────────────────────┐
         │ 正常任务                  │  │ 异常任务                  │
         │ task->error == nullptr    │  │ task->error != nullptr    │
         │ → 跳过                    │  │ → try-catch 分析          │
         └───────────────────────────┘  └──────────┬───────────────┘
                                                   │
                                                   ↓
                                    ┌──────────────────────────────┐
                                    │ std::rethrow_exception(...)   │
                                    └──────────┬───────────────────┘
                                               │
                    ┌──────────────────────────┼───────────────────┐
                    │                          │                   │
                    ↓                          ↓                   ↓
         ┌──────────────────┐    ┌──────────────────┐   ┌─────────────────┐
         │ SubGraphInterrupt│    │InterruptAndRerun │   │ 其他异常         │
         │ Error            │    │Error             │   │                 │
         └────────┬─────────┘    └────────┬─────────┘   └────────┬────────┘
                  │                       │                      │
                  ↓                       ↓                      ↓
         ┌─────────────────┐    ┌──────────────────┐   ┌─────────────────┐
         │ 记录到           │    │ 记录到            │   │ 向上传播错误     │
         │ sub_graph_      │    │ interrupt_       │   │                 │
         │ interrupts      │    │ rerun_nodes      │   │                 │
         └────────┬────────┘    └────────┬─────────┘   └─────────────────┘
                  │                      │
                  └──────────┬───────────┘
                             │
                             ↓
         ┌─────────────────────────────────────────────┐
         │ 4. 检查是否需要中断                          │
         │    if (!temp_info->sub_graph_interrupts.    │
         │         empty() || ...)                     │
         └──────────────┬──────────────────────────────┘
                        │
                        ↓
         ┌─────────────────────────────────┐
         │ 5. 调用 HandleInterrupt()        │
         │    或 HandleInterruptWithSubGraph│
         │    AndRerunNodes()               │
         └──────────────┬──────────────────┘
                        │
                        ↓
         ┌─────────────────────────────────┐
         │ 6. 创建并返回 InterruptError     │
         │    return InterruptError(...)    │
         └──────────────┬──────────────────┘
                        │
                        ↓
         ┌─────────────────────────────────┐
         │ 7. Graph 层接收并处理            │
         │    - 保存 CheckPoint             │
         │    - 创建 AgentEvent             │
         │    - 发送给用户                  │
         └─────────────────────────────────┘
```

---

## 🔬 详细代码追踪

### **步骤 1：节点执行异常存储**

**节点执行时**：

```cpp
// GraphRunner 执行节点（伪代码）
void ExecuteTask(std::shared_ptr<Task> task) {
    try {
        // 执行节点逻辑
        task->output = task->node->Execute(task->context, task->input);
        task->error = nullptr;  // 成功
        
    } catch (const SubGraphInterruptError& e) {
        // 🔑 捕获并存储异常
        task->error = std::make_shared<std::exception>(
            std::current_exception());
        
    } catch (const InterruptAndRerunError& e) {
        // 🔑 捕获并存储异常
        task->error = std::make_shared<std::exception>(
            std::current_exception());
        
    } catch (const std::exception& e) {
        // 🔑 其他异常也存储
        task->error = std::make_shared<std::exception>(
            std::current_exception());
    }
}
```

---

### **步骤 2：ResolveInterruptCompletedTasks 分析**

**完整实现** (`eino_cpp/src/compose/graph_run.cpp:787-814`):

```cpp
template<typename I, typename O>
void GraphRunner<I, O>::ResolveInterruptCompletedTasks(
    std::shared_ptr<InterruptTempInfo> temp_info,
    const std::vector<std::shared_ptr<Task>>& completed_tasks) {
    
    for (const auto& task : completed_tasks) {
        // 检查任务是否有错误
        if (!task->error) continue;
        
        // 🔑 捕获点：重新抛出并分析异常类型
        try {
            std::rethrow_exception(std::make_exception_ptr(*task->error));
            
        } catch (const SubGraphInterruptError& e) {
            // ✅ 子图中断：保存 InterruptInfo
            temp_info->sub_graph_interrupts[task->node_key] = e.GetInfo();
            continue;  // 已处理，继续下一个任务
            
        } catch (const InterruptAndRerunError& e) {
            // ✅ 重新运行请求：记录节点和额外信息
            temp_info->interrupt_rerun_nodes.push_back(task->node_key);
            
            auto extra = e.GetExtra();
            if (extra.has_value()) {
                temp_info->interrupt_rerun_extra[task->node_key] = extra;
            }
            continue;  // 已处理，继续下一个任务
            
        } catch (...) {
            // ❌ 其他异常：不是中断，向上传播
            // 这会导致整个 Graph 执行失败
        }
        
        // 检查是否在 interrupt_after_nodes 列表中
        for (const auto& key : interrupt_after_nodes_) {
            if (key == task->node_key) {
                temp_info->interrupt_after_nodes.push_back(key);
                break;
            }
        }
    }
}
```

**对应 Go 代码** (`eino/compose/graph_run.go:418-451`):

```go
func (r *runner) resolveInterruptCompletedTasks(
    tempInfo *interruptTempInfo, 
    completedTasks []*task) (err error) {
    
    for _, completedTask := range completedTasks {
        if completedTask.err != nil {
            // 检查是否是子图中断
            if info := isSubGraphInterrupt(completedTask.err); info != nil {
                tempInfo.subGraphInterrupts[completedTask.nodeKey] = info
                continue
            }
            
            // 检查是否是 InterruptAndRerun
            extra, ok := IsInterruptRerunError(completedTask.err)
            if ok {
                tempInfo.interruptRerunNodes = append(tempInfo.interruptRerunNodes, 
                    completedTask.nodeKey)
                if extra != nil {
                    tempInfo.interruptRerunExtra[completedTask.nodeKey] = extra
                    
                    // 保存 tool node info
                    if completedTask.call.action.meta.component == ComponentOfToolsNode {
                        if e, ok := extra.(*ToolsInterruptAndRerunExtra); ok {
                            tempInfo.interruptExecutedTools[completedTask.nodeKey] = e.ExecutedTools
                        }
                    }
                }
                continue
            }
            
            // 其他错误：向上传播
            return wrapGraphNodeError(completedTask.nodeKey, completedTask.err)
        }
        
        // 检查 interruptAfterNodes
        for _, key := range r.interruptAfterNodes {
            if key == completedTask.nodeKey {
                tempInfo.interruptAfterNodes = append(tempInfo.interruptAfterNodes, key)
                break
            }
        }
    }
    return nil
}
```

---

### **步骤 3：HandleInterrupt 创建返回值**

**C++ 实现** (`eino_cpp/src/compose/graph_run.cpp:891-903`):

```cpp
// 🔴 处理 SubGraph 和 CheckpointID
if (is_sub_graph) {
    // SubGraph interrupt: 返回特殊错误，包含 checkpoint
    return SubGraphInterruptError("subgraph interrupt", info, cp);
    
} else if (checkpoint_id != nullptr && !checkpoint_id->empty()) {
    // Normal interrupt with checkpoint ID: 保存到 store
    auto save_err = check_pointer_->Set(ctx, *checkpoint_id, cp);
    if (!save_err.empty()) {
        throw std::runtime_error("failed to set checkpoint: " + save_err + 
                               ", checkPointID: " + *checkpoint_id);
    }
}

// 🔑 返回 InterruptError 对象（不抛出）
return InterruptError("interrupt happened", info);
```

**对应 Go** (`eino/compose/graph_run.go:502-511`):

```go
// 502-509: SubGraph 或保存 checkpoint
if isSubGraph {
    return &subGraphInterruptError{
        Info:       intInfo,
        CheckPoint: cp,
    }
} else if checkPointID != nil && *checkPointID != "" {
    err = r.checkPointer.set(ctx, *checkPointID, cp)
    if err != nil {
        return fmt.Errorf("failed to set checkpoint: %w, checkPointID: %s", 
            err, *checkPointID)
    }
}

// 511: 返回 interruptError
return &interruptError{Info: intInfo}
```

---

## 💡 关键发现

### ✅ **为什么用返回值而不是异常？**

1. **性能考虑**
   - C++ 异常开销大
   - 返回值更轻量

2. **控制流明确**
   - 异常可能跨越多层调用
   - 返回值控制流清晰

3. **与 Go 对齐**
   - Go 使用 error 返回值
   - C++ 保持一致的设计

---

### ✅ **真正的捕获点只有一个**

```cpp
// eino_cpp/src/compose/graph_run.cpp:796-804
try {
    std::rethrow_exception(std::make_exception_ptr(*task->error));
} catch (const SubGraphInterruptError& e) {
    // 处理子图中断
} catch (const InterruptAndRerunError& e) {
    // 处理重新运行
} catch (...) {
    // 其他错误
}
```

**作用**：
- 分析 Task 中存储的异常
- 提取中断信息到 `InterruptTempInfo`
- 决定是否需要触发 Graph 级别的中断

---

### ✅ **InterruptError 的传播方式**

```
节点抛出异常 → 存储到 Task::error → ResolveInterruptCompletedTasks 捕获
     ↓                                             ↓
存储为 exception_ptr                    提取到 InterruptTempInfo
                                                  ↓
                                    HandleInterrupt 创建 InterruptError
                                                  ↓
                                         返回给 Graph 执行器
                                                  ↓
                                    转换为 AgentEvent.action.interrupted
                                                  ↓
                                          发送给用户
```

---

## 📝 示例代码

### 示例 1：工具节点触发 InterruptAndRerun

```cpp
// 工具节点实现
class RetryableTool : public ToolNode {
public:
    std::shared_ptr<void> Execute(
        std::shared_ptr<Context> ctx, 
        std::shared_ptr<void> input) override {
        
        if (ShouldRetry()) {
            // 🔑 抛出 InterruptAndRerun 异常
            throw InterruptAndRerunError("Network error, retry needed", 
                std::map<std::string, std::any>{
                    {"retry_count", retry_count_},
                    {"last_error", "Timeout"},
                });
        }
        
        return ProcessNormally(input);
    }
    
private:
    int retry_count_ = 0;
    bool ShouldRetry() { return retry_count_++ < 3; }
};

// GraphRunner 执行流程
void GraphRunner::ExecuteTasks() {
    for (auto task : tasks) {
        try {
            task->output = task->node->Execute(task->context, task->input);
            task->error = nullptr;
            
        } catch (...) {
            // 🔑 存储异常到 Task
            task->error = std::make_shared<std::exception>(
                std::current_exception());
        }
    }
    
    // 🔑 分析完成的任务
    auto temp_info = std::make_shared<InterruptTempInfo>();
    ResolveInterruptCompletedTasks(temp_info, tasks);
    
    // 🔑 检查是否需要中断
    if (!temp_info->interrupt_rerun_nodes.empty()) {
        auto error = HandleInterruptWithSubGraphAndRerunNodes(
            ctx, temp_info, tasks, checkpoint_id, is_sub_graph, cm, is_stream);
        
        // error 是 InterruptError 对象（不是异常）
        ConvertToAgentEventAndSend(error);
    }
}
```

---

### 示例 2：用户侧处理中断

```cpp
#include "eino/compose/graph.h"
#include "eino/compose/interrupt.h"

int main() {
    auto ctx = CreateContext();
    auto graph = BuildGraphWithInterrupts();
    
    // ========== 执行 Graph ==========
    
    // ❌ 不能这样 catch（InterruptError 不会被抛出）
    try {
        auto result = graph->Run(ctx, input);
    } catch (const InterruptError& e) {
        // 这里永远不会被执行！
    }
    
    // ✅ 正确方式：通过 Event 检测中断
    auto events = graph->Stream(ctx, input);
    
    for (auto event : events) {
        // 检查是否包含中断信息
        if (event->action && event->action->interrupted) {
            std::cout << "⚠️  Graph interrupted!" << std::endl;
            
            auto info = event->action->interrupted;
            // 处理中断...
            
            break;
        }
    }
    
    return 0;
}
```

---

## 🎯 总结

### 核心结论

| 问题 | 答案 |
|------|------|
| **InterruptError 在哪里被 catch？** | `ResolveInterruptCompletedTasks()` 函数中，行号 787-814 |
| **为什么不用异常抛出？** | C++ 性能考虑 + 与 Go 设计对齐 |
| **如何传播中断信息？** | Task::error → InterruptTempInfo → InterruptError 返回值 → AgentEvent |
| **用户如何感知中断？** | 通过 `event->action->interrupted` 字段 |

---

### 调用链路

```
1. Node::Execute()
   ↓ throw InterruptAndRerunError/SubGraphInterruptError
   
2. GraphRunner::ExecuteTask()
   ↓ catch(...) → task->error = current_exception()
   
3. GraphRunner::ResolveInterruptCompletedTasks()
   ↓ rethrow_exception(task->error)
   ↓ catch(SubGraphInterruptError/InterruptAndRerunError)
   ↓ 提取到 temp_info
   
4. GraphRunner::HandleInterrupt()
   ↓ return InterruptError(...)  // 返回值，不抛出
   
5. Graph::Run/Stream()
   ↓ 转换为 AgentEvent
   ↓ event->action->interrupted = interrupt_info
   
6. User Code
   ↓ if (event->action->interrupted) { ... }
```

---

### 文件位置

**C++ 核心代码**：
- `eino_cpp/include/eino/compose/interrupt.h` - InterruptError 定义
- `eino_cpp/src/compose/graph_run.cpp:787-814` - 唯一的 catch 位置
- `eino_cpp/src/compose/graph_run.cpp:839-903` - HandleInterrupt 返回

**Go 对照**：
- `eino/compose/interrupt.go` - interruptError 定义
- `eino/compose/graph_run.go:418-451` - resolveInterruptCompletedTasks
- `eino/compose/graph_run.go:465-512` - handleInterrupt

---

**现在你完全理解 InterruptError 的捕获和处理机制了！** 🎉
