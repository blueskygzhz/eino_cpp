/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Graph Stream Branch Example - Graph流式分支路由完整示例
 * 
 * 本示例演示:
 * 1. 如何在Graph中使用GraphBranch处理stream输入
 * 2. 流式数据的条件路由
 * 3. NewStreamGraphBranch的实际应用
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "eino/compose/graph.h"
#include "eino/compose/runnable.h"
#include "eino/compose/types_lambda.h"
#include "eino/compose/branch.h"
#include "eino/schema/stream.h"

using namespace eino;
using namespace eino::compose;
using namespace eino::schema;

// ============================================================================
// 数据结构定义
// ============================================================================

struct Message {
    std::string content;
    std::string priority;  // "high", "normal", "low"
    int id;
    
    Message() : id(0), content(""), priority("") {}
    Message(const std::string& c, const std::string& p, int i)
        : content(c), priority(p), id(i) {}
    
    // 复制构造函数
    Message(const Message& other) 
        : content(other.content), priority(other.priority), id(other.id) {}
    
    // 赋值运算符
    Message& operator=(const Message& other) {
        if (this != &other) {
            content = other.content;
            priority = other.priority;
            id = other.id;
        }
        return *this;
    }
};

// ============================================================================
// 辅助函数
// ============================================================================

void PrintSeparator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(70, '=') << std::endl;
}

// ============================================================================
// 示例 1: 基本的流式分支路由
// ============================================================================

void Example1_BasicStreamBranch() {
    PrintSeparator("Example 1: Basic Stream Branch with Priority Routing");
    
    // 创建 Graph
    auto graph = std::make_shared<Graph<Message, Message>>();
    
    // 节点 1: 高优先级处理器
    auto high_priority_handler = NewLambdaRunnable<Message, Message>(
        [](std::shared_ptr<Context> ctx, const Message& input, const std::vector<Option>& opts) {
            Message result = input;
            result.content = "[HIGH PRIORITY] " + result.content;
            std::cout << "  🔴 Processing HIGH priority message #" << result.id << std::endl;
            return result;
        }
    );
    
    // 节点 2: 普通优先级处理器
    auto normal_priority_handler = NewLambdaRunnable<Message, Message>(
        [](std::shared_ptr<Context> ctx, const Message& input, const std::vector<Option>& opts) {
            Message result = input;
            result.content = "[NORMAL] " + result.content;
            std::cout << "  🟡 Processing NORMAL priority message #" << result.id << std::endl;
            return result;
        }
    );
    
    // 节点 3: 低优先级处理器
    auto low_priority_handler = NewLambdaRunnable<Message, Message>(
        [](std::shared_ptr<Context> ctx, const Message& input, const std::vector<Option>& opts) {
            Message result = input;
            result.content = "[LOW] " + result.content;
            std::cout << "  🟢 Processing LOW priority message #" << result.id << std::endl;
            return result;
        }
    );
    
    // 添加节点到 Graph
    graph->AddNode("high_handler", high_priority_handler);
    graph->AddNode("normal_handler", normal_priority_handler);
    graph->AddNode("low_handler", low_priority_handler);
    
    // 创建流式分支条件函数
    StreamGraphBranchCondition<Message> priority_router = 
        [](void* ctx, std::shared_ptr<StreamReader<Message>> stream) -> std::string {
        
        std::cout << "\n  [Router] Reading first message from stream..." << std::endl;
        
        Message msg;
        std::string error;
        if (stream && stream->Recv(msg, error)) {
            std::cout << "  [Router] Message #" << msg.id 
                      << " with priority: " << msg.priority << std::endl;
            
            if (msg.priority == "high") {
                std::cout << "  [Router] → Routing to high_handler" << std::endl;
                return "high_handler";
            } else if (msg.priority == "low") {
                std::cout << "  [Router] → Routing to low_handler" << std::endl;
                return "low_handler";
            } else {
                std::cout << "  [Router] → Routing to normal_handler" << std::endl;
                return "normal_handler";
            }
        }
        
        throw std::runtime_error("Failed to read message from stream");
    };
    
    // 定义所有可能的目标节点
    std::set<std::string> end_nodes = {
        "high_handler",
        "normal_handler", 
        "low_handler"
    };
    
    // 创建 GraphBranch (使用 NewStreamGraphBranch)
    auto priority_branch = NewStreamGraphBranch<Message>(priority_router, end_nodes);
    
    // 添加分支到 Graph
    graph->AddBranch(Graph<Message, Message>::START_NODE, priority_branch);
    
    // 连接处理器到 END
    graph->AddEdge("high_handler", Graph<Message, Message>::END_NODE);
    graph->AddEdge("normal_handler", Graph<Message, Message>::END_NODE);
    graph->AddEdge("low_handler", Graph<Message, Message>::END_NODE);
    
    // 编译 Graph
    graph->Compile();
    std::cout << "\nGraph compiled successfully!" << std::endl;
    
    // 创建测试数据流
    auto input_stream = std::make_shared<SimpleStreamReader<Message>>();
    input_stream->Add(Message("Urgent task", "high", 1));
    input_stream->Add(Message("Routine check", "normal", 2));
    input_stream->Add(Message("Cleanup job", "low", 3));
    input_stream->Add(Message("Critical alert", "high", 4));
    
    std::cout << "\n[Processing Stream with 4 messages...]" << std::endl;
    
    auto ctx = Context::Background();
    auto output_stream = graph->Transform(ctx, input_stream);
    
    // 读取结果
    std::cout << "\n[Results]" << std::endl;
    Message result;
    int count = 1;
    while (output_stream && output_stream->Recv(result)) {
        std::cout << "  " << count++ << ". " << result.content << std::endl;
    }
}

