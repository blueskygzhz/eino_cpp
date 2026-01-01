/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Graph Output Stream Simple Example
 * 展示 Graph 输出类型为 StreamReader 的简化示例
 * 
 * 关键：Graph<Input, Output> 中 Output 就是普通类型（如 string）
 * 但节点可以返回 StreamReader，Graph 会自动处理
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
using namespace std;

void PrintSeparator(const string& title) {
    cout << "\n" << string(70, '=') << endl;
    cout << title << endl;
    cout << string(70, '=') << endl;
}

// ============================================================================
// 核心示例：Graph<string, string> 但使用 Stream 模式调用
// ============================================================================

void Example1_GraphStreamMode() {
    PrintSeparator("Example 1: Graph Stream Mode - The Right Way");
    
    // Graph 类型定义：Graph<Input, Output>
    // Input: string, Output: string
    auto graph = make_shared<Graph<string, string>>();
    
    cout << "\n[Graph Type]" << endl;
    cout << "Graph<string, string>  ← 输出类型是 string，不是 StreamReader" << endl;
    
    // 节点 1: 预处理
    auto preprocessor = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return "[INPUT] " + input;
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto stream = make_shared<SimpleStreamReader<string>>();
            stream->Add("[INPUT] ");
            for (char c : input) {
                stream->Add(string(1, c));
            }
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r, c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>();
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 节点 2: LLM 生成器
    auto llm_generator = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return input + " [Generated response]";
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            
            cout << "  [LLM] Generating tokens..." << endl;
            auto stream = make_shared<SimpleStreamReader<string>>();
            
            // 模拟 token-by-token 生成
            vector<string> tokens = {
                "Sure", ", ", "I", " ", "can", " ", "help", " ", "you", " ",
                "with", " ", "that", ".", " ", "Let", " ", "me", " ",
                "process", " ", "your", " ", "request", "."
            };
            
            for (const auto& token : tokens) {
                stream->Add(token);
            }
            
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r, c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>();
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 构建 Graph
    graph->AddNode("preprocess", preprocessor);
    graph->AddNode("llm", llm_generator);
    
    graph->AddEdge(Graph<string, string>::START_NODE, "preprocess");
    graph->AddEdge("preprocess", "llm");
    graph->AddEdge("llm", Graph<string, string>::END_NODE);
    
    graph->Compile();
    
    auto ctx = Context::Background();
    
    cout << "\n[Graph Structure]" << endl;
    cout << "START -> preprocess -> llm -> END" << endl;
    
    // 方式 1: Invoke 模式 - 返回完整字符串
    cout << "\n[Test 1: Invoke Mode]" << endl;
    cout << "Result type: string" << endl;
    string result = graph->Invoke(ctx, "Hello");
    cout << "Result: " << result << endl;
    
    // 方式 2: Stream 模式 - 返回 StreamReader<string> ✅
    cout << "\n[Test 2: Stream Mode]" << endl;
    cout << "Result type: StreamReader<string>  ← 这才是流式输出！" << endl;
    cout << "\nStreaming output:\n> ";
    
    auto stream = graph->Stream(ctx, "Hello");
    string chunk;
    while (stream && stream->Read(chunk)) {
        cout << chunk << flush;
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    cout << "\n\n✅ 看到了流式输出效果！" << endl;
}

// ============================================================================
// 示例 2: Transform 模式 - 流式输入，流式输出
// ============================================================================

void Example2_GraphTransformMode() {
    PrintSeparator("Example 2: Graph Transform Mode");
    
    auto graph = make_shared<Graph<string, string>>();
    
    cout << "\n[Graph Type]" << endl;
    cout << "Graph<string, string>" << endl;
    
    // 创建一个简单的转大写节点
    auto to_upper = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            string result = input;
            for (char& c : result) c = toupper(c);
            return result;
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto stream = make_shared<SimpleStreamReader<string>>();
            for (char c : input) {
                stream->Add(string(1, toupper(c)));
            }
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r, c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>();
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    graph->AddNode("to_upper", to_upper);
    graph->AddEdge(Graph<string, string>::START_NODE, "to_upper");
    graph->AddEdge("to_upper", Graph<string, string>::END_NODE);
    graph->Compile();
    
    auto ctx = Context::Background();
    
    // Transform 模式：StreamReader<string> -> StreamReader<string>
    cout << "\n[Test: Transform Mode]" << endl;
    cout << "Input type: StreamReader<string>" << endl;
    cout << "Output type: StreamReader<string>" << endl;
    
    // 创建输入流
    auto input_stream = make_shared<SimpleStreamReader<string>>();
    input_stream->Add("hello");
    input_stream->Add(" ");
    input_stream->Add("world");
    
    cout << "\nInput stream: [hello] [world]" << endl;
    cout << "Processing...\n> ";
    
    // Transform 调用
    auto output_stream = graph->Transform(ctx, input_stream);
    
    string chunk;
    while (output_stream && output_stream->Read(chunk)) {
        cout << chunk << flush;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    cout << "\n\n✅ Transform 模式：流进流出！" << endl;
}

// ============================================================================
// 示例 3: 实际场景 - LLM 问答系统
// ============================================================================

void Example3_LLMQASystem() {
    PrintSeparator("Example 3: LLM Q&A System with Stream Output");
    
    auto graph = make_shared<Graph<string, string>>();
    
    cout << "\n[Scenario]" << endl;
    cout << "User asks a question -> LLM generates response (streaming)" << endl;
    
    // Prompt 模板节点
    auto prompt_template = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& query, const vector<Option>& opts) -> string {
            return "User: " + query + "\nAssistant: ";
        },
        [](shared_ptr<Context> ctx, const string& query, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto s = make_shared<SimpleStreamReader<string>>();
            s->Add("User: " + query + "\nAssistant: ");
            return s;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r, c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>();
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // LLM 节点（模拟流式生成）
    auto llm = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& prompt, const vector<Option>& opts) -> string {
            return "I understand your question. Let me help you with that.";
        },
        [](shared_ptr<Context> ctx, const string& prompt, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            
            auto stream = make_shared<SimpleStreamReader<string>>();
            
            // 模拟 GPT 风格的 token 生成
            vector<string> tokens = {
                "I", " ", "understand", " ", "your", " ", "question", ".", " ",
                "Based", " ", "on", " ", "my", " ", "knowledge", ",", " ",
                "here", "'", "s", " ", "what", " ", "I", " ", "can", " ", "tell", " ", "you", ":", " ",
                "The", " ", "answer", " ", "is", " ", "quite", " ", "interesting", "."
            };
            
            for (const auto& token : tokens) {
                stream->Add(token);
            }
            
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r, c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>();
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 构建 Graph
    graph->AddNode("prompt", prompt_template);
    graph->AddNode("llm", llm);
    graph->AddEdge(Graph<string, string>::START_NODE, "prompt");
    graph->AddEdge("prompt", "llm");
    graph->AddEdge("llm", Graph<string, string>::END_NODE);
    graph->Compile();
    
    auto ctx = Context::Background();
    
    cout << "\n[User Query]" << endl;
    cout << "Q: What is artificial intelligence?" << endl;
    
    cout << "\n[LLM Response - Streaming]" << endl;
    cout << "A: ";
    
    // 使用 Stream 模式获取流式响应
    auto response_stream = graph->Stream(ctx, "What is artificial intelligence?");
    
    string token;
    while (response_stream && response_stream->Read(token)) {
        cout << token << flush;
        this_thread::sleep_for(chrono::milliseconds(40));
    }
    
    cout << "\n\n✅ LLM 流式生成完成！" << endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    cout << "\n";
    cout << "╔═══════════════════════════════════════════════════════════════╗" << endl;
    cout << "║      Eino C++ Compose - Graph Output Stream (Simple)         ║" << endl;
    cout << "║         Graph 输出流式数据的正确方式                           ║" << endl;
    cout << "╚═══════════════════════════════════════════════════════════════╝" << endl;
    
    try {
        Example1_GraphStreamMode();
        Example2_GraphTransformMode();
        Example3_LLMQASystem();
        
        PrintSeparator("Summary");
        cout << "\n✅ All examples completed!" << endl;
        cout << "\n[核心要点]" << endl;
        cout << "• Graph<I, O> 的 O 是普通类型（如 string），不是 StreamReader" << endl;
        cout << "• 调用 graph->Stream(ctx, input) 返回 StreamReader<O> ✅" << endl;
        cout << "• 节点实现 Stream 方法，Graph 自动串联成流式输出" << endl;
        cout << "• Transform 模式支持流式输入和流式输出" << endl;
        cout << "• 完美适配 LLM token-by-token 生成场景" << endl;
        cout << "\n[关键区别]" << endl;
        cout << "• graph->Invoke() -> O (完整结果)" << endl;
        cout << "• graph->Stream() -> StreamReader<O> (流式结果) ✅" << endl;
        cout << endl;
        
        return 0;
        
    } catch (const exception& e) {
        cerr << "\n❌ Error: " << e.what() << endl;
        return 1;
    }
}
