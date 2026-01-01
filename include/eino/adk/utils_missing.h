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

#ifndef EINO_CPP_ADK_UTILS_MISSING_H_
#define EINO_CPP_ADK_UTILS_MISSING_H_

#include "types.h"
#include "async_iterator.h"
#include "../schema/types.h"
#include "../components/tool.h"
#include <string>
#include <vector>
#include <memory>
#include <utility>

namespace eino {
namespace adk {

// ============================================================================
// 缺失能力补充 - 完全对齐eino/adk/utils.go
// ============================================================================

// GenTransferMessages 生成agent转移消息
// 对齐 eino/adk/utils.go:64-70
//
// 返回一对消息:
// - assistant message: 包含tool call
// - tool message: 包含transfer结果
std::pair<Message, Message> GenTransferMessages(
    void* ctx,
    const std::string& dest_agent_name);

// setAutomaticClose 为event的message stream设置自动关闭
// 对齐 eino/adk/utils.go:73-78
//
// 确保即使events没有被处理, MessageStream也会被关闭
void setAutomaticClose(std::shared_ptr<AgentEvent> event);

// AgentEventWrapper 包装AgentEvent, 缓存拼接后的消息
// 对齐 eino/adk内部实现
struct AgentEventWrapper {
    std::shared_ptr<AgentEvent> event;
    Message concatenated_message;  // 缓存拼接后的消息
};

// getMessageFromWrappedEvent 从wrapped event提取message
// 对齐 eino/adk/utils.go:80-136
//
// 处理streaming和non-streaming MessageVariant
Message getMessageFromWrappedEvent(const std::shared_ptr<AgentEventWrapper>& event);

// copyAgentEvent 拷贝AgentEvent
// 对齐 eino/adk/utils.go:138-188
//
// 如果MessageVariant是流式, MessageStream会被拷贝
// 这确保:
// - 每个副本有独立的MessageStream
// - 安全从MessageStream接收
// - Message chunks不会被拷贝 (使用共享指针)
std::shared_ptr<AgentEvent> copyAgentEvent(const std::shared_ptr<AgentEvent>& ae);

// GetMessage 从AgentEvent提取Message
// 对齐 eino/adk/utils.go:190-207
//
// 处理streaming和non-streaming MessageVariant
std::pair<Message, std::shared_ptr<AgentEvent>> GetMessage(
    std::shared_ptr<AgentEvent> event);

// genErrorIter 生成包含错误的iterator
// 对齐 eino/adk/utils.go:209-214
std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> genErrorIter(
    const std::string& error_msg);

// ============================================================================
// agent_tool辅助函数 - 对齐eino/adk/agent_tool.go
// ============================================================================

// getReactChatHistory 从State获取ReAct聊天历史
// 对齐 eino/adk/agent_tool.go:206-232
//
// 获取compose State中的消息历史并:
// 1. 移除最后一条assistant消息 (tool call消息)
// 2. 添加transfer messages
// 3. 过滤system消息
// 4. 重写assistant和tool消息
std::vector<Message> getReactChatHistory(
    void* ctx,
    const std::string& dest_agent_name);

// rewriteMessage 重写消息中的agent名称
// 对齐 eino内部实现
Message rewriteMessage(const Message& msg, const std::string& agent_name);

// getOptionsByAgentName 根据agent名称提取选项
// 对齐 eino/adk/agent_tool.go:202-212
std::vector<std::shared_ptr<AgentRunOption>> getOptionsByAgentName(
    const std::string& agent_name,
    const std::vector<std::shared_ptr<components::Option>>& opts);

// withAgentToolOptions 创建tool.Option包装
// 对齐 eino/adk/agent_tool.go:196-200
std::shared_ptr<components::Option> withAgentToolOptions(
    const std::string& agent_name,
    const std::vector<std::shared_ptr<AgentRunOption>>& opts);

// ============================================================================
// 辅助工具函数 - 新增补充
// ============================================================================

// GenerateUUID 生成UUID字符串
std::string GenerateUUID();

// GenTransferToAgentInstruction 生成transfer to agent的指令文本
// 对齐 eino/adk/utils.go:44-56
// 
// 生成类似如下的指令:
// """
// You have access to the following agents:
// - [agent1]: agent1 description
// - [agent2]: agent2 description
//
// Use the 'transfer_to_agent' tool to hand off tasks.
// """
std::string GenTransferToAgentInstruction(
    void* ctx, 
    const std::vector<std::shared_ptr<Agent>>& agents);

// ConcatInstructions 拼接多个指令字符串
// 对齐 eino/adk/utils.go:58-68
//
// 用 "\n\n" 连接非空的指令部分
std::string ConcatInstructions(const std::vector<std::string>& parts);

// CopyMap 拷贝一个 map
// 对齐 eino/adk/utils.go:48-53
template<typename K, typename V>
std::map<K, V> CopyMap(const std::map<K, V>& m) {
    return std::map<K, V>(m.begin(), m.end());
}

// GenErrorIter 生成包含错误的迭代器
// 对齐 eino/adk/utils.go:140-145
std::shared_ptr<AsyncIterator<AgentEvent>> GenErrorIter(const std::string& error);

} // namespace adk
} // namespace eino

#endif // EINO_CPP_ADK_UTILS_MISSING_H_
