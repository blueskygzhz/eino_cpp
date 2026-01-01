/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Graph Output Stream Example - Graph 输出类型为 Stream 的示例
 * 展示 Graph<Input, StreamReader<Output>> 的用法
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

// ============================================================================
// 辅助函数
// ============================================================================

void PrintSeparator(const string& title) {
    cout << "\n" << string(70, '=') << endl;
    cout << title << endl;
    cout << string(70, '=') << endl;
}

// ============================================================================
// 示例 1: Graph<string, StreamReader<string>> - 基础示例
// ============================================================================

void Example1_BasicStreamOutputGraph() {
    PrintSeparator("Example 1: Graph<string, StreamReader<string>>");
    
    // 关键：Graph 的输出类型是 StreamReader<string>
    auto graph = make_shared<Graph<string, shared_ptr<StreamReader<string>>>>();
    
    // 节点 1: 文本处理器 - 输出 string
    auto text_processor = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return "[PROCESSED] " + input;
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto stream = make_shared<SimpleStreamReader<string>>();
            stream->Add("[PROCESSED] " + input);
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
    
    // 节点 2: 流式生成器 - 输出 StreamReader<string> ✅
    auto stream_generator = NewLambdaRunnable<string, shared_ptr<StreamReader<string>>>(
        // Invoke: 返回流
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            
            auto stream = make_shared<SimpleStreamReader<string>>();
            
            // 将输入拆分成多个块
            cout << "  [Generator] Creating stream chunks..." << endl;
            for (char c : input) {
                stream->Add(string(1, c));
            }
            stream->Add(" [END]");
            
            return stream;
        },
        // Stream: 返回流的流（这里直接返回 Invoke 的结果）
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<shared_ptr<StreamReader<string>>>> {
            
            auto meta_stream = make_shared<SimpleStreamReader<shared_ptr<StreamReader<string>>>>();
            
            auto stream = make_shared<SimpleStreamReader<string>>();
            for (char c : input) {
                stream->Add(string(1, c));
            }
            stream->Add(" [END]");
            
            meta_stream->Add(stream);
            return meta_stream;
        },
        // Collect
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>();
            string c; while (input && input->Read(c)) o->Add(c); 
            return o;
        },
        // Transform
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<shared_ptr<StreamReader<string>>>> {
            auto meta = make_shared<SimpleStreamReader<shared_ptr<StreamReader<string>>>>();
            auto o = make_shared<SimpleStreamReader<string>>();
            string c; while (input && input->Read(c)) o->Add(c);
            meta->Add(o);
            return meta;
        }
    );
    
    // 构建 Graph
    graph->AddNode("processor", text_processor);
    graph->AddNode("generator", stream_generator);
    
    graph->AddEdge(Graph<string, shared_ptr<StreamReader<string>>>::START_NODE, "processor");
    graph->AddEdge("processor", "generator");
    graph->AddEdge("generator", Graph<string, shared_ptr<StreamReader<string>>>::END_NODE);
    
    graph->Compile();
    
    auto ctx = Context::Background();
    
    cout << "\n[Graph Type]" << endl;
    cout << "Graph<string, shared_ptr<StreamReader<string>>>" << endl;
    
    cout << "\n[Graph Structure]" << endl;
    cout << "START -> processor -> generator -> END" << endl;
    cout << "         (string)     (StreamReader<string>)" << endl;
    
    cout << "\n[Test: Invoke Mode]" << endl;
    auto result_stream = graph->Invoke(ctx, "Hello");
    
    cout << "Result type: StreamReader<string>" << endl;
    cout << "Reading stream: ";
    string chunk;
    while (result_stream && result_stream->Read(chunk)) {
        cout << chunk << flush;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    cout << endl;
}

// ============================================================================
// 示例 2: LLM 生成器 - 输出流式响应
// ============================================================================

