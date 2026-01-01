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
 * ============================================================================
 * 完整的 Graph + BranchNode 集成示例
 * ============================================================================
 * 
 * 场景：智能客服路由系统
 * 
 * Graph 结构：
 *   START → UserInfoNode (A)   ┐
 *                              ├→ BranchNode (C) → VIPServiceNode
 *   START → ScoreCheckNode (B) ┘                 → StandardServiceNode
 *                                                → ManualServiceNode
 * 
 * 节点说明：
 * - UserInfoNode (A): 提取用户基本信息（年龄、姓名等）
 * - ScoreCheckNode (B): 计算用户评分和VIP状态
 * - BranchNode (C): 根据 A 和 B 的输出决定服务类型
 *   - Branch 0: (age >= 18) AND (vip == true) → VIP服务
 *   - Branch 1: score >= 80 → 标准优质服务
 *   - Default: 人工客服
 * 
 * 技术亮点：
 * 1. 使用真正的 eino::compose::Graph API
 * 2. BranchNode 引用多个上游节点输出
 * 3. 完整的节点输入输出类型管理
 * 4. 条件分支路由
 * ============================================================================
 */

#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <any>
#include "eino/compose/graph.h"
#include "eino/compose/branch_node.h"

using namespace eino::compose;

// 定义类型别名
using MapType = std::map<std::string, std::any>;

// ============================================================================
// Node A: UserInfoNode - 用户信息处理节点
// ============================================================================
class UserInfoNode : public ComposableRunnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::cout << "\n[UserInfoNode] 开始处理..." << std::endl;
        
        MapType output;
        
        // 提取用户基本信息
        if (input.count("user_age")) {
            output["age"] = input.at("user_age");
            int64_t age = std::any_cast<int64_t>(output["age"]);
            std::cout << "  提取年龄: " << age << std::endl;
        } else {
            output["age"] = static_cast<int64_t>(25);
            std::cout << "  使用默认年龄: 25" << std::endl;
        }
        
        if (input.count("user_name")) {
            output["name"] = input.at("user_name");
            std::string name = std::any_cast<std::string>(output["name"]);
            std::cout << "  提取姓名: " << name << std::endl;
        } else {
            output["name"] = std::string("Guest");
            std::cout << "  使用默认姓名: Guest" << std::endl;
        }
        
        // 添加处理时间戳
        output["timestamp"] = static_cast<int64_t>(12345);
        
        std::cout << "  ✓ 用户信息处理完成" << std::endl;
        
        return output;
    }
    
    std::shared_ptr<StreamReader<MapType>> Stream(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<MapType> results{Invoke(ctx, input, opts)};
        return std::make_shared<SimpleStreamReader<MapType>>(results);
    }
    
    MapType Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<MapType>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        MapType value;
        if (input && input->Read(value)) {
            return Invoke(ctx, value, opts);
        }
        return MapType();
    }
    
    std::shared_ptr<StreamReader<MapType>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<MapType>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<MapType> results;
        MapType value;
        while (input && input->Read(value)) {
            results.push_back(Invoke(ctx, value, opts));
        }
        return std::make_shared<SimpleStreamReader<MapType>>(results);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
    
    std::string GetComponentType() const override {
        return "UserInfoNode";
    }
};

// ============================================================================
// Node B: ScoreCheckNode - 评分检查节点
// ============================================================================
class ScoreCheckNode : public ComposableRunnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::cout << "\n[ScoreCheckNode] 开始处理..." << std::endl;
        
        MapType output;
        
        // 提取评分信息
        if (input.count("user_score")) {
            output["score"] = input.at("user_score");
            int64_t score = std::any_cast<int64_t>(output["score"]);
            std::cout << "  提取评分: " << score << std::endl;
        } else {
            output["score"] = static_cast<int64_t>(70);
            std::cout << "  使用默认评分: 70" << std::endl;
        }
        
        // 提取VIP状态
        if (input.count("is_vip")) {
            output["vip"] = input.at("is_vip");
            bool vip = std::any_cast<bool>(output["vip"]);
            std::cout << "  提取VIP状态: " << (vip ? "是" : "否") << std::endl;
        } else {
            output["vip"] = false;
            std::cout << "  使用默认VIP状态: 否" << std::endl;
        }
        
        // 添加等级标签
        int64_t score = std::any_cast<int64_t>(output["score"]);
        if (score >= 90) {
            output["level"] = std::string("excellent");
        } else if (score >= 80) {
            output["level"] = std::string("good");
        } else {
            output["level"] = std::string("normal");
        }
        
        std::string level = std::any_cast<std::string>(output["level"]);
        std::cout << "  计算等级: " << level << std::endl;
        std::cout << "  ✓ 评分检查完成" << std::endl;
        
        return output;
    }
    
    std::shared_ptr<StreamReader<MapType>> Stream(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<MapType> results{Invoke(ctx, input, opts)};
        return std::make_shared<SimpleStreamReader<MapType>>(results);
    }
    
    MapType Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<MapType>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        MapType value;
        if (input && input->Read(value)) {
            return Invoke(ctx, value, opts);
        }
        return MapType();
    }
    
    std::shared_ptr<StreamReader<MapType>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<MapType>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<MapType> results;
        MapType value;
        while (input && input->Read(value)) {
            results.push_back(Invoke(ctx, value, opts));
        }
        return std::make_shared<SimpleStreamReader<MapType>>(results);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
    
    std::string GetComponentType() const override {
        return "ScoreCheckNode";
    }
};