// ============================================================================
// 示例 2: 多路分支路由
// ============================================================================

void Example2_MultiBranchRouting() {
    PrintSeparator("Example 2: Multi-Branch Routing with Tags");
    
    struct TaggedMessage {
        std::string content;
        std::vector<std::string> tags;  // 可能有多个标签
        
        TaggedMessage() : content(""), tags() {}
        TaggedMessage(const std::string& c, const std::vector<std::string>& t)
            : content(c), tags(t) {}
        
        // 复制构造函数
        TaggedMessage(const TaggedMessage& other)
            : content(other.content), tags(other.tags) {}
        
        // 赋值运算符
        TaggedMessage& operator=(const TaggedMessage& other) {
            if (this != &other) {
                content = other.content;
                tags = other.tags;
            }
            return *this;
        }
    };
    
    auto graph = std::make_shared<Graph<TaggedMessage, TaggedMessage>>();
    
    // 创建不同标签的处理器
    auto urgent_handler = NewLambdaRunnable<TaggedMessage, TaggedMessage>(
        [](std::shared_ptr<Context> ctx, const TaggedMessage& input, const std::vector<Option>& opts) {
            TaggedMessage result = input;
            result.content += " [URGENT-PROCESSED]";
            std::cout << "  🚨 Urgent handler: " << result.content << std::endl;
            return result;
        }
    );
    
    auto security_handler = NewLambdaRunnable<TaggedMessage, TaggedMessage>(
        [](std::shared_ptr<Context> ctx, const TaggedMessage& input, const std::vector<Option>& opts) {
            TaggedMessage result = input;
            result.content += " [SECURITY-CHECKED]";
            std::cout << "  🔒 Security handler: " << result.content << std::endl;
            return result;
        }
    );
    
    auto analytics_handler = NewLambdaRunnable<TaggedMessage, TaggedMessage>(
        [](std::shared_ptr<Context> ctx, const TaggedMessage& input, const std::vector<Option>& opts) {
            TaggedMessage result = input;
            result.content += " [ANALYTICS-LOGGED]";
            std::cout << "  📊 Analytics handler: " << result.content << std::endl;
            return result;
        }
    );
    
    graph->AddNode("urgent_handler", urgent_handler);
    graph->AddNode("security_handler", security_handler);
    graph->AddNode("analytics_handler", analytics_handler);
    
    // 多路分支条件: 根据标签路由到多个处理器
    StreamGraphMultiBranchCondition<TaggedMessage> multi_router =
        [](void* ctx, std::shared_ptr<StreamReader<TaggedMessage>> stream) -> std::set<std::string> {
        
        std::cout << "\n  [Multi-Router] Reading message..." << std::endl;
        
        TaggedMessage msg;
        if (stream && stream->Recv(msg)) {
            std::set<std::string> targets;
            
            std::cout << "  [Multi-Router] Message: " << msg.content << std::endl;
            std::cout << "  [Multi-Router] Tags: ";
            for (const auto& tag : msg.tags) {
                std::cout << tag << " ";
            }
            std::cout << std::endl;
            
            // 根据标签决定路由
            for (const auto& tag : msg.tags) {
                if (tag == "urgent") {
                    targets.insert("urgent_handler");
                } else if (tag == "security") {
                    targets.insert("security_handler");
                } else if (tag == "analytics") {
                    targets.insert("analytics_handler");
                }
            }
            
            std::cout << "  [Multi-Router] → Routing to " << targets.size() << " handlers" << std::endl;
            return targets;
        }
        
        throw std::runtime_error("Failed to read message");
    };
    
    std::set<std::string> end_nodes = {
        "urgent_handler",
        "security_handler",
        "analytics_handler"
    };
    
    auto multi_branch = NewStreamGraphMultiBranch<TaggedMessage>(multi_router, end_nodes);
    graph->AddBranch(Graph<TaggedMessage, TaggedMessage>::START_NODE, multi_branch);
    
    graph->AddEdge("urgent_handler", Graph<TaggedMessage, TaggedMessage>::END_NODE);
    graph->AddEdge("security_handler", Graph<TaggedMessage, TaggedMessage>::END_NODE);
    graph->AddEdge("analytics_handler", Graph<TaggedMessage, TaggedMessage>::END_NODE);
    
    graph->Compile();
    std::cout << "\nMulti-branch graph compiled!" << std::endl;
    
    // 测试数据
    auto input_stream = std::make_shared<SimpleStreamReader<TaggedMessage>>();
    input_stream->Add(TaggedMessage("System alert", {"urgent", "security"}));
    input_stream->Add(TaggedMessage("User login", {"security", "analytics"}));
    input_stream->Add(TaggedMessage("Critical error", {"urgent"}));
    
    std::cout << "\n[Processing 3 messages with multiple tags...]" << std::endl;
    
    auto ctx = Context::Background();
    auto output_stream = graph->Transform(ctx, input_stream);
    
    std::cout << "\n[Results]" << std::endl;
    TaggedMessage result;
    int count = 1;
    while (output_stream && output_stream->Recv(result)) {
        std::cout << "  " << count++ << ". " << result.content << std::endl;
    }
}

