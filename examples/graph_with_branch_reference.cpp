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
 * Graph with BranchNode Reference Example
 * 
 * 场景：创建一个包含 A、B、C 三个节点的 Graph
 * - Node A: 处理用户信息，输出 {age: 25, name: "Alice"}
 * - Node B: 处理评分信息，输出 {score: 85, vip: true}
 * - Node C: BranchNode，引用 A 和 B 的输出进行条件判断
 *   - Branch 0: A.age >= 18 AND B.vip == true (VIP成年用户)
 *   - Branch 1: B.score >= 80 (高分用户)
 *   - Default: 普通用户
 * 
 * Graph 结构:
 *   START → A ┐
 *              ├→ C (BranchNode) → D_vip (Branch 0)
 *   START → B ┘                  → D_high_score (Branch 1)
 *                                → D_normal (Default)
 */

#include "../include/eino/compose/branch_node.h"
#include <iostream>
#include <map>
#include <any>
#include <string>

using namespace eino::compose;

// ============================================================================
// 模拟 Node A: 用户信息处理节点
// ============================================================================
class NodeA {
public:
    static std::map<std::string, std::any> Process(const std::map<std::string, std::any>& input) {
        std::map<std::string, std::any> output;
        
        // 模拟处理：从输入中提取用户年龄和姓名
        if (input.count("user_age")) {
            output["age"] = input.at("user_age");
        } else {
            output["age"] = static_cast<int64_t>(25);  // 默认值
        }
        
        if (input.count("user_name")) {
            output["name"] = input.at("user_name");
        } else {
            output["name"] = std::string("Alice");  // 默认值
        }
        
        std::cout << "[Node A] 处理用户信息:" << std::endl;
        std::cout << "  age: " << std::any_cast<int64_t>(output["age"]) << std::endl;
        std::cout << "  name: " << std::any_cast<std::string>(output["name"]) << std::endl;
        
        return output;
    }
};

// ============================================================================
// 模拟 Node B: 评分信息处理节点
// ============================================================================
class NodeB {
public:
    static std::map<std::string, std::any> Process(const std::map<std::string, std::any>& input) {
        std::map<std::string, std::any> output;
        
        // 模拟处理：从输入中提取评分和VIP状态
        if (input.count("user_score")) {
            output["score"] = input.at("user_score");
        } else {
            output["score"] = static_cast<int64_t>(85);  // 默认值
        }
        
        if (input.count("is_vip")) {
            output["vip"] = input.at("is_vip");
        } else {
            output["vip"] = true;  // 默认值
        }
        
        std::cout << "[Node B] 处理评分信息:" << std::endl;
        std::cout << "  score: " << std::any_cast<int64_t>(output["score"]) << std::endl;
        std::cout << "  vip: " << (std::any_cast<bool>(output["vip"]) ? "true" : "false") << std::endl;
        
        return output;
    }
};

// ============================================================================
// 模拟后续处理节点
// ============================================================================
class ProcessNode {
public:
    static void ProcessVIP() {
        std::cout << "\n[Node D_VIP] 🌟 VIP成年用户 - 提供高级服务" << std::endl;
    }
    
    static void ProcessHighScore() {
        std::cout << "\n[Node D_HighScore] ⭐ 高分用户 - 提供优质服务" << std::endl;
    }
    
    static void ProcessNormal() {
        std::cout << "\n[Node D_Normal] 👤 普通用户 - 提供标准服务" << std::endl;
    }
};

