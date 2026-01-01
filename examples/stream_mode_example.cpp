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

/**
 * Stream Mode Example - 流式处理示例
 * 
 * 本示例展示如何使用 eino compose 构建流式处理 Graph：
 * 1. 创建流式处理节点
 * 2. 构建 DAG 流水线
 * 3. 使用 Transform 进行流式执行
 * 4. 实时处理流式输出
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>

#include "eino/compose/graph.h"
#include "eino/compose/runnable.h"
#include "eino/compose/stream_reader.h"
#include "eino/schema/stream.h"

using json = nlohmann::json;
using namespace eino::compose;
using namespace eino::schema;

// ============================================================================
// 辅助函数
// ============================================================================

void PrintSeparator(const std::string& title) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}

// ============================================================================
// 流式节点实现：文本分词器
// ============================================================================

class TokenizerNode : public Runnable<std::string, std::vector<std::string>> {
public:
    explicit TokenizerNode(const std::string& name) : name_(name) {}
    
    // Invoke: 非流式模式
    std::vector<std::string> Invoke(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = {}) override {
        
        std::cout << "[" << name_ << "] Invoke mode: Tokenizing \"" << input << "\"" << std::endl;
        
        std::vector<std::string> tokens;
        std::istringstream iss(input);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    // Stream: 将输入转为流式输出
    std::shared_ptr<StreamReader<std::vector<std::string>>> Stream(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = {}) override {
        
        std::cout << "[" << name_ << "] Stream mode: Streaming tokens from \"" << input << "\"" << std::endl;
        
        auto tokens = Invoke(ctx, input, opts);
        
        // 创建 SimpleStreamReader 并逐个添加 token
        auto stream = std::make_shared<SimpleStreamReader<std::vector<std::string>>>();
        for (const auto& token : tokens) {
            std::vector<std::string> single_token = {token};
            stream->Add(single_token);
            std::cout << "  -> Token streamed: " << token << std::endl;
        }
        
        return stream;
    }
    
    // Collect: 收集流式输入
    std::vector<std::string> Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::string>> input,
        const std::vector<Option>& opts = {}) override {
        
        std::vector<std::string> all_tokens;
        std::string chunk;
        while (input && input->Read(chunk)) {
            auto tokens = Invoke(ctx, chunk, opts);
            all_tokens.insert(all_tokens.end(), tokens.begin(), tokens.end());
        }
        return all_tokens;
    }
    
    // Transform: 流式输入 -> 流式输出
    std::shared_ptr<StreamReader<std::vector<std::string>>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::string>> input,
        const std::vector<Option>& opts = {}) override {
        
        std::cout << "[" << name_ << "] Transform mode: Processing stream" << std::endl;
        
        auto output = std::make_shared<SimpleStreamReader<std::vector<std::string>>>();
        
        std::string chunk;
        while (input && input->Read(chunk)) {
            auto tokens = Invoke(ctx, chunk, opts);
            output->Add(tokens);
        }
        
        return output;
    }
    
private:
    std::string name_;
};

// ============================================================================
// 流式节点实现：单词转大写
// ============================================================================

class UppercaseNode : public Runnable<std::vector<std::string>, std::vector<std::string>> {
public:
    explicit UppercaseNode(const std::string& name) : name_(name) {}
    
    std::vector<std::string> Invoke(
        std::shared_ptr<Context> ctx,
        const std::vector<std::string>& input,
        const std::vector<Option>& opts = {}) override {
        
        std::vector<std::string> result;
        for (const auto& word : input) {
            std::string upper = word;
            for (char& c : upper) {
                c = std::toupper(c);
            }
            result.push_back(upper);
        }
        
        std::cout << "[" << name_ << "] Processed " << input.size() << " words" << std::endl;
        return result;
    }
    
    std::shared_ptr<StreamReader<std::vector<std::string>>> Stream(
        std::shared_ptr<Context> ctx,
        const std::vector<std::string>& input,
        const std::vector<Option>& opts = {}) override {
        
        auto result = Invoke(ctx, input, opts);
        auto stream = std::make_shared<SimpleStreamReader<std::vector<std::string>>>();
        stream->Add(result);
        return stream;
    }
    
    std::vector<std::string> Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::vector<std::string>>> input,
        const std::vector<Option>& opts = {}) override {
        
        std::vector<std::string> all_words;
        std::vector<std::string> chunk;
        while (input && input->Read(chunk)) {
            auto processed = Invoke(ctx, chunk, opts);
            all_words.insert(all_words.end(), processed.begin(), processed.end());
        }
        return all_words;
    }
    
    std::shared_ptr<StreamReader<std::vector<std::string>>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::vector<std::string>>> input,
        const std::vector<Option>& opts = {}) override {
        
        std::cout << "[" << name_ << "] Transform mode" << std::endl;
        
        auto output = std::make_shared<SimpleStreamReader<std::vector<std::string>>>();
        
        std::vector<std::string> chunk;
        while (input && input->Read(chunk)) {
            auto processed = Invoke(ctx, chunk, opts);
            output->Add(processed);
            
            // 模拟实时处理
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        return output;
    }
    
private:
    std::string name_;
};

// ============================================================================
// 流式节点实现：计数器
// ============================================================================

class CounterNode : public Runnable<std::vector<std::string>, int> {
public:
    explicit CounterNode(const std::string& name) : name_(name) {}
    
    int Invoke(
        std::shared_ptr<Context> ctx,
        const std::vector<std::string>& input,
        const std::vector<Option>& opts = {}) override {
        
        int count = static_cast<int>(input.size());
        std::cout << "[" << name_ << "] Count: " << count << std::endl;
        return count;
    }
    
    std::shared_ptr<StreamReader<int>> Stream(
        std::shared_ptr<Context> ctx,
        const std::vector<std::string>& input,
        const std::vector<Option>& opts = {}) override {
        
        auto result = Invoke(ctx, input, opts);
        auto stream = std::make_shared<SimpleStreamReader<int>>();
        stream->Add(result);
        return stream;
    }
    
    int Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::vector<std::string>>> input,
        const std::vector<Option>& opts = {}) override {
        
        int total = 0;
        std::vector<std::string> chunk;
        while (input && input->Read(chunk)) {
            total += static_cast<int>(chunk.size());
        }
        
        std::cout << "[" << name_ << "] Total count: " << total << std::endl;
        return total;
    }
    
    std::shared_ptr<StreamReader<int>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::vector<std::string>>> input,
        const std::vector<Option>& opts = {}) override {
        
        std::cout << "[" << name_ << "] Transform mode: Counting stream chunks" << std::endl;
        
        auto output = std::make_shared<SimpleStreamReader<int>>();
        
        std::vector<std::string> chunk;
        while (input && input->Read(chunk)) {
            int count = static_cast<int>(chunk.size());
            output->Add(count);
            std::cout << "  -> Chunk count: " << count << std::endl;
        }
        
        return output;
    }
    
private:
    std::string name_;
};

// ============================================================================
// 示例 1: 基础流式处理
// ============================================================================

void Example1_BasicStreamProcessing() {
    PrintSeparator("Example 1: Basic Stream Processing");
    
    std::cout << "\n[Description]" << std::endl;
    std::cout << "使用单个节点进行流式处理：文本 -> 分词流" << std::endl;
    
    auto tokenizer = std::make_shared<TokenizerNode>("Tokenizer");
    auto ctx = Context::Background();
    
    std::string input = "Hello World from Eino Compose Stream Mode";
    std::cout << "\n[Input] " << input << std::endl;
    
    std::cout << "\n[Processing Stream]" << std::endl;
    auto stream = tokenizer->Stream(ctx, input);
    
    std::cout << "\n[Reading Stream Output]" << std::endl;
    std::vector<std::string> chunk;
    int count = 1;
    while (stream && stream->Read(chunk)) {
        std::cout << "Chunk " << count++ << ": [";
        for (size_t i = 0; i < chunk.size(); ++i) {
            std::cout << chunk[i];
            if (i < chunk.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }
}

// ============================================================================
// 示例 2: Graph 流式处理管道
// ============================================================================

void Example2_StreamPipeline() {
    PrintSeparator("Example 2: Stream Pipeline with Graph");
    
    std::cout << "\n[Description]" << std::endl;
    std::cout << "构建流式处理管道：输入 -> 分词 -> 转大写 -> 计数" << std::endl;
    
    // 创建 Graph
    auto graph = std::make_shared<Graph<std::string, int>>();
    
    // 创建节点
    auto tokenizer = std::make_shared<TokenizerNode>("Tokenizer");
    auto uppercase = std::make_shared<UppercaseNode>("Uppercase");
    auto counter = std::make_shared<CounterNode>("Counter");
    
    // 添加节点到 Graph
    graph->AddNode("tokenizer", tokenizer);
    graph->AddNode("uppercase", uppercase);
    graph->AddNode("counter", counter);
    
    // 连接边：START -> tokenizer -> uppercase -> counter -> END
    graph->AddEdge(Graph<std::string, int>::START_NODE, "tokenizer");
    graph->AddEdge("tokenizer", "uppercase");
    graph->AddEdge("uppercase", "counter");
    graph->AddEdge("counter", Graph<std::string, int>::END_NODE);
    
    // 编译 Graph
    std::cout << "\n[Graph Compilation]" << std::endl;
    graph->Compile();
    std::cout << "Graph compiled successfully!" << std::endl;
    std::cout << "Nodes: " << graph->GetNodeNames().size() << std::endl;
    std::cout << "Edges: " << graph->GetEdgeCount() << std::endl;
    
    // 创建输入流
    std::cout << "\n[Creating Input Stream]" << std::endl;
    auto input_stream = std::make_shared<SimpleStreamReader<std::string>>();
    input_stream->Add("hello world");
    input_stream->Add("eino compose stream");
    input_stream->Add("is powerful");
    
    std::cout << "Input stream contains 3 chunks" << std::endl;
    
    // 执行流式处理
    std::cout << "\n[Executing Stream Pipeline]" << std::endl;
    auto ctx = Context::Background();
    auto output_stream = graph->Transform(ctx, input_stream);
    
    // 读取流式输出
    std::cout << "\n[Stream Output]" << std::endl;
    int result;
    int chunk_num = 1;
    while (output_stream && output_stream->Read(result)) {
        std::cout << "Output chunk " << chunk_num++ << ": " << result << " words" << std::endl;
    }
}

// ============================================================================
// 示例 3: 对比 Invoke vs Transform
// ============================================================================

void Example3_InvokeVsTransform() {
    PrintSeparator("Example 3: Invoke vs Transform Comparison");
    
    // 创建相同的 Graph
    auto graph = std::make_shared<Graph<std::string, int>>();
    
    auto tokenizer = std::make_shared<TokenizerNode>("Tokenizer");
    auto uppercase = std::make_shared<UppercaseNode>("Uppercase");
    auto counter = std::make_shared<CounterNode>("Counter");
    
    graph->AddNode("tokenizer", tokenizer);
    graph->AddNode("uppercase", uppercase);
    graph->AddNode("counter", counter);
    
    graph->AddEdge(Graph<std::string, int>::START_NODE, "tokenizer");
    graph->AddEdge("tokenizer", "uppercase");
    graph->AddEdge("uppercase", "counter");
    graph->AddEdge("counter", Graph<std::string, int>::END_NODE);
    
    graph->Compile();
    
    auto ctx = Context::Background();
    std::string input = "hello world from eino";
    
    // 方式 1: Invoke（非流式）
    std::cout << "\n[Mode 1: Invoke (Non-streaming)]" << std::endl;
    std::cout << "Input: \"" << input << "\"" << std::endl;
    
    auto start1 = std::chrono::high_resolution_clock::now();
    int result1 = graph->Invoke(ctx, input);
    auto end1 = std::chrono::high_resolution_clock::now();
    
    std::cout << "Result: " << result1 << " words" << std::endl;
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count() << "ms" << std::endl;
    
    // 方式 2: Transform（流式）
    std::cout << "\n[Mode 2: Transform (Streaming)]" << std::endl;
    auto input_stream = std::make_shared<SimpleStreamReader<std::string>>();
    input_stream->Add(input);
    
    auto start2 = std::chrono::high_resolution_clock::now();
    auto output_stream = graph->Transform(ctx, input_stream);
    
    int result2;
    if (output_stream && output_stream->Read(result2)) {
        auto end2 = std::chrono::high_resolution_clock::now();
        std::cout << "Result: " << result2 << " words" << std::endl;
        std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count() << "ms" << std::endl;
    }
    
    std::cout << "\n[Summary]" << std::endl;
    std::cout << "Invoke:    适用于批量处理，一次性获得完整结果" << std::endl;
    std::cout << "Transform: 适用于流式处理，逐步产生结果，实时响应" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                  Eino C++ Compose Stream Mode Example                        ║" << std::endl;
    std::cout << "║                          流式处理模式完整示例                                 ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        // 运行所有示例
        Example1_BasicStreamProcessing();
        
        std::cout << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        Example2_StreamPipeline();
        
        std::cout << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        Example3_InvokeVsTransform();
        
        // 总结
        PrintSeparator("Execution Summary");
        std::cout << "\n✅ All stream processing examples completed successfully!" << std::endl;
        std::cout << "\n[Key Takeaways]" << std::endl;
        std::cout << "1. Stream 模式适合处理大数据流和实时场景" << std::endl;
        std::cout << "2. Transform 方法实现流式输入到流式输出的转换" << std::endl;
        std::cout << "3. Graph 可以构建复杂的流式处理管道" << std::endl;
        std::cout << "4. 每个节点都需要实现 Transform 方法支持流式处理" << std::endl;
        std::cout << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