// ============================================================================
// 示例 3: 流式分支与管道组合
// ============================================================================

void Example3_BranchWithPipeline() {
    PrintSeparator("Example 3: Stream Branch Combined with Pipeline");
    
    auto graph = std::make_shared<Graph<Message, Message>>();
    
    // 预处理节点 (在分支之前)
    auto preprocessor = NewLambdaRunnable<Message, Message>(
        [](std::shared_ptr<Context> ctx, const Message& input, const std::vector<Option>& opts) {
            Message result = input;
            result.content = "[PREPROCESSED] " + result.content;
            std::cout << "  ⚙️ Preprocessing message #" << result.id << std::endl;
            return result;
        }
    );
    
    // 快速通道处理器
    auto fast_track = NewLambdaRunnable<Message, Message>(
        [](std::shared_ptr<Context> ctx, const Message& input, const std::vector<Option>& opts) {
            Message result = input;
            result.content += " [FAST-TRACK]";
            std::cout << "  ⚡ Fast track processing" << std::endl;
            return result;
        }
    );
    
    // 标准通道处理器
    auto standard_track = NewLambdaRunnable<Message, Message>(
        [](std::shared_ptr<Context> ctx, const Message& input, const std::vector<Option>& opts) {
            Message result = input;
            result.content += " [STANDARD]";
            std::cout << "  🚶 Standard processing" << std::endl;
            return result;
        }
    );
    
    // 后处理节点 (分支之后)
    auto postprocessor = NewLambdaRunnable<Message, Message>(
        [](std::shared_ptr<Context> ctx, const Message& input, const std::vector<Option>& opts) {
            Message result = input;
            result.content += " [FINALIZED]";
            std::cout << "  ✅ Postprocessing complete" << std::endl;
            return result;
        }
    );
    
    // 构建 Graph: preprocessor -> branch -> [fast/standard] -> postprocessor
    graph->AddNode("preprocessor", preprocessor);
    graph->AddNode("fast_track", fast_track);
    graph->AddNode("standard_track", standard_track);
    graph->AddNode("postprocessor", postprocessor);
    
    graph->AddEdge(Graph<Message, Message>::START_NODE, "preprocessor");
    
    // 添加分支
    StreamGraphBranchCondition<Message> speed_router =
        [](void* ctx, std::shared_ptr<StreamReader<Message>> stream) -> std::string {
        Message msg;
        if (stream && stream->Recv(msg)) {
            std::cout << "  [Router] Checking priority: " << msg.priority << std::endl;
            if (msg.priority == "high") {
                std::cout << "  [Router] → Using fast track" << std::endl;
                return "fast_track";
            } else {
                std::cout << "  [Router] → Using standard track" << std::endl;
                return "standard_track";
            }
        }
        throw std::runtime_error("Router failed");
    };
    
    std::set<std::string> branch_ends = {"fast_track", "standard_track"};
    auto speed_branch = NewStreamGraphBranch<Message>(speed_router, branch_ends);
    
    graph->AddBranch("preprocessor", speed_branch);
    
    // 两个通道都连接到后处理器
    graph->AddEdge("fast_track", "postprocessor");
    graph->AddEdge("standard_track", "postprocessor");
    graph->AddEdge("postprocessor", Graph<Message, Message>::END_NODE);
    
    graph->Compile();
    std::cout << "\nPipeline with branch compiled!" << std::endl;
    
    // 测试
    auto input_stream = std::make_shared<SimpleStreamReader<Message>>();
    input_stream->Add(Message("Task A", "high", 1));
    input_stream->Add(Message("Task B", "normal", 2));
    input_stream->Add(Message("Task C", "high", 3));
    
    std::cout << "\n[Processing 3 tasks through pipeline...]" << std::endl;
    
    auto ctx = Context::Background();
    auto output_stream = graph->Transform(ctx, input_stream);
    
    std::cout << "\n[Final Results]" << std::endl;
    Message result;
    while (output_stream && output_stream->Recv(result)) {
        std::cout << "  ✓ " << result.content << std::endl;
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Eino C++ - Graph Stream Branch Complete Example           ║" << std::endl;
    std::cout << "║         Graph流式分支路由完整示例                              ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        Example1_BasicStreamBranch();
        Example2_MultiBranchRouting();
        Example3_BranchWithPipeline();
        
        PrintSeparator("Summary");
        std::cout << "\n✅ All examples completed successfully!\n" << std::endl;
        
        std::cout << "[Key Concepts]" << std::endl;
        std::cout << "• NewStreamGraphBranch: 处理stream输入的单路分支" << std::endl;
        std::cout << "• NewStreamGraphMultiBranch: 处理stream输入的多路分支" << std::endl;
        std::cout << "• GraphBranch读取第一个chunk做路由决策" << std::endl;
        std::cout << "• 分支可以与管道节点灵活组合" << std::endl;
        std::cout << "• Transform方法实现端到端的流式处理" << std::endl;
        std::cout << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
