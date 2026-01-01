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

#include "eino/flow/agent/react.h"
#include "eino/compose/graph.h"
#include "eino/compose/tool_node.h"
#include <stdexcept>

namespace eino {
namespace flow {
namespace agent {

// State for ReAct agent
struct ReActState {
    std::vector<schema::Message> messages;
    std::string return_directly_tool_call_id;
};

const std::string kNodeKeyTools = "tools";
const std::string kNodeKeyModel = "chat";

// ReActAgent implementation
class ReActAgentImpl : public ReActAgent {
public:
    ReActAgentImpl(const ReActConfig& config)
        : config_(config), max_step_(config.max_step > 0 ? config.max_step : 12) {
        
        if (!config_.tool_calling_model && !config_.model) {
            throw std::runtime_error("Either tool_calling_model or model must be provided");
        }
        
        // Use tool_calling_model if provided, otherwise fall back to model
        if (config_.tool_calling_model) {
            model_ = config_.tool_calling_model;
        } else {
            model_ = config_.model;
        }
        
        InitializeGraph();
    }
    
    ~ReActAgentImpl() override = default;
    
    schema::Message Generate(
        void* ctx,
        const std::vector<schema::Message>& messages,
        const std::vector<compose::Option>& opts) override {
        
        if (!graph_) {
            throw std::runtime_error("Graph not initialized");
        }
        
        // Initialize state
        ReActState state;
        state.messages = messages;
        
        // Run the graph
        auto result = graph_->Invoke(ctx, state, opts);
        
        // Extract final message
        if (result.messages.empty()) {
            throw std::runtime_error("No messages in result");
        }
        
        return result.messages.back();
    }
    
    std::shared_ptr<schema::StreamReader<schema::Message>> Stream(
        void* ctx,
        const std::vector<schema::Message>& messages,
        const std::vector<compose::Option>& opts) override {
        
        // For streaming, we need to handle message chunks
        // This is a simplified implementation
        auto reader = std::make_shared<schema::StreamReader<schema::Message>>();
        
        // Run generate and stream the result
        auto result = Generate(ctx, messages, opts);
        reader->Send(result);
        reader->Close();
        
        return reader;
    }

private:
    void InitializeGraph() {
        // Create graph builder
        auto builder = compose::GraphBuilder<ReActState>::Create();
        
        // Add model node
        builder->AddNode(kNodeKeyModel, [this](void* ctx, const ReActState& state) -> ReActState {
            ReActState new_state = state;
            
            // Apply message modifier if provided
            std::vector<schema::Message> input_messages = state.messages;
            if (config_.message_modifier) {
                input_messages = config_.message_modifier(ctx, input_messages);
            }
            
            // Call the model
            auto response = model_->Generate(ctx, input_messages);
            new_state.messages.push_back(response);
            
            return new_state;
        });
        
        // Add tools node
        builder->AddNode(kNodeKeyTools, [this](void* ctx, const ReActState& state) -> ReActState {
            ReActState new_state = state;
            
            if (state.messages.empty()) {
                return new_state;
            }
            
            const auto& last_message = state.messages.back();
            
            // Execute tool calls
            for (const auto& tool_call : last_message.tool_calls) {
                // Find and execute the tool
                // This requires tool execution logic
                // For now, create a placeholder tool response
                schema::Message tool_response;
                tool_response.role = schema::RoleType::kTool;
                tool_response.tool_call_id = tool_call.id;
                tool_response.content = "Tool executed successfully";  // Placeholder
                
                new_state.messages.push_back(tool_response);
            }
            
            return new_state;
        });
        
        // Add edges
        builder->AddEdge(compose::START, kNodeKeyModel);
        
        // Add conditional edge from model
        builder->AddConditionalEdge(kNodeKeyModel, 
            [](void* ctx, const ReActState& state) -> std::string {
                if (state.messages.empty()) {
                    return compose::END;
                }
                
                const auto& last_message = state.messages.back();
                
                // If there are tool calls, go to tools node
                if (!last_message.tool_calls.empty()) {
                    return kNodeKeyTools;
                }
                
                // Otherwise, end
                return compose::END;
            });
        
        // Add edge from tools back to model
        builder->AddEdge(kNodeKeyTools, kNodeKeyModel);
        
        // Compile the graph
        graph_ = builder->Compile();
    }
    
    ReActConfig config_;
    std::shared_ptr<components::ChatModel> model_;
    std::shared_ptr<compose::Graph<ReActState>> graph_;
    int max_step_;
};

std::shared_ptr<ReActAgent> ReActAgent::Create(const ReActConfig& config) {
    return std::make_shared<ReActAgentImpl>(config);
}

} // namespace agent
} // namespace flow
} // namespace eino
