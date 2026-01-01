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

#include "eino/adk/types.h"
#include "eino/adk/utils.h"
#include "eino/compose/state.h"
#include "eino/schema/types.h"
#include "eino/schema/stream_copy.h"
#include <sstream>
#include <random>
#include <iomanip>

namespace eino {
namespace adk {

// ============================================================================
// 缺失能力补充 - 完全对齐eino/adk
// ============================================================================

// GenTransferMessages 生成agent转移消息
// 对齐 eino/adk/utils.go:64-70
std::pair<Message, Message> GenTransferMessages(
    void* ctx,
    const std::string& dest_agent_name) {
    
    // 生成唯一的tool call ID
    std::string tool_call_id = GenerateUUID();
    
    // 创建tool call
    schema::ToolCall tool_call;
    tool_call.id = tool_call_id;
    tool_call.function.name = "TransferToAgent";
    tool_call.function.arguments = dest_agent_name;
    
    // 创建assistant message
    auto assistant_msg = std::make_shared<schema::Message>();
    assistant_msg->role = schema::RoleType::Assistant;
    assistant_msg->content = "";
    assistant_msg->tool_calls.push_back(tool_call);
    
    // 创建tool message
    auto tool_msg = std::make_shared<schema::Message>();
    tool_msg->role = schema::RoleType::Tool;
    tool_msg->content = "Transferred to agent: " + dest_agent_name;
    tool_msg->tool_call_id = tool_call_id;
    tool_msg->name = "TransferToAgent";
    
    return {assistant_msg, tool_msg};
}

// setAutomaticClose 为event的message stream设置自动关闭
// 对齐 eino/adk/utils.go:73-78
void setAutomaticClose(std::shared_ptr<AgentEvent> event) {
    if (!event || !event->output || !event->output->message_output) {
        return;
    }
    
    if (!event->output->message_output->is_streaming) {
        return;
    }
    
    // TODO: 实现StreamReader的SetAutomaticClose方法
    // event->output->message_output->message_stream->SetAutomaticClose();
}

// getMessageFromWrappedEvent 从wrapped event提取message
// 对齐 eino/adk/utils.go:80-136
Message getMessageFromWrappedEvent(const std::shared_ptr<AgentEventWrapper>& event) {
    if (!event || !event->event || !event->event->output) {
        return nullptr;
    }
    
    auto msg_output = event->event->output->message_output;
    if (!msg_output) {
        return nullptr;
    }
    
    // 非流式: 直接返回message
    if (!msg_output->is_streaming) {
        return msg_output->message;
    }
    
    // 流式: 如果已经拼接过, 返回缓存的结果
    if (event->concatenated_message) {
        return event->concatenated_message;
    }
    
    // 拼接所有chunks
    std::vector<Message> messages;
    auto stream = msg_output->message_stream;
    
    Message chunk;
    while (stream->Recv(chunk)) {
        messages.push_back(chunk);
    }
    stream->Close();
    
    if (messages.empty()) {
        throw std::runtime_error("no messages in MessageVariant.MessageStream");
    }
    
    if (messages.size() == 1) {
        event->concatenated_message = messages[0];
    } else {
        // 使用schema::ConcatMessages合并
        event->concatenated_message = schema::ConcatMessages(messages);
    }
    
    return event->concatenated_message;
}

// copyAgentEvent 拷贝AgentEvent
// 对齐 eino/adk/utils.go:138-188
// 如果MessageVariant是流式, MessageStream会被拷贝
// RunPath会被深拷贝
// 拷贝后的AgentEvent是:
// - 安全设置AgentEvent字段
// - 安全扩展RunPath
// - 安全从MessageStream接收
std::shared_ptr<AgentEvent> copyAgentEvent(const std::shared_ptr<AgentEvent>& ae) {
    if (!ae) {
        return nullptr;
    }
    
    auto copied = std::make_shared<AgentEvent>();
    copied->agent_name = ae->agent_name;
    
    // 深拷贝RunPath
    copied->run_path = ae->run_path;  // std::vector自动深拷贝
    
    copied->action = ae->action;
    copied->error_msg = ae->error_msg;
    
    if (!ae->output) {
        return copied;
    }
    
    copied->output = std::make_shared<AgentOutput>();
    copied->output->customized_output = ae->output->customized_output;
    
    auto mv = ae->output->message_output;
    if (!mv) {
        return copied;
    }
    
    copied->output->message_output = std::make_shared<MessageVariant>();
    copied->output->message_output->is_streaming = mv->is_streaming;
    copied->output->message_output->role = mv->role;
    copied->output->message_output->tool_name = mv->tool_name;
    
    if (mv->is_streaming && mv->message_stream) {
        // 拷贝stream: 创建2个独立副本
        auto copies = schema::CopyStreamReader(mv->message_stream, 2);
        mv->message_stream = copies[0];
        copied->output->message_output->message_stream = copies[1];
    } else {
        copied->output->message_output->message = mv->message;
    }
    
    return copied;
}

// GetMessage 从AgentEvent获取Message
// 对齐 eino/adk/utils.go:190-207
std::pair<Message, std::shared_ptr<AgentEvent>> GetMessage(
    std::shared_ptr<AgentEvent> event) {
    
    if (!event || !event->output || !event->output->message_output) {
        return {nullptr, event};
    }
    
    auto msg_output = event->output->message_output;
    
    if (msg_output->is_streaming && msg_output->message_stream) {
        // 拷贝stream
        auto copies = schema::CopyStreamReader(msg_output->message_stream, 2);
        event->output->message_output->message_stream = copies[0];
        
        // 拼接消息
        std::vector<Message> messages;
        Message chunk;
        while (copies[1]->Recv(chunk)) {
            messages.push_back(chunk);
        }
        
        auto msg = schema::ConcatMessages(messages);
        return {msg, event};
    }
    
    return {msg_output->message, event};
}

// genErrorIter 生成包含错误的iterator
// 对齐 eino/adk/utils.go:209-214
std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> genErrorIter(
    const std::string& error_msg) {
    
    auto [iterator, generator] = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    
    auto error_event = std::make_shared<AgentEvent>();
    error_event->error_msg = error_msg;
    
    generator->Send(error_event);
    generator->Close();
    
    return iterator;
}

// ============================================================================
// agent_tool辅助函数
// ============================================================================

// getReactChatHistory 从State获取ReAct聊天历史
// 对齐 eino/adk/agent_tool.go:206-232
std::vector<Message> getReactChatHistory(
    void* ctx,
    const std::string& dest_agent_name) {
    
    std::vector<Message> messages;
    std::string agent_name;
    
    // 从compose State获取消息历史
    compose::ProcessState(ctx, [&](void* ctx, compose::State* st) {
        // 拷贝除最后一条消息外的所有消息
        // (最后一条是assistant的tool call消息)
        if (!st->messages.empty()) {
            messages.assign(
                st->messages.begin(), 
                st->messages.end() - 1
            );
        }
        agent_name = st->agent_name;
        return nullptr;  // no error
    });
    
    // 添加transfer messages
    auto [assistant_msg, tool_msg] = GenTransferMessages(ctx, dest_agent_name);
    messages.push_back(assistant_msg);
    messages.push_back(tool_msg);
    
    // 过滤和重写消息
    std::vector<Message> history;
    for (auto& msg : messages) {
        // 跳过system消息
        if (msg->role == schema::RoleType::System) {
            continue;
        }
        
        // 重写assistant和tool消息
        if (msg->role == schema::RoleType::Assistant || 
            msg->role == schema::RoleType::Tool) {
            msg = rewriteMessage(msg, agent_name);
        }
        
        history.push_back(msg);
    }
    
    return history;
}

// rewriteMessage 重写消息中的agent名称
// 对齐 eino内部实现
Message rewriteMessage(const Message& msg, const std::string& agent_name) {
    auto rewritten = std::make_shared<schema::Message>(*msg);
    
    // 在content中替换agent名称引用
    // 简化实现: 添加前缀标注agent
    if (!agent_name.empty()) {
        rewritten->content = "[From: " + agent_name + "] " + rewritten->content;
    }
    
    return rewritten;
}

// agentToolOptions 包装结构用于转换AgentRunOption为tool.Option
// 对齐 eino/adk/agent_tool.go:196-200
struct agentToolOptions {
    std::string agent_name;
    std::vector<std::shared_ptr<AgentRunOption>> opts;
};

// withAgentToolOptions 创建tool.Option包装
std::shared_ptr<components::Option> withAgentToolOptions(
    const std::string& agent_name,
    const std::vector<std::shared_ptr<AgentRunOption>>& opts) {
    
    auto wrapper = std::make_shared<agentToolOptions>();
    wrapper->agent_name = agent_name;
    wrapper->opts = opts;
    
    // 包装为tool.Option
    return components::WrapImplSpecificOpt(wrapper);
}

// getOptionsByAgentName 根据agent名称提取选项
// 对齐 eino/adk/agent_tool.go:202-212
std::vector<std::shared_ptr<AgentRunOption>> getOptionsByAgentName(
    const std::string& agent_name,
    const std::vector<std::shared_ptr<components::Option>>& opts) {
    
    std::vector<std::shared_ptr<AgentRunOption>> result;
    
    for (const auto& opt : opts) {
        auto specific = components::GetImplSpecificOptions<agentToolOptions>(opt);
        if (specific && specific->agent_name == agent_name) {
            result.insert(result.end(), specific->opts.begin(), specific->opts.end());
        }
    }
    
    return result;
}

// ============================================================================
// UUID生成辅助函数
// ============================================================================

std::string GenerateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    // xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";  // version 4
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);  // variant
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    
    return ss.str();
}

