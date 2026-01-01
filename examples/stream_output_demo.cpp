/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Stream Output Demo - 展示真正的流式输出
 * 模拟 LLM 逐字生成的场景
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "eino/compose/graph.h"
#include "eino/compose/runnable.h"

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
// 示例 1: 真正的 Stream 输出 - 将字符串逐字符流式输出
// ============================================================================

void Example1_CharacterByCharacterStream() {
    PrintSeparator("Example 1: Character-by-Character Stream Output");
    
    // 创建一个 LambdaRunnable，实现 Invoke 和 Stream 方法
    auto text_generator = NewLambdaRunnable<std::string, std::string>(
        // Invoke: 非流式，直接返回完整结果
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) -> std::string {
            std::cout << "[Invoke] Processing: " << input << std::endl;
            return "[COMPLETE] " + input;
        },
        
        // Stream: 流式输出，逐字符返回
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            
            std::cout << "[Stream] Starting to stream: \"" << input << "\"" << std::endl;
            
            auto stream = std::make_shared<SimpleStreamReader<std::string>>();
            
            // 将输入字符串拆分成单个字符并添加到流中
            for (char c : input) {
                stream->Add(std::string(1, c));
            }
            
            // 添加结束标记
            stream->Add(" [END]");
            
            return stream;
        },
        
        // Collect: 流式输入 -> 非流式输出
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) -> std::string {
            std::string result;
            std::string chunk;
            while (input && input->Read(chunk)) {
                result += chunk;
            }
            return result;
        },
        
        // Transform: 流式输入 -> 流式输出
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            auto output = std::make_shared<SimpleStreamReader<std::string>>();
            std::string chunk;
            while (input && input->Read(chunk)) {
                output->Add(chunk);
            }
            return output;
        }
    );
    
    auto ctx = Context::Background();
    
    // 测试 1: Invoke 模式（非流式）
    std::cout << "\n[Test 1: Invoke Mode - Non-streaming]" << std::endl;
    std::string result = text_generator->Invoke(ctx, "Hello World");
    std::cout << "Result: " << result << std::endl;
    
    // 测试 2: Stream 模式（流式输出）
    std::cout << "\n[Test 2: Stream Mode - Character by character]" << std::endl;
    auto stream = text_generator->Stream(ctx, "Hello World");
    
    std::cout << "Stream output: ";
    std::string chunk;
    while (stream && stream->Read(chunk)) {
        std::cout << chunk << std::flush;  // 实时输出每个字符
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 模拟流式延迟
    }
    std::cout << std::endl;
}

// ============================================================================
// 示例 2: 模拟 LLM 流式生成 - 逐词输出
// ============================================================================

void Example2_WordByWordStream() {
    PrintSeparator("Example 2: Simulating LLM Token-by-Token Generation");
    
    auto llm_simulator = NewLambdaRunnable<std::string, std::string>(
        // Invoke: 返回完整响应
        [](std::shared_ptr<Context> ctx, const std::string& prompt, const std::vector<Option>& opts) -> std::string {
            return "This is a complete response to: " + prompt;
        },
        
        // Stream: 模拟 LLM 逐词生成
        [](std::shared_ptr<Context> ctx, const std::string& prompt, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            
            auto stream = std::make_shared<SimpleStreamReader<std::string>>();
            
            // 模拟 LLM 生成的响应，逐词添加
            std::vector<std::string> tokens = {
                "Sure", ", ", "I", " ", "can", " ", "help", " ", "you", " ",
                "with", " ", "that", ". ", "Let", " ", "me", " ", "process", " ",
                "your", " ", "request", ": ", "\"", prompt, "\""
            };
            
            for (const auto& token : tokens) {
                stream->Add(token);
            }
            
            return stream;
        },
        
        // Collect
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) -> std::string {
            std::string result;
            std::string chunk;
            while (input && input->Read(chunk)) result += chunk;
            return result;
        },
        
        // Transform
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            auto output = std::make_shared<SimpleStreamReader<std::string>>();
            std::string chunk;
            while (input && input->Read(chunk)) output->Add(chunk);
            return output;
        }
    );
    
    auto ctx = Context::Background();
    
    std::cout << "\n[Simulating LLM Stream Response]" << std::endl;
    std::cout << "Prompt: \"What is the weather today?\"" << std::endl;
    std::cout << "\nStreaming response:\n> ";
    
    auto stream = llm_simulator->Stream(ctx, "What is the weather today?");
    std::string token;
    while (stream && stream->Read(token)) {
        std::cout << token << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // 模拟生成延迟
    }
    std::cout << "\n\n[Stream completed]" << std::endl;
}

// ============================================================================
// 示例 3: Graph 中的流式输出
// ============================================================================

