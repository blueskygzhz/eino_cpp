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

#ifndef EINO_CPP_COMPONENTS_TOOL_H_
#define EINO_CPP_COMPONENTS_TOOL_H_

#include "../compose/runnable.h"
#include "../schema/types.h"
#include <string>
#include <vector>
#include <memory>

namespace eino {
namespace components {

// BaseTool gets tool info for ChatModel intent recognition
class BaseTool {
public:
    virtual ~BaseTool() = default;
    
    // GetToolInfo returns the tool information
    virtual schema::ToolInfo GetToolInfo(std::shared_ptr<compose::Context> ctx) = 0;
};

// InvokableTool is a tool that can be invoked synchronously
// Input/Output: argument string in JSON format
class InvokableTool : public BaseTool, public compose::Runnable<std::string, std::string> {
public:
    virtual ~InvokableTool() = default;
    
    // InvokableRun executes the tool with arguments in JSON format
    virtual std::string InvokableRun(
        std::shared_ptr<compose::Context> ctx,
        const std::string& arguments_in_json,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) = 0;
};

// StreamableTool is a tool that can be invoked with streaming output
class StreamableTool : public BaseTool, public compose::Runnable<std::string, std::string> {
public:
    virtual ~StreamableTool() = default;
    
    // StreamableRun executes the tool with streaming output
    virtual std::shared_ptr<compose::StreamReader<std::string>> StreamableRun(
        std::shared_ptr<compose::Context> ctx,
        const std::string& arguments_in_json,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) = 0;
};

// ToolsNode manages multiple tools and executes them based on tool calls
// Input: Message (with tool calls), Output: Message (with tool results)
class ToolsNode : public compose::Runnable<schema::Message, schema::Message> {
public:
    virtual ~ToolsNode() = default;
    
    // AddTool adds a tool to the node
    virtual void AddTool(std::shared_ptr<BaseTool> tool) = 0;
    
    // GetTools returns all registered tools
    virtual std::vector<std::shared_ptr<BaseTool>> GetTools() const = 0;
};

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_TOOL_H_
