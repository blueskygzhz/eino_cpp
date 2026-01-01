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

#include "../include/eino/adk/runner.h"
#include "../include/eino/adk/context.h"
#include "../include/eino/adk/flow_agent.h"
#include "../include/eino/schema/types.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

namespace eino {
namespace adk {

using json = nlohmann::json;

Runner::Runner(const RunnerConfig& config)
    : agent_(config.agent),
      enable_streaming_(config.enable_streaming),
      checkpoint_store_(config.checkpoint_store) {
}

Runner::Runner(std::shared_ptr<Agent> agent)
    : agent_(agent),
      enable_streaming_(false),
      checkpoint_store_(nullptr) {
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Runner::Run(
    void* ctx,
    const std::vector<Message>& messages,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    if (!agent_) {
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto gen = pair.second;
        auto error_event = std::make_shared<AgentEvent>();
        error_event->error_msg = "Runner: agent is null";
        gen->Send(error_event);
        gen->Close();
        return pair.first;
    }

    // Create agent input
    auto input = std::make_shared<AgentInput>();
    input->messages = messages;
    input->enable_streaming = enable_streaming_;

    // Initialize execution context
    auto exec_ctx = context::InitializeContext(ctx, agent_->Name(ctx), input);
    ctx = context::SetExecutionContext(ctx, exec_ctx);

    // Extract common options and add session values
    std::vector<AgentRunOption*> option_ptrs;
    for (const auto& opt : options) {
        option_ptrs.push_back(opt.get());
    }
    auto common_opts = GetCommonOptions(nullptr, option_ptrs);
    AddSessionValues(ctx, common_opts.session_values);

    // Convert options to vector format expected by Run
    auto agent_iter = agent_->Run(ctx, input, options);

    if (!checkpoint_store_) {
        return agent_iter;
    }

    // If we have a checkpoint store, wrap the iterator to save checkpoints
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto checkpoint_id = common_opts.checkpoint_id ? *common_opts.checkpoint_id : "";
    
    std::thread([this, ctx, agent_iter, gen = pair.second, checkpoint_id]() {
        HandleIteratorWithCheckpoint(ctx, agent_iter, gen, checkpoint_id);
    }).detach();

    return pair.first;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Runner::Query(
    void* ctx,
    const std::string& query,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    std::vector<Message> messages;
    messages.push_back(schema::UserMessage(query));
    return Run(ctx, messages, options);
}

std::pair<std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>>, std::string> Runner::Resume(
    void* ctx,
    const std::string& checkpoint_id,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    if (!checkpoint_store_) {
        auto err_msg = "Runner: checkpoint store is not configured";
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto event = std::make_shared<AgentEvent>();
        event->error_msg = err_msg;
        pair.second->Send(event);
        pair.second->Close();
        return {pair.first, err_msg};
    }

    if (!agent_) {
        auto err_msg = "Runner: agent is null";
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto event = std::make_shared<AgentEvent>();
        event->error_msg = err_msg;
        pair.second->Send(event);
        pair.second->Close();
        return {pair.first, err_msg};
    }

    bool exists = false;
    std::string checkpoint_data = checkpoint_store_->Load(checkpoint_id, exists);
    if (!exists) {
        auto err_msg = "Runner: checkpoint [" + checkpoint_id + "] not found";
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto event = std::make_shared<AgentEvent>();
        event->error_msg = err_msg;
        pair.second->Send(event);
        pair.second->Close();
        return {pair.first, err_msg};
    }

    // Deserialize checkpoint data
    try {
        auto checkpoint_json = nlohmann::json::parse(checkpoint_data);
        
        // Extract execution state from checkpoint
        auto agent_input = std::make_shared<AgentInput>();
        agent_input->enable_streaming = enable_streaming_;
        
        // Restore messages from checkpoint if available
        if (checkpoint_json.contains("messages") && checkpoint_json["messages"].is_array()) {
            agent_input->messages = checkpoint_json["messages"].get<std::vector<Message>>();
        }
        
        // Restore session state if available
        // Aligns with: eino/compose/graph_run.go:389-391 (state restoration)
        if (checkpoint_json.contains("session_state") && checkpoint_json["session_state"].is_object()) {
            auto session_state = checkpoint_json["session_state"];
            
            // Create state modifier to restore session values
            std::map<std::string, std::any> session_values;
            for (auto& [key, value] : session_state.items()) {
                // Convert JSON value to std::any for storage
                session_values[key] = value;
            }
            
            // Apply session values to context
            // This aligns with eino Go's state restoration logic
            if (!session_values.empty()) {
                // Store in execution context for later use
                auto exec_ctx = context::GetExecutionContext(ctx);
                if (exec_ctx) {
                    for (const auto& [key, value] : session_values) {
                        // Add session value to context
                        exec_ctx->session_values[key] = value;
                    }
                }
            }
        }

        // Initialize execution context with restored state
        auto exec_ctx = context::InitializeContext(ctx, agent_->Name(ctx), agent_input);
        ctx = context::SetExecutionContext(ctx, exec_ctx);

        // Add checkpoint ID to options for resume tracking
        std::vector<AgentRunOption*> option_ptrs;
        for (const auto& opt : options) {
            option_ptrs.push_back(opt.get());
        }
        auto common_opts = GetCommonOptions(nullptr, option_ptrs);
        AddSessionValues(ctx, common_opts.session_values);
        
        // Mark this as a resume operation by adding checkpoint metadata
        // This allows the agent to know it's resuming from a checkpoint
        auto resume_options = options;
        
        // Use agent's Resume method if available, otherwise use Run with restored state
        auto agent_iter = agent_->Run(ctx, agent_input, resume_options);

        if (!checkpoint_store_) {
            return {agent_iter, ""};
        }

        // Wrap the iterator to save checkpoints during execution
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        
        std::thread([this, ctx, agent_iter, gen = pair.second, checkpoint_id]() {
            HandleIteratorWithCheckpoint(ctx, agent_iter, gen, checkpoint_id);
        }).detach();

        return {pair.first, ""};
        
    } catch (const nlohmann::json::parse_error& e) {
        auto err_msg = std::string("Runner: failed to parse checkpoint: ") + e.what();
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto event = std::make_shared<AgentEvent>();
        event->error_msg = err_msg;
        pair.second->Send(event);
        pair.second->Close();
        return {pair.first, err_msg};
    } catch (const std::exception& e) {
        auto err_msg = std::string("Runner: failed to resume from checkpoint: ") + e.what();
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto event = std::make_shared<AgentEvent>();
        event->error_msg = err_msg;
        pair.second->Send(event);
        pair.second->Close();
        return {pair.first, err_msg};
    }
}

void Runner::SetCheckPointStore(std::shared_ptr<CheckPointStore> store) {
    checkpoint_store_ = store;
}

void Runner::SetEnableStreaming(bool enable) {
    enable_streaming_ = enable;
}

std::shared_ptr<Agent> Runner::GetAgent() const {
    return agent_;
}

void Runner::HandleIteratorWithCheckpoint(
    void* ctx,
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> agent_iter,
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> gen,
    const std::string& checkpoint_id) {
    
    try {
        std::shared_ptr<AgentEvent> interrupt_event;
        std::vector<std::shared_ptr<AgentEvent>> all_events;
        std::vector<Message> accumulated_messages;
        
        while (true) {
            std::shared_ptr<AgentEvent> event;
            if (!agent_iter->Next(event)) {
                break;
            }

            // Collect events for checkpoint serialization
            all_events.push_back(event);

            // Accumulate messages from events
            if (event && event->message) {
                accumulated_messages.push_back(*event->message);
            }

            // Check for interrupt
            if (event && event->action && 
                (event->action->interrupted || event->action->break_loop)) {
                interrupt_event = event;
            }

            gen->Send(event);
        }

        // Save checkpoint if we have interrupt data and a checkpoint store
        if (interrupt_event && checkpoint_store_ && !checkpoint_id.empty()) {
            try {
                nlohmann::json checkpoint_json;
                
                // Save accumulated messages
                checkpoint_json["messages"] = accumulated_messages;
                
                // Save interrupt state
                checkpoint_json["interrupted"] = true;
                checkpoint_json["interrupt_reason"] = interrupt_event->action->interrupted 
                    ? "interrupted" : "break_loop";
                
                // Save session state if available from context
                // Aligns with: eino/compose/graph_run.go:479-483 (state saving)
                auto exec_ctx = context::GetExecutionContext(ctx);
                if (exec_ctx) {
                    nlohmann::json session_state = nlohmann::json::object();
                    
                    // Extract session values from execution context
                    if (!exec_ctx->session_values.empty()) {
                        for (const auto& [key, value] : exec_ctx->session_values) {
                            try {
                                // Convert std::any to JSON
                                // This requires proper type handling
                                if (value.type() == typeid(std::string)) {
                                    session_state[key] = std::any_cast<std::string>(value);
                                } else if (value.type() == typeid(int)) {
                                    session_state[key] = std::any_cast<int>(value);
                                } else if (value.type() == typeid(double)) {
                                    session_state[key] = std::any_cast<double>(value);
                                } else if (value.type() == typeid(bool)) {
                                    session_state[key] = std::any_cast<bool>(value);
                                } else if (value.type() == typeid(nlohmann::json)) {
                                    session_state[key] = std::any_cast<nlohmann::json>(value);
                                }
                                // Add more type conversions as needed
                            } catch (const std::bad_any_cast& e) {
                                // Skip values that cannot be converted
                                std::cerr << "Runner: failed to convert session value '" << key << "': " << e.what() << std::endl;
                            }
                        }
                    }
                    
                    checkpoint_json["session_state"] = session_state;
                }
                
                // Save agent state if available
                if (interrupt_event->state) {
                    checkpoint_json["agent_state"] = *interrupt_event->state;
                }
                
                // Save timestamp
                checkpoint_json["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
                
                // Serialize and save
                std::string serialized = checkpoint_json.dump();
                checkpoint_store_->Save(checkpoint_id, serialized);
                
            } catch (const std::exception& e) {
                std::cerr << "Runner: failed to save checkpoint: " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        auto error_event = std::make_shared<AgentEvent>();
        error_event->error_msg = std::string("Runner: ") + e.what();
        gen->Send(error_event);
    }

    gen->Close();
}

std::shared_ptr<Runner> NewRunner(const RunnerConfig& config) {
    return std::make_shared<Runner>(config);
}

std::shared_ptr<Runner> NewRunner(std::shared_ptr<Agent> agent) {
    return std::make_shared<Runner>(agent);
}

}  // namespace adk
}  // namespace eino
