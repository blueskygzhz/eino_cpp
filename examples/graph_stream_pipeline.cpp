/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Graph Stream Pipeline Example - Graph 流式输出完整示例
 * 展示如何构建一个完整的流式处理管道
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>

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

void StreamDelay(int ms = 50) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}

// ============================================================================
// 示例 1: 简单的流式处理 Graph
// ============================================================================

void Example1_SimpleStreamGraph() {
    PrintSeparator("Example 1: Simple Stream Graph");
    
    auto graph = make_shared<Graph<string, string>>();
    
    // 节点 1: 预处理 - 添加标题
    auto preprocessor = NewLambdaRunnable<string, string>(
        // Invoke
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return "[INPUT] " + input;
        },
        // Stream - 逐字符输出
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            cout << "  [Preprocessor] Streaming input..." << endl;
            auto stream = make_shared<SimpleStreamReader<string>>();
            stream->Add("[INPUT] ");
            for (char c : input) {
                stream->Add(string(1, c));
            }
            return stream;
        },
        // Collect
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string result;
            string chunk;
            while (input && input->Read(chunk)) result += chunk;
            return result;
        },
        // Transform
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto output = make_shared<SimpleStreamReader<string>>();
            string chunk;
            while (input && input->Read(chunk)) output->Add(chunk);
            return output;
        }
    );
    
    // 节点 2: 转大写
    auto upper_converter = NewLambdaRunnable<string, string>(
        // Invoke
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            string result = input;
            for (char& c : result) c = toupper(c);
            return result;
        },
        // Stream
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            cout << "  [UpperConverter] Converting to uppercase..." << endl;
            auto stream = make_shared<SimpleStreamReader<string>>();
            for (char c : input) {
                stream->Add(string(1, toupper(c)));
            }
            return stream;
        },
        // Collect
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string result;
            string chunk;
            while (input && input->Read(chunk)) result += chunk;
            return result;
        },
        // Transform
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto output = make_shared<SimpleStreamReader<string>>();
            string chunk;
            while (input && input->Read(chunk)) output->Add(chunk);
            return output;
        }
    );
    
    // 节点 3: 后处理 - 添加结束标记
    auto postprocessor = NewLambdaRunnable<string, string>(
        // Invoke
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return input + " [DONE]";
        },
        // Stream
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            cout << "  [Postprocessor] Adding completion marker..." << endl;
            auto stream = make_shared<SimpleStreamReader<string>>();
            for (char c : input) {
                stream->Add(string(1, c));
            }
            stream->Add(" [DONE]");
            return stream;
        },
        // Collect
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string result;
            string chunk;
            while (input && input->Read(chunk)) result += chunk;
            return result;
        },
        // Transform
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) 
            -> shared_ptr<StreamReader<string>> {
            auto output = make_shared<SimpleStreamReader<string>>();
            string chunk;
            while (input && input->Read(chunk)) output->Add(chunk);
            return output;
        }
    );
    
    // 构建 Graph
    graph->AddNode("preprocess", preprocessor);
    graph->AddNode("to_upper", upper_converter);
    graph->AddNode("postprocess", postprocessor);
    
    graph->AddEdge(Graph<string, string>::START_NODE, "preprocess");
    graph->AddEdge("preprocess", "to_upper");
    graph->AddEdge("to_upper", "postprocess");
    graph->AddEdge("postprocess", Graph<string, string>::END_NODE);
    
    graph->Compile();
    
    auto ctx = Context::Background();
    
    cout << "\n[Graph Structure]" << endl;
    cout << "START -> preprocess -> to_upper -> postprocess -> END" << endl;
    
    cout << "\n[Test 1: Invoke Mode]" << endl;
    string result = graph->Invoke(ctx, "hello world");
    cout << "Result: " << result << endl;
    
    cout << "\n[Test 2: Stream Mode]" << endl;
    cout << "Streaming output:\n> ";
    auto stream = graph->Stream(ctx, "hello world");
    string chunk;
    while (stream && stream->Read(chunk)) {
        cout << chunk << flush;
        StreamDelay(50);
    }
    cout << endl;
}

