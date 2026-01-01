# OnEndWithStreamOutput 实现细节

## 📋 概述

`OnEndWithStreamOutput` 是用于处理**流式输出结束**时的 callback 函数,在组件返回 `StreamReader` 时触发。

---

## 🎯 函数签名

### C++ 版本

```cpp
// 文件: eino_cpp/include/eino/compose/utils.h:92-107

template <typename T>
std::pair<Context, std::shared_ptr<schema::StreamReader<T>>> OnEndWithStreamOutput(
    const Context& ctx,
    std::shared_ptr<schema::StreamReader<T>> output) {
    
    // 1. 从 Context 获取 handlers
    auto handlers = callbacks::GetHandlersFromContext(ctx);
    if (handlers.empty()) {
        return {ctx, output};
    }
    
    // 2. 依次调用每个 handler 的 OnEndWithStreamOutput
    Context new_ctx = ctx;
    for (auto& handler : handlers) {
        new_ctx = handler->OnEndWithStreamOutput(new_ctx, output);
    }
    
    // 3. 返回更新后的 context 和原始 output
    return {new_ctx, output};
}
```

### Go 版本

```go
// 文件: eino/compose/utils.go:69-73

func onEndWithStreamOutput[T any](ctx context.Context, output *schema.StreamReader[T]) (
    context.Context, *schema.StreamReader[T]) {
    
    return icb.On(ctx, output, icb.OnEndWithStreamOutputHandle[T], 
                  callbacks.TimingOnEndWithStreamOutput, false)
}

// 实际处理函数: eino/internal/callbacks/inject.go:164-177
func OnEndWithStreamOutputHandle[T any](ctx context.Context, output *schema.StreamReader[T],
    runInfo *RunInfo, handlers []Handler) (context.Context, *schema.StreamReader[T]) {

    cpy := output.Copy  // ⭐ 获取复制函数

    handle := func(ctx context.Context, handler Handler, out *schema.StreamReader[T]) context.Context {
        // 转换类型为 CallbackOutput
        out_ := schema.StreamReaderWithConvert(out, func(i T) (CallbackOutput, error) {
            return i, nil
        })
        return handler.OnEndWithStreamOutput(ctx, runInfo, out_)
    }

    return OnWithStreamHandle(ctx, output, handlers, cpy, handle)
}
```

---

## 🔄 调用流程对比

### C++ 版本调用流程

```
1. LambdaRunnable::Stream() 被调用
   ↓
2. OnStart(ctx, input)  // 前置 callback
   ↓
3. stream_func_(ctx, input, opts)  // 执行用户函数,返回 StreamReader<O>
   ↓
4. OnEndWithStreamOutput(ctx, output_stream)  ← 我们在这里
   ↓
   4.1 callbacks::GetHandlersFromContext(ctx)  // 获取所有 handlers
   ↓
   4.2 遍历每个 handler:
       handler->OnEndWithStreamOutput(new_ctx, output)
       ↓
       [Handler 内部处理]
       - 可能读取 stream
       - 可能记录日志
       - 可能发送事件
   ↓
5. 返回 {updated_ctx, original_output_stream}
```

### Go 版本调用流程

```
1. runnablePacker.s() 被调用 (已被 streamWithCallbacks 包装)
   ↓
2. onStart(ctx, input)
   ↓
3. 原始 stream 函数执行,返回 *schema.StreamReader[O]
   ↓
4. onEndWithStreamOutput(ctx, output)  ← 我们在这里
   ↓
   4.1 icb.On(...) 框架函数
   ↓
   4.2 OnEndWithStreamOutputHandle(...) 处理函数
       ├─ output.Copy  // ⭐ 创建 stream 副本(重要!)
       ├─ 遍历 handlers
       │  ├─ StreamReaderWithConvert 转换类型
       │  └─ handler.OnEndWithStreamOutput(ctx, runInfo, out_)
       │
   ↓
5. 返回 {updated_ctx, original/new_output_stream}
```

