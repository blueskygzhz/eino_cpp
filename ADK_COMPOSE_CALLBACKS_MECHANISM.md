# ADK 和 Compose 交互的 Callbacks 机制实现原理

## 📚 核心概要

**Callbacks 机制**是 eino_cpp 中 **ADK 层**和 **Compose 层**之间的**桥梁**，实现了：

1. **ADK → Compose**：将 ADK 的事件生成逻辑注入到 Compose 的执行流程
2. **Compose → ADK**：Compose 执行过程中触发回调，ADK 接收并转换为 `AgentEvent`
3. **实时流式**：支持流式输出的实时回调

---

## 🏗️ 整体架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                         完整调用链                                    │
└─────────────────────────────────────────────────────────────────────┘

[1] 用户调用 ChatModelAgent::Run()
    ↓
[2] ADK 层创建 ReactCallbackHandler
    ↓
[3] 通过 GenReactCallbacks() 包装为 compose::Option
    ↓
[4] 作为参数传递给 Graph::Stream() 或 Graph::Invoke()
    ↓
[5] Compose 层执行节点（ChatModel, Tool, ToolsNode）
    ↓ (触发回调)
[6] ReactCallbackHandler 接收回调
    ↓ (转换)
[7] 生成 AgentEvent 并发送给 AsyncGenerator
    ↓
[8] 用户通过 AsyncIterator 接收 AgentEvent
```

---

## 🎯 核心组件

### **1. ADK 层：ReactCallbackHandler**

负责接收 Compose 层的回调并转换为 `AgentEvent`。

```cpp
// 📍 eino_cpp/include/eino/adk/callbacks.h:42-117
class ReactCallbackHandler {
public:
    ReactCallbackHandler(
        std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> generator,
        const std::string& agent_name,
        bool enable_streaming,
        MockStore* store)
        : generator_(generator),
          agent_name_(agent_name),
          enable_streaming_(enable_streaming),
          store_(store) {}
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ 核心回调方法（对应 Compose 层的不同组件）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // ChatModel 完成回调
    void OnChatModelEnd(
        void* ctx,
        const callbacks::RunInfo& info,
        const schema::Message& output);
    
    // ChatModel 流式输出回调
    void OnChatModelEndWithStreamOutput(
        void* ctx,
        const callbacks::RunInfo& info,
        std::shared_ptr<schema::StreamReader<schema::Message>> output);
    
    // Tool 完成回调
    void OnToolEnd(
        void* ctx,
        const callbacks::RunInfo& info,
        const std::string& tool_response,
        const std::string& tool_call_id);
    
    // Tool 流式输出回调
    void OnToolEndWithStreamOutput(
        void* ctx,
        const callbacks::RunInfo& info,
        std::shared_ptr<schema::StreamReader<std::string>> output,
        const std::string& tool_call_id);
    
    // ToolsNode 完成回调
    void OnToolsNodeEnd(
        void* ctx,
        const callbacks::RunInfo& info,
        const std::vector<schema::Message>& messages);
    
    // ToolsNode 流式输出回调
    void OnToolsNodeEndWithStreamOutput(
        void* ctx,
        const callbacks::RunInfo& info,
        std::shared_ptr<schema::StreamReader<std::vector<schema::Message>>> output);
    
    // Graph 错误回调
    void OnGraphError(
        void* ctx,
        const callbacks::RunInfo& info,
        const std::exception& error);
    
private:
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> generator_;
    std::string agent_name_;
    bool enable_streaming_;
    MockStore* store_;
    
    // 存储 return-directly 工具事件
    std::atomic<AgentEvent*> return_directly_tool_event_{nullptr};
};
```

---

### **2. Compose 层：Handler 接口**

Compose 层的通用回调接口，所有组件都会调用。

```cpp
// 📍 eino_cpp/include/eino/callbacks/interface.h:52-70
class Handler {
public:
    virtual ~Handler() = default;
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ 核心回调方法（Compose 组件调用）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // 节点开始执行前
    virtual void OnStart(const RunInfo& info, const CallbackInput& input) {}
    