void Example2_LLMGeneratorGraph() {
    PrintSeparator("Example 2: LLM Generator - Output Stream");
    
    // Graph 输出类型是 StreamReader<string>
    auto graph = make_shared<Graph<string, shared_ptr<StreamReader<string>>>>();
    
    // 节点 1: Prompt 模板
    auto prompt_builder = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return "User: " + input + "\nAssistant: ";
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto s = make_shared<SimpleStreamReader<string>>();
            s->Add("User: " + input + "\nAssistant: ");
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
    
    // 节点 2: LLM 调用 - 返回 StreamReader<string> ✅
    auto llm_call = NewLambdaRunnable<string, shared_ptr<StreamReader<string>>>(
        // Invoke: 返回流式响应
        [](shared_ptr<Context> ctx, const string& prompt, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            
            cout << "  [LLM] Generating stream response..." << endl;
            auto stream = make_shared<SimpleStreamReader<string>>();
            
            // 模拟 LLM token-by-token 生成
            vector<string> tokens = {
                "I", " ", "understand", " ", "your", " ", "question", ".", " ",
                "Let", " ", "me", " ", "help", " ", "you", " ", "with", " ", "that", "."
            };
            
            for (const auto& token : tokens) {
                stream->Add(token);
            }
            
            return stream;
        },
        // Stream
        [](shared_ptr<Context> ctx, const string& prompt, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<shared_ptr<StreamReader<string>>>> {
            auto meta = make_shared<SimpleStreamReader<shared_ptr<StreamReader<string>>>>();
            
            auto stream = make_shared<SimpleStreamReader<string>>();
            vector<string> tokens = {
                "I", " ", "understand", " ", "your", " ", "question", ".", " ",
                "Let", " ", "me", " ", "help", " ", "you", " ", "with", " ", "that", "."
            };
            for (const auto& token : tokens) stream->Add(token);
            
            meta->Add(stream);
            return meta;
        },
        // Collect
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>();
            string c; while (input && input->Read(c)) o->Add(c);
            return o;
        },
        // Transform
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<shared_ptr<StreamReader<string>>>> {
            auto meta = make_shared<SimpleStreamReader<shared_ptr<StreamReader<string>>>>();
            auto o = make_shared<SimpleStreamReader<string>>();
            string c; while (input && input->Read(c)) o->Add(c);
            meta->Add(o);
            return meta;
        }
    );
    
    // 构建 Graph
    graph->AddNode("prompt", prompt_builder);
    graph->AddNode("llm", llm_call);
    
    graph->AddEdge(Graph<string, shared_ptr<StreamReader<string>>>::START_NODE, "prompt");
    graph->AddEdge("prompt", "llm");
    graph->AddEdge("llm", Graph<string, shared_ptr<StreamReader<string>>>::END_NODE);
    
    graph->Compile();
    
    auto ctx = Context::Background();
    
    cout << "\n[Graph Type]" << endl;
    cout << "Graph<string, shared_ptr<StreamReader<string>>>" << endl;
    
    cout << "\n[User Query]" << endl;
    cout << "Q: What is AI?" << endl;
    
    cout << "\n[LLM Response - Streaming]" << endl;
    cout << "A: ";
    
    auto response_stream = graph->Invoke(ctx, "What is AI?");
    
    string token;
    while (response_stream && response_stream->Read(token)) {
        cout << token << flush;
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    cout << "\n\n[Stream completed]" << endl;
}

// ============================================================================
// 示例 3: 数据流生成器
// ============================================================================

void Example3_DataStreamGenerator() {
    PrintSeparator("Example 3: Data Stream Generator");
    
    // Graph<int, StreamReader<string>> - 输入数字，输出字符串流
    auto graph = make_shared<Graph<int, shared_ptr<StreamReader<string>>>>();
    
    // 节点 1: 数字处理
    auto number_processor = NewLambdaRunnable<int, int>(
        [](shared_ptr<Context> ctx, const int& input, const vector<Option>& opts) -> int {
            cout << "  [Processor] Received: " << input << endl;
            return input * 2;
        },
        [](shared_ptr<Context> ctx, const int& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<int>> {
            auto s = make_shared<SimpleStreamReader<int>>();
            s->Add(input * 2);
            return s;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<int>> input, const vector<Option>& opts) -> int {
            int r = 0, c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<int>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<int>> {
            auto o = make_shared<SimpleStreamReader<int>>();
            int c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 节点 2: 流式生成器 - 生成倒计数流 ✅
    auto countdown_generator = NewLambdaRunnable<int, shared_ptr<StreamReader<string>>>(
        // Invoke: 返回倒计数流
        [](shared_ptr<Context> ctx, const int& num, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            
            cout << "  [Generator] Creating countdown stream from " << num << endl;
            auto stream = make_shared<SimpleStreamReader<string>>();
            
            for (int i = num; i >= 0; --i) {
                stream->Add(to_string(i));
                if (i > 0) stream->Add(", ");
            }
            stream->Add(" [Blast off!]");
            
            return stream;
        },
        // Stream
        [](shared_ptr<Context> ctx, const int& num, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<shared_ptr<StreamReader<string>>>> {
            auto meta = make_shared<SimpleStreamReader<shared_ptr<StreamReader<string>>>>();
            
            auto stream = make_shared<SimpleStreamReader<string>>();
            for (int i = num; i >= 0; --i) {
                stream->Add(to_string(i));
                if (i > 0) stream->Add(", ");
            }
            stream->Add(" [Blast off!]");
            
            meta->Add(stream);
            return meta;
        },
        // Collect
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<int>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>();
            int c; 
            while (input && input->Read(c)) {
                o->Add(to_string(c));
            }
            return o;
        },
        // Transform
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<int>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<shared_ptr<StreamReader<string>>>> {
            auto meta = make_shared<SimpleStreamReader<shared_ptr<StreamReader<string>>>>();
            auto o = make_shared<SimpleStreamReader<string>>();
            int c;
            while (input && input->Read(c)) {
                o->Add(to_string(c));
            }
            meta->Add(o);
            return meta;
        }
    );
    
    // 构建 Graph
    graph->AddNode("processor", number_processor);
    graph->AddNode("generator", countdown_generator);
    
    graph->AddEdge(Graph<int, shared_ptr<StreamReader<string>>>::START_NODE, "processor");
    graph->AddEdge("processor", "generator");
    graph->AddEdge("generator", Graph<int, shared_ptr<StreamReader<string>>>::END_NODE);
    
    graph->Compile();
    
    auto ctx = Context::Background();
    
    cout << "\n[Graph Type]" << endl;
    cout << "Graph<int, shared_ptr<StreamReader<string>>>" << endl;
    
    cout << "\n[Test: Input = 5]" << endl;
    auto countdown_stream = graph->Invoke(ctx, 5);
    
    cout << "Countdown: ";
    string chunk;
    while (countdown_stream && countdown_stream->Read(chunk)) {
        cout << chunk << flush;
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    cout << endl;
}

// ============================================================================
// 示例 4: 复杂类型 - 输出结构化数据流
// ============================================================================

struct DataChunk {
    int id;
    string content;
    
    string ToString() const {
        return "[" + to_string(id) + ": " + content + "]";
    }
};

void Example4_StructuredDataStream() {
    PrintSeparator("Example 4: Structured Data Stream Output");
    
    // Graph<string, StreamReader<DataChunk>>
    auto graph = make_shared<Graph<string, shared_ptr<StreamReader<DataChunk>>>>();
    
    // 节点: 数据流生成器
    auto data_generator = NewLambdaRunnable<string, shared_ptr<StreamReader<DataChunk>>>(
        // Invoke: 返回结构化数据流
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<DataChunk>> {
            
            cout << "  [Generator] Creating data chunks for: " << input << endl;
            auto stream = make_shared<SimpleStreamReader<DataChunk>>();
            
            // 生成多个数据块
            for (int i = 1; i <= 5; ++i) {
                DataChunk chunk;
                chunk.id = i;
                chunk.content = input + " - Part " + to_string(i);
                stream->Add(chunk);
            }
            
            return stream;
        },
        // Stream
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<shared_ptr<StreamReader<DataChunk>>>> {
            auto meta = make_shared<SimpleStreamReader<shared_ptr<StreamReader<DataChunk>>>>();
            
            auto stream = make_shared<SimpleStreamReader<DataChunk>>();
            for (int i = 1; i <= 5; ++i) {
                DataChunk chunk;
                chunk.id = i;
                chunk.content = input + " - Part " + to_string(i);
                stream->Add(chunk);
            }
            
            meta->Add(stream);
            return meta;
        },
        // Collect
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<DataChunk>> {
            auto o = make_shared<SimpleStreamReader<DataChunk>>();
            string c;
            int id = 1;
            while (input && input->Read(c)) {
                DataChunk chunk;
                chunk.id = id++;
                chunk.content = c;
                o->Add(chunk);
            }
            return o;
        },
        // Transform
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<shared_ptr<StreamReader<DataChunk>>>> {
            auto meta = make_shared<SimpleStreamReader<shared_ptr<StreamReader<DataChunk>>>>();
            auto o = make_shared<SimpleStreamReader<DataChunk>>();
            string c;
            int id = 1;
            while (input && input->Read(c)) {
                DataChunk chunk;
                chunk.id = id++;
                chunk.content = c;
                o->Add(chunk);
            }
            meta->Add(o);
            return meta;
        }
    );
    
    // 构建 Graph（单节点）
    graph->AddNode("generator", data_generator);
    graph->AddEdge(Graph<string, shared_ptr<StreamReader<DataChunk>>>::START_NODE, "generator");
    graph->AddEdge("generator", Graph<string, shared_ptr<StreamReader<DataChunk>>>::END_NODE);
    graph->Compile();
    
    auto ctx = Context::Background();
    
    cout << "\n[Graph Type]" << endl;
    cout << "Graph<string, shared_ptr<StreamReader<DataChunk>>>" << endl;
    
    cout << "\n[Test: Input = \"Document\"]" << endl;
    auto data_stream = graph->Invoke(ctx, "Document");
    
    cout << "Data chunks:" << endl;
    DataChunk chunk;
    while (data_stream && data_stream->Read(chunk)) {
        cout << "  " << chunk.ToString() << endl;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    cout << "\n";
    cout << "╔═══════════════════════════════════════════════════════════════╗" << endl;
    cout << "║        Eino C++ Compose - Graph Output Stream Example        ║" << endl;
    cout << "║           Graph<Input, StreamReader<Output>> 示例             ║" << endl;
    cout << "╚═══════════════════════════════════════════════════════════════╝" << endl;
    
    try {
        Example1_BasicStreamOutputGraph();
        Example2_LLMGeneratorGraph();
        Example3_DataStreamGenerator();
        Example4_StructuredDataStream();
        
        PrintSeparator("Summary");
        cout << "\n✅ All Graph<Input, StreamReader<Output>> examples completed!" << endl;
        cout << "\n[Key Points]" << endl;
        cout << "• Graph 的输出类型可以是 StreamReader<T>" << endl;
        cout << "• 最后一个节点必须返回 StreamReader 类型" << endl;
        cout << "• Invoke() 返回 StreamReader，可以逐块读取" << endl;
        cout << "• 适用于 LLM 生成、数据流处理等场景" << endl;
        cout << "• 支持任意类型的流式输出" << endl;
        cout << endl;
        
        return 0;
        
    } catch (const exception& e) {
        cerr << "\n❌ Error: " << e.what() << endl;
        return 1;
    }
}