---

## 🔑 关键差异: Stream 复制机制

### Go 版本的 Stream Copy

Go 版本有一个重要特性:**Stream 复制机制**

```go
// Go 的 StreamReader 有 Copy 方法
type StreamReader[T any] struct {
    Copy func() *StreamReader[T]  // ⭐ 支持复制
}

// 在 callback 处理中
func OnEndWithStreamOutputHandle[T any](...) {
    cpy := output.Copy  // 获取复制函数
    
    // OnWithStreamHandle 会为每个 handler 创建独立的 stream 副本
    // 这样每个 handler 都能独立读取完整的 stream
}
```

**为什么需要复制?**

```go
// 场景: 多个 handlers 需要读取同一个 stream

// ❌ 没有复制的情况:
handler1.OnEnd(stream)  // handler1 读取 stream 到结尾
handler2.OnEnd(stream)  // handler2 拿到的是已耗尽的 stream,无法读取!

// ✅ 有复制的情况:
handler1.OnEnd(stream.Copy())  // handler1 读取副本1
handler2.OnEnd(stream.Copy())  // handler2 读取副本2,完全独立
```

### C++ 版本的处理

C++ 版本**当前没有实现** Stream 复制机制:

```cpp
// 当前实现
template <typename T>
std::pair<Context, std::shared_ptr<StreamReader<T>>> OnEndWithStreamOutput(...) {
    Context new_ctx = ctx;
    for (auto& handler : handlers) {
        // ⚠️ 所有 handlers 共享同一个 output StreamReader
        // 如果某个 handler 读取了 stream,后续 handler 无法读取
        new_ctx = handler->OnEndWithStreamOutput(new_ctx, output);
    }
    return {new_ctx, output};
}
```

**潜在问题**:
- 如果第一个 handler 读取了 stream,后续 handler 拿到的是已耗尽的 stream
- 需要 handlers 自己注意不要消费 stream,或者重置 stream

**未来优化方向**:
```cpp
// 建议添加 StreamReader 复制功能
template<typename T>
class StreamReader {
public:
    virtual std::shared_ptr<StreamReader<T>> Copy() = 0;  // 新增
};

// 在 OnEndWithStreamOutput 中使用
template <typename T>
std::pair<Context, std::shared_ptr<StreamReader<T>>> OnEndWithStreamOutput(...) {
    Context new_ctx = ctx;
    for (auto& handler : handlers) {
        // ✅ 每个 handler 获得独立副本
        auto output_copy = output->Copy();
        new_ctx = handler->OnEndWithStreamOutput(new_ctx, output_copy);
    }
    return {new_ctx, output};
}
```

---

## 🎯 使用场景

### 场景 1: 日志记录

```cpp
class LoggingHandler : public callbacks::Handler {
public:
    Context OnEndWithStreamOutput(const Context& ctx, 
                                   std::shared_ptr<StreamReader<T>> output) override {
        std::cout << "[LOG] Stream output generated" << std::endl;
        
        // ⚠️ 注意: 不应该读取 stream,因为会消费它
        // 如果需要查看内容,应该等待 Copy 机制实现
        
        return ctx;
    }
};
```

### 场景 2: 事件发送 (ADK Agent)

```cpp
class AgentEventHandler : public callbacks::Handler {
public:
    Context OnEndWithStreamOutput(const Context& ctx,
                                   std::shared_ptr<StreamReader<Message>> stream) override {
        // ADK 场景: 将 stream 包装为 AgentEvent 并发送
        auto event = CreateStreamEvent(stream);
        event_generator_->Send(event);
        
        // ⚠️ 这里没有消费 stream,只是传递引用
        return ctx;
    }
};
```

### 场景 3: 监控统计

```cpp
class MetricsHandler : public callbacks::Handler {
public:
    Context OnEndWithStreamOutput(const Context& ctx,
                                   std::shared_ptr<StreamReader<T>> output) override {
        // 记录流式输出的时间戳
        auto now = std::chrono::system_clock::now();
        metrics_.record_stream_end(now);
        
        // 不读取 stream 内容
        return ctx;
    }
};
```