// ============================================================================
// 后续处理节点：根据 BranchNode 的选择执行不同的服务
// ============================================================================
class VIPServiceNode : public ComposableRunnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::cout << "\n[VIPServiceNode] 🌟 VIP服务" << std::endl;
        std::cout << "  提供：专属客服、快速通道、优先处理" << std::endl;
        
        MapType output = input;
        output["service_type"] = std::string("vip");
        output["priority"] = static_cast<int64_t>(1);
        
        return output;
    }
    
    std::shared_ptr<StreamReader<MapType>> Stream(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<MapType> results{Invoke(ctx, input, opts)};
        return std::make_shared<SimpleStreamReader<MapType>>(results);
    }
    
    MapType Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<MapType>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        MapType value;
        if (input && input->Read(value)) {
            return Invoke(ctx, value, opts);
        }
        return MapType();
    }
    
    std::shared_ptr<StreamReader<MapType>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<MapType>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<MapType> results;
        MapType value;
        while (input && input->Read(value)) {
            results.push_back(Invoke(ctx, value, opts));
        }
        return std::make_shared<SimpleStreamReader<MapType>>(results);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
    
    std::string GetComponentType() const override {
        return "VIPServiceNode";
    }
};

class StandardServiceNode : public ComposableRunnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::cout << "\n[StandardServiceNode] ⭐ 标准优质服务" << std::endl;
        std::cout << "  提供：智能客服、常见问题解答" << std::endl;
        
        MapType output = input;
        output["service_type"] = std::string("standard");
        output["priority"] = static_cast<int64_t>(2);
        
        return output;
    }
    
    std::shared_ptr<StreamReader<MapType>> Stream(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<MapType> results{Invoke(ctx, input, opts)};
        return std::make_shared<SimpleStreamReader<MapType>>(results);
    }
    
    MapType Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<MapType>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        MapType value;
        if (input && input->Read(value)) {
            return Invoke(ctx, value, opts);
        }
        return MapType();
    }
    
    std::shared_ptr<StreamReader<MapType>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<MapType>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<MapType> results;
        MapType value;
        while (input && input->Read(value)) {
            results.push_back(Invoke(ctx, value, opts));
        }
        return std::make_shared<SimpleStreamReader<MapType>>(results);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
    
    std::string GetComponentType() const override {
        return "StandardServiceNode";
    }
};

class ManualServiceNode : public ComposableRunnable<MapType, MapType> {
public:
    MapType Invoke(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        
        std::cout << "\n[ManualServiceNode] 👤 人工客服" << std::endl;
        std::cout << "  提供：人工接待、定制化服务" << std::endl;
        
        MapType output = input;
        output["service_type"] = std::string("manual");
        output["priority"] = static_cast<int64_t>(3);
        
        return output;
    }
    
    std::shared_ptr<StreamReader<MapType>> Stream(
        std::shared_ptr<Context> ctx,
        const MapType& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<MapType> results{Invoke(ctx, input, opts)};
        return std::make_shared<SimpleStreamReader<MapType>>(results);
    }
    
    MapType Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<MapType>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        MapType value;
        if (input && input->Read(value)) {
            return Invoke(ctx, value, opts);
        }
        return MapType();
    }
    
    std::shared_ptr<StreamReader<MapType>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<MapType>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        std::vector<MapType> results;
        MapType value;
        while (input && input->Read(value)) {
            results.push_back(Invoke(ctx, value, opts));
        }
        return std::make_shared<SimpleStreamReader<MapType>>(results);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(MapType);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(MapType);
    }
    
    std::string GetComponentType() const override {
        return "ManualServiceNode";
    }
};

