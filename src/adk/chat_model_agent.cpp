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

#include "eino/adk/chat_model_agent.h"
#include "eino/adk/context.h"
#include "eino/adk/react.h"
#include "eino/adk/instruction.h"
#include "eino/adk/async_iterator.h"
#include "eino/adk/utils.h"
#include "eino/adk/tools.h"
#include "eino/adk/callbacks.h"
#include "eino/adk/session.h"
#include "eino/schema/types.h"
#include "eino/compose/chain.h"
#include "eino/compose/graph.h"
#include "eino/compose/graph_compile_options.h"
#include "eino/compose/checkpoint.h"
#include "eino/components/model.h"
#include <memory>
#include <thread>
#include <stdexcept>
#include <limits>

namespace eino {
namespace adk {

ChatModelAgent::ChatModelAgent(void* ctx, const std::shared_ptr<ChatModelAgentConfig>& config)
    : name_(config->name),
      description_(config->description),
      instruction_(config->instruction),
      model_(config->model),
      tools_config_(config->tools_config),
      gen_model_input_(config->gen_model_input),
      exit_tool_(config->exit_tool),
      output_key_(config->output_key),
      max_iterations_(config->max_iterations > 0 ? config->max_iterations : 20) {

    // Process middlewares
    for (const auto& middleware : config->middlewares) {
        if (middleware.before_chat_model) {
            before_chat_models_.push_back(middleware.before_chat_model);
        }
        if (middleware.after_chat_model) {
            after_chat_models_.push_back(middleware.after_chat_model);
        }
        // Merge additional instruction
        if (!middleware.additional_instruction.empty()) {
            instruction_ += "\n\n" + middleware.additional_instruction;
        }
        // Merge additional tools
        tools_config_.tools.insert(
            tools_config_.tools.end(),
            middleware.additional_tools.begin(),
            middleware.additional_tools.end());
    }
}

ChatModelAgent::~ChatModelAgent() = default;

std::string ChatModelAgent::Name(void* ctx) {
    return name_;
}

std::string ChatModelAgent::Description(void* ctx) {
    return description_;
}

// BuildRunFunc implementation - aligns with eino adk ChatModelAgent.buildRunFunc
// Go reference: eino/adk/chatmodel.go lines 563-700
void ChatModelAgent::BuildRunFunc(void* ctx) {
    std::lock_guard<std::mutex> lock(build_mutex_);
    
    // Already built, skip
    if (run_func_built_.load()) {
        return;
    }
    
    // Copy configuration for lambda capture (avoid capturing 'this')
    std::string instruction = instruction_;
    std::string agent_name = name_;
    std::string output_key = output_key_;
    auto model = static_cast<components::ToolCallingChatModel*>(model_);
    auto gen_model_input = gen_model_input_;
    
    // Prepare tools configuration
    ToolsConfig tools_config = tools_config_;
    std::map<std::string, bool> return_directly = tools_config_.return_directly;
    
    // 1. Add transfer tool if has sub-agents
    std::vector<std::shared_ptr<Agent>> transfer_to_agents = sub_agents_;
    if (!disallow_transfer_to_parent_ && parent_agent_) {
        transfer_to_agents.push_back(parent_agent_);
    }
    
    if (!transfer_to_agents.empty()) {
        // Generate transfer instruction (aligns with go line 575-576)
        auto transfer_inst = GenTransferToAgentInstruction(ctx, transfer_to_agents);
        instruction = instruction + "\n\n" + transfer_inst;
        
        // Add transferToAgent tool (aligns with go line 578-579)
        auto transfer_tool = std::make_shared<TransferToAgentTool>();
        tools_config.tools.push_back(transfer_tool);
        return_directly[TransferToAgentTool::kToolName] = true;
    }
    
    // 2. Add exit tool if configured (aligns with go line 582-590)
    if (exit_tool_) {
        tools_config.tools.push_back(exit_tool_);
        
        // Get exit tool name (aligns with go line 584-589)
        auto exit_info = exit_tool_->Info(ctx);
        return_directly[exit_info.name] = true;
    }
    
    // 3. Build execution function based on whether we have tools
    if (tools_config.tools.empty()) {
        // ===================================================================
        // SIMPLE CHAIN: No tools, direct model call
        // Aligns with go line 592-643
        // ===================================================================
        run_func_ = [agent_name, instruction, model, gen_model_input, output_key](
            void* ctx,
            const std::shared_ptr<AgentInput>& input,
            const std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>>& generator,
            const std::vector<std::shared_ptr<AgentRunOption>>& options) {
            
            try {
                // Build simple chain: genModelInput -> ChatModel
                // Aligns with go line 594-600
                auto chain_builder = compose::NewChain<AgentInput, schema::Message>();
                
                // Add lambda to transform input
                chain_builder->AppendLambda(
                    compose::InvokableLambda([ctx, instruction, gen_model_input](
                        void* ctx2, const std::shared_ptr<AgentInput>& input2) 
                        -> std::vector<schema::Message> {
                        
                        if (gen_model_input) {
                            return gen_model_input(ctx2, instruction, input2);
                        }
                        
                        // Default genModelInput
                        std::vector<schema::Message> messages;
                        if (!instruction.empty()) {
                            schema::Message sys_msg;
                            sys_msg.role = schema::RoleType::System;
                            sys_msg.content = instruction;
                            messages.push_back(sys_msg);
                        }
                        messages.insert(messages.end(), 
                                      input2->messages.begin(), 
                                      input2->messages.end());
                        return messages;
                    })
                );
                
                // Add chat model
                chain_builder->AppendChatModel(model);
                
                // Compile chain
                auto runnable = chain_builder->Compile(ctx);
                if (!runnable) {
                    auto err_event = std::make_shared<AgentEvent>();
                    err_event->agent_name = agent_name;
                    err_event->error_msg = "Failed to compile chain";
                    generator->Send(err_event);
                    return;
                }
                
                // Execute: Stream or Invoke (aligns with go line 605-610)
                schema::Message msg;
                std::shared_ptr<schema::StreamReader<schema::Message>> msg_stream;
                std::string error;
                
                if (input->enable_streaming) {
                    msg_stream = runnable->Stream(ctx, input);
                } else {
                    try {
                        msg = runnable->Invoke(ctx, input);
                    } catch (const std::exception& e) {
                        error = e.what();
                    }
                }
                
                // Send event (aligns with go line 612-634)
                if (error.empty()) {
                    std::shared_ptr<AgentEvent> event;
                    
                    if (!output_key.empty()) {
                        // Need to save output to session
                        if (msg_stream) {
                            // Copy stream first (aligns with go line 617-620)
                            auto copies = msg_stream->Copy(2);
                            event = EventFromMessage(msg, copies[1], schema::RoleType::Assistant, "");
                            msg_stream = copies[0];
                        } else {
                            event = EventFromMessage(msg, nullptr, schema::RoleType::Assistant, "");
                        }
                        
                        // Send event ASAP before blocking on session write
                        generator->Send(event);
                        
                        // Save to session (may block if streaming) - aligns with go line 625-630
                        auto err = SetOutputToSession(ctx, &msg, msg_stream, output_key);
                        if (!err.empty()) {
                            auto err_event = std::make_shared<AgentEvent>();
                            err_event->agent_name = agent_name;
                            err_event->error_msg = err;
                            generator->Send(err_event);
                        }
                    } else {
                        event = EventFromMessage(msg, msg_stream, schema::RoleType::Assistant, "");
                        generator->Send(event);
                    }
                } else {
                    auto err_event = std::make_shared<AgentEvent>();
                    err_event->agent_name = agent_name;
                    err_event->error_msg = error;
                    generator->Send(err_event);
                }
                
                generator->Close();
                
            } catch (const std::exception& e) {
                auto err_event = std::make_shared<AgentEvent>();
                err_event->agent_name = agent_name;
                err_event->error_msg = std::string("Simple chain execution error: ") + e.what();
                generator->Send(err_event);
                generator->Close();
            }
        };
        
    } else {
        // ===================================================================
        // REACT GRAPH: Model + Tools loop
        // Aligns with go line 645-700
        // ===================================================================
        
        // Build ReAct configuration (aligns with go line 646-653)
        ReactConfig react_config;
        react_config.model = model_;
        react_config.tools_config = &tools_config;
        react_config.tools_return_directly = return_directly;
        react_config.agent_name = agent_name;
        react_config.max_iterations = max_iterations_;
        react_config.before_chat_model = before_chat_models_;
        react_config.after_chat_model = after_chat_models_;
        
        // Build ReAct graph (aligns with go line 655-659)
        auto react_graph = NewReact(ctx, react_config);
        if (!react_graph) {
            // Error building graph
            run_func_ = [agent_name](
                void* ctx,
                const std::shared_ptr<AgentInput>& input,
                const std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>>& generator,
                const std::vector<std::shared_ptr<AgentRunOption>>& options) {
                
                auto err_event = std::make_shared<AgentEvent>();
                err_event->agent_name = agent_name;
                err_event->error_msg = "Failed to build ReAct graph";
                generator->Send(err_event);
                generator->Close();
            };
            frozen_.store(true);
            run_func_built_.store(true);
            return;
        }
        
        // Create ReAct runFunc (aligns with go line 661-700)
        run_func_ = [agent_name, instruction, gen_model_input, output_key, react_graph](
            void* ctx,
            const std::shared_ptr<AgentInput>& input,
            const std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>>& generator,
            const std::vector<std::shared_ptr<AgentRunOption>>& options) {
            
            try {
                // Build chain: genModelInput -> ReAct Graph
                // Aligns with go line 672-679
                auto chain_builder = compose::NewChain<AgentInput, schema::Message>();
                
                // Add genModelInput lambda
                chain_builder->AppendLambda(
                    compose::InvokableLambda([ctx, instruction, gen_model_input](
                        void* ctx2, const std::shared_ptr<AgentInput>& input2) 
                        -> std::vector<schema::Message> {
                        
                        if (gen_model_input) {
                            return gen_model_input(ctx2, instruction, input2);
                        }
                        
                        // Default genModelInput
                        std::vector<schema::Message> messages;
                        if (!instruction.empty()) {
                            schema::Message sys_msg;
                            sys_msg.role = schema::RoleType::System;
                            sys_msg.content = instruction;
                            messages.push_back(sys_msg);
                        }
                        messages.insert(messages.end(), 
                                      input2->messages.begin(), 
                                      input2->messages.end());
                        return messages;
                    })
                );
                
                // Add ReAct graph
                chain_builder->AppendGraph(react_graph);
                
                // Compile with options (aligns with go line 662-671)
                std::vector<compose::GraphCompileOption> compile_opts;
                
                // Add GraphName option (aligns with go line 665)
                compile_opts.push_back(compose::WithGraphName(agent_name));
                
                // Add CheckPointStore if available (aligns with go line 666)
                // TODO: Get checkpoint store from options or context
                // if (store) {
                //     compile_opts.push_back(compose::WithCheckPointStore(store));
                // }
                
                // Add Serializer (aligns with go line 667)
                // Note: In Go, they use &gobSerializer{}, we can use default JSON serializer
                // TODO: Create and add serializer if needed
                // auto serializer = std::make_shared<compose::JSONSerializer>();
                // compile_opts.push_back(compose::WithSerializer(serializer));
                
                // Add MaxRunSteps (aligns with go line 669)
                compile_opts.push_back(compose::WithMaxRunSteps(std::numeric_limits<int>::max()));
                
                auto runnable = chain_builder->Compile(ctx, compile_opts);
                if (!runnable) {
                    auto err_event = std::make_shared<AgentEvent>();
                    err_event->agent_name = agent_name;
                    err_event->error_msg = "Failed to compile ReAct chain";
                    generator->Send(err_event);
                    return;
                }
                
                // Generate callbacks (aligns with go line 684-685)
                auto callback_opt = GenReactCallbacks(agent_name, generator, 
                                                     input->enable_streaming, nullptr);
                
                // Execute: Stream or Invoke (aligns with go line 687-692)
                schema::Message msg;
                std::shared_ptr<schema::StreamReader<schema::Message>> msg_stream;
                std::string error;
                
                try {
                    if (input->enable_streaming) {
                        msg_stream = runnable->Stream(ctx, input, callback_opt);
                    } else {
                        msg = runnable->Invoke(ctx, input, callback_opt);
                    }
                } catch (const std::exception& e) {
                    error = e.what();
                }
                
                // Send output as AgentEvent (aligns with go line 694-703)
                if (error.empty()) {
                    std::shared_ptr<AgentEvent> event;
                    
                    if (!output_key.empty()) {
                        // Save output to session
                        if (msg_stream) {
                            // For streaming: copy stream before consuming
                            auto copies = msg_stream->Copy(2);
                            event = EventFromMessage(nullptr, copies[1], 
                                schema::RoleType::Assistant, "");
                            msg_stream = copies[0];
                        } else {
                            event = EventFromMessage(&msg, nullptr,
                                schema::RoleType::Assistant, "");
                        }
                        
                        // Send event first, then save to session
                        generator->Send(event);
                        
                        // Save to session (may block if streaming) - aligns with go line 696-699
                        auto err = SetOutputToSession(ctx, 
                            msg_stream ? nullptr : &msg, 
                            msg_stream ? msg_stream : nullptr, 
                            output_key);
                        if (!err.empty()) {
                            auto err_event = std::make_shared<AgentEvent>();
                            err_event->agent_name = agent_name;
                            err_event->error_msg = err;
                            generator->Send(err_event);
                        }
                    } else {
                        // Just send the event
                        event = EventFromMessage(msg_stream ? nullptr : &msg, 
                            msg_stream, schema::RoleType::Assistant, "");
                        generator->Send(event);
                    }
                } else {
                    // Send error event
                    auto err_event = std::make_shared<AgentEvent>();
                    err_event->agent_name = agent_name;
                    err_event->error_msg = error;
                    generator->Send(err_event);
                }
                
                generator->Close();
                
            } catch (const std::exception& e) {
                auto err_event = std::make_shared<AgentEvent>();
                err_event->agent_name = agent_name;
                err_event->error_msg = std::string("ReAct execution error: ") + e.what();
                generator->Send(err_event);
                generator->Close();
            }
        };
    }
    
    // 4. Freeze agent configuration (aligns with go line 706)
    frozen_.store(true);
    run_func_built_.store(true);
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> ChatModelAgent::Run(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {

    // LazyBuild: Build run function on first call (aligns with go line 710)
    if (!run_func_built_.load()) {
        BuildRunFunc(ctx);
    }

    auto iterator_pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iterator = iterator_pair.first;
    auto generator = iterator_pair.second;

    // Execute run_func_ in a separate thread (aligns with go line 712-728)
    // The goroutine pattern with panic recovery
    std::thread([this, ctx, input, options, generator]() {
        try {
            // Call the built run function
            if (run_func_) {
                run_func_(ctx, input, generator, options);
            } else {
                auto err_event = std::make_shared<AgentEvent>();
                err_event->agent_name = name_;
                err_event->error_msg = "Run function not built";
                generator->Send(err_event);
                generator->Close();
            }
        } catch (const std::exception& e) {
            // Panic recovery (aligns with go line 714-718)
            auto error_event = std::make_shared<AgentEvent>();
            error_event->agent_name = name_;
            error_event->error_msg = std::string("ChatModelAgent::Run panic: ") + e.what();
            generator->Send(error_event);
            generator->Close();
        } catch (...) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->agent_name = name_;
            error_event->error_msg = "ChatModelAgent::Run unknown panic";
            generator->Send(error_event);
            generator->Close();
        }
    }).detach();
    
    return iterator;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> ChatModelAgent::Resume(
    void* ctx,
    const std::shared_ptr<ResumeInfo>& info,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {

    // Align with eino adk ChatModelAgent.Resume implementation
    // Go reference: eino/adk/chatmodel.go lines 730-755
    
    // LazyBuild: Build run function if not built yet
    if (!run_func_built_.load()) {
        BuildRunFunc(ctx);
    }
    
    auto iterator_pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto iterator = iterator_pair.first;
    auto generator = iterator_pair.second;
    
    // Execute run_func_ with resumed state (aligns with go line 738-753)
    std::thread([this, ctx, info, options, generator]() {
        try {
            if (!info || !info->data) {
                auto error_event = std::make_shared<AgentEvent>();
                error_event->agent_name = name_;
                error_event->error_msg = "Invalid resume info";
                generator->Send(error_event);
                generator->Close();
                return;
            }
            
            // Create AgentInput with streaming flag from ResumeInfo (aligns with go line 749)
            auto input = std::make_shared<AgentInput>();
            input->enable_streaming = info->enable_streaming;
            
            // TODO: Create resume store from info->data (aligns with go line 749)
            // auto store = NewResumeStore(info->data);
            
            // Call run_func_ with resume context
            if (run_func_) {
                // Pass resume store in options
                // For now, call without store (will need implementation)
                run_func_(ctx, input, generator, options);
            } else {
                auto err_event = std::make_shared<AgentEvent>();
                err_event->agent_name = name_;
                err_event->error_msg = "Run function not built";
                generator->Send(err_event);
                generator->Close();
            }
            
        } catch (const std::exception& e) {
            // Panic recovery (aligns with go line 741-745)
            auto error_event = std::make_shared<AgentEvent>();
            error_event->agent_name = name_;
            error_event->error_msg = std::string("ChatModelAgent::Resume panic: ") + e.what();
            generator->Send(error_event);
            generator->Close();
        } catch (...) {
            auto error_event = std::make_shared<AgentEvent>();
            error_event->agent_name = name_;
            error_event->error_msg = "ChatModelAgent::Resume unknown panic";
            generator->Send(error_event);
            generator->Close();
        }
    }).detach();

    return iterator;
}

void ChatModelAgent::OnSetSubAgents(void* ctx, const std::vector<std::shared_ptr<Agent>>& agents) {
    if (frozen_.load()) {
        throw std::runtime_error("Agent has been frozen after run");
    }
    if (!sub_agents_.empty()) {
        throw std::runtime_error("Agent's sub-agents has already been set");
    }
    sub_agents_ = agents;
}

void ChatModelAgent::OnSetAsSubAgent(void* ctx, const std::shared_ptr<Agent>& parent) {
    if (frozen_.load()) {
        throw std::runtime_error("Agent has been frozen after run");
    }
    if (parent_agent_) {
        throw std::runtime_error("Agent has already been set as a sub-agent");
    }
    parent_agent_ = parent;
}

void ChatModelAgent::OnDisallowTransferToParent(void* ctx) {
    if (frozen_.load()) {
        throw std::runtime_error("Agent has been frozen after run");
    }
    disallow_transfer_to_parent_ = true;
}

std::shared_ptr<ChatModelAgent> NewChatModelAgent(
    void* ctx,
    const std::shared_ptr<ChatModelAgentConfig>& config) {

    if (config->name.empty()) {
        throw std::runtime_error("Agent 'Name' is required");
    }
    if (config->description.empty()) {
        throw std::runtime_error("Agent 'Description' is required");
    }
    if (!config->model) {
        throw std::runtime_error("Agent 'Model' is required");
    }

    return std::make_shared<ChatModelAgent>(ctx, config);
}

}  // namespace adk
}  // namespace eino