---

## 📊 与其他 Callback 函数对比

| 函数                      | 何时调用             | 输入类型           | 输出类型           | 主要用途                 |
|---------------------------|---------------------|-------------------|-------------------|------------------------|
| `OnStart<T>`              | 非流式输入开始前    | `const T&`        | `T`               | 预处理输入,记录开始     |
| `OnEnd<T>`                | 非流式输出结束后    | `const T&`        | `T`               | 后处理输出,记录结束     |
| `OnStartWithStreamInput`  | 流式输入开始前      | `StreamReader<T>` | `StreamReader<T>` | 处理流式输入           |
| **`OnEndWithStreamOutput`** | **流式输出结束后** | `StreamReader<T>` | `StreamReader<T>` | **处理流式输出**       |
| `OnError`                 | 执行出错时          | `string`          | `string`          | 错误处理,记录异常       |

### 配对使用

```cpp
// Stream 方法: OnStart + OnEndWithStreamOutput
auto output_stream = runnable->Stream(ctx, input, opts);
// 内部: OnStart(input) → Execute → OnEndWithStreamOutput(output_stream)

// Collect 方法: OnStartWithStreamInput + OnEnd
auto output = runnable->Collect(ctx, input_stream, opts);
// 内部: OnStartWithStreamInput(input_stream) → Execute → OnEnd(output)

// Transform 方法: OnStartWithStreamInput + OnEndWithStreamOutput
auto output_stream = runnable->Transform(ctx, input_stream, opts);
// 内部: OnStartWithStreamInput(input_stream) → Execute → OnEndWithStreamOutput(output_stream)
```

---

## 🔍 深入细节

### Handler 接口定义

```cpp
// 假设的 Handler 基类 (实际定义可能在其他文件)
namespace callbacks {

class Handler {
public:
    virtual ~Handler() = default;
    
    // 非流式 callbacks
    virtual Context OnStart(const Context& ctx, const CallbackInput& input) {
        return ctx;
    }
    
    virtual Context OnEnd(const Context& ctx, const CallbackOutput& output) {
        return ctx;
    }
    
    // 流式 callbacks
    virtual Context OnStartWithStreamInput(
        const Context& ctx,
        std::shared_ptr<StreamReader<CallbackInput>> input) {
        return ctx;
    }
    
    // ⭐ 我们关注的函数
    virtual Context OnEndWithStreamOutput(
        const Context& ctx,
        std::shared_ptr<StreamReader<CallbackOutput>> output) {
        return ctx;  // 默认实现: 不做任何处理
    }
    
    virtual Context OnError(const Context& ctx, const std::string& error) {
        return ctx;
    }
};

} // namespace callbacks
```

### Context 中的 Handlers 存储

```cpp
namespace callbacks {

// 从 Context 获取 handlers
std::vector<std::shared_ptr<Handler>> GetHandlersFromContext(const Context& ctx) {
    // 从 context 的内部存储中提取 handlers
    // 实现细节取决于 Context 的设计
    
    json handlers_json;
    if (ctx.GetValue("__callbacks_handlers__", handlers_json)) {
        // 反序列化 handlers
        return DeserializeHandlers(handlers_json);
    }
    
    return {};  // 没有 handlers
}

// 向 Context 添加 handlers
Context AppendHandlers(const Context& ctx, 
                       const RunInfo& info,
                       const std::vector<std::shared_ptr<Handler>>& handlers) {
    Context new_ctx = ctx;
    
    // 序列化并存储 handlers 到 context
    json handlers_json = SerializeHandlers(handlers);
    new_ctx.SetValue("__callbacks_handlers__", handlers_json);
    new_ctx.SetValue("__callbacks_run_info__", SerializeRunInfo(info));
    
    return new_ctx;
}

} // namespace callbacks
```