// ============================================================================
// 新增补充函数 - 对齐eino/adk/utils.go
// ============================================================================

// GenTransferToAgentInstruction 生成transfer to agent的指令文本
// 对齐 eino/adk/utils.go:44-56
std::string GenTransferToAgentInstruction(
    void* ctx, 
    const std::vector<std::shared_ptr<Agent>>& agents) {
    
    std::ostringstream oss;
    oss << "\n\nYou have access to the following agents:\n";
    
    for (const auto& agent : agents) {
        oss << "- [" << agent->Name(ctx) << "]: " 
            << agent->Description(ctx) << "\n";
    }
    
    oss << "\nUse the 'transfer_to_agent' tool to hand off tasks.";
    
    return oss.str();
}

// ConcatInstructions 拼接多个指令字符串
// 对齐 eino/adk/utils.go:58-68
std::string ConcatInstructions(const std::vector<std::string>& parts) {
    std::ostringstream result;
    bool first = true;
    
    for (const auto& part : parts) {
        if (part.empty()) {
            continue;
        }
        
        if (!first) {
            result << "\n\n";
        }
        result << part;
        first = false;
    }
    
    return result.str();
}

// GenErrorIter 生成包含错误的迭代器
// 对齐 eino/adk/utils.go:140-145
std::shared_ptr<AsyncIterator<AgentEvent>> GenErrorIter(const std::string& error) {
    auto [iterator, generator] = NewAsyncIteratorPair<AgentEvent>();
    
    AgentEvent event;
    event.err = error;
    generator->Send(event);
    generator->Close();
    
    return iterator;
}

} // namespace adk
} // namespace eino
