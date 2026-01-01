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

#include "../include/eino/compose/branch_node.h"
#include "../include/eino/compose/graph.h"
#include "../include/eino/compose/graph_run.h"
#include <iostream>
#include <map>
#include <any>
#include <string>

using namespace eino::compose;

// 模拟一个简单的任务节点
// 接收一个 map，输出一个 map，将输入中的 "value" 字段值 +1
class SimpleTaskNode : public Runnable<std::map<std::string, std::any>, std::map<std::string, std::any>> {
public:
    std::map<std::string, std::any> Invoke(std::shared_ptr<Context> ctx, const std::map<std::string, std::any>& input) override {
        std::map<std::string, std::any> output = input;
        if (input.count("value") && input.at("value").type() == typeid(int64_t)) {
            int64_t val = std::any_cast<int64_t>(input.at("value"));
            output["value"] = val + 1;
            std::cout << "[Node] Processed value: " << val << " -> " << val + 1 << std::endl;
        } else {
             std::cout << "[Node] Pass through" << std::endl;
        }
        return output;
    }
};

void RunBranchNodeGraphExample() {
    std::cout << "\n=== BranchNode Graph Example ===" << std::endl;

    // 1. 创建图
    Graph<std::map<std::string, std::any>, std::map<std::string, std::any>> graph;

    // 2. 添加起始节点 (输入处理)
    auto start_node = std::make_shared<SimpleTaskNode>();
    graph.AddNode("StartNode", start_node);

    // 3. 创建 BranchNode 配置
    // 分支逻辑：
    // Branch 0: value >= 10 (High Value)
    // Branch 1: value < 10 (Low Value)
    // Default: (Wait, BranchNode logic handles unmatched cases as default branch, which is index 2 here if we had 2 conditions)
    // NOTE: In this example we cover all cases with >= 10 and < 10 implicitly if we set it up right, or use default.
    // Let's setup:
    // Branch 0: value >= 10
    // Default (Branch 1): Any other case
    
    BranchNodeConfig branch_config;
    branch_config.AddSingleCondition(BranchOperator::GreaterOrEqual); // Condition for Branch 0
    
    // 创建 BranchNode
    // BranchNode 的输入来源于图中的前驱节点
    auto branch_node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, branch_config);
    
    graph.AddBranch("BranchDecision", branch_node);

    // 4. 添加后续节点
    auto high_value_handler = std::make_shared<SimpleTaskNode>();
    auto low_value_handler = std::make_shared<SimpleTaskNode>();
    
    graph.AddNode("HighValueHandler", high_value_handler);
    graph.AddNode("LowValueHandler", low_value_handler);

    // 5. 连接节点
    // StartNode -> BranchDecision
    graph.AddEdge("StartNode", "BranchDecision");

    // BranchDecision -> HighValueHandler (Branch 0: value >= 10)
    graph.AddBranchEdge("BranchDecision", 0, "HighValueHandler");
    
    // BranchDecision -> LowValueHandler (Default Branch: when condition 0 fails)
    // The BranchNode returns index = num_conditions if no condition matches.
    // Here we have 1 condition. So index 0 is match, index 1 is no match (default).
    graph.AddBranchEdge("BranchDecision", 1, "LowValueHandler");

    // 结束连接 (可选，如果需要汇聚，这里为了简单直接结束)
    graph.AddEdge("HighValueHandler", END);
    graph.AddEdge("LowValueHandler", END);

    // 6. 编译图
    std::cout << "Compiling graph..." << std::endl;
    auto runnable = graph.Compile();

    // 7. 运行图 - Case 1: Low Value
    {
        std::cout << "\n--- Running Case 1: Input value = 5 ---" << std::endl;
        std::map<std::string, std::any> input;
        input["value"] = static_cast<int64_t>(5);
        
        // BranchNode 需要特定的输入格式来做判断
        // 在实际的图执行中，BranchNode 的输入通常由前驱节点产生。
        // 这里我们需要注意：
        // BranchNode 期望的输入是 map<string, any>，其中 key 对应 condition index。
        // 但是，如果 BranchNode 是图的一部分，它的输入通常是业务数据。gr
        // eino::compose::BranchNode 的设计似乎是直接接收包含 "0", "1" 等 key 的 map 作为判断依据。
        // 这意味着前驱节点需要输出符合这种格式的数据，或者我们需要一个适配器。
        
        // 为了简化演示，我们假设 StartNode 输出的数据已经被格式化为 BranchNode 需要的格式，
        // 或者我们在这里手动构造输入数据，让 StartNode 透传。
        
        // 构造符合 BranchNode 要求的输入
        // 我们希望判断的是 value >= 10。
        // BranchNode config index 0: left >= right
        // 我们需要构造 input["0"]["left"] = value, input["0"]["right"] = 10
        
        std::map<std::string, std::any> branch_condition;
        branch_condition["left"] = static_cast<int64_t>(5);
        branch_condition["right"] = static_cast<int64_t>(10);
        
        std::map<std::string, std::any> run_input;
        run_input["value"] = static_cast<int64_t>(5); // 业务数据
        run_input["0"] = branch_condition;            // 分支判断数据
        
        // 注意：SimpleTaskNode 只是简单地将 "value" + 1，它不会生成 "0" 这样的分支判断数据。
        // 在真实场景中，我们可能需要一个专门的 "特征提取节点" 或者让 StartNode 生成这些数据。
        // 这里为了让例子跑通，我们让 StartNode 透传所有字段。
        
        auto result = runnable->Invoke(nullptr, run_input);
        
        if (result.count("value")) {
             std::cout << "Result value: " << std::any_cast<int64_t>(result["value"]) << std::endl;
        }
    }

    // 8. 运行图 - Case 2: High Value
    {
        std::cout << "\n--- Running Case 2: Input value = 15 ---" << std::endl;
        
        std::map<std::string, std::any> branch_condition;
        branch_condition["left"] = static_cast<int64_t>(15);
        branch_condition["right"] = static_cast<int64_t>(10);
        
        std::map<std::string, std::any> run_input;
        run_input["value"] = static_cast<int64_t>(15);
        run_input["0"] = branch_condition;
        
        auto result = runnable->Invoke(nullptr, run_input);
        
         if (result.count("value")) {
             std::cout << "Result value: " << std::any_cast<int64_t>(result["value"]) << std::endl;
        }
    }
}

int main() {
    try {
        RunBranchNodeGraphExample();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