---

## 🚀 完整使用示例

### 示例 1: 基本使用

```cpp
#include "eino/compose/runnable.h"
#include "eino/compose/utils.h"

// 1. 定义 Handler
class MyStreamHandler : public callbacks::Handler {
public:
    Context OnEndWithStreamOutput(
        const Context& ctx,
        std::shared_ptr<StreamReader<std::string>> output) override {
        
        std::cout << "[MyStreamHandler] Stream output completed!" << std::endl;
        
        // ⚠️ 当前不建议读取 stream (等待 Copy 机制)
        // std::string value;
        // while (output->Read(value)) {
        //     std::cout << value << std::endl;
        // }
        
        return ctx;
    }
};

// 2. 创建 Runnable
auto stream_func = [](std::shared_ptr<Context> ctx,
                      const std::string& input,
                      const std::vector<Option>& opts) 
                      -> std::shared_ptr<StreamReader<std::string>> {
    // 生成流式输出
    auto stream = std::make_shared<SimpleStreamReader<std::string>>();
    stream->Add("Line 1");
    stream->Add("Line 2");
    stream->Add("Line 3");
    return stream;
};

InvokeFunc<std::string, std::string> null_invoke = nullptr;
auto runnable = std::make_shared<LambdaRunnable<std::string, std::string>>(
    null_invoke, stream_func, nullptr, nullptr
);

// 3. 设置 callback
auto handler = std::make_shared<MyStreamHandler>();
auto ctx = Context::Background();
ctx = callbacks::AppendHandlers(ctx, callbacks::RunInfo{}, {handler});

// 4. 调用 Stream 方法
auto result_stream = runnable->Stream(ctx, "input", {});

// 内部执行流程:
// 1. OnStart(ctx, "input")
// 2. stream_func(ctx, "input", opts)
// 3. OnEndWithStreamOutput(ctx, result_stream)  ← MyStreamHandler 被调用!
//    └─ [MyStreamHandler] Stream output completed!

// 5. 读取结果
std::string line;
while (result_stream->Read(line)) {
    std::cout << line << std::endl;
}
// 输出:
// Line 1
// Line 2
// Line 3
```

### 示例 2: 多个 Handlers

```cpp
class Handler1 : public callbacks::Handler {
public:
    Context OnEndWithStreamOutput(const Context& ctx, auto output) override {
        std::cout << "[Handler1] Triggered" << std::endl;
        return ctx;
    }
};

class Handler2 : public callbacks::Handler {
public:
    Context OnEndWithStreamOutput(const Context& ctx, auto output) override {
        std::cout << "[Handler2] Triggered" << std::endl;
        return ctx;
    }
};

// 设置多个 handlers
auto handler1 = std::make_shared<Handler1>();
auto handler2 = std::make_shared<Handler2>();

ctx = callbacks::AppendHandlers(ctx, info, {handler1, handler2});

// 调用时,两个 handlers 都会被触发
runnable->Stream(ctx, "test", {});

// 输出:
// [Handler1] Triggered
// [Handler2] Triggered
```

---

## ⚠️ 注意事项和最佳实践

### 1. Stream 消费问题

```cpp
// ❌ 错误: Handler 消费了 stream
class BadHandler : public callbacks::Handler {
public:
    Context OnEndWithStreamOutput(const Context& ctx, auto output) override {
        std::string value;
        while (output->Read(value)) {  // ❌ 消费了 stream!
            ProcessValue(value);
        }
        return ctx;
    }
};

// 问题: 后续的 handler 或用户代码无法读取 stream
```

```cpp
// ✅ 正确: 不消费 stream,只传递或复制
class GoodHandler : public callbacks::Handler {
public:
    Context OnEndWithStreamOutput(const Context& ctx, auto output) override {
        // 只记录事件,不读取内容
        LogEvent("Stream completed");
        
        // 或者传递引用给其他组件(不读取)
        event_sender_->SendStream(output);
        
        return ctx;
    }
};
```

