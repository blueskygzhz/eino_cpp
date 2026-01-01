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

#ifndef EINO_CPP_COMPOSE_TOOL_NODE_H_
#define EINO_CPP_COMPOSE_TOOL_NODE_H_

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include "../schema/message.h"
#include "../components/tool.h"
#include "runnable.h"

namespace eino {
namespace compose {

// ToolInput represents input for a single tool call
// Aligns with eino compose.ToolInput
// Go reference: eino/compose/tool_node.go lines 77-88
struct ToolInput {
    std::string name;        // Name of the tool
    std::string arguments;   // Tool call arguments (JSON string)
    std::string call_id;     // Unique call ID
    std::vector<std::shared_ptr<tool::Option>> call_options;  // Tool execution options
    
    ToolInput() = default;
    ToolInput(const std::string& n, const std::string& args, const std::string& id)
        : name(n), arguments(args), call_id(id) {}
};

// ToolOutput represents non-streaming tool execution result
// Aligns with eino compose.ToolOutput
// Go reference: eino/compose/tool_node.go lines 91-94
struct ToolOutput {
    std::string result;  // Tool execution result
    
    ToolOutput() = default;
    ToolOutput(const std::string& r) : result(r) {}
};

// StreamToolOutput represents streaming tool execution result
// Aligns with eino compose.StreamToolOutput
// Go reference: eino/compose/tool_node.go lines 97-100
struct StreamToolOutput {
    std::shared_ptr<StreamReader<std::string>> result;  // Streaming result
    
    StreamToolOutput() = default;
    StreamToolOutput(std::shared_ptr<StreamReader<std::string>> r) : result(r) {}
};

// InvokableToolEndpoint is the endpoint for non-streaming tool execution
// Aligns with eino compose.InvokableToolEndpoint
// Go reference: eino/compose/tool_node.go line 102
using InvokableToolEndpoint = std::function<
    std::shared_ptr<ToolOutput>(void* ctx, const std::shared_ptr<ToolInput>& input)>;

// StreamableToolEndpoint is the endpoint for streaming tool execution
// Aligns with eino compose.StreamableToolEndpoint
// Go reference: eino/compose/tool_node.go line 104
using StreamableToolEndpoint = std::function<
    std::shared_ptr<StreamToolOutput>(void* ctx, const std::shared_ptr<ToolInput>& input)>;

// InvokableToolMiddleware wraps InvokableToolEndpoint for custom processing
// Aligns with eino compose.InvokableToolMiddleware
// Go reference: eino/compose/tool_node.go lines 107-108
using InvokableToolMiddleware = std::function<InvokableToolEndpoint(InvokableToolEndpoint)>;

// StreamableToolMiddleware wraps StreamableToolEndpoint for custom processing
// Aligns with eino compose.StreamableToolMiddleware
// Go reference: eino/compose/tool_node.go lines 111-112
using StreamableToolMiddleware = std::function<StreamableToolEndpoint(StreamableToolEndpoint)>;

// ToolMiddleware contains both invokable and streamable middleware
// Aligns with eino compose.ToolMiddleware
// Go reference: eino/compose/tool_node.go lines 115-123
struct ToolMiddleware {
    InvokableToolMiddleware invokable;    // For non-streaming tools
    StreamableToolMiddleware streamable;  // For streaming tools
    
    ToolMiddleware() = default;
};

// ToolsNodeConfig configuration for ToolsNode
// Aligns with eino compose.ToolsNodeConfig
// Go reference: eino/compose/tool_node.go lines 125-170
struct ToolsNodeConfig {
    // Tools that can be called (must implement InvokableTool or StreamableTool)
    std::vector<std::shared_ptr<tool::BaseTool>> tools;
    
    // Handler for unknown tools (LLM hallucination)
    std::function<std::string(void* ctx, const std::string& name, const std::string& input)> 
        unknown_tools_handler;
    
    // Execute tools sequentially vs in parallel
    bool execute_sequentially = false;
    
    // Handler for tool arguments preprocessing
    std::function<std::string(void* ctx, const std::string& name, const std::string& input)>
        tool_arguments_handler;
    
