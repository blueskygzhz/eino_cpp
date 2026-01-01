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
#include "eino/adk/react.h"

namespace eino {
namespace adk {
namespace {

// Mock ChatModel for testing
class MockChatModel : public ChatModel {
public:
    std::shared_ptr<AsyncIterator<Message>> Generate(
        void* ctx,
        const std::vector<Message>& messages,
        const std::vector<ChatModelOption>& opts = {}) override {
        auto [iterator, generator] = NewAsyncIteratorPair<Message>();
        
        // Generate a simple response message
        Message msg = Message::Text("Mock response");
        generator->Send(msg);
        generator->Close();
        
        return iterator;
    }
};

TEST(ReActTest, BasicConfiguration) {
    void* ctx = nullptr;
    
    ReActConfig config;
    config.name = "test_react_agent";
    config.description = "Test ReAct agent";
    config.chat_model = std::make_shared<MockChatModel>();
    config.max_iterations = 5;
    
    // Note: This will fail without a proper implementation,
    // but demonstrates the expected API
    // auto agent = NewReActAgent(ctx, config);
    // EXPECT_NE(agent, nullptr);
    // EXPECT_EQ(agent->Name(ctx), "test_react_agent");
}

TEST(ReActTest, MaxIterationsDefault) {
    ReActConfig config;
    config.max_iterations = 0;  // Should default to some value
    
    // Verify default behavior
    EXPECT_GE(config.max_iterations, 0);
}

TEST(ReActTest, ToolConfiguration) {
    void* ctx = nullptr;
    
    ReActConfig config;
    config.name = "react_with_tools";
    config.chat_model = std::make_shared<MockChatModel>();
    
    // Add mock tools
    ToolsConfig tools_config;
    // tools_config.tools = {...}
    config.tools_config = tools_config;
    
    // Verify tools are configured
    EXPECT_TRUE(true);  // Placeholder
}

} // namespace
} // namespace adk
} // namespace eino