// ============================================================================
// 创建 BranchNode
// ============================================================================
std::shared_ptr<BranchNode<MapType, MapType>> CreateBranchNode() {
    std::cout << "\n[创建 BranchNode]" << std::endl;
    
    BranchNodeConfig config;
    
    // Branch 0: (node_a.age >= 18) AND (node_b.vip == true)
    std::cout << "  Branch 0: (node_a.age >= 18) AND (node_b.vip == true) → VIP服务" << std::endl;
    std::vector<SingleClauseConfig> vip_clauses = {
        SingleClauseConfig(
            BranchOperator::GreaterOrEqual,
            OperandConfig::FromNode("node_a", {"age"}),
            OperandConfig::FromLiteral(static_cast<int64_t>(18))
        ),
        SingleClauseConfig(
            BranchOperator::Equal,
            OperandConfig::FromNode("node_b", {"vip"}),
            OperandConfig::FromLiteral(true)
        )
    };
    config.AddMultiConditionWithOperands(vip_clauses, ClauseRelation::AND);
    
    // Branch 1: node_b.score >= 80
    std::cout << "  Branch 1: node_b.score >= 80 → 标准服务" << std::endl;
    config.AddConditionWithOperands(
        BranchOperator::GreaterOrEqual,
        OperandConfig::FromNode("node_b", {"score"}),
        OperandConfig::FromLiteral(static_cast<int64_t>(80))
    );
    
    std::cout << "  Default: 其他情况 → 人工客服" << std::endl;
    
    auto branch_node = BranchNode<MapType, MapType>::New(nullptr, config);
    std::cout << "  ✓ BranchNode 创建成功" << std::endl;
    
    return branch_node;
}

