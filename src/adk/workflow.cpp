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

#include "../include/eino/adk/workflow.h"
#include "../include/eino/adk/context.h"
#include <thread>
#include <future>
#include <vector>

namespace eino {
namespace adk {

std::shared_ptr<AgentAction> NewBreakLoopAction(const std::string& agent_name) {
    auto action = std::make_shared<AgentAction>();
    auto break_loop = std::make_shared<BreakLoopAction>();
    break_loop->from = agent_name;
    break_loop->done = false;
    action->break_loop = break_loop.get();
    return action;
}

// WorkflowAgent helper methods
std::pair<bool, bool> WorkflowAgent::ExecuteSequentialInternal(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options,
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> gen,
    const std::shared_ptr<WorkflowInterruptInfo>& interrupt_info,
    int iterations) {

    auto sub_agents = GetSubAgents();
    size_t start_index = 0;

    if (interrupt_info && interrupt_info->sequential_interrupt_index >= 0) {
        start_index = interrupt_info->sequential_interrupt_index;
    }

    for (size_t i = start_index; i < sub_agents.size(); ++i) {
        auto agent = sub_agents[i];
        
        // Execute sub-agent
        std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> agent_iter;
        if (interrupt_info && i == start_index && interrupt_info->sequential_interrupt_index >= 0) {
            // Resume from interrupt
            if (auto resumable = std::dynamic_pointer_cast<ResumableAgent>(agent)) {
                auto resume_info = std::make_shared<ResumeInfo>();
                resume_info->enable_streaming = input ? input->enable_streaming : false;
                if (interrupt_info->sequential_interrupt_info) {
                    resume_info->interrupt_info = *interrupt_info->sequential_interrupt_info;
                }
                agent_iter = resumable->Resume(ctx, resume_info, options);
            } else {
                auto event = std::make_shared<AgentEvent>();
                event->error_msg = "Cannot resume: agent is not resumable";
                gen->Send(event);
                return {true, false};
            }
        } else {
            // Normal execution
            agent_iter = agent->Run(ctx, input, options);
        }

        // Process events
        std::shared_ptr<AgentEvent> last_event;
        while (agent_iter->Next(last_event)) {
            // Check for interrupt
            if (last_event && last_event->action) {
                if (last_event->action->interrupted) {
                    // Wrap interrupt in workflow context
                    auto workflow_interrupt = std::make_shared<WorkflowInterruptInfo>();
                    workflow_interrupt->orig_input = input;
                    workflow_interrupt->sequential_interrupt_index = i;
                    workflow_interrupt->sequential_interrupt_info = last_event->action->interrupted;
                    workflow_interrupt->loop_iterations = iterations;

                    auto wrapped_event = std::make_shared<AgentEvent>(*last_event);
                    wrapped_event->action->interrupted = std::make_shared<InterruptInfo>();
                    wrapped_event->action->interrupted->data = workflow_interrupt.get();
                    
                    gen->Send(wrapped_event);
                    return {true, true};  // exit=true, interrupted=true
                }

                if (last_event->action->exit) {
                    gen->Send(last_event);
                    return {true, false};  // exit=true, interrupted=false
                }

                if (CheckBreakLoop(last_event->action, iterations)) {
                    gen->Send(last_event);
                    return {true, false};
                }
            }

            gen->Send(last_event);
        }
    }

    return {false, false};  // exit=false, interrupted=false
}

void WorkflowAgent::ExecuteParallelInternal(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options,
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> gen,
    const std::shared_ptr<WorkflowInterruptInfo>& interrupt_info) {

    auto sub_agents = GetSubAgents();
    std::vector<std::future<void>> futures;
    std::vector<std::shared_ptr<AgentEvent>> interrupt_events;
    std::mutex interrupt_mutex;

    // Launch parallel execution
    for (size_t i = 0; i < sub_agents.size(); ++i) {
        auto agent = sub_agents[i];
        size_t agent_index = i;

        futures.push_back(std::async(std::launch::async, [this, ctx, input, options, agent, agent_index, 
                                                         interrupt_info, gen, &interrupt_events, 
                                                         &interrupt_mutex]() {
            auto agent_iter = agent->Run(ctx, input, options);
            std::shared_ptr<AgentEvent> event;
            
            while (agent_iter->Next(event)) {
                if (event && event->action && event->action->interrupted) {
                    std::lock_guard<std::mutex> lock(interrupt_mutex);
                    interrupt_events.push_back(event);
                    break;
                }
                gen->Send(event);
            }
        }));
    }

    // Wait for all tasks
    for (auto& fut : futures) {
        try {
            fut.get();
        } catch (const std::exception& e) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = std::string("Parallel execution error: ") + e.what();
            gen->Send(error_event);
        }
    }

