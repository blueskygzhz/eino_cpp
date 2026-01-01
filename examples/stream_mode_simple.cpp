/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Stream Mode Simple Example - 简化的流式处理示例
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>

#include "eino/compose/graph.h"
#include "eino/compose/runnable.h"
#include "eino/compose/types_lambda.h"

using namespace eino::compose;

// ============================================================================
// 辅助函数
// ============================================================================

void PrintSeparator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(70, '=') << std::endl;
}

// ============================================================================
// 示例 1: 简单的流式处理
// ============================================================================

void Example1_BasicStream() {
    PrintSeparator("Example 1: Basic Stream with Lambda");
    
    // 创建一个简单的 Lambda 节点，将字符串转为大写
    auto to_upper = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) {
            std::string result = input;
            for (char& c : result) {
                c = std::toupper(c);
            }
            std::cout << "  Processing: \"" << input << "\" -> \"" << result << "\"" << std::endl;
            return result;
        }
    );
    
    auto ctx = Context::Background();
    
    // 方式 1: Invoke (非流式)
    std::cout << "\n[Mode 1: Invoke]" << std::endl;
    std::string result1 = to_upper->Invoke(ctx, "hello world");
    std::cout << "Result: " << result1 << std::endl;
    
    // 方式 2: Stream (流式输出)
    std::cout << "\n[Mode 2: Stream]" << std::endl;
    auto stream = to_upper->Stream(ctx, "hello stream");
    std::string chunk;
    while (stream && stream->Read(chunk)) {
        std::cout << "Stream chunk: " << chunk << std::endl;
    }
}

// ============================================================================
// 示例 2: Graph 流式处理管道
// ============================================================================

void Example2_StreamPipeline() {
    PrintSeparator("Example 2: Stream Pipeline with Graph");
    
    // 创建 Graph
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // 节点 1: 转大写
    auto to_upper = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) {
            std::string result = input;
            for (char& c : result) {
                c = std::toupper(c);
            }
            std::cout << "  [ToUpper] \"" << input << "\" -> \"" << result << "\"" << std::endl;
            return result;
        }
    );
    
    // 节点 2: 反转字符串
    auto reverse = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) {
            std::string result(input.rbegin(), input.rend());
            std::cout << "  [Reverse] \"" << input << "\" -> \"" << result << "\"" << std::endl;
            return result;
        }
    );
    
    // 构建 Graph: START -> to_upper -> reverse -> END
    graph->AddNode("to_upper", to_upper);
    graph->AddNode("reverse", reverse);
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "to_upper");
    graph->AddEdge("to_upper", "reverse");
    graph->AddEdge("reverse", Graph<std::string, std::string>::END_NODE);
    
    // 编译
    graph->Compile();
    std::cout << "\nGraph compiled with " << graph->GetNodeNames().size() << " nodes" << std::endl;
    
    auto ctx = Context::Background();
    
    // 测试 1: Invoke (非流式)
    std::cout << "\n[Test 1: Invoke Mode]" << std::endl;
    std::string result = graph->Invoke(ctx, "hello");
    std::cout << "Final result: " << result << std::endl;
    
    // 测试 2: Transform (流式)
    std::cout << "\n[Test 2: Transform Mode]" << std::endl;
    auto input_stream = std::make_shared<SimpleStreamReader<std::string>>();
    input_stream->Add("hello");
    input_stream->Add("world");
    input_stream->Add("stream");
    
    std::cout << "Processing stream with 3 items..." << std::endl;
    auto output_stream = graph->Transform(ctx, input_stream);
    
    std::string chunk;
    int count = 1;
    while (output_stream && output_stream->Read(chunk)) {
        std::cout << "  Output " << count++ << ": \"" << chunk << "\"" << std::endl;
    }
}

// ============================================================================
// 示例 3: 多输入流式处理
// ============================================================================

void Example3_MultipleInputs() {
    PrintSeparator("Example 3: Processing Multiple Stream Inputs");
    
    // 创建一个简单的处理管道
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // 添加前缀的节点
    auto add_prefix = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) {
            std::string result = "[PROCESSED] " + input;
            return result;
        }
    );
    
    graph->AddNode("add_prefix", add_prefix);
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "add_prefix");
    graph->AddEdge("add_prefix", Graph<std::string, std::string>::END_NODE);
    graph->Compile();
    
    // 创建输入流
    auto input_stream = std::make_shared<SimpleStreamReader<std::string>>();
    std::vector<std::string> inputs = {
        "First message",
        "Second message",
        "Third message",
        "Fourth message",
        "Fifth message"
    };
    
    for (const auto& msg : inputs) {
        input_stream->Add(msg);
    }
    
    std::cout << "\nProcessing " << inputs.size() << " messages through stream pipeline..." << std::endl;
    
    auto ctx = Context::Background();
    auto output_stream = graph->Transform(ctx, input_stream);
    
    std::cout << "\n[Stream Output]" << std::endl;
    std::string result;
    while (output_stream && output_stream->Read(result)) {
        std::cout << "  ✓ " << result << std::endl;
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║       Eino C++ Compose - Stream Mode Simple Example          ║" << std::endl;
    std::cout << "║                流式处理模式简化示例                            ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        Example1_BasicStream();
        Example2_StreamPipeline();
        Example3_MultipleInputs();
        
        PrintSeparator("Summary");
        std::cout << "\n✅ All examples completed successfully!" << std::endl;
        std::cout << "\n[Key Points]" << std::endl;
        std::cout << "• Stream 模式适合处理流式数据和实时响应" << std::endl;
        std::cout << "• Transform 方法实现流式输入到流式输出" << std::endl;
        std::cout << "• Graph 可以轻松构建流式处理管道" << std::endl;
        std::cout << "• Lambda 节点简化了节点创建过程" << std::endl;
        std::cout << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
