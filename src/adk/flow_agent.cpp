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

#include "../../include/eino/adk/flow_agent.h"
#include "../../include/eino/adk/async_iterator.h"
#include "../../include/eino/adk/context.h"
#include <thread>
#include <mutex>

namespace eino {
namespace adk {

FlowAgent::FlowAgent()
    : name_("FlowAgent"),
      description_("Default flow agent with sequential execution") {}

std::string FlowAgent::Name(void* ctx) {
    return name_;
}

std::string FlowAgent::Description(void* ctx) {
    return description_;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> FlowAgent::Run(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    // Align with eino adk flowAgent.Run()
    // Go reference: eino/adk/flow.go lines 298-311
    
    std::string agentName = Name(ctx);
    
    // Initialize run context if needed
    auto runCtx = context::GetExecutionContext(ctx);
    if (!runCtx) {
        runCtx = context::InitializeContext(ctx, agentName, input);
        ctx = context::SetExecutionContext(ctx, runCtx);
    }
    
    // Generate agent input from run context (history + current input)
    // CRITICAL: This ensures sub-agents get full conversation history
    std::shared_ptr<AgentInput> processedInput;
    try {
        bool skipTransferMessages = false;  // Get from options if needed
        processedInput = genAgentInput(ctx, runCtx, skipTransferMessages);
    } catch (const std::exception& e) {
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto error_event = std::make_shared<AgentEvent>();
        error_event->error_msg = std::string("genAgentInput failed: ") + e.what();
        pair.second->Send(error_event);
        pair.second->Close();
        return pair.first;
    }
    
    if (sub_agents_.empty()) {
        // No sub-agents, return error
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto iter = pair.first;
        auto gen = pair.second;
        auto error_event = std::make_shared<AgentEvent>();
        error_event->error_msg = "No sub-agents configured in FlowAgent";
        gen->Send(error_event);
        gen->Close();
        return iter;
    }

    // Route to appropriate execution mode
    switch (execution_mode_) {
        case MODE_SEQUENTIAL:
            return ExecuteSequential(ctx, processedInput, options);
        case MODE_PARALLEL:
            return ExecuteParallel(ctx, processedInput, options);
        case MODE_LOOP:
            return ExecuteLoop(ctx, processedInput, options);
        default:
            // Default to sequential
            return ExecuteSequential(ctx, processedInput, options);
    }
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> FlowAgent::Resume(
    void* ctx,
    const std::shared_ptr<ResumeInfo>& info,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    // Align with eino adk flowAgent.Resume()
    // Go reference: eino/adk/flow.go lines 272-309
    
    // Get run context from ctx
    auto runCtx = context::GetExecutionContext(ctx);
    if (!runCtx) {
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto error_event = std::make_shared<AgentEvent>();
        error_event->error_msg = "failed to resume agent: run context is empty";
        pair.second->Send(error_event);
        pair.second->Close();
        return pair.first;
    }
    
    // Determine target agent name from run path
    std::string agentName = Name(ctx);
    std::string targetName = agentName;
    
    auto run_path = runCtx->GetRunPath();
    if (!run_path.empty()) {
        // Get the last agent name in run path
        targetName = run_path.back().agent_name;
    }
    
    // If target is not this agent, find and delegate to target
    if (agentName != targetName) {
        auto targetAgent = GetAgent(ctx, targetName);
        if (!targetAgent) {
            auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = "failed to resume agent: cannot find agent: " + targetName;
            pair.second->Send(error_event);
            pair.second->Close();
            return pair.first;
        }
        
        // Delegate to target agent
        if (auto flowTarget = std::dynamic_pointer_cast<FlowAgent>(targetAgent)) {
            return flowTarget->Resume(ctx, info, options);
        } else if (auto resumableTarget = std::dynamic_pointer_cast<ResumableAgent>(targetAgent)) {
            return resumableTarget->Resume(ctx, info, options);
        } else {
            auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = "failed to resume agent: target agent is not resumable: " + targetName;
            pair.second->Send(error_event);
            pair.second->Close();
            return pair.first;
        }
    }
    
    // Resume current agent
    // For FlowAgent, we need to resume the appropriate sub-agent based on state
    // For now, return error as full resume logic needs more context
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto error_event = std::make_shared<AgentEvent>();
    error_event->error_msg = "FlowAgent Resume: full implementation requires checkpoint state";
    pair.second->Send(error_event);
    pair.second->Close();
    return pair.first;
}

void FlowAgent::OnSetSubAgents(void* ctx, const std::vector<std::shared_ptr<Agent>>& agents) {
    sub_agents_ = agents;
}

void FlowAgent::OnSetAsSubAgent(void* ctx, const std::shared_ptr<Agent>& parent) {
    parent_agent_ = parent;
}

void FlowAgent::OnDisallowTransferToParent(void* ctx) {
    disallow_transfer_to_parent_ = true;
}

void FlowAgent::SetName(const std::string& name) {
    name_ = name;
}

void FlowAgent::SetDescription(const std::string& desc) {
    description_ = desc;
}

void FlowAgent::SetHistoryRewriter(HistoryRewriter rewriter) {
    history_rewriter_ = rewriter;
}

void FlowAgent::SetDisallowTransferToParent(bool disallow) {
    disallow_transfer_to_parent_ = disallow;
}

void FlowAgent::SetCheckPointStore(void* store) {
    checkpoint_store_ = store;
}

std::vector<std::shared_ptr<Agent>> FlowAgent::GetSubAgents() const {
    return sub_agents_;
}

std::shared_ptr<Agent> FlowAgent::GetParentAgent() const {
    return parent_agent_;
}

bool FlowAgent::IsTransferToParentDisallowed() const {
    return disallow_transfer_to_parent_;
}

void* FlowAgent::GetCheckPointStore() const {
    return checkpoint_store_;
}

// DeepCopy implementation - aligns with eino adk flowAgent.deepCopy()
// Go reference: eino/adk/flow.go lines 32-46
std::shared_ptr<FlowAgent> FlowAgent::DeepCopy() const {
    auto copy = std::make_shared<FlowAgent>();
    
    // Copy basic properties
    copy->name_ = name_;
    copy->description_ = description_;
    copy->disallow_transfer_to_parent_ = disallow_transfer_to_parent_;
    copy->history_rewriter_ = history_rewriter_;
    copy->execution_mode_ = execution_mode_;
    
    // Share parent agent (not deep copied)
    copy->parent_agent_ = parent_agent_;
    
    // Share checkpoint store (not deep copied)
    copy->checkpoint_store_ = checkpoint_store_;
    
    // Deep copy sub-agents
    copy->sub_agents_.reserve(sub_agents_.size());
    for (const auto& sub : sub_agents_) {
        // Try to cast to FlowAgent for deep copy
        if (auto flow_sub = std::dynamic_pointer_cast<FlowAgent>(sub)) {
            copy->sub_agents_.push_back(flow_sub->DeepCopy());
        } else {
            // Non-flow agents are shared (not deep copied)
            copy->sub_agents_.push_back(sub);
        }
    }
    
    return copy;
}

// GetAgent implementation - aligns with eino adk flowAgent.getAgent()
// Go reference: eino/adk/flow.go lines 142-162
std::shared_ptr<Agent> FlowAgent::GetAgent(void* ctx, const std::string& name) {
    // 1. Check self
    if (Name(ctx) == name) {
        return shared_from_this();
    }
    
    // 2. Check sub-agents (DFS search)
    for (const auto& sub : sub_agents_) {
        if (sub->Name(ctx) == name) {
            return sub;
        }
        
        // Recursively check if sub is also a FlowAgent
        if (auto flow_sub = std::dynamic_pointer_cast<FlowAgent>(sub)) {
            auto found = flow_sub->GetAgent(ctx, name);
            if (found) {
                return found;
            }
        }
    }
    
    // 3. Check parent (if exists and transfer is allowed)
    if (parent_agent_ && !disallow_transfer_to_parent_) {
        if (auto flow_parent = std::dynamic_pointer_cast<FlowAgent>(parent_agent_)) {
            return flow_parent->GetAgent(ctx, name);
        } else if (parent_agent_->Name(ctx) == name) {
            return parent_agent_;
        }
    }
    
    // Not found
    return nullptr;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> FlowAgent::ExecuteSequential(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iter = pair.first;
    auto gen = pair.second;

    // Capture options by value
    auto opts_copy = options;
    std::thread worker([this, ctx, input, opts_copy, gen]() {
        try {
            // Execute sub-agents sequentially
            auto current_input = input;
            std::shared_ptr<AgentAction> lastAction;  // Track last action
            
            for (size_t i = 0; i < sub_agents_.size(); ++i) {
                auto agent = sub_agents_[i];
                
                // Execute current agent
                auto agent_iter = agent->Run(ctx, current_input, opts_copy);
                
                // Collect all events and forward them
                std::shared_ptr<AgentEvent> last_event;
                std::shared_ptr<AgentEvent> event;
                while (agent_iter->Next(event)) {
                    last_event = event;
                    gen->Send(event);
                    
                    // Check for agent transfer
                    if (event->action && event->action->transfer_to_agent) {
                        // Transfer will be handled after loop completes
                        // Store the action and break the sequential execution
                        lastAction = event->action;
                        break;
                    }
                }
                
                // If we have a transfer action, handle it outside the loop
                if (lastAction && lastAction->transfer_to_agent) {
                    break;
                }
                
                // Prepare input for next agent from last event output
                if (last_event && last_event->output && last_event->output->message_output) {
                    auto next_input = std::make_shared<AgentInput>();
                    next_input->messages = {last_event->output->message_output->message};
                    next_input->enable_streaming = input->enable_streaming;
                    current_input = next_input;
                }
            }
            
            // Handle transfer action if present
            HandleTransferAction(ctx, lastAction, gen, opts_copy);
            
        } catch (const std::exception& e) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = e.what();
            gen->Send(error_event);
        }
        gen->Close();
    });
    worker.detach();

    return iter;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> FlowAgent::ExecuteParallel(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    // Align with eino parallel execution pattern
    // Go doesn't have explicit ExecuteParallel, but uses Graph parallel edges
    
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iter = pair.first;
    auto gen = pair.second;

    // Capture options by value
    auto opts_copy = options;
    std::thread worker([this, ctx, input, opts_copy, gen]() {
        try {
            // Use threads for parallel execution
            std::vector<std::thread> threads;
            std::mutex output_mutex;
            std::vector<std::shared_ptr<AgentEvent>> all_events;
            bool has_error = false;
            std::string error_msg;
            
            // Launch all sub-agents in parallel
            for (const auto& agent : sub_agents_) {
                threads.emplace_back([&, agent, ctx, input, opts_copy, gen]() {
                    try {
                        auto agent_iter = agent->Run(ctx, input, opts_copy);
                        
                        std::shared_ptr<AgentEvent> event;
                        while (agent_iter->Next(event)) {
                            std::lock_guard<std::mutex> lock(output_mutex);
                            all_events.push_back(event);
                            gen->Send(event);
                        }
                    } catch (const std::exception& e) {
                        std::lock_guard<std::mutex> lock(output_mutex);
                        has_error = true;
                        error_msg = e.what();
                    }
                });
            }
            
            // Wait for all threads to complete
            for (auto& t : threads) {
                if (t.joinable()) {
                    t.join();
                }
            }
            
            // Send error if any sub-agent failed
            if (has_error) {
                auto error_event = std::make_shared<AgentEvent>();
                error_event->error_msg = "Parallel execution error: " + error_msg;
                gen->Send(error_event);
            }
            
        } catch (const std::exception& e) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = std::string("ExecuteParallel error: ") + e.what();
            gen->Send(error_event);
        }
        gen->Close();
    });
    worker.detach();

    return iter;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> FlowAgent::ExecuteLoop(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    // Align with eino loop execution pattern
    // Execute sub-agents repeatedly until break condition
    
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iter = pair.first;
    auto gen = pair.second;

    // Capture options by value
    auto opts_copy = options;
    std::thread worker([this, ctx, input, opts_copy, gen]() {
        try {
            auto current_input = input;
            int iteration = 0;
            const int max_iterations = 100;  // Safety limit
            bool should_break = false;
            
            while (iteration < max_iterations && !should_break) {
                // Execute all sub-agents in sequence within each iteration
                for (const auto& agent : sub_agents_) {
                    auto agent_iter = agent->Run(ctx, current_input, opts_copy);
                    
                    std::shared_ptr<AgentEvent> last_event;
                    std::shared_ptr<AgentEvent> event;
                    while (agent_iter->Next(event)) {
                        last_event = event;
                        gen->Send(event);
                        
                        // Check for break_loop action
                        if (event->action && event->action->break_loop) {
                            should_break = true;
                            break;
                        }
                        
                        // Check for exit action
                        if (event->action && event->action->exit) {
                            should_break = true;
                            break;
                        }
                    }
                    
                    if (should_break) {
                        break;
                    }
                    
                    // Prepare input for next agent/iteration from last event
                    if (last_event && last_event->output && 
                        last_event->output->message_output) {
                        auto next_input = std::make_shared<AgentInput>();
                        next_input->messages = {last_event->output->message_output->message};
                        next_input->enable_streaming = input->enable_streaming;
                        current_input = next_input;
                    }
                }
                
                iteration++;
            }
            
            // Warn if max iterations reached
            if (iteration >= max_iterations && !should_break) {
                auto warning_event = std::make_shared<AgentEvent>();
                warning_event->error_msg = "Loop reached maximum iterations limit";
                gen->Send(warning_event);
            }
            
        } catch (const std::exception& e) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = std::string("ExecuteLoop error: ") + e.what();
            gen->Send(error_event);
        }
        gen->Close();
    });
    worker.detach();

    return iter;
}

// BelongToRunPath checks if event's run path belongs to current run path
// Aligns with eino adk belongToRunPath()
// Go reference: eino/adk/flow.go lines 164-178
bool FlowAgent::BelongToRunPath(
    const std::vector<RunStep>& eventRunPath,
    const std::vector<RunStep>& runPath) {
    
    if (runPath.size() < eventRunPath.size()) {
        return false;
    }
    
    for (size_t i = 0; i < eventRunPath.size(); ++i) {
        if (!runPath[i].Equals(eventRunPath[i])) {
            return false;
        }
    }
    
    return true;
}

// RewriteMessage rewrites message from other agent for context
// Aligns with eino adk rewriteMessage()
// Go reference: eino/adk/flow.go lines 180-204
Message FlowAgent::RewriteMessage(const Message& msg, const std::string& agentName) {
    std::ostringstream sb;
    sb << "For context:";
    
    if (msg.role == schema::RoleType::kAssistant) {
        if (!msg.content.empty()) {
            sb << " [" << agentName << "] said: " << msg.content << ".";
        }
        if (!msg.tool_calls.empty()) {
            for (const auto& tc : msg.tool_calls) {
                const auto& f = tc.function;
                sb << " [" << agentName << "] called tool: `" << f.name 
                   << "` with arguments: " << f.arguments << ".";
            }
        }
    } else if (msg.role == schema::RoleType::kTool && !msg.content.empty()) {
        sb << " [" << agentName << "] `" << msg.tool_name 
           << "` tool returned result: " << msg.content << ".";
    }
    
    return schema::UserMessage(sb.str());
}

// DefaultHistoryRewriter rewrites history entries for current agent
// Aligns with eino adk buildDefaultHistoryRewriter()
// Go reference: eino/adk/flow.go lines 273-296
std::vector<Message> FlowAgent::DefaultHistoryRewriter(
    void* ctx,
    const std::vector<HistoryEntry>& entries,
    const std::string& agentName) {
    
    std::vector<Message> messages;
    messages.reserve(entries.size());
    
    for (const auto& entry : entries) {
        Message msg = entry.message;
        
        // If not user input, rewrite the message
        if (!entry.is_user_input) {
            if (entry.agent_name != agentName) {
                msg = RewriteMessage(entry.message, entry.agent_name);
            }
        }
        
        messages.push_back(msg);
    }
    
    return messages;
}

// genAgentInput generates agent input from run context
// Aligns with eino adk flowAgent.genAgentInput()
// Go reference: eino/adk/flow.go lines 220-270
std::shared_ptr<AgentInput> FlowAgent::genAgentInput(
    void* ctx,
    std::shared_ptr<ExecutionContext> runCtx,
    bool skipTransferMessages) {
    
    // Deep copy root input
    auto input = std::make_shared<AgentInput>();
    auto root_input = runCtx->GetRootInput();
    if (root_input) {
        input->messages = root_input->messages;
        input->enable_streaming = root_input->enable_streaming;
    }
    
    auto runPath = runCtx->GetRunPath();
    auto session = runCtx->GetSession();
    if (!session) {
        return input;
    }
    
    auto events = session->GetEvents();
    std::vector<HistoryEntry> historyEntries;
    
    // 1. Add user input messages
    for (const auto& m : input->messages) {
        HistoryEntry entry;
        entry.is_user_input = true;
        entry.message = m;
        historyEntries.push_back(entry);
    }
    
    // 2. Build history from session events
    for (const auto& event : events) {
        if (!event) {
            continue;
        }
        
        // Check if event belongs to current run path
        if (!BelongToRunPath(event->run_path, runPath)) {
            continue;
        }
        
        // Skip transfer messages if requested
        if (skipTransferMessages && event->action && event->action->transfer_to_agent) {
            // If skipped message is Tool role, remove previous entry (transfer message)
            if (event->output && 
                event->output->message_output && 
                event->output->message_output->message &&
                event->output->message_output->message->role == schema::RoleType::kTool &&
                !historyEntries.empty()) {
                historyEntries.pop_back();
            }
            continue;
        }
        
        // Extract message from event
        Message msg;
        bool has_message = false;
        
        if (event->output && event->output->message_output) {
            auto msg_variant = event->output->message_output;
            
            if (msg_variant->is_streaming) {
                // For streaming, need to consume and concat
                // For now, skip streaming messages in history
                // Full implementation requires ConcatMessageStream
                continue;
            } else if (msg_variant->message) {
                msg = *msg_variant->message;
                has_message = true;
            }
        }
        
        if (!has_message) {
            continue;
        }
        
        HistoryEntry entry;
        entry.is_user_input = false;
        entry.agent_name = event->agent_name;
        entry.message = msg;
        historyEntries.push_back(entry);
    }
    
    // 3. Use history rewriter to rewrite history
    std::vector<Message> messages;
    if (history_rewriter_) {
        messages = history_rewriter_(ctx, historyEntries);
    } else {
        messages = DefaultHistoryRewriter(ctx, historyEntries, name_);
    }
    
    input->messages = messages;
    
    return input;
}

// HandleTransferAction processes agent transfer action
// Align with eino adk flowAgent.run() transfer logic
// Go reference: eino/adk/flow.go lines 431-455
void FlowAgent::HandleTransferAction(
    void* ctx,
    std::shared_ptr<AgentAction> action,
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> gen,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    if (!action) {
        return;
    }
    
    // Check for interrupt - append to run context
    if (action->interrupted) {
        auto runCtx = context::GetExecutionContext(ctx);
        if (runCtx) {
            PushInterruptRunContext(ctx, runCtx);
        }
        return;
    }
    
    // Check for exit - stop processing
    if (action->exit) {
        return;
    }
    
    // Handle transfer to another agent
    if (action->transfer_to_agent) {
        std::string destName = action->transfer_to_agent->dest_agent_name;
        
        // Find target agent
        auto agentToRun = GetAgent(ctx, destName);
        if (!agentToRun) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->error_msg = "transfer failed: agent '" + destName + 
                                    "' not found when transferring from '" + Name(ctx) + "'";
            gen->Send(error_event);
            return;
        }
        
        // Run the target agent (with null input - gets from runCtx)
        auto subAIter = agentToRun->Run(ctx, nullptr, options);
        
        // Forward all events from transferred agent
        std::shared_ptr<AgentEvent> subEvent;
        while (subAIter->Next(subEvent)) {
            gen->Send(subEvent);
        }
    }
}

std::shared_ptr<FlowAgent> NewFlowAgent() {
    return std::make_shared<FlowAgent>();
}

std::shared_ptr<FlowAgent> NewFlowAgent(const std::string& name, const std::string& desc) {
    auto agent = std::make_shared<FlowAgent>();
    agent->SetName(name);
    agent->SetDescription(desc);
    return agent;
}

}  // namespace adk
}  // namespace eino
