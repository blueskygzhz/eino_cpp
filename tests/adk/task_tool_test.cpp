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

#include <gtest/gtest.h>
#include "eino/adk/task_tool.h"
#include "eino/adk/chat_model_agent.h"

namespace eino {
namespace adk {
namespace {

// Mock Agent for testing
class MockAgent : public Agent {
public:
    explicit MockAgent(const std::string& name, const std::string& desc)
        : name_(name), description_(desc) {}
    
    std::string Name(void* ctx) const override { return name_; }
    std::string Description(void* ctx) const override { return description_; }
    
    std::shared_ptr<AsyncIterator<AgentEvent>> Run(
        void* ctx,
        const AgentInput& input,
        const std::vector<AgentRunOption>& opts = {}) override {
        // Mock implementation
        auto [iterator, generator] = NewAsyncIteratorPair<AgentEvent>();
        
        AgentEvent event;
        event.agent_name = name_;
        event.output = std::make_shared<AgentOutput>();
        event.output->message_output = std::make_shared<MessageVariant>();
        event.output->message_output->is_streaming = false;
        event.output->message_output->message = Message::Text("Mock result from " + name_);
        
        generator->Send(event);
        generator->Close();
        
        return iterator;
    }
    
private:
    std::string name_;
    std::string description_;
};

// Mock Tool wrapping MockAgent
class MockAgentTool : public Tool {
public:
    explicit MockAgentTool(std::shared_ptr<Agent> agent)
        : agent_(agent) {}
    
    ToolInfo Info(void* ctx) const override {
        ToolInfo info;
        info.name = agent_->Name(ctx);
        info.desc = agent_->Description(ctx);
        return info;
    }
    
    std::string InvokableRun(
        void* ctx,
        const std::string& arguments_json,
        const std::vector<ToolOption>& opts = {}) override {
        return "Result from " + agent_->Name(ctx) + ": " + arguments_json;
    }
    
private:
    std::shared_ptr<Agent> agent_;
};

TEST(TaskToolTest, BasicConstruction) {
    void* ctx = nullptr;
    
    // Create mock subagents
    auto agent1 = std::make_shared<MockAgent>("researcher", "Research agent");
    auto agent2 = std::make_shared<MockAgent>("coder", "Coding agent");
    
    std::vector<std::shared_ptr<Agent>> agents = {agent1, agent2};
    
    // Create agent tools
    std::map<std::string, std::shared_ptr<Tool>> agent_tools;
    agent_tools["researcher"] = std::make_shared<MockAgentTool>(agent1);
    agent_tools["coder"] = std::make_shared<MockAgentTool>(agent2);
    
    // Create TaskTool
    TaskTool task_tool(ctx, agent_tools, agents, nullptr);
    
    // Verify tool info
    ToolInfo info = task_tool.Info(ctx);
    EXPECT_EQ(info.name, "task");
    EXPECT_FALSE(info.desc.empty());
    EXPECT_TRUE(info.desc.find("researcher") != std::string::npos);
    EXPECT_TRUE(info.desc.find("coder") != std::string::npos);
}

TEST(TaskToolTest, InvokableRun) {
    void* ctx = nullptr;
    
    auto agent1 = std::make_shared<MockAgent>("researcher", "Research agent");
    std::vector<std::shared_ptr<Agent>> agents = {agent1};
    
    std::map<std::string, std::shared_ptr<Tool>> agent_tools;
    agent_tools["researcher"] = std::make_shared<MockAgentTool>(agent1);
    
    TaskTool task_tool(ctx, agent_tools, agents, nullptr);
    
    // Test valid invocation
    std::string input = R"({"subagent_type": "researcher", "description": "Search for papers"})";
    std::string result = task_tool.InvokableRun(ctx, input);
    
    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.find("researcher") != std::string::npos);
}

TEST(TaskToolTest, InvalidSubagentType) {
    void* ctx = nullptr;
    
    auto agent1 = std::make_shared<MockAgent>("researcher", "Research agent");
    std::vector<std::shared_ptr<Agent>> agents = {agent1};
    
    std::map<std::string, std::shared_ptr<Tool>> agent_tools;
    agent_tools["researcher"] = std::make_shared<MockAgentTool>(agent1);
    
    TaskTool task_tool(ctx, agent_tools, agents, nullptr);
    
    // Test invalid subagent type
    std::string input = R"({"subagent_type": "nonexistent", "description": "Test"})";
    std::string result = task_tool.InvokableRun(ctx, input);
    
    EXPECT_TRUE(result.find("not found") != std::string::npos);
}

TEST(TaskToolTest, CustomDescriptionGenerator) {
    void* ctx = nullptr;
    
    auto agent1 = std::make_shared<MockAgent>("test_agent", "Test description");
    std::vector<std::shared_ptr<Agent>> agents = {agent1};
    
    std::map<std::string, std::shared_ptr<Tool>> agent_tools;
    agent_tools["test_agent"] = std::make_shared<MockAgentTool>(agent1);
    
    // Custom description generator
    auto custom_gen = [](void* ctx, const std::vector<std::shared_ptr<Agent>>& agents) {
        return "CUSTOM DESCRIPTION";
    };
    
    TaskTool task_tool(ctx, agent_tools, agents, custom_gen);
    
    ToolInfo info = task_tool.Info(ctx);
    EXPECT_EQ(info.desc, "CUSTOM DESCRIPTION");
}

} // namespace
} // namespace adk
} // namespace eino
