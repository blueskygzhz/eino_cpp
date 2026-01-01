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

/**
 * 流式执行Agent完整示例
 * 
 * 本示例演示ChatModelAgent的流式执行全过程，包括：
 * 1. Agent配置与构建（LazyBuild机制）
 * 2. 流式执行的数据流转
 * 3. ReAct循环的逐步执行
 * 4. Callbacks回调机制
 * 5. AgentEvent的流式输出
 */

#include "eino/adk/chat_model_agent.h"
#include "eino/adk/types.h"
#include "eino/components/model.h"
#include "eino/components/tool.h"
#include "eino/schema/types.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using namespace eino;
using namespace eino::adk;
using namespace eino::schema;
using namespace eino::components;

// ============================================================================
// 第一步：定义模拟的ChatModel（支持流式输出）
// ============================================================================

/**
 * MockStreamingChatModel 模拟一个支持流式输出的ChatModel
 * 
 * 流式输出场景：逐个Token返回，而不是等待完整响应
 * 适用于：实时用户体验、长文本生成、降低首字节延迟
 */
class MockStreamingChatModel : public ToolCallingChatModel {
public:
    MockStreamingChatModel(const std::string& name) : name_(name) {}

    // ========================================================================
    // Stream方法：核心流式执行逻辑
    // ========================================================================
    std::shared_ptr<StreamReader<Message>> Stream(
        void* ctx,
        const std::vector<Message>& messages,
        const std::vector<ToolInfo>& tools,
        const CallbacksOption* callbacks = nullptr) override {
        
        std::cout << "\n[ChatModel::Stream] 开始流式生成..." << std::endl;
        std::cout << "  输入消息数: " << messages.size() << std::endl;
        std::cout << "  可用工具数: " << tools.size() << std::endl;

        // 创建StreamReader用于返回流式数据
        auto reader = std::make_shared<SimpleStreamReader<Message>>();
        
        // 在独立线程中逐步生成响应（模拟真实LLM的Token-by-Token输出）
        std::thread([reader, tools, messages]() {
            try {
                // 检查是否有工具调用请求
                bool has_tool_request = false;
                for (const auto& msg : messages) {
                    if (msg.role == RoleType::User && 
                        msg.content.find("weather") != std::string::npos) {
                        has_tool_request = true;
                        break;
                    }
                }

                if (has_tool_request && !tools.empty()) {
                    // ============================================================
                    // 场景1：生成工具调用（Tool Call）
                    // ============================================================
                    std::cout << "  [ChatModel] 检测到工具调用需求" << std::endl;
                    
                    // 第一个Chunk：开始Assistant响应
                    Message chunk1;
                    chunk1.role = RoleType::Assistant;
                    chunk1.content = "我来";
                    std::cout << "  [Stream Chunk 1] content: \"" << chunk1.content << "\"" << std::endl;
                    reader->Send(chunk1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    // 第二个Chunk：继续生成文本
                    Message chunk2;
                    chunk2.role = RoleType::Assistant;
                    chunk2.content = "帮你查询";
                    std::cout << "  [Stream Chunk 2] content: \"" << chunk2.content << "\"" << std::endl;
                    reader->Send(chunk2);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    // 第三个Chunk：工具调用信息
                    Message chunk3;
                    chunk3.role = RoleType::Assistant;
                    chunk3.content = "天气...";
                    
                    // 构造ToolCall结构
                    ToolCall tool_call;
                    tool_call.id = "call_001";
                    tool_call.type = "function";
                    
                    ToolCallFunction func;
                    func.name = "get_weather";
                    func.arguments = "{\"city\":\"北京\"}";
                    tool_call.function = func;
                    
                    chunk3.tool_calls.push_back(tool_call);
                    
                    std::cout << "  [Stream Chunk 3] ToolCall: " << func.name 
                              << "(" << func.arguments << ")" << std::endl;
                    reader->Send(chunk3);
                    
                } else {
                    // ============================================================
                    // 场景2：普通文本响应（无工具调用）
                    // ============================================================
                    std::cout << "  [ChatModel] 生成普通文本响应" << std::endl;
                    
                    std::vector<std::string> tokens = {
                        "你好", "！", "我是", "一个", "AI", "助手", "。"
                    };
                    
                    for (size_t i = 0; i < tokens.size(); ++i) {
                        Message chunk;
                        chunk.role = RoleType::Assistant;
                        chunk.content = tokens[i];
                        
                        std::cout << "  [Stream Chunk " << (i+1) << "] \""
                                  << chunk.content << "\"" << std::endl;
                        reader->Send(chunk);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                }

                std::cout << "  [ChatModel::Stream] 流式生成完成\n" << std::endl;
                reader->Close();
                
            } catch (const std::exception& e) {
                std::cerr << "  [ChatModel::Stream] 错误: " << e.what() << std::endl;
                reader->Close();
            }
        }).detach();

        return reader;
    }

    // Invoke方法：非流式执行（阻塞等待完整响应）
    Message Invoke(
        void* ctx,
        const std::vector<Message>& messages,
        const std::vector<ToolInfo>& tools,
        const CallbacksOption* callbacks = nullptr) override {
        
        std::cout << "\n[ChatModel::Invoke] 非流式执行..." << std::endl;
        
        Message response;
        response.role = RoleType::Assistant;
        response.content = "这是一个完整的响应（非流式）";
        
        return response;
    }

    ToolInfo Info(void* ctx) override {
        ToolInfo info;
        info.name = name_;
        info.description = "模拟流式ChatModel";
        return info;
    }

private:
    std::string name_;
};

// ============================================================================
// 第二步：定义工具（Tool）
// ============================================================================

/**
 * WeatherTool: 查询天气的工具
 * 在ReAct循环中，当ChatModel生成ToolCall时会调用此工具
 */
class WeatherTool : public BaseTool {
public:
    ToolInfo Info(void* ctx) override {
        ToolInfo info;
        info.name = "get_weather";
        info.description = "获取指定城市的天气信息";
        
        // 定义参数Schema
        info.params_def = R"({
            "type": "object",
            "properties": {
                "city": {
                    "type": "string",
                    "description": "城市名称"
                }
            },
            "required": ["city"]
        })";
        
        return info;
    }