void Example3_GraphStreamOutput() {
    PrintSeparator("Example 3: Stream Output in Graph Pipeline");
    
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // 节点 1: 预处理 - 添加提示词前缀
    auto preprocessor = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) -> std::string {
            std::cout << "  [Preprocessor] Adding prompt prefix" << std::endl;
            return "PROMPT: " + input;
        },
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            auto stream = std::make_shared<SimpleStreamReader<std::string>>();
            stream->Add("PROMPT: " + input);
            return stream;
        },
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) -> std::string {
            std::string result; std::string chunk;
            while (input && input->Read(chunk)) result += chunk;
            return result;
        },
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            auto output = std::make_shared<SimpleStreamReader<std::string>>();
            std::string chunk;
            while (input && input->Read(chunk)) output->Add(chunk);
            return output;
        }
    );
    
    // 节点 2: 生成器 - 流式生成响应
    auto generator = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) -> std::string {
            return "Generated response for: " + input;
        },
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            
            std::cout << "  [Generator] Streaming response..." << std::endl;
            auto stream = std::make_shared<SimpleStreamReader<std::string>>();
            
            // 模拟分段生成
            std::vector<std::string> chunks = {
                "Analyzing", " your", " input", "...\n",
                "Response", ": ", "Processing", " complete", "!"
            };
            
            for (const auto& chunk : chunks) {
                stream->Add(chunk);
            }
            
            return stream;
        },
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) -> std::string {
            std::string result; std::string chunk;
            while (input && input->Read(chunk)) result += chunk;
            return result;
        },
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            auto output = std::make_shared<SimpleStreamReader<std::string>>();
            std::string chunk;
            while (input && input->Read(chunk)) output->Add(chunk);
            return output;
        }
    );
    
    // 构建 Graph
    graph->AddNode("preprocess", preprocessor);
    graph->AddNode("generate", generator);
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "preprocess");
    graph->AddEdge("preprocess", "generate");
    graph->AddEdge("generate", Graph<std::string, std::string>::END_NODE);
    graph->Compile();
    
    auto ctx = Context::Background();
    
    // 测试流式输出
    std::cout << "\n[Test: Graph Stream Mode]" << std::endl;
    std::cout << "Input: \"Tell me a story\"" << std::endl;
    std::cout << "\nStream output:\n> ";
    
    auto stream = graph->Stream(ctx, "Tell me a story");
    std::string chunk;
    while (stream && stream->Read(chunk)) {
        std::cout << chunk << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
    std::cout << "\n\n[Graph stream completed]" << std::endl;
}

// ============================================================================
// 示例 4: 多节点流式处理链
// ============================================================================

void Example4_MultiNodeStreamChain() {
    PrintSeparator("Example 4: Multi-Node Stream Processing Chain");
    
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // 节点 1: 输入分析
    auto analyzer = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) -> std::string { 
            return input; 
        },
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            auto stream = std::make_shared<SimpleStreamReader<std::string>>();
            stream->Add("[ANALYZING] ");
            for (char c : input) {
                stream->Add(std::string(1, c));
            }
            stream->Add(" ");
            return stream;
        },
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) -> std::string {
            std::string result; std::string chunk;
            while (input && input->Read(chunk)) result += chunk;
            return result;
        },
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            auto output = std::make_shared<SimpleStreamReader<std::string>>();
            std::string chunk;
            while (input && input->Read(chunk)) output->Add(chunk);
            return output;
        }
    );
    
    // 节点 2: 处理器
    auto processor = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) -> std::string { 
            return input; 
        },
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            auto stream = std::make_shared<SimpleStreamReader<std::string>>();
            stream->Add("-> [PROCESSING] ");
            return stream;
        },
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) -> std::string {
            std::string result; std::string chunk;
            while (input && input->Read(chunk)) result += chunk;
            return result;
        },
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            auto output = std::make_shared<SimpleStreamReader<std::string>>();
            std::string chunk;
            while (input && input->Read(chunk)) output->Add(chunk);
            return output;
        }
    );
    
    // 节点 3: 输出格式化
    auto formatter = NewLambdaRunnable<std::string, std::string>(
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) -> std::string { 
            return input; 
        },
        [](std::shared_ptr<Context> ctx, const std::string& input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            auto stream = std::make_shared<SimpleStreamReader<std::string>>();
            stream->Add("-> [DONE]");
            return stream;
        },
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) -> std::string {
            std::string result; std::string chunk;
            while (input && input->Read(chunk)) result += chunk;
            return result;
        },
        [](std::shared_ptr<Context> ctx, std::shared_ptr<StreamReader<std::string>> input, const std::vector<Option>& opts) 
            -> std::shared_ptr<StreamReader<std::string>> {
            auto output = std::make_shared<SimpleStreamReader<std::string>>();
            std::string chunk;
            while (input && input->Read(chunk)) output->Add(chunk);
            return output;
        }
    );
    
    // 构建流式处理链
    graph->AddNode("analyzer", analyzer);
    graph->AddNode("processor", processor);
    graph->AddNode("formatter", formatter);
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "analyzer");
    graph->AddEdge("analyzer", "processor");
    graph->AddEdge("processor", "formatter");
    graph->AddEdge("formatter", Graph<std::string, std::string>::END_NODE);
    graph->Compile();
    
    auto ctx = Context::Background();
    
    std::cout << "\n[Stream Chain Processing]" << std::endl;
    std::cout << "Input: \"TEST\"" << std::endl;
    std::cout << "\nStream flow:\n";
    
    auto stream = graph->Stream(ctx, "TEST");
    std::string chunk;
    while (stream && stream->Read(chunk)) {
        std::cout << chunk << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║          Eino C++ Compose - Stream Output Demo               ║" << std::endl;
    std::cout << "║              真正的流式输出示例                                ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        Example1_CharacterByCharacterStream();
        Example2_WordByWordStream();
        Example3_GraphStreamOutput();
        Example4_MultiNodeStreamChain();
        
        PrintSeparator("Summary");
        std::cout << "\n✅ All stream output examples completed!" << std::endl;
        std::cout << "\n[Key Points]" << std::endl;
        std::cout << "• NewLambdaRunnable 可以同时提供 Invoke 和 Stream 实现" << std::endl;
        std::cout << "• Stream 方法返回 StreamReader<O>，支持逐块读取" << std::endl;
        std::cout << "• 适合模拟 LLM token-by-token 生成场景" << std::endl;
        std::cout << "• Graph 的 Stream 方法会串联所有节点的流式输出" << std::endl;
        std::cout << "• 使用 SimpleStreamReader 作为流式数据容器" << std::endl;
        std::cout << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
