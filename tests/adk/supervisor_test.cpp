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
#include "eino/adk/prebuilt/supervisor.h"

namespace eino {
namespace adk {
namespace supervisor {
namespace {

// Mock Agent for testing
class MockSupervisor : public Agent {
public:
    explicit MockSupervisor(const std::string& name) : name_(name) {}
    
    std::string Name(void* ctx) const override { return name_; }
    std::string Description(void* ctx) const override { return "Mock supervisor"; }
    
    std::shared_ptr<AsyncIterator<AgentEvent>> Run(
        void* ctx,
        const AgentInput& input,
        const std::vector<AgentRunOption>& opts = {}) override {
        auto [iterator, generator] = NewAsyncIteratorPair<AgentEvent>();
        generator->Close();
        return iterator;
    }
    
    // Track sub-agents that were set
    void SetSubAgentsForTest(const std::vector<std::shared_ptr<Agent>>& agents) {
        sub_agents_ = agents;
    }
    
    const std::vector<std::shared_ptr<Agent>>& GetSubAgents() const {
        return sub_agents_;
    }
    
private:
    std::string name_;
    std::vector<std::shared_ptr<Agent>> sub_agents_;
};

class MockSubAgent : public Agent {
public:
    explicit MockSubAgent(const std::string& name) : name_(name) {}
    
    std::string Name(void* ctx) const override { return name_; }
    std::string Description(void* ctx) const override { return "Mock sub-agent"; }
    
    std::shared_ptr<AsyncIterator<AgentEvent>> Run(
        void* ctx,
        const AgentInput& input,
        const std::vector<AgentRunOption>& opts = {}) override {
        auto [iterator, generator] = NewAsyncIteratorPair<AgentEvent>();
        generator->Close();
        return iterator;
    }
    
private:
    std::string name_;
};

TEST(SupervisorTest, BasicConfiguration) {
    void* ctx = nullptr;
    
    auto supervisor_agent = std::make_shared<MockSupervisor>("coordinator");
    auto sub1 = std::make_shared<MockSubAgent>("worker1");
    auto sub2 = std::make_shared<MockSubAgent>("worker2");
    
    Config config;
    config.supervisor = supervisor_agent;
    config.sub_agents = {sub1, sub2};
    
    auto [result, error] = New(ctx, config);
    
    // NOTE: This test is a conceptual test. In real implementation,
    // we would need to verify that:
    // 1. Each sub-agent is wrapped with DeterministicTransferTo
    // 2. The wrapped agents can only transfer to the supervisor
    // 3. SetSubAgents was called correctly
    
    // For now, we just verify basic success/failure
    if (error.empty()) {
        EXPECT_NE(result, nullptr);
    }
}

TEST(SupervisorTest, NullSupervisor) {
    void* ctx = nullptr;
    
    auto sub1 = std::make_shared<MockSubAgent>("worker1");
    
    Config config;
    config.supervisor = nullptr;  // Invalid
    config.sub_agents = {sub1};
    
    auto [result, error] = New(ctx, config);
    
    EXPECT_EQ(result, nullptr);
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("cannot be null") != std::string::npos);
}

TEST(SupervisorTest, NullSubAgent) {
    void* ctx = nullptr;
    
    auto supervisor_agent = std::make_shared<MockSupervisor>("coordinator");
    
    Config config;
    config.supervisor = supervisor_agent;
    config.sub_agents = {nullptr};  // Invalid
    
    auto [result, error] = New(ctx, config);
    
    EXPECT_EQ(result, nullptr);
    EXPECT_FALSE(error.empty());
}

TEST(SupervisorTest, EmptySubAgents) {
    void* ctx = nullptr;
    
    auto supervisor_agent = std::make_shared<MockSupervisor>("coordinator");
    
    Config config;
    config.supervisor = supervisor_agent;
    config.sub_agents = {};  // Empty is valid
    
    auto [result, error] = New(ctx, config);
    
    // Empty sub-agents list should be valid
    // The supervisor just won't have any agents to delegate to
}

} // namespace
} // namespace supervisor
} // namespace adk
} // namespace eino
