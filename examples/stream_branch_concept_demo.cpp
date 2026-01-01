/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Stream Branch Concept Demo - Stream分支概念演示
 * 
 * 说明: 由于GCC 4.9.3不支持C++17和std::any, 本示例演示GraphBranch处理stream的核心概念
 * 实际使用时需要C++17编译器
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <queue>

// ============================================================================
// 简化的Stream实现 (概念演示)
// ============================================================================

template<typename T>
class SimpleStream {
public:
    void Add(const T& item) {
        items_.push(item);
    }
    
    bool Read(T& item) {
        if (items_.empty()) {
            return false;
        }
        item = items_.front();
        items_.pop();
        return true;
    }
    
    bool HasData() const {
        return !items_.empty();
    }
    
    size_t Size() const {
        return items_.size();
    }
    
private:
    std::queue<T> items_;
};

// ============================================================================
// 数据结构
// ============================================================================

struct Message {
    std::string content;
    std::string priority;  // "high", "normal", "low"
    int id;
    
    Message() : content(""), priority(""), id(0) {}
    Message(const std::string& c, const std::string& p, int i)
        : content(c), priority(p), id(i) {}
};

// ============================================================================
// GraphBranch概念模拟
// ============================================================================

// StreamGraphBranchCondition: 读取stream并返回路由决策
typedef std::string (*StreamConditionFunc)(SimpleStream<Message>&);

class StreamBranchRouter {
public:
    StreamBranchRouter(StreamConditionFunc condition)
        : condition_func_(condition) {}
    
    // Collect: GraphBranch处理stream的核心方法
    // 这个方法读取stream的第一个元素做决策,然后路由到对应节点
    std::string Collect(SimpleStream<Message>& stream) {
        std::cout << "\n  [GraphBranch::Collect] Reading first chunk from stream..." << std::endl;
        
        if (!stream.HasData()) {
            throw std::runtime_error("Stream is empty!");
        }
        
        // 核心: 调用条件函数决定路由
        std::string target_node = condition_func_(stream);
        
        std::cout << "  [GraphBranch::Collect] Routing to: " << target_node << std::endl;
        return target_node;
    }
    
private:
    StreamConditionFunc condition_func_;
};

// ============================================================================
// 节点处理器模拟
// ============================================================================

class MessageProcessor {
public:
    MessageProcessor(const std::string& name, const std::string& prefix)
        : name_(name), prefix_(prefix) {}
    
    // Transform: 流式处理方法,逐个处理stream中的消息
    void Transform(SimpleStream<Message>& input_stream, SimpleStream<Message>& output_stream) {
        std::cout << "\n  [" << name_ << "] Processing stream..." << std::endl;
        
        Message msg;
        int count = 0;
        while (input_stream.Read(msg)) {
            count++;
            msg.content = prefix_ + msg.content;
            std::cout << "    • Message #" << msg.id << ": " << msg.content << std::endl;
            output_stream.Add(msg);
        }
        
        std::cout << "  [" << name_ << "] Processed " << count << " messages" << std::endl;
    }
    
private:
    std::string name_;
    std::string prefix_;
};

// ============================================================================
// 示例 1: 基本的Stream分支路由
// ============================================================================

std::string PriorityRouter(SimpleStream<Message>& stream) {
    Message msg;
    
    // ⭐ 关键点: 只读取第一个元素做决策
    if (stream.Read(msg)) {
        std::cout << "  [Router] Message #" << msg.id 
                  << " with priority: " << msg.priority << std::endl;
        
        if (msg.priority == "high") {
            std::cout << "  [Router] Decision: Route to HIGH handler" << std::endl;
            return "high_handler";
        } else if (msg.priority == "low") {
            std::cout << "  [Router] Decision: Route to LOW handler" << std::endl;
            return "low_handler";
        } else {
            std::cout << "  [Router] Decision: Route to NORMAL handler" << std::endl;
            return "normal_handler";
        }
    }
    
    throw std::runtime_error("Failed to read from stream");
}

