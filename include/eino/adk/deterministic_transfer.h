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

#ifndef EINO_CPP_ADK_DETERMINISTIC_TRANSFER_H_
#define EINO_CPP_ADK_DETERMINISTIC_TRANSFER_H_

// Aligns with eino/adk/deterministic_transfer.go
//
// Deterministic Transfer 是一个Agent包装器，用于在Agent执行完成后
// 自动生成向指定Agent的转移动作（TransferToAgent）。
//
// 核心功能：
// 1. 包装任意Agent，在执行完成后自动添加转移动作
// 2. 支持多个目标Agent（按顺序依次转移）
// 3. 如果Agent被中断，不会添加转移动作（等待恢复后继续）
// 4. 支持ResumableAgent（恢复后也会自动添加转移）

#include "agent.h"
#include "types.h"
#include "async_iterator.h"
#include <memory>
#include <vector>
#include <string>

namespace eino {
namespace adk {

// DeterministicTransferConfig 配置确定性转移行为
struct DeterministicTransferConfig {
    // 要包装的Agent
    std::shared_ptr<Agent> agent;
    
    // 目标Agent名称列表（按顺序转移）
    std::vector<std::string> to_agent_names;
};

// AgentWithDeterministicTransferTo 创建带确定性转移的Agent包装器
//
// 用法示例：
//   auto config = std::make_shared<DeterministicTransferConfig>();
//   config->agent = my_agent;
//   config->to_agent_names = {"NextAgent1", "NextAgent2"};
//   auto wrapped_agent = AgentWithDeterministicTransferTo(ctx, config);
//
// 工作原理：
// 1. 执行原始Agent
// 2. 收集所有AgentEvent
// 3. 在最后一个事件后，检查是否被中断
// 4. 如果未中断，依次生成转移到目标Agent的事件
//
// 对齐: eino/adk/deterministic_transfer.go AgentWithDeterministicTransferTo
std::shared_ptr<Agent> AgentWithDeterministicTransferTo(
    void* ctx,
    const std::shared_ptr<DeterministicTransferConfig>& config);

// 内部实现类：普通Agent的确定性转移包装器
class AgentWithDeterministicTransferToImpl : public Agent {
public:
    AgentWithDeterministicTransferToImpl(
        std::shared_ptr<Agent> agent,
        const std::vector<std::string>& to_agent_names);
    
    virtual ~AgentWithDeterministicTransferToImpl() = default;
    
    std::string Name(void* ctx) override;
    std::string Description(void* ctx) override;
    
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

private:
    std::shared_ptr<Agent> agent_;
    std::vector<std::string> to_agent_names_;
};

// 内部实现类：ResumableAgent的确定性转移包装器
class ResumableAgentWithDeterministicTransferToImpl : public ResumableAgent {
public:
    ResumableAgentWithDeterministicTransferToImpl(
        std::shared_ptr<ResumableAgent> agent,
        const std::vector<std::string>& to_agent_names);
    
    virtual ~ResumableAgentWithDeterministicTransferToImpl() = default;
    
    std::string Name(void* ctx) override;
    std::string Description(void* ctx) override;
    
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;
    
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Resume(
        void* ctx,
        const std::shared_ptr<ResumeInfo>& info,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

private:
    std::shared_ptr<ResumableAgent> agent_;
    std::vector<std::string> to_agent_names_;
};

// 辅助函数：为事件流添加转移动作
// 对齐: eino/adk/deterministic_transfer.go appendTransferAction
void AppendTransferAction(
    void* ctx,
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> agent_iter,
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> generator,
    const std::vector<std::string>& to_agent_names);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_DETERMINISTIC_TRANSFER_H_