    // 节点执行完成后
    virtual void OnEnd(const RunInfo& info, const CallbackOutput& output) {}
    
    // 节点执行出错
    virtual void OnError(const RunInfo& info, const std::string& error) {}
    
    // 流式输入处理前
    virtual void OnStartWithStreamInput(const RunInfo& info, const CallbackInput& input) {}
    
    // 流式输出处理后
    virtual void OnEndWithStreamOutput(const RunInfo& info, const CallbackOutput& output) {}
};
```

---

### **3. 桥接层：GenReactCallbacks()**

将 `ReactCallbackHandler` 包装为 Compose 可识别的 `compose::Option`。

```cpp
// 📍 eino_cpp/src/adk/callbacks.cpp:195-256
compose::Option GenReactCallbacks(
    const std::string& agent_name,
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> generator,
    bool enable_streaming,
    MockStore* store) {
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: 创建 ReactCallbackHandler
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto handler = std::make_shared<ReactCallbackHandler>(
        generator, agent_name, enable_streaming, store);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: 为 ChatModel 创建回调处理器
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto cm_handler = callbacks::HandlerBuilder()
        .WithOnEnd([handler](const callbacks::RunInfo& info, 
                            const callbacks::CallbackOutput& output) {
            // ⭐ 提取 Message 并调用 ReactCallbackHandler
            auto msg = output.output.get<schema::Message>();
            handler->OnChatModelEnd(nullptr, info, msg);
        })
        .WithOnEndWithStreamOutput([handler](const callbacks::RunInfo& info, 
                                             const callbacks::CallbackOutput& output) {
            // ⭐ 提取 StreamReader 并调用 ReactCallbackHandler
            auto stream = output.output.get<std::shared_ptr<schema::StreamReader<schema::Message>>>();
            handler->OnChatModelEndWithStreamOutput(nullptr, info, stream);
        })
        .Build();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: 为 Tool 创建回调处理器
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto tool_handler = callbacks::HandlerBuilder()
        .WithOnEnd([handler](const callbacks::RunInfo& info, 
                            const callbacks::CallbackOutput& output) {
            auto response = output.output.get<std::string>();
            auto call_id = output.extra.at("tool_call_id").get<std::string>();
            handler->OnToolEnd(nullptr, info, response, call_id);
        })
        .WithOnEndWithStreamOutput([handler](const callbacks::RunInfo& info, 
                                             const callbacks::CallbackOutput& output) {
            auto stream = output.output.get<std::shared_ptr<schema::StreamReader<std::string>>>();
            auto call_id = output.extra.at("tool_call_id").get<std::string>();
            handler->OnToolEndWithStreamOutput(nullptr, info, stream, call_id);
        })
        .Build();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 4: 为 ToolsNode 创建回调处理器
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto tools_node_handler = callbacks::HandlerBuilder()
        .WithOnEnd([handler](const callbacks::RunInfo& info, 
                            const callbacks::CallbackOutput& output) {
            auto messages = output.output.get<std::vector<schema::Message>>();
            handler->OnToolsNodeEnd(nullptr, info, messages);
        })
        .WithOnEndWithStreamOutput([handler](const callbacks::RunInfo& info, 
                                             const callbacks::CallbackOutput& output) {
            auto stream = output.output.get<std::shared_ptr<schema::StreamReader<std::vector<schema::Message>>>>();
            handler->OnToolsNodeEndWithStreamOutput(nullptr, info, stream);
        })
        .Build();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 5: 为 Graph 创建错误处理器
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto graph_handler = callbacks::HandlerBuilder()
        .WithOnError([handler](const callbacks::RunInfo& info, 
                              const std::string& error) {
            handler->OnGraphError(nullptr, info, std::runtime_error(error));
        })
        .Build();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 6: 合并所有处理器，返回 compose::Option
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    return compose::WithCallbacks({
        cm_handler, 
        tool_handler, 
        tools_node_handler, 
        graph_handler
    });
}
```

---

## 🔄 完整交互流程

### **场景：ChatModel 执行并生成 AgentEvent**

```
┌────────────────────────────────────────────────────────────────────┐
│                  ChatModel 执行 + Callback 流程                      │
└────────────────────────────────────────────────────────────────────┘