void Example1_BasicStreamBranch() {
    std::cout << "\n";
    std::cout << "========================================================================" << std::endl;
    std::cout << "Example 1: Basic Stream Branch with Priority Routing" << std::endl;
    std::cout << "========================================================================" << std::endl;
    
    // 创建输入stream
    SimpleStream<Message> input_stream;
    input_stream.Add(Message("Urgent task", "high", 1));
    input_stream.Add(Message("Routine check", "normal", 2));
    input_stream.Add(Message("Cleanup job", "low", 3));
    input_stream.Add(Message("Critical alert", "high", 4));
    
    std::cout << "\n[Step 1] Created input stream with " << input_stream.Size() << " messages" << std::endl;
    
    // 创建GraphBranch路由器
    StreamBranchRouter router(PriorityRouter);
    
    std::cout << "\n[Step 2] GraphBranch analyzing stream..." << std::endl;
    
    // ⭐ 核心: GraphBranch的Collect方法读取第一个chunk做决策
    std::string target = router.Collect(input_stream);
    
    std::cout << "\n[Step 3] Routing decision: " << target << std::endl;
    std::cout << "          Remaining messages in stream: " << input_stream.Size() << std::endl;
    
    // 创建对应的处理器
    MessageProcessor* processor = NULL;
    if (target == "high_handler") {
        processor = new MessageProcessor("HighPriorityHandler", "[🔴 HIGH] ");
    } else if (target == "normal_handler") {
        processor = new MessageProcessor("NormalHandler", "[🟡 NORMAL] ");
    } else {
        processor = new MessageProcessor("LowPriorityHandler", "[🟢 LOW] ");
    }
    
    // 处理剩余的stream
    SimpleStream<Message> output_stream;
    std::cout << "\n[Step 4] Selected handler processing stream..." << std::endl;
    processor->Transform(input_stream, output_stream);
    
    // 显示结果
    std::cout << "\n[Results] Final output stream:" << std::endl;
    Message result;
    int count = 1;
    while (output_stream.Read(result)) {
        std::cout << "  " << count++ << ". " << result.content << std::endl;
    }
    
    delete processor;
}

// ============================================================================
// 示例 2: Stream分支的关键特性演示
// ============================================================================

void Example2_StreamBranchFeatures() {
    std::cout << "\n";
    std::cout << "========================================================================" << std::endl;
    std::cout << "Example 2: Key Features of Stream Branch" << std::endl;
    std::cout << "========================================================================" << std::endl;
    
    std::cout << "\n[Feature 1] GraphBranch只读取第一个chunk做决策" << std::endl;
    std::cout << "   • 保持stream的完整性" << std::endl;
    std::cout << "   • 剩余数据继续传递给下游节点" << std::endl;
    std::cout << "   • 适合实时流式场景" << std::endl;
    
    // 演示
    SimpleStream<Message> demo_stream;
    demo_stream.Add(Message("First", "high", 1));
    demo_stream.Add(Message("Second", "normal", 2));
    demo_stream.Add(Message("Third", "low", 3));
    
    std::cout << "\n   Initial stream size: " << demo_stream.Size() << std::endl;
    
    StreamBranchRouter router(PriorityRouter);
    std::string route = router.Collect(demo_stream);
    
    std::cout << "   After Collect, stream size: " << demo_stream.Size() << std::endl;
    std::cout << "   → Only first chunk was consumed for routing!" << std::endl;
    
    std::cout << "\n[Feature 2] NewStreamGraphBranch vs NewGraphBranch" << std::endl;
    std::cout << "   • NewStreamGraphBranch: 直接接收 StreamReader<T>" << std::endl;
    std::cout << "   • NewGraphBranch: 接收普通类型T, 自动读取第一个chunk" << std::endl;
    std::cout << "   • 两者都调用 Collect() 方法处理stream" << std::endl;
    
    std::cout << "\n[Feature 3] 在Graph中的完整流程" << std::endl;
    std::cout << "   1. 上游节点产生 StreamReader<T>" << std::endl;
    std::cout << "   2. GraphBranch::Collect 读取第一个chunk" << std::endl;
    std::cout << "   3. 条件函数决定路由到哪个节点" << std::endl;
    std::cout << "   4. 目标节点继续处理剩余的stream" << std::endl;
    std::cout << "   5. 输出结果聚合为新的stream" << std::endl;
}

// ============================================================================
// 示例 3: 实际使用场景
// ============================================================================

