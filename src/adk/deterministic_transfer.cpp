/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Licensed under the Apache License, Version 2.0 (the \"License\");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an \"AS IS\" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../../include/eino/adk/deterministic_transfer.h"
#include "../../include/eino/adk/utils.h"
#include "../../include/eino/schema/types.h"
#include <thread>

namespace eino {
namespace adk {

// ============================================================================
// Factory Function
// ============================================================================

std::shared_ptr<Agent> AgentWithDeterministicTransferTo(
    void* ctx,
    const std::shared_ptr<DeterministicTransferConfig>& config) {
    
    if (!config || !config->agent) {
        throw std::invalid_argument("DeterministicTransferConfig requires valid agent");
    }
    
    // 检查是否是ResumableAgent
    auto resumable_agent = std::dynamic_pointer_cast<ResumableAgent>(config->agent);
    if (resumable_agent) {
        return std::make_shared<ResumableAgentWithDeterministicTransferToImpl>(
            resumable_agent, config->to_agent_names);
    }
    
    return std::make_shared<AgentWithDeterministicTransferToImpl>(
        config->agent, config->to_agent_names);
}

// ============================================================================
// AgentWithDeterministicTransferToImpl
// ============================================================================

AgentWithDeterministicTransferToImpl::AgentWithDeterministicTransferToImpl(
    std::shared_ptr<Agent> agent,
    const std::vector<std::string>& to_agent_names)
    : agent_(agent), to_agent_names_(to_agent_names) {}

std::string AgentWithDeterministicTransferToImpl::Name(void* ctx) {
    return agent_->Name(ctx);
}

std::string AgentWithDeterministicTransferToImpl::Description(void* ctx) {
    return agent_->Description(ctx);
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> 
AgentWithDeterministicTransferToImpl::Run(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    // 执行原始Agent
    auto agent_iter = agent_->Run(ctx, input, options);
    
    // 创建新的迭代器对
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iterator = pair.first;
    auto generator = pair.second;
    
    // 在后台线程中添加转移动作
    std::thread([ctx, agent_iter, generator, to_agent_names = to_agent_names_]() {
        AppendTransferAction(ctx, agent_iter, generator, to_agent_names);
    }).detach();
    
    return iterator;
}

// ============================================================================
// ResumableAgentWithDeterministicTransferToImpl
// ============================================================================

ResumableAgentWithDeterministicTransferToImpl::ResumableAgentWithDeterministicTransferToImpl(
    std::shared_ptr<ResumableAgent> agent,
    const std::vector<std::string>& to_agent_names)
    : agent_(agent), to_agent_names_(to_agent_names) {}

std::string ResumableAgentWithDeterministicTransferToImpl::Name(void* ctx) {
    return agent_->Name(ctx);
}

std::string ResumableAgentWithDeterministicTransferToImpl::Description(void* ctx) {
    return agent_->Description(ctx);
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> 
ResumableAgentWithDeterministicTransferToImpl::Run(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    auto agent_iter = agent_->Run(ctx, input, options);
    
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iterator = pair.first;
    auto generator = pair.second;
    
    std::thread([ctx, agent_iter, generator, to_agent_names = to_agent_names_]() {
        AppendTransferAction(ctx, agent_iter, generator, to_agent_names);
    }).detach();
    
    return iterator;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> 
ResumableAgentWithDeterministicTransferToImpl::Resume(
    void* ctx,
    const std::shared_ptr<ResumeInfo>& info,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    auto agent_iter = agent_->Resume(ctx, info, options);
    
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iterator = pair.first;
    auto generator = pair.second;
    
    std::thread([ctx, agent_iter, generator, to_agent_names = to_agent_names_]() {
        AppendTransferAction(ctx, agent_iter, generator, to_agent_names);
    }).detach();
    
    return iterator;
}

// ============================================================================
// Helper Functions
// ============================================================================

void AppendTransferAction(
    void* ctx,
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> agent_iter,
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> generator,
    const std::vector<std::string>& to_agent_names) {
    
    try {
        bool interrupted = false;
        std::shared_ptr<AgentEvent> event;
        
        // 转发所有原始事件
        while (agent_iter->Next(event)) {
            generator->Send(event);
            
            // 检查是否被中断
            if (event->action && event->action->interrupted) {
                interrupted = true;
            } else {
                interrupted = false;
            }
        }
        
        // 如果被中断，不添加转移动作（等待恢复）
        if (interrupted) {
            generator->Close();
            return;
        }
        
        // 为每个目标Agent生成转移事件
        for (const auto& to_agent_name : to_agent_names) {
            // 生成转移消息
            auto [assistant_msg, tool_msg] = GenTransferMessages(ctx, to_agent_name);
            
            // 生成Assistant消息事件
            auto assistant_event = EventFromMessage(
                assistant_msg, 
                nullptr,  // no error
                schema::RoleType::kAssistant,
                "");
            generator->Send(assistant_event);
            
            // 生成Tool消息事件（带转移动作）
            auto tool_event = EventFromMessage(
                tool_msg,
                nullptr,
                schema::RoleType::kTool,
                tool_msg.tool_name);
            
            // 添加转移动作
            tool_event->action = std::make_shared<AgentAction>();
            tool_event->action->transfer_to_agent = std::make_shared<TransferToAgentAction>();
            tool_event->action->transfer_to_agent->dest_agent_name = to_agent_name;
            
            generator->Send(tool_event);
        }
        
    } catch (const std::exception& e) {
        // 发送错误事件
        auto error_event = std::make_shared<AgentEvent>();
        error_event->error_msg = std::string("AppendTransferAction error: ") + e.what();
        generator->Send(error_event);
    }
    
    generator->Close();
}

}  // namespace adk
}  // namespace eino