[1] ADK: ChatModelAgent::BuildRunFunc()
    ├─ 创建 ReactCallbackHandler(generator, agent_name, ...)
    ├─ 调用 GenReactCallbacks() 生成 callback_opt
    ├─ 将 callback_opt 传递给 Graph
    │
[2] ADK: agent->Run(ctx, input, {callback_opt})
    ├─ runnable->Stream(ctx, input, callback_opt)
    │  ↓
[3] Compose: Graph::Stream()
    ├─ 执行 ChatModel 节点
    │  ├─ ChatModel::Stream(ctx, messages)
    │  ├─ ... ChatModel 生成响应 ...
    │  │
[4] Compose: ChatModel 完成，触发回调
    ├─ callbacks::OnEndWithStreamOutput(ctx, output_stream)
    │  ├─ 从 ctx 提取 CallbackManager
    │  ├─ 遍历所有 Handler
    │  │  ├─ Handler::OnEndWithStreamOutput(info, output)
    │  │  │
[5] ADK: cm_handler 接收回调
    ├─ lambda 函数被调用
    │  ├─ 提取 stream = output.output.get<StreamReader<Message>>()
    │  ├─ 调用 handler->OnChatModelEndWithStreamOutput(ctx, info, stream)
    │  │
[6] ADK: ReactCallbackHandler::OnChatModelEndWithStreamOutput()
    ├─ auto event = EventFromMessage(nullptr, stream, Assistant, "")
    ├─ generator_->Send(event)  // ⭐ 发送 AgentEvent
    │
[7] 用户侧接收
    ├─ event_iter->Next(event)
    ├─ 处理 event->message