    // Handle interrupts
    if (!interrupt_events.empty()) {
        auto workflow_interrupt = std::make_shared<WorkflowInterruptInfo>();
        workflow_interrupt->orig_input = input;
        
        for (size_t i = 0; i < interrupt_events.size(); ++i) {
            workflow_interrupt->parallel_interrupt_info[i] = 
                interrupt_events[i]->action->interrupted;
        }

        auto wrapped_event = std::make_shared<AgentEvent>(*interrupt_events[0]);
        wrapped_event->action->interrupted = std::make_shared<InterruptInfo>();
        wrapped_event->action->interrupted->data = workflow_interrupt.get();
        gen->Send(wrapped_event);
    }
}

void WorkflowAgent::ExecuteLoopInternal(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options,
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> gen,
    const std::shared_ptr<WorkflowInterruptInfo>& interrupt_info) {

    int max_iterations = GetMaxIterations();
    int start_iteration = 0;

    if (interrupt_info) {
        start_iteration = interrupt_info->loop_iterations;
    }

    for (int iter = start_iteration; max_iterations == 0 || iter < max_iterations; ++iter) {
        auto [exit, interrupted] = ExecuteSequentialInternal(
            ctx, input, options, gen, interrupt_info, iter);

        if (interrupted) {
            return;
        }
        if (exit) {
            return;
        }

        // Clear interrupt info for next iteration
        interrupt_info = nullptr;
    }
}

bool WorkflowAgent::CheckBreakLoop(std::shared_ptr<AgentAction> action, int iterations) {
    if (!action || !action->break_loop) {
        return false;
    }
    auto break_action = std::static_pointer_cast<BreakLoopAction>(
        std::shared_ptr<void>(action->break_loop));
    if (break_action && !break_action->done) {
        break_action->done = true;
        break_action->current_iterations = iterations;
        return true;
    }
    return false;
}

// SequentialAgent Implementation
SequentialAgent::SequentialAgent() 
    : WorkflowAgent() {
    SetName("SequentialAgent");
    SetDescription("Executes sub-agents sequentially");
}

SequentialAgent::SequentialAgent(const std::string& name, const std::string& description)
    : WorkflowAgent() {
    SetName(name);
    SetDescription(description.empty() ? "Sequential workflow agent" : description);
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> SequentialAgent::Run(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {

    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iter = pair.first;
    auto gen = pair.second;

    std::thread([this, ctx, input, options, gen]() {
        try {
            ExecuteSequentialInternal(ctx, input, options, gen);
        } catch (const std::exception& e) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = std::string("SequentialAgent error: ") + e.what();
            gen->Send(error_event);
        }
        gen->Close();
    }).detach();

    return iter;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> SequentialAgent::Resume(
    void* ctx,
    const std::shared_ptr<ResumeInfo>& info,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {

    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iter = pair.first;
    auto gen = pair.second;

    std::thread([this, ctx, info, options, gen]() {
        try {
            auto interrupt_info = std::static_pointer_cast<WorkflowInterruptInfo>(
                std::shared_ptr<void>(info->interrupt_info.data));
            ExecuteSequentialInternal(ctx, nullptr, options, gen, interrupt_info);
        } catch (const std::exception& e) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = std::string("SequentialAgent resume error: ") + e.what();
            gen->Send(error_event);
        }
        gen->Close();
    }).detach();

    return iter;
}

// ParallelAgent Implementation
ParallelAgent::ParallelAgent()
    : WorkflowAgent() {
    SetName("ParallelAgent");
    SetDescription("Executes sub-agents in parallel");
}

ParallelAgent::ParallelAgent(const std::string& name, const std::string& description)
    : WorkflowAgent() {
    SetName(name);
    SetDescription(description.empty() ? "Parallel workflow agent" : description);
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> ParallelAgent::Run(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {

    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iter = pair.first;
    auto gen = pair.second;

    std::thread([this, ctx, input, options, gen]() {
        try {
            ExecuteParallelInternal(ctx, input, options, gen);
        } catch (const std::exception& e) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = std::string("ParallelAgent error: ") + e.what();
            gen->Send(error_event);
        }
        gen->Close();
    }).detach();

    return iter;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> ParallelAgent::Resume(
    void* ctx,
    const std::shared_ptr<ResumeInfo>& info,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {

    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iter = pair.first;
    auto gen = pair.second;

    std::thread([this, ctx, info, options, gen]() {
        try {
            auto interrupt_info = std::static_pointer_cast<WorkflowInterruptInfo>(
                std::shared_ptr<void>(info->interrupt_info.data));
            ExecuteParallelInternal(ctx, nullptr, options, gen, interrupt_info);
        } catch (const std::exception& e) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = std::string("ParallelAgent resume error: ") + e.what();
            gen->Send(error_event);
        }
        gen->Close();
    }).detach();

    return iter;
}

// LoopAgent Implementation
LoopAgent::LoopAgent()
    : WorkflowAgent(), max_iterations_(0) {
    SetName("LoopAgent");
    SetDescription("Repeatedly executes sub-agents");
}

LoopAgent::LoopAgent(const std::string& name, const std::string& description, int max_iterations)
    : WorkflowAgent(), max_iterations_(max_iterations) {
    SetName(name);
    SetDescription(description.empty() ? "Loop workflow agent" : description);
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> LoopAgent::Run(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {

    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iter = pair.first;
    auto gen = pair.second;

    std::thread([this, ctx, input, options, gen]() {
        try {
            ExecuteLoopInternal(ctx, input, options, gen);
        } catch (const std::exception& e) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = std::string("LoopAgent error: ") + e.what();
            gen->Send(error_event);
        }
        gen->Close();
    }).detach();

    return iter;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> LoopAgent::Resume(
    void* ctx,
    const std::shared_ptr<ResumeInfo>& info,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {

    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iter = pair.first;
    auto gen = pair.second;

    std::thread([this, ctx, info, options, gen]() {
        try {
            auto interrupt_info = std::static_pointer_cast<WorkflowInterruptInfo>(
                std::shared_ptr<void>(info->interrupt_info.data));
            ExecuteLoopInternal(ctx, nullptr, options, gen, interrupt_info);
        } catch (const std::exception& e) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = std::string("LoopAgent resume error: ") + e.what();
            gen->Send(error_event);
        }
        gen->Close();
    }).detach();

    return iter;
}

// Factory functions
std::shared_ptr<SequentialAgent> NewSequentialAgent(const SequentialAgentConfig& config) {
    auto agent = std::make_shared<SequentialAgent>(config.name, config.description);
    agent->OnSetSubAgents(nullptr, config.sub_agents);
    return agent;
}

std::shared_ptr<ParallelAgent> NewParallelAgent(const ParallelAgentConfig& config) {
    auto agent = std::make_shared<ParallelAgent>(config.name, config.description);
    agent->OnSetSubAgents(nullptr, config.sub_agents);
    return agent;
}

std::shared_ptr<LoopAgent> NewLoopAgent(const LoopAgentConfig& config) {
    auto agent = std::make_shared<LoopAgent>(config.name, config.description, config.max_iterations);
    agent->OnSetSubAgents(nullptr, config.sub_agents);
    return agent;
}

}  // namespace adk
}  // namespace eino