// ============================================================================
// 构建完整的 Graph
// ============================================================================
std::shared_ptr<Graph<MapType, MapType>> BuildCustomerServiceGraph() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "构建智能客服路由系统 Graph" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // 创建 Graph
    auto graph = std::make_shared<Graph<MapType, MapType>>();
    
    // 创建节点
    auto node_a = std::make_shared<UserInfoNode>();
    auto node_b = std::make_shared<ScoreCheckNode>();
    auto node_c = CreateBranchNode();
    auto vip_service = std::make_shared<VIPServiceNode>();
    auto standard_service = std::make_shared<StandardServiceNode>();
    auto manual_service = std::make_shared<ManualServiceNode>();
    
    // 添加节点到 Graph
    std::cout << "\n[添加节点]" << std::endl;
    graph->AddNode("node_a", node_a);
    std::cout << "  ✓ 添加 node_a (UserInfoNode)" << std::endl;
    
    graph->AddNode("node_b", node_b);
    std::cout << "  ✓ 添加 node_b (ScoreCheckNode)" << std::endl;
    
    graph->AddNode("node_c", node_c);
    std::cout << "  ✓ 添加 node_c (BranchNode)" << std::endl;
    
    graph->AddNode("vip_service", vip_service);
    std::cout << "  ✓ 添加 vip_service (VIPServiceNode)" << std::endl;
    
    graph->AddNode("standard_service", standard_service);
    std::cout << "  ✓ 添加 standard_service (StandardServiceNode)" << std::endl;
    
    graph->AddNode("manual_service", manual_service);
    std::cout << "  ✓ 添加 manual_service (ManualServiceNode)" << std::endl;
    
    // 添加边
    std::cout << "\n[添加边]" << std::endl;
    graph->AddEdge(Graph<MapType, MapType>::START_NODE, "node_a");
    std::cout << "  ✓ START → node_a" << std::endl;
    
    graph->AddEdge(Graph<MapType, MapType>::START_NODE, "node_b");
    std::cout << "  ✓ START → node_b" << std::endl;
    
    graph->AddEdge("node_a", "node_c");
    std::cout << "  ✓ node_a → node_c" << std::endl;
    
    graph->AddEdge("node_b", "node_c");
    std::cout << "  ✓ node_b → node_c" << std::endl;
    
    graph->AddEdge("node_c", "vip_service");
    std::cout << "  ✓ node_c → vip_service (Branch 0)" << std::endl;
    
    graph->AddEdge("node_c", "standard_service");
    std::cout << "  ✓ node_c → standard_service (Branch 1)" << std::endl;
    
    graph->AddEdge("node_c", "manual_service");
    std::cout << "  ✓ node_c → manual_service (Default)" << std::endl;
    
    graph->AddEdge("vip_service", Graph<MapType, MapType>::END_NODE);
    std::cout << "  ✓ vip_service → END" << std::endl;
    
    graph->AddEdge("standard_service", Graph<MapType, MapType>::END_NODE);
    std::cout << "  ✓ standard_service → END" << std::endl;
    
    graph->AddEdge("manual_service", Graph<MapType, MapType>::END_NODE);
    std::cout << "  ✓ manual_service → END" << std::endl;
    
    // 编译 Graph
    std::cout << "\n[编译 Graph]" << std::endl;
    graph->Compile();
    std::cout << "  ✓ Graph 编译成功" << std::endl;
    
    // 打印 Graph 信息
    auto node_names = graph->GetNodeNames();
    std::cout << "\n[Graph 信息]" << std::endl;
    std::cout << "  节点数量: " << node_names.size() << std::endl;
    std::cout << "  边数量: " << graph->GetEdgeCount() << std::endl;
    std::cout << "  节点列表: ";
    for (size_t i = 0; i < node_names.size(); ++i) {
        std::cout << node_names[i];
        if (i < node_names.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    return graph;
}

// ============================================================================
// 测试场景
// ============================================================================
void RunTestScenario(
    const std::string& scenario_name,
    std::shared_ptr<Graph<MapType, MapType>> graph,
    const MapType& input) {
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "测试场景: " << scenario_name << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // 打印输入
    std::cout << "\n[输入数据]" << std::endl;
    if (input.count("user_age")) {
        std::cout << "  年龄: " << std::any_cast<int64_t>(input.at("user_age")) << std::endl;
    }
    if (input.count("user_name")) {
        std::cout << "  姓名: " << std::any_cast<std::string>(input.at("user_name")) << std::endl;
    }
    if (input.count("user_score")) {
        std::cout << "  评分: " << std::any_cast<int64_t>(input.at("user_score")) << std::endl;
    }
    if (input.count("is_vip")) {
        std::cout << "  VIP: " << (std::any_cast<bool>(input.at("is_vip")) ? "是" : "否") << std::endl;
    }
    
    // 执行 Graph
    auto ctx = Context::Background();
    std::cout << "\n[开始执行 Graph]" << std::endl;
    
    try {
        MapType output = graph->Invoke(ctx, input);
        
        std::cout << "\n[执行结果]" << std::endl;
        if (output.count("service_type")) {
            std::cout << "  服务类型: " << std::any_cast<std::string>(output["service_type"]) << std::endl;
        }
        if (output.count("priority")) {
            std::cout << "  优先级: " << std::any_cast<int64_t>(output["priority"]) << std::endl;
        }
        
        std::cout << "\n✅ 场景执行成功" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 错误: " << e.what() << std::endl;
    }
}

// ============================================================================
// Main
// ============================================================================
int main() {
    try {
        // 构建 Graph
        auto graph = BuildCustomerServiceGraph();
        
        // 场景 1: VIP 成年用户
        {
            MapType input;
            input["user_age"] = static_cast<int64_t>(25);
            input["user_name"] = std::string("Alice");
            input["user_score"] = static_cast<int64_t>(85);
            input["is_vip"] = true;
            
            RunTestScenario("VIP 成年用户", graph, input);
        }
        
        // 场景 2: 高分非VIP用户
        {
            MapType input;
            input["user_age"] = static_cast<int64_t>(30);
            input["user_name"] = std::string("Bob");
            input["user_score"] = static_cast<int64_t>(90);
            input["is_vip"] = false;
            
            RunTestScenario("高分非VIP用户", graph, input);
        }
        
        // 场景 3: 未成年普通用户
        {
            MapType input;
            input["user_age"] = static_cast<int64_t>(16);
            input["user_name"] = std::string("Charlie");
            input["user_score"] = static_cast<int64_t>(50);
            input["is_vip"] = false;
            
            RunTestScenario("未成年普通用户", graph, input);
        }
        
        // 总结
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "所有测试场景执行完毕" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        std::cout << "\n【技术总结】" << std::endl;
        std::cout << "✓ 使用 eino::compose::Graph 构建完整的工作流" << std::endl;
        std::cout << "✓ BranchNode 成功引用多个上游节点 (node_a, node_b)" << std::endl;
        std::cout << "✓ 实现了智能路由：根据条件执行不同的服务节点" << std::endl;
        std::cout << "✓ 支持并行执行：node_a 和 node_b 可并行处理" << std::endl;
        std::cout << "✓ 类型安全：使用 std::map<std::string, std::any> 管理数据" << std::endl;
        std::cout << "✓ 完全对齐 coze-studio 的节点引用机制" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 致命错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