void Example3_RealWorldScenario() {
    std::cout << "\n";
    std::cout << "========================================================================" << std::endl;
    std::cout << "Example 3: Real-World Scenario - LLM Stream Routing" << std::endl;
    std::cout << "========================================================================" << std::endl;
    
    std::cout << "\n[Scenario] LLM流式输出根据内容类型路由" << std::endl;
    std::cout << "\n假设场景:" << std::endl;
    std::cout << "• LLM产生流式输出" << std::endl;
    std::cout << "• 根据第一个chunk的内容类型决定后续处理" << std::endl;
    std::cout << "• 不同类型走不同的处理管道" << std::endl;
    
    // 模拟LLM输出
    SimpleStream<Message> llm_stream;
    llm_stream.Add(Message("Code: def hello():", "high", 1));  // 第一个chunk识别为代码
    llm_stream.Add(Message("    print('hello')", "high", 2));
    llm_stream.Add(Message("    return True", "high", 3));
    
    std::cout << "\n[LLM Output Stream] 3 chunks generated" << std::endl;
    
    // 内容类型路由器
    auto content_router = [](SimpleStream<Message>& stream) -> std::string {
        Message first_chunk;
        if (stream.Read(first_chunk)) {
            std::cout << "  [Content Router] First chunk: " << first_chunk.content << std::endl;
            
            // 根据内容决定路由
            if (first_chunk.content.find("Code:") == 0) {
                std::cout << "  [Content Router] Detected CODE → code_formatter" << std::endl;
                return "code_formatter";
            } else if (first_chunk.content.find("Query:") == 0) {
                std::cout << "  [Content Router] Detected QUERY → query_executor" << std::endl;
                return "query_executor";
            } else {
                std::cout << "  [Content Router] Detected TEXT → text_renderer" << std::endl;
                return "text_renderer";
            }
        }
        return "default_handler";
    };
    
    StreamBranchRouter llm_router(content_router);
    
    std::cout << "\n[Routing Decision]" << std::endl;
    std::string handler = llm_router.Collect(llm_stream);
    
    std::cout << "\n[Processing]" << std::endl;
    MessageProcessor formatter("CodeFormatter", "[FORMATTED] ");
    SimpleStream<Message> output;
    formatter.Transform(llm_stream, output);
    
    std::cout << "\n[Final Output]" << std::endl;
    Message chunk;
    while (output.Read(chunk)) {
        std::cout << "  " << chunk.content << std::endl;
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         Stream Branch Concept Demonstration                   ║" << std::endl;
    std::cout << "║           GraphBranch处理Stream的核心概念                      ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        Example1_BasicStreamBranch();
        Example2_StreamBranchFeatures();
        Example3_RealWorldScenario();
        
        std::cout << "\n";
        std::cout << "========================================================================" << std::endl;
        std::cout << "Summary - BranchNode vs GraphBranch for Stream" << std::endl;
        std::cout << "========================================================================" << std::endl;
        std::cout << "\n[BranchNode]" << std::endl;
        std::cout << "• 设计用途: 条件判断节点" << std::endl;
        std::cout << "• Stream支持: ❌ 不支持 (抛出异常)" << std::endl;
        std::cout << "• 输入模式: Invoke() - 需要完整数据" << std::endl;
        std::cout << "• 使用场景: 基于完整输入的条件分支" << std::endl;
        
        std::cout << "\n[GraphBranch]" << std::endl;
        std::cout << "• 设计用途: Graph中的路由节点" << std::endl;
        std::cout << "• Stream支持: ✅ 支持 (Collect方法)" << std::endl;
        std::cout << "• 输入模式: Collect() - 读取第一个chunk决策" << std::endl;
        std::cout << "• 使用场景: 流式数据的路由分支" << std::endl;
        
        std::cout << "\n[API对比]" << std::endl;
        std::cout << "• NewStreamGraphBranch<T>()    - Stream单路分支" << std::endl;
        std::cout << "• NewStreamGraphMultiBranch<T>() - Stream多路分支" << std::endl;
        std::cout << "• NewGraphBranch<T>()          - 普通分支(自动处理stream)" << std::endl;
        
        std::cout << "\n[关键机制]" << std::endl;
        std::cout << "1. Collect只读取第一个chunk做路由决策" << std::endl;
        std::cout << "2. 剩余stream数据完整传递给目标节点" << std::endl;
        std::cout << "3. 保持流式处理的实时性和完整性" << std::endl;
        std::cout << "4. 适合LLM流式输出的动态路由" << std::endl;
        
        std::cout << "\n✅ All concept demonstrations completed!" << std::endl;
        std::cout << "\n注意: 实际使用需要C++17编译器和完整的eino_cpp库" << std::endl;
        std::cout << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