    // Middleware for tool calls
    std::vector<ToolMiddleware> tool_call_middlewares;
    
    ToolsNodeConfig() = default;
};

// ToolsNode executes tool calls from AssistantMessage
// Aligns with eino compose.ToolsNode
// Go reference: eino/compose/tool_node.go lines 64-75
//
// Interface:
//   Invoke(ctx, Message) -> vector<Message>
//   Stream(ctx, Message) -> StreamReader<vector<Message>>
//
// Input: AssistantMessage containing ToolCalls
// Output: Array of ToolMessage in same order as ToolCalls
class ToolsNode : public ComposableRunnable<schema::Message, std::vector<schema::Message>> {
public:
    // Create new ToolsNode with configuration
    // Aligns with eino compose.NewToolNode
    // Go reference: eino/compose/tool_node.go lines 172-262
    static std::shared_ptr<ToolsNode> New(
        void* ctx, 
        const ToolsNodeConfig& config);
    
    virtual ~ToolsNode() = default;
    
    // Invoke executes tools in the message's ToolCalls
    std::vector<schema::Message> Invoke(
        std::shared_ptr<Context> ctx,
        const schema::Message& input) override;
    
    // Stream executes tools and returns streaming results
    std::shared_ptr<StreamReader<std::vector<schema::Message>>> Stream(
        std::shared_ptr<Context> ctx,
        const schema::Message& input) override;
    
    // Collect not supported for ToolsNode
    std::vector<schema::Message> Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<schema::Message>> input) override {
        throw std::runtime_error("ToolsNode: Collect not supported");
    }
    
    // Transform not supported for ToolsNode
    std::shared_ptr<StreamReader<std::vector<schema::Message>>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<schema::Message>> input) override {
        throw std::runtime_error("ToolsNode: Transform not supported");
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(schema::Message);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(std::vector<schema::Message>);
    }
    
    std::string GetComponentType() const override {
        return "ToolsNode";
    }

protected:
    ToolsNode() = default;
    
    // Execute a single tool call (non-streaming)
    // Aligns with eino compose.ToolsNode.executeTool
    schema::Message ExecuteTool(
        void* ctx,
        const schema::ToolCall& tool_call,
        const std::vector<std::shared_ptr<tool::Option>>& options);
    
    // Execute a single tool call (streaming)
    std::shared_ptr<StreamReader<schema::Message>> ExecuteToolStream(
        void* ctx,
        const schema::ToolCall& tool_call,
        const std::vector<std::shared_ptr<tool::Option>>& options);
    
    // Find tool by name
    std::shared_ptr<tool::BaseTool> FindTool(const std::string& name);
    
    // Apply middleware chain to endpoint
    InvokableToolEndpoint ApplyInvokableMiddleware(InvokableToolEndpoint endpoint);
    StreamableToolEndpoint ApplyStreamableMiddleware(StreamableToolEndpoint endpoint);
    
private:
    // Tool map for fast lookup
    std::map<std::string, std::shared_ptr<tool::BaseTool>> tool_map_;
    
    // Configuration
    ToolsNodeConfig config_;
    
    // Executed tools tracking (for checkpoint/resume)
    std::map<std::string, std::string> executed_tools_;
};

// ToolsNodeOption for configuring ToolsNode at graph level
// Aligns with eino compose.ToolsNodeOption
// Go reference: eino/compose/tool_node.go lines 34-58
struct ToolsNodeOption {
    std::vector<std::shared_ptr<tool::Option>> tool_options;
    std::vector<std::shared_ptr<tool::BaseTool>> tool_list;
    std::map<std::string, std::string> executed_tools;
};

// WithToolOption adds tool options
// Aligns with eino compose.WithToolOption
inline ToolsNodeOption WithToolOption(const std::vector<std::shared_ptr<tool::Option>>& opts) {
    ToolsNodeOption option;
    option.tool_options = opts;
    return option;
}

// WithToolList sets tool list
// Aligns with eino compose.WithToolList
inline ToolsNodeOption WithToolList(const std::vector<std::shared_ptr<tool::BaseTool>>& tools) {
    ToolsNodeOption option;
    option.tool_list = tools;
    return option;
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_TOOL_NODE_H_