// ============================================================================
// 示例 2: 模拟 LLM 问答系统的流式 Graph
// ============================================================================

void Example2_LLMQAStreamGraph() {
    PrintSeparator("Example 2: LLM Q&A Stream Pipeline");
    
    auto graph = make_shared<Graph<string, string>>();
    
    // 节点 1: Query 分析器
    auto query_analyzer = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return "[QUERY: " + input + "]";
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto stream = make_shared<SimpleStreamReader<string>>();
            stream->Add("[ANALYZING QUERY] ");
            stream->Add(input);
            stream->Add("\n");
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r; string c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>(); 
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 节点 2: 检索相关内容
    auto retriever = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return input + "\n[Retrieved: Context about the query]";
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto stream = make_shared<SimpleStreamReader<string>>();
            stream->Add("[RETRIEVING] ");
            vector<string> docs = {"Doc1", ", ", "Doc2", ", ", "Doc3"};
            for (const auto& doc : docs) {
                stream->Add(doc);
            }
            stream->Add("\n");
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r; string c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>(); 
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 节点 3: LLM 生成回答
    auto llm_generator = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return "Based on the context, here is the answer to your question.";
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto stream = make_shared<SimpleStreamReader<string>>();
            
            // 模拟 LLM token-by-token 生成
            vector<string> tokens = {
                "[GENERATING] ", 
                "Based", " ", "on", " ", "the", " ", "retrieved", " ", 
                "documents", ",", " ", "here", " ", "is", " ", "the", " ",
                "answer", ":", " ", "The", " ", "weather", " ", "is", " ",
                "sunny", " ", "today", "."
            };
            
            for (const auto& token : tokens) {
                stream->Add(token);
            }
            
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r; string c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>(); 
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 节点 4: 格式化输出
    auto formatter = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return input + "\n\n[END OF RESPONSE]";
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto stream = make_shared<SimpleStreamReader<string>>();
            stream->Add(input);
            stream->Add("\n\n[END OF RESPONSE]");
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r; string c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>(); 
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 构建 RAG Pipeline
    graph->AddNode("analyzer", query_analyzer);
    graph->AddNode("retriever", retriever);
    graph->AddNode("generator", llm_generator);
    graph->AddNode("formatter", formatter);
    
    graph->AddEdge(Graph<string, string>::START_NODE, "analyzer");
    graph->AddEdge("analyzer", "retriever");
    graph->AddEdge("retriever", "generator");
    graph->AddEdge("generator", "formatter");
    graph->AddEdge("formatter", Graph<string, string>::END_NODE);
    
    graph->Compile();
    
    auto ctx = Context::Background();
    
    cout << "\n[Graph Structure - RAG Pipeline]" << endl;
    cout << "START -> analyzer -> retriever -> generator -> formatter -> END" << endl;
    
    cout << "\n[User Query]" << endl;
    cout << "Q: What is the weather today?" << endl;
    
    cout << "\n[Stream Response]" << endl;
    cout << "A: ";
    auto stream = graph->Stream(ctx, "What is the weather today?");
    string chunk;
    while (stream && stream->Read(chunk)) {
        cout << chunk << flush;
        StreamDelay(40);
    }
    cout << endl;
}

// ============================================================================
// 示例 3: 多分支流式 Graph
// ============================================================================

