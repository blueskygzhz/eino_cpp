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

// Checkpoint Example - 演示如何使用 checkpoint 功能进行中断和恢复

#include "eino/adk/runner.h"
#include "eino/adk/types.h"
#include "eino/compose/checkpoint.h"
#include <iostream>
#include <map>
#include <memory>

using namespace eino::adk;
using namespace eino::compose;

// ============================================================================
// 1. 实现简单的内存 CheckPointStore
// ============================================================================

class MemoryCheckPointStore : public CheckPointStore {
public:
    void Save(const std::string& checkpoint_id, const std::string& data) override {
        store_[checkpoint_id] = data;
        std::cout << "[CheckPointStore] Saved checkpoint: " << checkpoint_id 
                  << " (size: " << data.size() << " bytes)\n";
    }

    std::string Load(const std::string& checkpoint_id, bool& exists) override {
        auto it = store_.find(checkpoint_id);
        if (it != store_.end()) {
            exists = true;
            std::cout << "[CheckPointStore] Loaded checkpoint: " << checkpoint_id 
                      << " (size: " << it->second.size() << " bytes)\n";
            return it->second;
        }
        exists = false;
        std::cout << "[CheckPointStore] Checkpoint not found: " << checkpoint_id << "\n";
        return "";
    }

    bool Exists(const std::string& checkpoint_id) override {
        return store_.find(checkpoint_id) != store_.end();
    }

    void Delete(const std::string& checkpoint_id) override {
        store_.erase(checkpoint_id);
        std::cout << "[CheckPointStore] Deleted checkpoint: " << checkpoint_id << "\n";
    }

private:
    std::map<std::string, std::string> store_;
};

// ============================================================================
// 2. 演示基本的 Checkpoint 使用
// ============================================================================

void BasicCheckpointExample() {
    std::cout << "\n=== Basic Checkpoint Example ===\n\n";

    // 创建 checkpoint store
    auto store = std::make_shared<MemoryCheckPointStore>();

    // 创建 CheckPointer (使用默认 JSON serializer)
    auto checkpointer = std::make_shared<CheckPointer>(store);

    // 创建一个 checkpoint
    auto cp = std::make_shared<CheckPoint>();
    
    // 添加 channel 数据
    nlohmann::json channel_data;
    channel_data["key1"] = "value1";
    channel_data["key2"] = 42;
    cp->channels["main_channel"] = channel_data;

    // 添加 input 数据
    nlohmann::json input_data;
    input_data["user_message"] = "Hello, AI!";
    cp->inputs["input_node"] = input_data;

    // 添加 state
    nlohmann::json state;
    state["step"] = 1;
    state["status"] = "processing";
    cp->state = state;

    // 保存 checkpoint
    auto ctx = std::make_shared<Context>();
    auto err = checkpointer->Set(ctx, "checkpoint_001", cp);
    if (!err.empty()) {
        std::cerr << "Error saving checkpoint: " << err << "\n";
        return;
    }
    std::cout << "✅ Checkpoint saved successfully\n\n";

    // 加载 checkpoint
    auto [loaded_cp, existed, load_err] = checkpointer->Get(ctx, "checkpoint_001");
    if (!load_err.empty()) {
        std::cerr << "Error loading checkpoint: " << load_err << "\n";
        return;
    }
    if (!existed) {
        std::cerr << "Checkpoint does not exist\n";
        return;
    }

    std::cout << "✅ Checkpoint loaded successfully\n";
    std::cout << "Loaded data:\n";
    std::cout << "  - Channels: " << loaded_cp->channels.size() << "\n";
    std::cout << "  - Inputs: " << loaded_cp->inputs.size() << "\n";
    std::cout << "  - State: " << loaded_cp->state.dump(2) << "\n";
}

// ============================================================================
// 3. 演示 Stream 转换
// ============================================================================

void StreamConversionExample() {
    std::cout << "\n=== Stream Conversion Example ===\n\n";

    auto store = std::make_shared<MemoryCheckPointStore>();
    auto checkpointer = std::make_shared<CheckPointer>(store);

    // 创建包含 stream 标记的 checkpoint
    auto cp = std::make_shared<CheckPoint>();
    
    nlohmann::json stream_data;
    stream_data["_stream"] = true;
    stream_data["_stream_data"] = nlohmann::json::array({
        "chunk1", "chunk2", "chunk3"
    });
    cp->channels["stream_channel"] = stream_data;

    std::cout << "Before conversion (stream format):\n";
    std::cout << cp->channels["stream_channel"].dump(2) << "\n\n";

    // 转换 stream 为 non-stream (用于序列化)
    auto err = checkpointer->ConvertCheckPoint(cp, true);
    if (!err.empty()) {
        std::cerr << "Error converting checkpoint: " << err << "\n";
        return;
    }

    std::cout << "After conversion (non-stream format):\n";
    std::cout << cp->channels["stream_channel"].dump(2) << "\n\n";

    // 恢复 stream 格式
    err = checkpointer->RestoreCheckPoint(cp, true);
    if (!err.empty()) {
        std::cerr << "Error restoring checkpoint: " << err << "\n";
        return;
    }

    std::cout << "After restoration (stream format):\n";
    std::cout << cp->channels["stream_channel"].dump(2) << "\n";
    std::cout << "✅ Stream conversion completed\n";
}

