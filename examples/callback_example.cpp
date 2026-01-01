/*
 * Copyright 2024 CloudWeGo Authors
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

/**
 * Callback Example
 * 
 * 演示如何在 eino_cpp 中使用 callback 机制
 * 
 * 对齐 Go 版本: eino/compose/runnable.go 中的 callback 示例
 */

#include <iostream>
#include <memory>
#include <string>

#include "eino/compose/runnable.h"
#include "eino/callbacks/handler.h"
#include "eino/callbacks/context.h"

using namespace eino;
using namespace eino::compose;

// 示例 Handler: 记录组件执行
class LoggingHandler : public callbacks::Handler {
public:
    LoggingHandler(const std::string& name) : name_(name) {}
    
    Context OnStart(const Context& ctx, const callbacks::CallbackInput& input) override {
        std::cout << "[" << name_ << "] OnStart - Component execution started" << std::endl;
        std::cout << "[" << name_ << "] Input type: " << typeid(input).name() << std::endl;
        return ctx;
    }
    
    Context OnEnd(const Context& ctx, const callbacks::CallbackOutput& output) override {
        std::cout << "[" << name_ << "] OnEnd - Component execution finished" << std::endl;
        std::cout << "[" << name_ << "] Output type: " << typeid(output).name() << std::endl;
        return ctx;
    }
    
    Context OnError(const Context& ctx, const std::string& error) override {
        std::cerr << "[" << name_ << "] OnError - Error occurred: " << error << std::endl;
        return ctx;
    }
    
    // 流式输入/输出的回调
    template<typename T>
    Context OnStartWithStreamInput(const Context& ctx, std::shared_ptr<schema::StreamReader<T>> input) {
        std::cout << "[" << name_ << "] OnStartWithStreamInput - Streaming input started" << std::endl;
        return ctx;
    }
    
    template<typename T>
    Context OnEndWithStreamOutput(const Context& ctx, std::shared_ptr<schema::StreamReader<T>> output) {
        std::cout << "[" << name_ << "] OnEndWithStreamOutput - Streaming output finished" << std::endl;
        return ctx;
    }

private:
    std::string name_;
};

// 示例1: 简单的 Invoke 调用
void example_invoke_with_callbacks() {
    std::cout << "\n=== Example 1: Invoke with Callbacks ===" << std::endl;
    
    // 1. 创建一个简单的 Lambda Runnable
    auto greet_func = [](std::shared_ptr<Context> ctx, 
                         const std::string& input, 
                         const std::vector<Option>& opts) -> std::string {
        std::cout << "  [Function] Processing: " << input << std::endl;
        return "Hello, " + input + "!";
    };
    
    auto runnable = std::make_shared<LambdaRunnable<std::string, std::string>>(greet_func);
    
    // 2. 创建 callback handler
    auto handler = std::make_shared<LoggingHandler>("GreetHandler");
    
    // 3. 设置 callback 到 context
    auto ctx = Context::Background();
    callbacks::RunInfo ri;
    ri.name = "greet_component";
    ri.component = 1;  // Component::Lambda
    ctx = callbacks::AppendHandlers(ctx, ri, {handler});
    
    // 4. 调用 - callbacks 会自动触发!
    try {
        auto result = runnable->Invoke(ctx, "World", {});
        std::cout << "  Result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  Exception: " << e.what() << std::endl;
    }
}

// 示例2: Stream 调用
void example_stream_with_callbacks() {
    std::cout << "\n=== Example 2: Stream with Callbacks ===" << std::endl;
    
    // 1. 创建一个生成多个输出的 Stream Runnable
    auto stream_func = [](std::shared_ptr<Context> ctx, 
                          const std::string& input, 
                          const std::vector<Option>& opts) -> std::shared_ptr<StreamReader<std::string>> {
        std::cout << "  [Function] Streaming for: " << input << std::endl;
        
        std::vector<std::string> results;
        for (int i = 1; i <= 3; ++i) {
            results.push_back(input + " #" + std::to_string(i));
        }
        
        return std::make_shared<SimpleStreamReader<std::string>>(results);
    };
    
    InvokeFunc<std::string, std::string> null_invoke = nullptr;
    auto runnable = std::make_shared<LambdaRunnable<std::string, std::string>>(
        null_invoke, stream_func, nullptr, nullptr);
    
    // 2. 设置 callback
    auto handler = std::make_shared<LoggingHandler>("StreamHandler");
    auto ctx = Context::Background();
    callbacks::RunInfo ri;
    ri.name = "stream_component";
    ctx = callbacks::AppendHandlers(ctx, ri, {handler});
    
    // 3. 调用 Stream
    try {
        auto stream = runnable->Stream(ctx, "Item", {});
        
        std::cout << "  Streaming results:" << std::endl;
        std::string value;
        while (stream && stream->Read(value)) {
            std::cout << "    - " << value << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "  Exception: " << e.what() << std::endl;
    }
}

// 示例3: 错误处理
void example_error_handling() {
    std::cout << "\n=== Example 3: Error Handling with Callbacks ===" << std::endl;
    
    // 1. 创建一个会抛出异常的 Runnable
    auto error_func = [](std::shared_ptr<Context> ctx, 
                         const std::string& input, 
                         const std::vector<Option>& opts) -> std::string {
        std::cout << "  [Function] About to throw error..." << std::endl;
        throw std::runtime_error("Intentional error for testing!");
        return "This won't be reached";
    };
    
    auto runnable = std::make_shared<LambdaRunnable<std::string, std::string>>(error_func);
    
    // 2. 设置 callback
    auto handler = std::make_shared<LoggingHandler>("ErrorHandler");
    auto ctx = Context::Background();
    callbacks::RunInfo ri;
    ri.name = "error_component";
    ctx = callbacks::AppendHandlers(ctx, ri, {handler});
    
    // 3. 调用并捕获错误
    try {
        auto result = runnable->Invoke(ctx, "test", {});
    } catch (const std::exception& e) {
        std::cout << "  Caught exception (as expected): " << e.what() << std::endl;
    }
}

// 主函数
int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "Eino C++ Callback Mechanism Examples" << std::endl;
    std::cout << "Aligned with: eino/compose/runnable.go callbacks" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    example_invoke_with_callbacks();
    example_stream_with_callbacks();
    example_error_handling();
    
    std::cout << "\n==================================================" << std::endl;
    std::cout << "All examples completed!" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    return 0;
}