void Example3_BranchedStreamGraph() {
    PrintSeparator("Example 3: Branched Stream Graph");
    
    auto graph = make_shared<Graph<string, string>>();
    
    // 主干节点：输入处理
    auto input_processor = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return "[PROCESSED] " + input;
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto stream = make_shared<SimpleStreamReader<string>>();
            stream->Add("[INPUT] ");
            stream->Add(input);
            stream->Add(" ");
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r; string c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>(); 
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 分支 A: 情感分析
    auto sentiment_analyzer = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return input + " [Sentiment: Positive]";
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto stream = make_shared<SimpleStreamReader<string>>();
            stream->Add("-> [SENTIMENT: Positive] ");
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r; string c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>(); 
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 分支 B: 关键词提取
    auto keyword_extractor = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return input + " [Keywords: AI, Stream, Graph]";
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto stream = make_shared<SimpleStreamReader<string>>();
            stream->Add("-> [KEYWORDS: AI, Stream, Graph] ");
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r; string c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>(); 
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 汇总节点
    auto aggregator = NewLambdaRunnable<string, string>(
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> string {
            return input + "\n[ANALYSIS COMPLETE]";
        },
        [](shared_ptr<Context> ctx, const string& input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto stream = make_shared<SimpleStreamReader<string>>();
            stream->Add(input);
            stream->Add("\n[COMPLETE]");
            return stream;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> string {
            string r; string c; while (input && input->Read(c)) r += c; return r;
        },
        [](shared_ptr<Context> ctx, shared_ptr<StreamReader<string>> input, const vector<Option>& opts) -> shared_ptr<StreamReader<string>> {
            auto o = make_shared<SimpleStreamReader<string>>(); 
            string c; while (input && input->Read(c)) o->Add(c); return o;
        }
    );
    
    // 构建多分支 Graph
    graph->AddNode("input_proc", input_processor);
    graph->AddNode("sentiment", sentiment_analyzer);
    graph->AddNode("keywords", keyword_extractor);
    graph->AddNode("aggregator", aggregator);
    
    graph->AddEdge(Graph<string, string>::START_NODE, "input_proc");
    graph->AddEdge("input_proc", "sentiment");
    graph->AddEdge("input_proc", "keywords");
    graph->AddEdge("sentiment", "aggregator");
    graph->AddEdge("keywords", "aggregator");
    graph->AddEdge("aggregator", Graph<string, string>::END_NODE);
    
    graph->Compile();
    
    auto ctx = Context::Background();
    
    cout << "\n[Graph Structure - Branched Pipeline]" << endl;
    cout << "                  ┌─> sentiment ─┐" << endl;
    cout << "START -> input_proc              aggregator -> END" << endl;
    cout << "                  └─> keywords ──┘" << endl;
    
    cout << "\n[Test: Stream Mode]" << endl;
    cout << "Input: \"I love AI and streaming!\"" << endl;
    cout << "\nStream output:\n> ";
    auto stream = graph->Stream(ctx, "I love AI and streaming!");
    string chunk;
    while (stream && stream->Read(chunk)) {
        cout << chunk << flush;
        StreamDelay(60);
    }
    cout << endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    cout << "\n";
    cout << "╔═══════════════════════════════════════════════════════════════╗" << endl;
    cout << "║         Eino C++ Compose - Graph Stream Pipeline             ║" << endl;
    cout << "║              Graph 流式输出完整示例                            ║" << endl;
    cout << "╚═══════════════════════════════════════════════════════════════╝" << endl;
    
    try {
        Example1_SimpleStreamGraph();
        Example2_LLMQAStreamGraph();
        Example3_BranchedStreamGraph();
        
        PrintSeparator("Summary");
        cout << "\n✅ All Graph stream examples completed successfully!" << endl;
        cout << "\n[Key Features Demonstrated]" << endl;
        cout << "• 每个节点都实现了完整的 Stream 方法" << endl;
        cout << "• Graph.Stream() 自动串联所有节点的流式输出" << endl;
        cout << "• 支持线性管道、分支合并等复杂拓扑" << endl;
        cout << "• 完美模拟 RAG、LLM 等实际应用场景" << endl;
        cout << "• 流式输出提供实时反馈，提升用户体验" << endl;
        cout << endl;
        
        return 0;
        
    } catch (const exception& e) {
        cerr << "\n❌ Error: " << e.what() << endl;
        return 1;
    }
}