    ToolMessage InvokableTool(
        void* ctx,
        const std::string& arguments,
        const CallbacksOption* callbacks = nullptr) override {
        
        std::cout << "\n[Tool::InvokableTool] 执行工具调用..." << std::endl;
        std::cout << "  工具名称: get_weather" << std::endl;
        std::cout << "  参数: " << arguments << std::endl;
        
        // 模拟工具执行
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        ToolMessage result;
        result.role = RoleType::Tool;
        result.content = "北京天气：晴天，温度25°C";
        
        std::cout << "  工具返回: " << result.content << std::endl;
        
        return result;
    }
};

// ============================================================================
// 第三步：创建并执行流式Agent
// ============================================================================

void StreamingAgentExample() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "流式Agent执行示例 - 详细执行过程" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    // ========================================================================
    // 步骤1：准备组件
    // ========================================================================
    std::cout << "\n[步骤1] 准备Agent组件..." << std::endl;
    
    void* ctx = nullptr;
    
    // 1.1 创建ChatModel
    auto chat_model = std::make_shared<MockStreamingChatModel>("gpt-4");
    std::cout << "  ✓ 创建ChatModel: gpt-4" << std::endl;
    
    // 1.2 创建工具
    auto weather_tool = std::make_shared<WeatherTool>();
    std::cout << "  ✓ 创建Tool: get_weather" << std::endl;

    // ========================================================================
    // 步骤2：配置ChatModelAgent
    // ========================================================================
    std::cout << "\n[步骤2] 配置ChatModelAgent..." << std::endl;
    
    auto config = std::make_shared<ChatModelAgentConfig>();
    config->name = "weather_assistant";
    config->description = "天气查询助手";
    config->instruction = "你是一个天气助手，可以帮用户查询天气信息。";
    config->model = chat_model.get();
    config->max_iterations = 10;  // ReAct最大循环次数
    
    // 配置工具
    config->tools_config.tools.push_back(weather_tool.get());
    config->tools_config.return_directly["get_weather"] = false;  // 工具执行后继续循环
    
    std::cout << "  ✓ Agent名称: " << config->name << std::endl;
    std::cout << "  ✓ 最大迭代次数: " << config->max_iterations << std::endl;
    std::cout << "  ✓ 工具数量: " << config->tools_config.tools.size() << std::endl;

    // ========================================================================
    // 步骤3：创建Agent（此时仅配置，未构建执行图）
    // ========================================================================
    std::cout << "\n[步骤3] 创建ChatModelAgent..." << std::endl;
    
    auto agent = NewChatModelAgent(ctx, config);
    std::cout << "  ✓ Agent创建成功" << std::endl;
    std::cout << "  ℹ 注意：此时Compose Graph尚未构建（LazyBuild机制）" << std::endl;

    // ========================================================================
    // 步骤4：准备输入（启用流式输出）
    // ========================================================================
    std::cout << "\n[步骤4] 准备AgentInput..." << std::endl;
    
    auto input = std::make_shared<AgentInput>();
    input->enable_streaming = true;  // 关键：启用流式输出
    
    // 添加用户消息
    Message user_msg;
    user_msg.role = RoleType::User;
    user_msg.content = "北京天气怎么样？";
    input->messages.push_back(user_msg);
    
    std::cout << "  ✓ 用户消息: \"" << user_msg.content << "\"" << std::endl;
    std::cout << "  ✓ enable_streaming: true" << std::endl;

    // ========================================================================
    // 步骤5：执行Agent（LazyBuild触发）
    // ========================================================================
    std::cout << "\n[步骤5] 调用Agent::Run()..." << std::endl;
    std::cout << "\n" << std::string(70, '-') << std::endl;
    std::cout << "执行流程开始（详细日志）" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    auto event_iterator = agent->Run(ctx, input, {});
    
    std::cout << "\n  ✓ AsyncIterator已返回" << std::endl;
    std::cout << "  ℹ Agent在独立线程中执行，主线程将消费AgentEvent流\n" << std::endl;

    // ========================================================================
    // 步骤6：消费AgentEvent流（实时处理事件）
    // ========================================================================
    std::cout << "[步骤6] 消费AgentEvent流..." << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    int event_count = 0;
    
    // 阻塞读取事件，直到流关闭
    while (true) {
        auto event = event_iterator->Next();
        
        if (!event) {
            std::cout << "\n[AgentEvent流] 流已关闭" << std::endl;
            break;  // 流结束
        }

        event_count++;
        std::cout << "\n[AgentEvent #" << event_count << "] 收到事件" << std::endl;
        std::cout << "  Agent名称: " << event->agent_name << std::endl;
        
        // 检查错误
        if (event->HasError()) {
            std::cout << "  ❌ 错误: " << event->error_msg << std::endl;
            continue;
        }

        // 处理输出
        if (event->output && event->output->message_output) {
            auto msg_var = event->output->message_output;
            
            std::cout << "  输出类型: " 
                      << (msg_var->is_streaming ? "流式" : "非流式") << std::endl;
            std::cout << "  角色: " 
                      << (msg_var->role == RoleType::Assistant ? "Assistant" : "Tool") 
                      << std::endl;

            if (msg_var->is_streaming && msg_var->message_stream) {
                // ============================================================
                // 关键：处理流式输出
                // ============================================================
                std::cout << "  [流式内容] 开始接收..." << std::endl;
                
                auto stream = msg_var->message_stream;
                int chunk_count = 0;
                std::string full_content;
                
                while (true) {
                    auto chunk = stream->Recv();
                    if (!chunk) {
                        break;  // 流结束
                    }
                    
                    chunk_count++;
                    full_content += chunk->content;
                    
                    std::cout << "    [Chunk " << chunk_count << "] \""
                              << chunk->content << "\"";
                    
                    // 显示ToolCall（如果有）
                    if (!chunk->tool_calls.empty()) {
                        std::cout << " + ToolCall[" << chunk->tool_calls.size() << "]";
                    }
                    std::cout << std::endl;
                }
                
                std::cout << "  [流式内容] 完成，共 " << chunk_count << " 个Chunk" << std::endl;
                std::cout << "  [完整内容] \"" << full_content << "\"" << std::endl;
                
            } else if (!msg_var->is_streaming && msg_var->message) {
                // 非流式输出
                std::cout << "  内容: \"" << msg_var->message->content << "\"" << std::endl;
            }
        }

        // 处理Action
        if (event->action) {
            std::cout << "  动作:" << std::endl;
            
            if (event->action->exit) {
                std::cout << "    - Exit: true（Agent执行完成）" << std::endl;
            }
            if (event->action->transfer_to_agent) {
                std::cout << "    - TransferTo: " 
                          << event->action->transfer_to_agent->dest_agent_name << std::endl;
            }
            if (event->action->interrupted) {
                std::cout << "    - Interrupted: 需要Resume" << std::endl;
            }
        }
    }

    std::cout << std::string(70, '-') << std::endl;
    std::cout << "执行流程结束" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    // ========================================================================
    // 步骤7：总结执行过程
    // ========================================================================
    std::cout << "\n[步骤7] 执行完成总结" << std::endl;
    std::cout << "  总事件数: " << event_count << std::endl;
    std::cout << "  ✓ Agent执行成功" << std::endl;
}

