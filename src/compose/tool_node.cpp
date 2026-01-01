/*
 * Copyright 2024 CloudWeGo Authors
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

#include "../../include/eino/compose/tool_node.h"
#include <thread>
#include <future>
#include <sstream>

namespace eino {
namespace compose {

// New creates a new ToolsNode with configuration
// Aligns with eino compose.NewToolNode
// Go reference: eino/compose/tool_node.go lines 172-262
std::shared_ptr<ToolsNode> ToolsNode::New(
    void* ctx, 
    const ToolsNodeConfig& config) {
    
    auto node = std::shared_ptr<ToolsNode>(new ToolsNode());
    node->config_ = config;
    
    // Build tool map for fast lookup
    for (const auto& tool : config.tools) {
        if (!tool) {
            continue;
        }
        
        auto info = tool->Info(ctx);
        if (info) {
            node->tool_map_[info->name] = tool;
        }
    }
    
    return node;
}

// FindTool finds a tool by name
std::shared_ptr<tool::BaseTool> ToolsNode::FindTool(const std::string& name) {
    auto it = tool_map_.find(name);
    if (it != tool_map_.end()) {
        return it->second;
    }
    return nullptr;
}

// ApplyInvokableMiddleware applies middleware chain to invokable endpoint
InvokableToolEndpoint ToolsNode::ApplyInvokableMiddleware(InvokableToolEndpoint endpoint) {
    InvokableToolEndpoint result = endpoint;
    
    // Apply middleware in reverse order (last middleware wraps first)
    for (auto it = config_.tool_call_middlewares.rbegin(); 
         it != config_.tool_call_middlewares.rend(); ++it) {
        if (it->invokable) {
            result = it->invokable(result);
        }
    }
    
    return result;
}

// ApplyStreamableMiddleware applies middleware chain to streamable endpoint
StreamableToolEndpoint ToolsNode::ApplyStreamableMiddleware(StreamableToolEndpoint endpoint) {
    StreamableToolEndpoint result = endpoint;
    
    // Apply middleware in reverse order
    for (auto it = config_.tool_call_middlewares.rbegin(); 
         it != config_.tool_call_middlewares.rend(); ++it) {
        if (it->streamable) {
            result = it->streamable(result);
        }
    }
    
    return result;
}

// ExecuteTool executes a single tool call (non-streaming)
// Aligns with eino compose.ToolsNode.executeTool
// Go reference: eino/compose/tool_node.go lines 333-401
schema::Message ToolsNode::ExecuteTool(
    void* ctx,
    const schema::ToolCall& tool_call,
    const std::vector<std::shared_ptr<tool::Option>>& options) {
    
    std::string tool_name = tool_call.function.name;
    std::string arguments = tool_call.function.arguments;
    std::string call_id = tool_call.id;
    
    // Apply arguments handler if configured
    if (config_.tool_arguments_handler) {
        try {
            arguments = config_.tool_arguments_handler(ctx, tool_name, arguments);
        } catch (const std::exception& e) {
            // Return error message
            return schema::ToolMessage(
                call_id, 
                tool_name,
                std::string("arguments handler error: ") + e.what());
        }
    }
    
    // Find tool
    auto tool = FindTool(tool_name);
    
    // Handle unknown tool
    if (!tool) {
        if (config_.unknown_tools_handler) {
            try {
                std::string result = config_.unknown_tools_handler(ctx, tool_name, arguments);
                return schema::ToolMessage(call_id, tool_name, result);
            } catch (const std::exception& e) {
                return schema::ToolMessage(
                    call_id, 
                    tool_name,
                    std::string("unknown tool handler error: ") + e.what());
            }
        } else {
            return schema::ToolMessage(
                call_id, 
                tool_name,
                std::string("tool not found: ") + tool_name);
        }
    }
    
    // Create tool input
    auto tool_input = std::make_shared<ToolInput>(tool_name, arguments, call_id);
    tool_input->call_options = options;
    
    // Check if tool is invokable
    auto invokable_tool = std::dynamic_pointer_cast<tool::InvokableTool>(tool);
    if (!invokable_tool) {
        return schema::ToolMessage(
            call_id, 
            tool_name,
            std::string("tool is not invokable: ") + tool_name);
    }
    
    // Create endpoint
    InvokableToolEndpoint endpoint = [invokable_tool](
        void* ctx, const std::shared_ptr<ToolInput>& input) -> std::shared_ptr<ToolOutput> {
        
        try {
            std::string result = invokable_tool->InvokeTool(ctx, input->arguments);
            return std::make_shared<ToolOutput>(result);
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("tool execution error: ") + e.what());
        }
    };
    
    // Apply middleware
    endpoint = ApplyInvokableMiddleware(endpoint);
    
    // Execute tool
    try {
        auto output = endpoint(ctx, tool_input);
        return schema::ToolMessage(call_id, tool_name, output->result);
    } catch (const std::exception& e) {
        return schema::ToolMessage(
            call_id, 
            tool_name,
            std::string("tool execution failed: ") + e.what());
    }
}

// ExecuteToolStream executes a single tool call (streaming)
// Aligns with eino compose.ToolsNode.executeToolStream
std::shared_ptr<StreamReader<schema::Message>> ToolsNode::ExecuteToolStream(
    void* ctx,
    const schema::ToolCall& tool_call,
    const std::vector<std::shared_ptr<tool::Option>>& options) {
    
    std::string tool_name = tool_call.function.name;
    std::string arguments = tool_call.function.arguments;
    std::string call_id = tool_call.id;
    
    // Apply arguments handler if configured
    if (config_.tool_arguments_handler) {
        try {
            arguments = config_.tool_arguments_handler(ctx, tool_name, arguments);
        } catch (const std::exception& e) {
            // Return error stream
            auto error_msg = schema::ToolMessage(
                call_id, 
                tool_name,
                std::string("arguments handler error: ") + e.what());
            return std::make_shared<SimpleStreamReader<schema::Message>>(
                std::vector<schema::Message>{error_msg});
        }
    }
    
    // Find tool
    auto tool = FindTool(tool_name);
    
    // Handle unknown tool
    if (!tool) {
        if (config_.unknown_tools_handler) {
            try {
                std::string result = config_.unknown_tools_handler(ctx, tool_name, arguments);
                auto msg = schema::ToolMessage(call_id, tool_name, result);
                return std::make_shared<SimpleStreamReader<schema::Message>>(
                    std::vector<schema::Message>{msg});
            } catch (const std::exception& e) {
                auto error_msg = schema::ToolMessage(
                    call_id, 
                    tool_name,
                    std::string("unknown tool handler error: ") + e.what());
                return std::make_shared<SimpleStreamReader<schema::Message>>(
                    std::vector<schema::Message>{error_msg});
            }
        } else {
            auto error_msg = schema::ToolMessage(
                call_id, 
                tool_name,
                std::string("tool not found: ") + tool_name);
            return std::make_shared<SimpleStreamReader<schema::Message>>(
                std::vector<schema::Message>{error_msg});
        }
    }
    
    // Create tool input
    auto tool_input = std::make_shared<ToolInput>(tool_name, arguments, call_id);
    tool_input->call_options = options;
    
    // Check if tool is streamable
    auto streamable_tool = std::dynamic_pointer_cast<tool::StreamableTool>(tool);
    if (!streamable_tool) {
        // Fallback to non-streaming
        auto msg = ExecuteTool(ctx, tool_call, options);
        return std::make_shared<SimpleStreamReader<schema::Message>>(
            std::vector<schema::Message>{msg});
    }
    
    // Create endpoint
    StreamableToolEndpoint endpoint = [streamable_tool, call_id, tool_name](
        void* ctx, const std::shared_ptr<ToolInput>& input) 
        -> std::shared_ptr<StreamToolOutput> {
        
        try {
            auto result_stream = streamable_tool->StreamTool(ctx, input->arguments);
            return std::make_shared<StreamToolOutput>(result_stream);
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("tool execution error: ") + e.what());
        }
    };
    
    // Apply middleware
    endpoint = ApplyStreamableMiddleware(endpoint);
    
    // Execute tool
    try {
        auto stream_output = endpoint(ctx, tool_input);
        
        // Convert string stream to Message stream
        // TODO: Implement StreamReader transformation
        // For now, collect all chunks and return as single message
        std::ostringstream oss;
        std::string chunk;
        while (stream_output->result->Read(chunk)) {
            oss << chunk;
        }
        
        auto msg = schema::ToolMessage(call_id, tool_name, oss.str());
        return std::make_shared<SimpleStreamReader<schema::Message>>(
            std::vector<schema::Message>{msg});
    } catch (const std::exception& e) {
        auto error_msg = schema::ToolMessage(
            call_id, 
            tool_name,
            std::string("tool execution failed: ") + e.what());
        return std::make_shared<SimpleStreamReader<schema::Message>>(
            std::vector<schema::Message>{error_msg});
    }
}

// Invoke executes all tool calls in the message
// Aligns with eino compose.ToolsNode.Invoke
// Go reference: eino/compose/tool_node.go lines 264-331
std::vector<schema::Message> ToolsNode::Invoke(
    std::shared_ptr<Context> ctx,
    const schema::Message& input) {
    
    void* raw_ctx = ctx ? ctx->GetRawContext() : nullptr;
    
    if (input.tool_calls.empty()) {
        return {};
    }
    
    std::vector<schema::Message> results;
    results.reserve(input.tool_calls.size());
    
    // Sequential execution
    if (config_.execute_sequentially) {
        for (const auto& tool_call : input.tool_calls) {
            auto msg = ExecuteTool(raw_ctx, tool_call, {});
            results.push_back(msg);
        }
        return results;
    }
    
    // Parallel execution
    std::vector<std::future<schema::Message>> futures;
    futures.reserve(input.tool_calls.size());
    
    for (const auto& tool_call : input.tool_calls) {
        futures.push_back(std::async(std::launch::async, 
            [this, raw_ctx, tool_call]() {
                return ExecuteTool(raw_ctx, tool_call, {});
            }));
    }
    
    // Collect results in order
    for (auto& future : futures) {
        try {
            results.push_back(future.get());
        } catch (const std::exception& e) {
            // Create error message
            schema::Message error_msg;
            error_msg.role = schema::RoleType::kTool;
            error_msg.content = std::string("parallel execution error: ") + e.what();
            results.push_back(error_msg);
        }
    }
    
    return results;
}

// Stream executes all tool calls and returns streaming results
// Aligns with eino compose.ToolsNode.Stream
std::shared_ptr<StreamReader<std::vector<schema::Message>>> ToolsNode::Stream(
    std::shared_ptr<Context> ctx,
    const schema::Message& input) {
    
    // For now, use Invoke and wrap as stream
    // Full streaming implementation requires async iterator
    auto results = Invoke(ctx, input);
    return std::make_shared<SimpleStreamReader<std::vector<schema::Message>>>(
        std::vector<std::vector<schema::Message>>{results});
}

} // namespace compose
} // namespace eino