### 2. Context 更新

```cpp
// ✅ 正确: 返回更新后的 Context
class GoodHandler : public callbacks::Handler {
public:
    Context OnEndWithStreamOutput(const Context& ctx, auto output) override {
        Context new_ctx = ctx;
        new_ctx.SetValue("stream_completed", true);
        new_ctx.SetValue("stream_timestamp", GetTimestamp());
        return new_ctx;  // ✅ 返回更新后的 context
    }
};
```

### 3. 异常安全

```cpp
class SafeHandler : public callbacks::Handler {
public:
    Context OnEndWithStreamOutput(const Context& ctx, auto output) override {
        try {
            // 处理逻辑
            ProcessStream(output);
        } catch (const std::exception& e) {
            // 捕获异常,记录日志,但不中断流程
            std::cerr << "Handler error: " << e.what() << std::endl;
        }
        return ctx;  // 总是返回 context
    }
};
```

---

## 🔮 未来改进

### 1. 添加 Stream Copy 支持

```cpp
// 目标实现
template<typename T>
class StreamReader {
public:
    virtual std::shared_ptr<StreamReader<T>> Copy() = 0;
};

template<typename T>
class SimpleStreamReader : public StreamReader<T> {
public:
    std::shared_ptr<StreamReader<T>> Copy() override {
        auto copy = std::make_shared<SimpleStreamReader<T>>();
        copy->data_ = this->data_;  // 复制数据
        copy->position_ = 0;         // 重置位置
        return copy;
    }
};

// 在 OnEndWithStreamOutput 中使用
template <typename T>
std::pair<Context, std::shared_ptr<StreamReader<T>>> OnEndWithStreamOutput(...) {
    Context new_ctx = ctx;
    for (auto& handler : handlers) {
        auto output_copy = output->Copy();  // ✅ 每个 handler 获得独立副本
        new_ctx = handler->OnEndWithStreamOutput(new_ctx, output_copy);
    }
    return {new_ctx, output};
}
```

### 2. 添加 Handler 优先级

```cpp
class PrioritizedHandler : public callbacks::Handler {
public:
    virtual int GetPriority() const { return 0; }  // 新增
};

// 在 OnEndWithStreamOutput 中按优先级排序
template <typename T>
std::pair<Context, std::shared_ptr<StreamReader<T>>> OnEndWithStreamOutput(...) {
    auto handlers = callbacks::GetHandlersFromContext(ctx);
    
    // 按优先级排序
    std::sort(handlers.begin(), handlers.end(), 
              [](auto& a, auto& b) { return a->GetPriority() > b->GetPriority(); });
    
    // 执行
    Context new_ctx = ctx;
    for (auto& handler : handlers) {
        new_ctx = handler->OnEndWithStreamOutput(new_ctx, output);
    }
    return {new_ctx, output};
}
```

---

## 📝 总结

### 核心要点

1. **作用**: 在流式输出完成后触发 callback handlers
2. **调用时机**: `Stream()` 和 `Transform()` 方法返回后
3. **关键特性**: 
   - 遍历所有 handlers
   - 更新 Context
   - 传递 StreamReader (注意不消费)

### 与 Go 版本的差异

| 特性           | Go 版本                   | C++ 版本                |
|----------------|---------------------------|-------------------------|
| Stream 复制    | ✅ 支持 (output.Copy)     | ❌ 暂未支持             |
| Handler 执行   | ✅ 每个 handler 独立副本  | ⚠️ 共享同一个 stream    |
| 类型转换       | ✅ StreamReaderWithConvert| ⚠️ 直接传递             |

### 最佳实践

1. **不要在 handler 中消费 stream** (等待 Copy 机制实现)
2. **正确更新并返回 Context**
3. **做好异常处理**
4. **保持 handler 轻量级**

这就是 `OnEndWithStreamOutput` 的完整实现细节! 🚀