// ============================================================================
// 第四步：执行流程架构说明
// ============================================================================

void PrintArchitecture() {
    std::cout << R"(
================================================================================
流式Agent执行架构
================================================================================

┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. ADK Layer（用户接口层）                                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│   ChatModelAgent::Run(input)                                                │
│         ↓                                                                   │
│   [LazyBuild触发] BuildRunFunc() → 构建Compose结构                          │
│         ↓                                                                   │
│   AsyncIterator<AgentEvent> ← 立即返回给用户                                │
│         ↓                                                                   │
│   [独立线程] 执行run_func_                                                   │
└─────────────────────────────────────────────────────────────────────────────┘
                                 ↓
┌─────────────────────────────────────────────────────────────────────────────┐
│ 2. Compose Layer（执行引擎层）                                               │
├─────────────────────────────────────────────────────────────────────────────┤
│   Chain: genModelInput → Graph/ChatModel                                   │
│                                                                             │
│   [有工具] ReAct Graph结构:                                                 │
│                                                                             │
│        START                                                                │
│          ↓                                                                  │
│     ChatModel ────→ [生成Message]                                           │
│          ↓                                                                  │
│     [是否有ToolCall?]                                                       │
│       ↙     ↘                                                               │
│     是        否                                                            │
│      ↓         ↓                                                            │
│   ToolsNode   END（输出AgentEvent）                                         │
│      ↓                                                                      │
│   [执行工具]                                                                │
│      ↓                                                                      │
│   [return_directly?]                                                        │
│     ↙    ↘                                                                  │
│   是      否                                                                │
│    ↓       ↓                                                                │
│   END   ChatModel（循环）                                                   │
│                                                                             │
│   [无工具] Simple Chain:                                                    │
│      genModelInput → ChatModel → END                                        │
└─────────────────────────────────────────────────────────────────────────────┘
                                 ↓
┌─────────────────────────────────────────────────────────────────────────────┐
│ 3. Components Layer（组件执行层）                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│   ChatModel::Stream(messages, tools, callbacks)                            │
│         ↓                                                                   │
│   返回 StreamReader<Message>                                                │
│         ↓                                                                   │
│   [异步线程] 逐个Token生成:                                                  │
│      Chunk1: "你好"                                                         │
│      Chunk2: "，"                                                           │
│      Chunk3: "..." + ToolCall                                               │
│         ↓                                                                   │
│   Callbacks触发（实时）:                                                     │
│      - onChatModelStart()                                                   │
│      - onChatModelStream(chunk)  ← 每个Chunk                                │
│      - onChatModelEnd(complete_msg)                                         │
└─────────────────────────────────────────────────────────────────────────────┘
                                 ↓
┌─────────────────────────────────────────────────────────────────────────────┐
│ 4. Event Layer（事件流转层）                                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│   Callbacks → 构造 AgentEvent                                               │
│         ↓                                                                   │
│   AgentEvent {                                                              │
│     agent_name: "weather_assistant"                                         │
│     output: {                                                               │
│       message_output: {                                                     │
│         is_streaming: true                                                  │
│         message_stream: StreamReader<Message>                               │
│         role: Assistant                                                     │
│       }                                                                     │
│     }                                                                       │
│     action: { exit: false }                                                 │
│   }                                                                         │
│         ↓                                                                   │
│   generator->Send(event)  ← 发送到AsyncIterator                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                 ↓
┌─────────────────────────────────────────────────────────────────────────────┐
│ 5. User Layer（用户消费层）                                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│   while (event = iterator->Next()) {                                        │
│     if (event->output->message_output->is_streaming) {                      │
│       auto stream = event->output->message_output->message_stream;          │
│       while (chunk = stream->Recv()) {                                      │
│         // 实时显示：chunk->content                                          │
│         display(chunk->content);  ← 用户看到逐字输出                         │
│       }                                                                     │
│     }                                                                       │
│   }                                                                         │
└─────────────────────────────────────────────────────────────────────────────┘

================================================================================
关键特性
================================================================================

1. LazyBuild机制：
   - 首次Run()调用时触发BuildRunFunc()
   - 根据配置（有无工具）构建Simple Chain或ReAct Graph
   - 构建完成后frozen=true，配置不可修改

2. 流式执行：
   - enable_streaming=true → 调用Stream()而非Invoke()
   - StreamReader<Message>在独立线程中逐Token生成
   - 每个Chunk立即通过Callbacks转换为AgentEvent

3. ReAct循环：
   - ChatModel生成ToolCall → ToolsNode执行工具 → 返回ChatModel
   - 循环直到：无ToolCall、return_directly=true、达到max_iterations

4. 异步非阻塞：
   - Run()立即返回AsyncIterator
   - Agent在独立线程执行
   - 用户通过Next()阻塞等待事件（生产者-消费者模式）

5. Callbacks实时监控：
   - onChatModelStream(chunk) ← 每个Token
   - onToolEnd(result) ← 工具执行完成
   - 转换为AgentEvent实时发送

================================================================================
)" << std::endl;
}

// ============================================================================
// Main函数
// ============================================================================

int main() {
    try {
        // 打印架构说明
        PrintArchitecture();
        
        // 执行流式Agent示例
        StreamingAgentExample();
        
        std::cout << "\n示例执行完成！" << std::endl;
        
        // 等待所有异步线程完成
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