```

---

## 📊 关键实现细节

### **1. OnChatModelEnd 实现**

```cpp
// 📍 eino_cpp/src/adk/callbacks.cpp:31-47
void ReactCallbackHandler::OnChatModelEnd(
    void* ctx,
    const callbacks::RunInfo& info,
    const schema::Message& output) {
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ 核心转换：Message → AgentEvent
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto event = EventFromMessage(
        const_cast<schema::Message*>(&output),
        nullptr,  // 不是流式
        schema::RoleType::kAssistant,
        ""  // 无工具名称
    );
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ 发送到 AsyncGenerator（用户侧会接收）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    if (generator_) {
        generator_->Send(event);
    }
}
```

**对应 Go 实现**：

```go
// 📍 eino/adk/chatmodel.go:402-408
func (h *cbHandler) onChatModelEnd(ctx context.Context, info callbacks.RunInfo, msg Message) context.Context {
    e := EventFromMessage(msg, nil, schema.Assistant, "")
    h.Send(e)
    return ctx
}
```

---

### **2. OnChatModelEndWithStreamOutput 实现**

```cpp
// 📍 eino_cpp/src/adk/callbacks.cpp:49-65
void ReactCallbackHandler::OnChatModelEndWithStreamOutput(
    void* ctx,
    const callbacks::RunInfo& info,
    std::shared_ptr<schema::StreamReader<schema::Message>> output) {
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ 核心转换：StreamReader → AgentEvent
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto event = EventFromMessage(
        nullptr,  // 不是单个 Message
        output.get(),  // 流式 StreamReader
        schema::RoleType::kAssistant,
        ""
    );
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ 发送（用户会逐块接收）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    if (generator_) {
        generator_->Send(event);
    }
}
```

**关键区别**：
- **非流式**：`EventFromMessage(msg, nullptr, ...)`
- **流式**：`EventFromMessage(nullptr, stream, ...)`

---

### **3. OnToolEnd 实现（Return-Directly 机制）**

```cpp
// 📍 eino_cpp/src/adk/callbacks.cpp:72-102
void ReactCallbackHandler::OnToolEnd(
    void* ctx,
    const callbacks::RunInfo& info,
    const std::string& tool_response,
    const std::string& tool_call_id) {
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: 创建 ToolMessage
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto msg = schema::ToolMessage(tool_response, tool_call_id, info.name);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: 创建 Event
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto event = EventFromMessage(&msg, nullptr, schema::RoleType::kTool, info.name);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: Pop Tool Generated Action（如 exit, transfer）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto action = PopToolGenAction(ctx, info.name);
    if (action) {
        event->action = action;  // ⭐ 附加 Action
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 4: 检查是否是 Return-Directly 工具
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto [return_directly_id, has_return_directly] = GetReturnDirectlyToolCallID(ctx);
    
    if (has_return_directly && return_directly_id == tool_call_id) {
        // ⚠️ Return-Directly 工具：延迟发送
        // 等待所有工具完成后再发送（在 OnToolsNodeEnd 中）
        return_directly_tool_event_.store(event.get());
    } else {
        // ✅ 普通工具：立即发送
        if (generator_) {
            generator_->Send(event);
        }
    }
}
```

**Return-Directly 机制说明**：

某些工具（如 `transfer_to_agent`, `exit`）需要在**所有工具执行完成后**才发送事件，避免中断其他工具的执行。

```cpp
// 📍 eino_cpp/src/adk/callbacks.cpp:149-156
void ReactCallbackHandler::OnToolsNodeEnd(
    void* ctx,
    const callbacks::RunInfo& info,
    const std::vector<schema::Message>& messages) {
    
    // ⭐ 发送延迟的 Return-Directly 工具事件
    SendReturnDirectlyToolEvent();
}
```

---

### **4. Compose 层如何触发回调**

Compose 层的组件（如 ChatModel、Tool）在执行完成后，会调用：

```cpp
// 📍 eino_cpp/include/eino/callbacks/callback.h:234-238
template<typename T>
std::pair<Context, T> OnEnd(const Context& ctx, T output) {
    return On<T>(ctx, output, OnEndHandle<T>, CallbackTiming::kOnEnd, false);
}
```

**核心函数 `On()`**：

```cpp
// 📍 eino_cpp/include/eino/callbacks/callback.h:43-101
template<typename T>
std::pair<Context, T> On(
    const Context& ctx,
    T in_out,
    HandleFunc<T> handle,
    CallbackTiming timing,
    bool start) {
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: 从 Context 提取 CallbackManager
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto mgr = ManagerFromCtx(ctx);
    if (!mgr) {
        return {ctx, in_out};  // 无回调
    }
    
    auto n_mgr = std::make_shared<CallbackManager>(*mgr);
    
    RunInfo* info = nullptr;
    Context new_ctx = ctx;
    
    if (start) {
        // 开始时：提取并存储 RunInfo
        info = const_cast<RunInfo*>(&n_mgr->GetRunInfo());
        new_ctx = CtxWithRunInfo(new_ctx, *info);
        n_mgr = n_mgr->WithRunInfo(RunInfo{});
    } else {
        // 结束时：从 Context 恢复 RunInfo
        info = RunInfoFromCtx(new_ctx);
        if (!info) {
            info = const_cast<RunInfo*>(&n_mgr->GetRunInfo());
        }
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: 过滤符合 Timing 的 Handler
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    std::vector<std::shared_ptr<Handler>> filtered_handlers;
    auto all_handlers = n_mgr->GetAllHandlers();
    
    for (const auto& handler : all_handlers) {
        auto timing_handler = std::dynamic_pointer_cast<HandlerWithTiming>(handler);
        if (!timing_handler || timing_handler->Check(timing)) {
            filtered_handlers.push_back(handler);
        }
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: 执行 Handle 函数（调用所有 Handler）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    T out;
    std::tie(new_ctx, out) = handle(new_ctx, in_out, info, filtered_handlers);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 4: 更新 Context
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    new_ctx = CtxWithManager(new_ctx, n_mgr);
    
    return {new_ctx, out};
}
```

**OnEndHandle 实现**：

```cpp
// 📍 eino_cpp/include/eino/callbacks/callback.h:129-152
template<typename T>
std::pair<Context, T> OnEndHandle(
    const Context& ctx,
    T output,
    const RunInfo* run_info,
    const std::vector<std::shared_ptr<Handler>>& handlers) {
    
    Context new_ctx = ctx;
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ 遍历所有 Handler 并调用 OnEnd
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    for (const auto& handler : handlers) {
        try {
            CallbackOutput cb_output;
            cb_output.output = output;
            
            // ⭐ 调用 Handler::OnEnd（这会触发 ADK 的回调）
            handler->OnEnd(*run_info, cb_output);
            
        } catch (const std::exception& e) {
            // 回调错误不应中断流程
        }
    }
    
    return {new_ctx, output};
}
```

---

## 🌊 流式回调的特殊处理

### **问题**

流式输出（`StreamReader`）是**惰性的**，只有在 `Recv()` 时才产生数据。但 Callback 需要**立即**被触发。

### **解决方案**

ADK 层接收到 `StreamReader` 后，会**包装它**并在 `EventFromMessage` 中处理：

```cpp
// 伪代码：EventFromMessage 的流式处理
std::shared_ptr<AgentEvent> EventFromMessage(
    const Message* msg,
    StreamReader<Message>* stream,
    RoleType role,
    const std::string& tool_name) {
    
    auto event = std::make_shared<AgentEvent>();
    event->agent_name = agent_name;
    
    if (msg) {
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // 非流式：直接设置 Message
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        event->message = std::make_shared<Message>(*msg);
        
    } else if (stream) {
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // 流式：包装 StreamReader
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        event->message_stream = std::make_shared<MessageStream>(stream);
        
        // ⭐ 关键：用户会逐块接收
        // 每次调用 event->message_stream->Recv() 时，会从底层 stream 读取
    }
    
    return event;
}
```

**用户侧接收**：

```cpp
// 用户代码
auto event_iter = agent->Run(ctx, input, {});

std::shared_ptr<AgentEvent> event;
while (event_iter->Next(event)) {
    if (event->message_stream) {
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ 流式接收：逐块读取
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        while (true) {
            auto chunk = event->message_stream->Recv();
            if (!chunk) break;
            
            std::cout << chunk->Content;  // 实时打印
        }
    } else if (event->message) {
        // 非流式：一次性获取
        std::cout << event->message->Content << std::endl;
    }
}
```

---

## 🔧 完整示例：端到端流程

```cpp
#include "eino/adk/chat_model_agent.h"
#include "eino/compose/graph.h"
#include <iostream>

using namespace eino;
using namespace eino::adk;

void ExampleCallbackMechanism() {
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: 创建 ChatModelAgent
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto config = std::make_shared<ChatModelAgentConfig>();
    config->name = "MyAgent";
    config->model = chat_model;
    config->tools_config = std::make_shared<ToolsConfig>();
    config->tools_config->tools = {search_tool, calculator_tool};
    
    auto agent = NewChatModelAgent(nullptr, config);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: 用户调用 Run（内部会设置 Callbacks）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto input = std::make_shared<AgentInput>();
    input->messages = {schema::UserMessage("Search for weather")};
    input->enable_streaming = true;
    
    auto event_iter = agent->Run(nullptr, input, {});
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 内部流程（用户看不到）：
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    /*
    ChatModelAgent::Run() {
        // 1. 创建 AsyncIteratorPair
        auto [iterator, generator] = NewAsyncIteratorPair<AgentEvent>();
        
        // 2. 启动线程执行
        std::thread([this, generator]() {
            // 3. 创建 Callbacks
            auto callback_opt = GenReactCallbacks(
                name_, 
                generator,  // ⭐ 传递 generator
                enable_streaming, 
                store);
            
            // 4. 调用 Graph 并传递 callback_opt
            auto msg_stream = runnable->Stream(ctx, input, callback_opt);
            
            // 5. Graph 执行过程中：
            //    - ChatModel 完成 → OnChatModelEndWithStreamOutput 被触发
            //    - Tool 完成 → OnToolEnd 被触发
            //    - ToolsNode 完成 → OnToolsNodeEnd 被触发
            
            // 6. ReactCallbackHandler 接收回调：
            //    - 转换为 AgentEvent
            //    - 调用 generator->Send(event)
            
            generator->Close();
        }).detach();
        
        return iterator;  // ⭐ 返回给用户
    }
    */
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: 用户接收 AgentEvent（由 Callback 生成）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    std::shared_ptr<AgentEvent> event;
    while (event_iter->Next(event)) {
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // Event 1: ChatModel 开始生成（流式）
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        if (event->message_stream) {
            std::cout << "[ChatModel Output (Stream)]" << std::endl;
            
            while (true) {
                auto chunk = event->message_stream->Recv();
                if (!chunk) break;
                
                std::cout << chunk->Content;
                
                // 检查工具调用
                if (!chunk->ToolCalls.empty()) {
                    std::cout << "\n[Tool Calls Detected]" << std::endl;
                    for (const auto& tc : chunk->ToolCalls) {
                        std::cout << "  - " << tc.name << "(" << tc.arguments << ")" << std::endl;
                    }
                }
            }
            std::cout << std::endl;
        }
        
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // Event 2: Tool 执行结果
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        if (event->message && event->message->Role == schema::RoleType::kTool) {
            std::cout << "[Tool Response]" << std::endl;
            std::cout << "  Tool: " << event->message->Name << std::endl;
            std::cout << "  Result: " << event->message->Content << std::endl;
            
            // 检查是否有 Action（如 exit, transfer）
            if (event->action) {
                if (event->action->exit) {
                    std::cout << "  → Exit requested" << std::endl;
                }
                if (event->action->transfer_to_agent) {
                    std::cout << "  → Transfer to: " << event->action->transfer_to_agent->to << std::endl;
                }
            }
        }
        
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // Event 3: 错误
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        if (!event->error_msg.empty()) {
            std::cout << "[Error] " << event->error_msg << std::endl;
        }
    }
}
```

**输出示例**：

```
[ChatModel Output (Stream)]
I need to search for the weather.
[Tool Calls Detected]
  - search({"query": "weather today"})

[Tool Response]
  Tool: search
  Result: Sunny, 25°C

[ChatModel Output (Stream)]
The weather today is sunny with a temperature of 25°C.
```

---

## 🎯 关键设计要点

### **1. 回调的注入时机**

```cpp
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 📍 eino/adk/chatmodel.go:684-692
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// STEP 1: 生成 Callback Option
callOpt := genReactCallbacks(a.name, generator, input.EnableStreaming, store)

// STEP 2: 与用户提供的 opts 合并
if input.EnableStreaming {
    msgStream, err = runnable.Stream(ctx, input, append(opts, callOpt)...)
} else {
    msg, err = runnable.Invoke(ctx, input, append(opts, callOpt)...)
}
```

**关键**：`callOpt` 作为**最后一个参数**传递，确保它不会被用户的 opts 覆盖。

---

### **2. Callback 的执行顺序**

```cpp
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// OnStart: 反向执行（后注册的先执行）
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
for (int i = handlers.size() - 1; i >= 0; --i) {
    handlers[i]->OnStart(info, input);
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// OnEnd: 正向执行（先注册的先执行）
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
for (const auto& handler : handlers) {
    handler->OnEnd(info, output);
}
```

**原因**：类似于中间件（Middleware）模式，先进后出。

---

### **3. 错误处理**

```cpp
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ Callback 中的异常不应中断主流程
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
for (const auto& handler : handlers) {
    try {
        handler->OnEnd(*run_info, cb_output);
    } catch (const std::exception& e) {
        // 静默忽略（或记录日志）
        // 不要传播异常
    }
}
```

**原因**：Callback 是**观察者模式**，不应影响核心业务逻辑。

---

### **4. Context 的传递**

```cpp
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ CallbackManager 存储在 Context 中
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// 注入时
ctx = CtxWithManager(ctx, callback_manager);

// 提取时
auto mgr = ManagerFromCtx(ctx);

// 执行时
std::tie(new_ctx, output) = callbacks::OnEnd(ctx, output);
```

**优势**：无需在每个函数签名中传递 CallbackManager，保持接口简洁。

---

## 📝 最佳实践

### ✅ DO

1. **使用 HandlerBuilder 创建 Handler**
   ```cpp
   auto handler = callbacks::HandlerBuilder()
       .WithOnEnd([](const RunInfo& info, const CallbackOutput& output) {
           // 处理逻辑
       })
       .Build();
   ```

2. **在 Callback 中捕获异常**
   ```cpp
   .WithOnEnd([](const RunInfo& info, const CallbackOutput& output) {
       try {
           // 可能抛出异常的代码
       } catch (const std::exception& e) {
           // 记录日志，但不要重新抛出
       }
   })
   ```

3. **为不同组件创建专门的 Handler**
   ```cpp
   auto cm_handler = /* ChatModel 回调 */;
   auto tool_handler = /* Tool 回调 */;
   auto graph_handler = /* Graph 回调 */;
   
   return compose::WithCallbacks({cm_handler, tool_handler, graph_handler});
   ```

### ❌ DON'T

1. **不要在 Callback 中抛出未捕获的异常**
   ```cpp
   // ❌ 错误
   .WithOnEnd([](const RunInfo& info, const CallbackOutput& output) {
       throw std::runtime_error("Oops");  // 会中断主流程
   })
   
   // ✅ 正确
   .WithOnEnd([](const RunInfo& info, const CallbackOutput& output) {
       try {
           // ...
       } catch (...) {
           // 处理或记录
       }
   })
   ```

2. **不要在 Callback 中修改主流程的数据**
   ```cpp
   // ❌ 错误
   .WithOnEnd([&some_state](const RunInfo& info, const CallbackOutput& output) {
       some_state.value = 42;  // 副作用，难以调试
   })
   
   // ✅ 正确：只观察，不修改
   .WithOnEnd([](const RunInfo& info, const CallbackOutput& output) {
       std::cout << "Output: " << output.output << std::endl;
   })
   ```

3. **不要忘记处理流式输出**
   ```cpp
   // ❌ 错误：只处理 OnEnd
   .WithOnEnd([](const RunInfo& info, const CallbackOutput& output) {
       // 流式输出不会触发这里
   })
   
   // ✅ 正确：同时处理流式和非流式
   .WithOnEnd([](const RunInfo& info, const CallbackOutput& output) {
       // 非流式
   })
   .WithOnEndWithStreamOutput([](const RunInfo& info, const CallbackOutput& output) {
       // 流式
   })
   ```

---

## 🎓 总结

1. **Callbacks 是 ADK 和 Compose 的桥梁**
   - ADK 注入回调
   - Compose 触发回调
   - ADK 接收并转换为 AgentEvent

2. **核心组件**
   ```
   ReactCallbackHandler → HandlerBuilder → compose::WithCallbacks → Handler::OnEnd → generator->Send
   ```

3. **关键机制**
   - **注入**：`GenReactCallbacks()` 包装为 `compose::Option`
   - **触发**：`callbacks::OnEnd()` 遍历所有 Handler
   - **转换**：`ReactCallbackHandler` 将输出转为 `AgentEvent`
   - **发送**：`AsyncGenerator::Send()` 传递给用户

4. **支持流式**
   - 非流式：`EventFromMessage(msg, nullptr, ...)`
   - 流式：`EventFromMessage(nullptr, stream, ...)`

5. **设计模式**
   - **观察者模式**：Callback 观察执行过程
   - **中间件模式**：Callback 链式执行
   - **策略模式**：不同组件用不同 Handler

---

需要我：
1. 详细解释某个特定的回调场景？
2. 分析流式回调的完整实现？
3. 提供更复杂的自定义 Callback 示例？
4. 解释 Go 版本和 C++ 版本的差异？