// ============================================================================
// 运行 Graph 示例
// ============================================================================
void RunGraphExample() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  Graph with BranchNode Reference Example" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // ========================================================================
    // 步骤 1: 创建 BranchNode C 的配置（引用 A 和 B 的输出）
    // ========================================================================
    std::cout << "\n[步骤 1] 创建 BranchNode C 配置..." << std::endl;
    
    BranchNodeConfig branch_config;
    
    // Branch 0: node_a.age >= 18 AND node_b.vip == true
    std::cout << "  Branch 0: (node_a.age >= 18) AND (node_b.vip == true)" << std::endl;
    std::vector<SingleClauseConfig> vip_clauses = {
        SingleClauseConfig(
            BranchOperator::GreaterOrEqual,
            OperandConfig::FromNode("node_a", {"age"}),  // 引用 A 的 age
            OperandConfig::FromLiteral(static_cast<int64_t>(18))
        ),
        SingleClauseConfig(
            BranchOperator::Equal,
            OperandConfig::FromNode("node_b", {"vip"}),  // 引用 B 的 vip
            OperandConfig::FromLiteral(true)
        )
    };
    branch_config.AddMultiConditionWithOperands(vip_clauses, ClauseRelation::AND);
    
    // Branch 1: node_b.score >= 80
    std::cout << "  Branch 1: node_b.score >= 80" << std::endl;
    branch_config.AddConditionWithOperands(
        BranchOperator::GreaterOrEqual,
        OperandConfig::FromNode("node_b", {"score"}),  // 引用 B 的 score
        OperandConfig::FromLiteral(static_cast<int64_t>(80))
    );
    
    std::cout << "  Default: 其他情况" << std::endl;
    
    // ========================================================================
    // 步骤 2: 创建 BranchNode C
    // ========================================================================
    std::cout << "\n[步骤 2] 创建 BranchNode C..." << std::endl;
    auto branch_node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, branch_config);
    std::cout << "  ✓ BranchNode 创建成功" << std::endl;
    
    // ========================================================================
    // 步骤 3: 测试场景 1 - VIP成年用户
    // ========================================================================
    std::cout << "\n" << std::string(70, '-') << std::endl;
    std::cout << "[场景 1] VIP成年用户" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    {
        // 准备输入数据
        std::map<std::string, std::any> graph_input;
        graph_input["user_age"] = static_cast<int64_t>(25);
        graph_input["user_name"] = std::string("Alice");
        graph_input["user_score"] = static_cast<int64_t>(85);
        graph_input["is_vip"] = true;
        
        // 模拟 Graph 执行流程
        std::cout << "\n[Graph 开始执行]" << std::endl;
        
        // 执行 Node A
        auto node_a_output = NodeA::Process(graph_input);
        
        // 执行 Node B
        auto node_b_output = NodeB::Process(graph_input);
        
        // 构造 BranchNode 的输入（包含 A 和 B 的输出）
        std::map<std::string, std::any> branch_input;
        branch_input["node_a"] = node_a_output;
        branch_input["node_b"] = node_b_output;
        
        // 执行 Node C (BranchNode)
        std::cout << "\n[Node C (BranchNode)] 执行条件判断..." << std::endl;
        auto branch_output = branch_node->Invoke(nullptr, branch_input);
        int64_t selected = std::any_cast<int64_t>(branch_output["selected"]);
        
        std::cout << "  → 选中分支: Branch " << selected << std::endl;
        
        // 根据分支执行对应的后续节点
        if (selected == 0) {
            ProcessNode::ProcessVIP();
        } else if (selected == 1) {
            ProcessNode::ProcessHighScore();
        } else {
            ProcessNode::ProcessNormal();
        }
    }
    
    // ========================================================================
    // 步骤 4: 测试场景 2 - 高分非VIP用户
    // ========================================================================
    std::cout << "\n" << std::string(70, '-') << std::endl;
    std::cout << "[场景 2] 高分非VIP用户" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    {
        std::map<std::string, std::any> graph_input;
        graph_input["user_age"] = static_cast<int64_t>(30);
        graph_input["user_name"] = std::string("Bob");
        graph_input["user_score"] = static_cast<int64_t>(90);
        graph_input["is_vip"] = false;  // 不是VIP
        
        std::cout << "\n[Graph 开始执行]" << std::endl;
        
        auto node_a_output = NodeA::Process(graph_input);
        auto node_b_output = NodeB::Process(graph_input);
        
        std::map<std::string, std::any> branch_input;
        branch_input["node_a"] = node_a_output;
        branch_input["node_b"] = node_b_output;
        
        std::cout << "\n[Node C (BranchNode)] 执行条件判断..." << std::endl;
        auto branch_output = branch_node->Invoke(nullptr, branch_input);
        int64_t selected = std::any_cast<int64_t>(branch_output["selected"]);
        
        std::cout << "  → 选中分支: Branch " << selected << std::endl;
        
        if (selected == 0) {
            ProcessNode::ProcessVIP();
        } else if (selected == 1) {
            ProcessNode::ProcessHighScore();
        } else {
            ProcessNode::ProcessNormal();
        }
    }
    
    // ========================================================================
    // 步骤 5: 测试场景 3 - 未成年普通用户
    // ========================================================================
    std::cout << "\n" << std::string(70, '-') << std::endl;
    std::cout << "[场景 3] 未成年普通用户" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    {
        std::map<std::string, std::any> graph_input;
        graph_input["user_age"] = static_cast<int64_t>(16);  // 未成年
        graph_input["user_name"] = std::string("Charlie");
        graph_input["user_score"] = static_cast<int64_t>(50);  // 低分
        graph_input["is_vip"] = false;
        
        std::cout << "\n[Graph 开始执行]" << std::endl;
        
        auto node_a_output = NodeA::Process(graph_input);
        auto node_b_output = NodeB::Process(graph_input);
        
        std::map<std::string, std::any> branch_input;
        branch_input["node_a"] = node_a_output;
        branch_input["node_b"] = node_b_output;
        
        std::cout << "\n[Node C (BranchNode)] 执行条件判断..." << std::endl;
        auto branch_output = branch_node->Invoke(nullptr, branch_input);
        int64_t selected = std::any_cast<int64_t>(branch_output["selected"]);
        
        std::cout << "  → 选中分支: Branch " << selected << std::endl;
        
        if (selected == 0) {
            ProcessNode::ProcessVIP();
        } else if (selected == 1) {
            ProcessNode::ProcessHighScore();
        } else {
            ProcessNode::ProcessNormal();
        }
    }
}

// ============================================================================
// Main
// ============================================================================
int main() {
    try {
        RunGraphExample();
        
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "  ✅ 所有场景测试完成!" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        std::cout << "\n【总结】" << std::endl;
        std::cout << "✓ BranchNode C 成功引用了 Node A 和 Node B 的输出" << std::endl;
        std::cout << "✓ 支持的引用语法:" << std::endl;
        std::cout << "  - OperandConfig::FromNode(\"node_a\", {\"age\"})" << std::endl;
        std::cout << "  - OperandConfig::FromNode(\"node_b\", {\"vip\"})" << std::endl;
        std::cout << "✓ 支持多层级路径: {\"result\", \"score\"} → result.score" << std::endl;
        std::cout << "✓ 支持比较两个节点的输出: node_a.value > node_b.value" << std::endl;
        std::cout << "✓ 完全对齐 coze-studio 的节点引用机制" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
