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

#include "eino/adk/react.h"
#include "eino/adk/context.h"
#include "eino/adk/tools.h"
#include "eino/compose/graph.h"
#include "eino/compose/tool_node.h"
#include "eino/compose/branch.h"
#include "eino/compose/state.h"
#include "eino/compose/lambda.h"
#include "eino/schema/message.h"
#include <memory>
#include <string>
#include <algorithm>

namespace eino {
namespace adk {

// Node keys - aligns with Go constants
constexpr const char* NODE_KEY_MODEL = "chat";
constexpr const char* NODE_KEY_TOOLS = "tools";
constexpr const char* NODE_KEY_DIRECT_RETURN = "direct_return";

// =============================================================================
// Helper Functions for return_directly tool detection
// Aligns with: eino/flow/agent/react/react.go:341-353
// =============================================================================

// Get return_directly tool call ID from message
// Aligns with: getReturnDirectlyToolCallID in react.go:341-353
std::string GetReturnDirectlyToolCallID(
    const schema::Message* input,
    const std::map<std::string, bool>& tool_return_directly) {
    
    if (tool_return_directly.empty() || !input) {
        return "";
    }
    
    for (const auto& tool_call : input->tool_calls) {
        auto it = tool_return_directly.find(tool_call.function.name);
        if (it != tool_return_directly.end() && it->second) {
            return tool_call.id;
        }
    }
    
    return "";
}

// =============================================================================
// State Management Functions
// =============================================================================

std::shared_ptr<ReactState> GetOrCreateReactState(void* ctx) {
    bool found = false;
    void* state_ptr = GetSessionValue(ctx, kReactStateKey, &found);
    if (found && state_ptr) {
        return *static_cast<std::shared_ptr<ReactState>*>(state_ptr);
    }
    
    // Create new state
    auto state = std::make_shared<ReactState>();
    auto* state_holder = new std::shared_ptr<ReactState>(state);
    AddSessionValue(ctx, kReactStateKey, state_holder);
    return state;
}

std::tuple<std::string, bool> GetReturnDirectlyToolCallID(void* ctx) {
    auto state = GetOrCreateReactState(ctx);
    if (!state) {
        return {"", false};
    }
    
    if (state->return_directly_tool_call_id.empty()) {
        return {"", false};
    }
    
    return {state->return_directly_tool_call_id, true};
}

// =============================================================================
// Build Return Directly Logic
// Aligns with: buildReturnDirectly in react.go:297-339
// =============================================================================

std::string BuildReturnDirectly(
    compose::Graph<std::vector<schema::Message>, schema::Message>* graph) {
    
    if (!graph) {
        return "graph is null";
    }
    
    // Create directReturn lambda
    // Aligns with: react.go:298-315
    auto direct_return = compose::TransformableLambda(
        [](void* ctx,
           std::shared_ptr<schema::StreamReader<std::vector<schema::Message>>> msgs_stream)
        -> std::shared_ptr<schema::StreamReader<schema::Message>> {
            
            return schema::StreamReaderWithConvert<std::vector<schema::Message>, schema::Message>(
                msgs_stream,
                [ctx](const std::vector<schema::Message>& msgs) -> std::tuple<schema::Message, std::string> {
                    schema::Message result_msg;
                    std::string error;
                    
                    // Process state to find the return_directly message
                    // Aligns with: react.go:300-312
                    compose::ProcessState<ReactState>(ctx,
                        [&msgs, &result_msg](void* ctx2, ReactState* state) -> std::string {
                            for (const auto& msg : msgs) {
                                if (msg.tool_call_id == state->return_directly_tool_call_id) {
                                    result_msg = msg;
                                    return "";
                                }
                            }
                            return "";
                        });
                    
                    if (result_msg.content.empty() && result_msg.tool_call_id.empty()) {
                        return {result_msg, "schema::ErrNoValue"};
                    }
                    
                    return {result_msg, ""};
                });
        });
    
    // Add direct_return node
    // Aligns with: react.go:318-320
    auto err = graph->AddLambdaNode(NODE_KEY_DIRECT_RETURN, direct_return);
    if (!err.empty()) {
        return err;
    }
    
    // Add branch from tools node
    // Aligns with: react.go:323-337
    auto tools_branch = compose::NewStreamGraphBranch<std::vector<schema::Message>>(
        [](void* ctx,
           std::shared_ptr<schema::StreamReader<std::vector<schema::Message>>> msgs_stream)
        -> std::tuple<std::string, std::string> {
            
            msgs_stream->Close();
            
            std::string end_node;
            std::string error;
            
            // Check state for return_directly flag
            compose::ProcessState<ReactState>(ctx,
                [&end_node](void* ctx2, ReactState* state) -> std::string {
                    if (!state->return_directly_tool_call_id.empty()) {
                        end_node = NODE_KEY_DIRECT_RETURN;
                    } else {
                        end_node = NODE_KEY_MODEL;
                    }
                    return "";
                });
            
            return {end_node, error};
        },
        std::map<std::string, bool>{{NODE_KEY_MODEL, true}, {NODE_KEY_DIRECT_RETURN, true}}
    );
    
    err = graph->AddBranch(NODE_KEY_TOOLS, tools_branch);
    if (!err.empty()) {
        return err;
    }
    
    // Add edge from direct_return to END
    // Aligns with: react.go:339
    return graph->AddEdge(NODE_KEY_DIRECT_RETURN, compose::END);
}

// =============================================================================
// NewReact - Create ReAct Graph
// Completely rewritten to align with: NewAgent in react.go:174-295
// =============================================================================

std::shared_ptr<compose::Graph<std::vector<schema::Message>, schema::Message>> NewReact(
    void* ctx,
    const ReactConfig& config) {
    
    // Validate config
    if (!config.model) {
        // Return nullptr for error
        return nullptr;
    }
    
    if (!config.tools_config || config.tools_config->tools.empty()) {
        return nullptr;
    }
    
    // Determine node names
    // Aligns with: react.go:182-198
    std::string graph_name = config.graph_name.empty() ? "ReActAgent" : config.graph_name;
    std::string model_node_name = config.model_node_name.empty() ? "ChatModel" : config.model_node_name;
    std::string tools_node_name = config.tools_node_name.empty() ? "Tools" : config.tools_node_name;
    
    // Get tool infos for ChatModel
    // Aligns with: genToolInfos in react.go:355-367
    std::vector<schema::ToolInfo> tool_infos;
    for (const auto& tool : config.tools_config->tools) {
        auto info = tool->Info(ctx);
        tool_infos.push_back(info);
    }
    
    // Bind tools to ChatModel
    // Aligns with: agent.ChatModelWithTools in react.go:202
    auto chat_model_with_tools = config.model->BindTools(tool_infos);
    
    // Create ToolsNode
    // Aligns with: compose.NewToolNode in react.go:204-206
    auto tools_node = compose::NewToolNode(ctx, config.tools_config);
    if (!tools_node) {
        return nullptr;
    }
    
    // Create Graph with state
    // Aligns with: compose.NewGraph with WithGenLocalState in react.go:208-210
    auto graph = compose::NewGraph<std::vector<schema::Message>, schema::Message>(
        compose::WithGenLocalState<ReactState>(
            [max_step = config.max_step](void* ctx) -> std::shared_ptr<ReactState> {
                auto state = std::make_shared<ReactState>();
                state->messages.reserve(max_step + 1);
                return state;
            }));
    
    // =========================================================================
    // Add ChatModel Node with Pre-Handler
    // Aligns with: react.go:212-226
    // =========================================================================
    
    auto model_pre_handle = compose::WithStatePreHandler<std::vector<schema::Message>, ReactState>(
        [message_rewriter = config.message_rewriter,
         message_modifier = config.message_modifier]
        (void* ctx,
         const std::vector<schema::Message>& input,
         ReactState* state) -> std::tuple<std::vector<schema::Message>, std::string> {
            
            // Append input messages to state
            // Aligns with: react.go:213
            state->messages.insert(state->messages.end(), input.begin(), input.end());
            
            // Apply message rewriter if configured
            // Aligns with: react.go:215-217
            if (message_rewriter) {
                state->messages = message_rewriter(ctx, state->messages);
            }
            
            // Apply message modifier if configured
            // Aligns with: react.go:219-221
            if (!message_modifier) {
                return {state->messages, ""};
            }
            
            std::vector<schema::Message> modified_input = state->messages;
            return {message_modifier(ctx, modified_input), ""};
        });
    
    // Prepare options for AddChatModelNode
    // Aligns with: react.go:254 (WithStatePreHandler, WithNodeName)
    std::vector<compose::GraphAddNodeOpt> model_opts = {
        model_pre_handle,
        compose::WithNodeName(model_node_name)
    };
    
    auto err = graph->AddChatModelNode(
        NODE_KEY_MODEL,
        chat_model_with_tools,
        model_opts);
    
    if (!err.empty()) {
        return nullptr;
    }
    
    // Add START -> model edge
    // Aligns with: react.go:231-233
    err = graph->AddEdge(compose::START, NODE_KEY_MODEL);
    if (!err.empty()) {
        return nullptr;
    }
    
    // =========================================================================
    // Add Tools Node with Pre-Handler
    // Aligns with: react.go:235-243
    // =========================================================================
    
    auto tools_node_pre_handle = compose::WithStatePreHandler<schema::Message, ReactState>(
        [tool_return_directly = config.tools_return_directly]
        (void* ctx,
         const schema::Message* input,
         ReactState* state) -> std::tuple<schema::Message, std::string> {
            
            // Handle rerun/resume case where input is null
            // Aligns with: react.go:236-238
            if (!input) {
                return {state->messages.back(), ""};
            }
            
            // Append message to state
            // Aligns with: react.go:239
            state->messages.push_back(*input);
            
            // Check for return_directly tool
            // Aligns with: react.go:240
            state->return_directly_tool_call_id =
                GetReturnDirectlyToolCallID(input, tool_return_directly);
            
            return {*input, ""};
        });
    
    // Prepare options for AddToolsNode
    // Aligns with: react.go:245 (WithStatePreHandler, WithNodeName)
    std::vector<compose::GraphAddNodeOpt> tools_opts = {
        tools_node_pre_handle,
        compose::WithNodeName(tools_node_name)
    };
    
    err = graph->AddToolsNode(
        NODE_KEY_TOOLS,
        tools_node,
        tools_opts);
    
    if (!err.empty()) {
        return nullptr;
    }
    
    // =========================================================================
    // Add Branch from ChatModel Node
    // Aligns with: react.go:245-253
    // =========================================================================
    
    auto model_post_branch = compose::NewStreamGraphBranch<schema::Message>(
        [tool_call_checker = config.stream_tool_call_checker]
        (void* ctx,
         std::shared_ptr<schema::StreamReader<schema::Message>> sr)
        -> std::tuple<std::string, std::string> {
            
            // Use tool call checker to determine if we should continue
            // Aligns with: react.go:246-249
            auto [is_tool_call, err] = tool_call_checker(ctx, sr);
            if (!err.empty()) {
                return {"", err};
            }
            
            if (is_tool_call) {
                return {NODE_KEY_TOOLS, ""};
            }
            
            return {compose::END, ""};
        },
        std::map<std::string, bool>{{NODE_KEY_TOOLS, true}, {compose::END, true}}
    );
    
    err = graph->AddBranch(NODE_KEY_MODEL, model_post_branch);
    if (!err.empty()) {
        return nullptr;
    }
    
    // =========================================================================
    // Build Return Directly Logic
    // Aligns with: buildReturnDirectly in react.go:255-257
    // =========================================================================
    
    err = BuildReturnDirectly(graph.get());
    if (!err.empty()) {
        return nullptr;
    }
    
    // =========================================================================
    // Compile Graph
    // Aligns with: react.go:259-265
    // =========================================================================
    
    std::vector<compose::GraphCompileOption> compile_opts = {
        compose::WithMaxRunSteps(config.max_step),
        compose::WithNodeTriggerMode(compose::AnyPredecessor),
        compose::WithGraphName(graph_name)
    };
    
    auto runnable = graph->Compile(ctx, compile_opts);
    if (!runnable) {
        return nullptr;
    }
    
    return runnable;
}

}  // namespace adk
}  // namespace eino