// ============================================================================
// 4. 演示 Runner 的 Checkpoint 集成
// ============================================================================

void RunnerCheckpointExample() {
    std::cout << "\n=== Runner Checkpoint Integration Example ===\n\n";

    // 注意：这个示例展示了 API 使用方式
    // 实际运行需要一个完整的 Agent 实现

    std::cout << "Example code for Runner with checkpoints:\n\n";
    
    std::cout << R"(
// 1. 创建 checkpoint store
auto store = std::make_shared<MemoryCheckPointStore>();

// 2. 配置 Runner
RunnerConfig config;
config.agent = my_agent;  // 你的 Agent 实例
config.enable_streaming = true;
config.checkpoint_store = store;

auto runner = NewRunner(config);

// 3. 运行并自动保存 checkpoint
std::vector<Message> messages = {UserMessage("Hello")};
auto options = std::vector<std::shared_ptr<AgentRunOption>>{
    std::make_shared<WithCheckPointID>("my_checkpoint")
};

auto event_iter = runner->Run(ctx, messages, options);

// 4. 处理事件
while (event_iter->Next(event)) {
    if (event->action && event->action->interrupted) {
        std::cout << "Execution interrupted, checkpoint saved\n";
        break;
    }
    // 处理其他事件
}

// 5. 从 checkpoint 恢复
auto [resume_iter, error] = runner->Resume(ctx, "my_checkpoint", options);
if (error.empty()) {
    std::cout << "Resumed from checkpoint successfully\n";
    // 继续处理事件
    while (resume_iter->Next(event)) {
        // 处理事件
    }
}
    )";

    std::cout << "\n✅ Runner checkpoint integration example shown\n";
}

// ============================================================================
// 5. 演示 Nested Subgraph Checkpoints
// ============================================================================

void NestedCheckpointExample() {
    std::cout << "\n=== Nested Subgraph Checkpoint Example ===\n\n";

    auto store = std::make_shared<MemoryCheckPointStore>();
    auto checkpointer = std::make_shared<CheckPointer>(store);

    // 创建主 checkpoint
    auto main_cp = std::make_shared<CheckPoint>();
    nlohmann::json main_state;
    main_state["level"] = "main";
    main_cp->state = main_state;

    // 创建子图 checkpoint
    auto sub_cp1 = std::make_shared<CheckPoint>();
    nlohmann::json sub_state1;
    sub_state1["level"] = "sub1";
    sub_cp1->state = sub_state1;

    auto sub_cp2 = std::make_shared<CheckPoint>();
    nlohmann::json sub_state2;
    sub_state2["level"] = "sub2";
    sub_cp2->state = sub_state2;

    // 建立嵌套关系
    main_cp->sub_graphs["subgraph1"] = sub_cp1;
    main_cp->sub_graphs["subgraph2"] = sub_cp2;

    std::cout << "Created nested checkpoint structure:\n";
    std::cout << "  - Main checkpoint with state: " << main_cp->state.dump() << "\n";
    std::cout << "  - Subgraph1 with state: " << sub_cp1->state.dump() << "\n";
    std::cout << "  - Subgraph2 with state: " << sub_cp2->state.dump() << "\n\n";

    // 保存
    auto ctx = std::make_shared<Context>();
    auto err = checkpointer->Set(ctx, "nested_checkpoint", main_cp);
    if (!err.empty()) {
        std::cerr << "Error: " << err << "\n";
        return;
    }

    // 加载并验证
    auto [loaded, existed, load_err] = checkpointer->Get(ctx, "nested_checkpoint");
    if (!load_err.empty() || !existed) {
        std::cerr << "Error loading: " << load_err << "\n";
        return;
    }

    std::cout << "Loaded nested checkpoint:\n";
    std::cout << "  - Main state: " << loaded->state.dump() << "\n";
    std::cout << "  - Subgraphs count: " << loaded->sub_graphs.size() << "\n";
    for (const auto& [key, sub_cp] : loaded->sub_graphs) {
        std::cout << "    * " << key << ": " << sub_cp->state.dump() << "\n";
    }
    std::cout << "✅ Nested checkpoint verified\n";
}

// ============================================================================
// Main Function
// ============================================================================

int main() {
    std::cout << "==============================================\n";
    std::cout << "  eino_cpp Checkpoint Functionality Examples  \n";
    std::cout << "==============================================\n";

    try {
        // 运行所有示例
        BasicCheckpointExample();
        StreamConversionExample();
        NestedCheckpointExample();
        RunnerCheckpointExample();

        std::cout << "\n==============================================\n";
        std::cout << "  All Examples Completed Successfully ✅     \n";
        std::cout << "==============================================\n";

    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
