/*
 * BranchNode 简单工作示例 (兼容 C++14)
 * 
 * 场景：用户订单路由系统
 * 流程：订单信息提取 -> 用户信息查询 -> 智能路由决策 -> 不同服务处理
 * 
 * 图结构：
 *   START -> order_processor -> user_lookup -> smart_router (BranchNode)
 *                                               ├─[0]→ vip_service -> END
 *                                               ├─[1]→ regular_service -> END
 *                                               └─[2]→ new_user_service -> END
 */

#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include "../include/eino/compose/graph.h"
#include "../include/eino/compose/branch_node.h"
#include "../include/eino/compose/runnable.h"

using namespace eino::compose;

// 简单的数据结构替代 std::any
struct OrderData {
    std::string order_id;
    std::string user_id;
    double amount;
    int vip_level;
    int member_years;
    std::string service_type;
    double discount;
    std::string priority;
    bool welcome_gift;
    
    OrderData() : amount(0), vip_level(0), member_years(0), 
                  discount(1.0), welcome_gift(false) {}
};

// 为了兼容 BranchNode，我们需要将 OrderData 转换为 map<string, any>
// 但由于没有 any，我们使用 nlohmann::json 代替
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// ==================== 自定义 Runnable 节点 ====================

// 订单处理器：提取订单金额
class OrderProcessor : public Runnable<json, json> {
public:
    json Invoke(
        std::shared_ptr<Context> ctx,
        const json& input,
        const std::vector<Option>& opts = {}) override {
        
        std::cout << "\n[OrderProcessor] 处理订单..." << std::endl;
        
        json output = input;
        
        if (input.contains("order_id")) {
            std::cout << "  订单ID: " << input["order_id"].get<std::string>() << std::endl;
        }
        
        if (input.contains("amount")) {
            std::cout << "  订单金额: " << input["amount"].get<double>() << std::endl;
        }
        
        return output;
    }
    
    std::string GetComponentType() const override {
        return "OrderProcessor";
    }
};

// 用户查询器：查询用户信息
class UserLookup : public Runnable<json, json> {
public:
    json Invoke(
        std::shared_ptr<Context> ctx,
        const json& input,
        const std::vector<Option>& opts = {}) override {
        
        std::cout << "\n[UserLookup] 查询用户信息..." << std::endl;
        
        json output = input;
        
        if (input.contains("user_id")) {
            std::string user_id = input["user_id"].get<std::string>();
            std::cout << "  用户ID: " << user_id << std::endl;
            
            // 模拟数据库查询
            if (user_id == "user_vip") {
                output["vip_level"] = 3;
                output["member_years"] = 5;
                std::cout << "  VIP等级: 3, 会员年限: 5年" << std::endl;
            } else if (user_id == "user_regular") {
                output["vip_level"] = 0;
                output["member_years"] = 2;
                std::cout << "  VIP等级: 0, 会员年限: 2年" << std::endl;
            } else {
                output["vip_level"] = 0;
                output["member_years"] = 0;
                std::cout << "  新用户" << std::endl;
            }
        }
        
        return output;
    }
    
    std::string GetComponentType() const override {
        return "UserLookup";
    }
};

// VIP 服务处理器
class VIPService : public Runnable<json, json> {
public:
    json Invoke(
        std::shared_ptr<Context> ctx,
        const json& input,
        const std::vector<Option>& opts = {}) override {
        
        std::cout << "\n[VIPService] 🌟 VIP专属服务处理" << std::endl;
        
        json output = input;
        output["service_type"] = "VIP";
        output["discount"] = 0.8;  // 8折
        output["priority"] = "HIGH";
        
        std::cout << "  ✓ 享受8折优惠" << std::endl;
        std::cout << "  ✓ 高优先级处理" << std::endl;
        std::cout << "  ✓ 专属客服" << std::endl;
        
        return output;
    }
    
    std::string GetComponentType() const override {
        return "VIPService";
    }
};

// 普通服务处理器
class RegularService : public Runnable<json, json> {
public:
    json Invoke(
        std::shared_ptr<Context> ctx,
        const json& input,
        const std::vector<Option>& opts = {}) override {
        
        std::cout << "\n[RegularService] 📦 普通服务处理" << std::endl;
        
        json output = input;
        output["service_type"] = "REGULAR";
        output["discount"] = 1.0;  // 无折扣
        output["priority"] = "NORMAL";
        
        std::cout << "  ✓ 标准处理流程" << std::endl;
        
        return output;
    }
    
    std::string GetComponentType() const override {
        return "RegularService";
    }
};

// 新用户服务处理器
class NewUserService : public Runnable<json, json> {
public:
    json Invoke(
        std::shared_ptr<Context> ctx,
        const json& input,
        const std::vector<Option>& opts = {}) override {
        
        std::cout << "\n[NewUserService] 🎁 新用户欢迎服务" << std::endl;
        
        json output = input;
        output["service_type"] = "NEW_USER";
        output["discount"] = 0.9;  // 9折
        output["priority"] = "NORMAL";
        output["welcome_gift"] = true;
        
        std::cout << "  ✓ 新用户专享9折" << std::endl;
        std::cout << "  ✓ 赠送新人礼包" << std::endl;
        
        return output;
    }
    
    std::string GetComponentType() const override {
        return "NewUserService";
    }
};

// ==================== 主程序 ====================

void RunExample(const std::string& scenario_name,
                const std::string& user_id,
                const std::string& order_id,
                double amount) {
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "场景: " << scenario_name << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    try {
        // 1. 创建 Graph
        auto graph = std::make_shared<Graph<json, json>>();
        
        // 2. 添加业务节点
        graph->AddNode("order_processor", std::make_shared<OrderProcessor>());
        graph->AddNode("user_lookup", std::make_shared<UserLookup>());
        graph->AddNode("vip_service", std::make_shared<VIPService>());
        graph->AddNode("regular_service", std::make_shared<RegularService>());
        graph->AddNode("new_user_service", std::make_shared<NewUserService>());
        
        // 3. 配置 BranchNode (智能路由器)
        BranchConfig branch_config;
        
        // 分支 0: VIP用户路由 (vip_level >= 2 且 订单金额 >= 500)
        BranchCase vip_case;
        vip_case.AddConditionWithOperands(
            BranchOperator::GreaterOrEqual,
            OperandConfig::FromNode("user_lookup", {"vip_level"}),
            OperandConfig::FromLiteral(2)
        );
        vip_case.AddConditionWithOperands(
            BranchOperator::GreaterOrEqual,
            OperandConfig::FromNode("order_processor", {"amount"}),
            OperandConfig::FromLiteral(500.0)
        );
        vip_case.set_logic(BranchLogic::And);  // 两个条件都满足
        branch_config.AddCase(vip_case);
        
        // 分支 1: 普通老用户路由 (member_years >= 1)
        BranchCase regular_case;
        regular_case.AddConditionWithOperands(
            BranchOperator::GreaterOrEqual,
            OperandConfig::FromNode("user_lookup", {"member_years"}),
            OperandConfig::FromLiteral(1)
        );
        branch_config.AddCase(regular_case);
        
        // 分支 2 (default): 新用户路由
        branch_config.set_default_branch(2);
        
        // 4. 添加 BranchNode
        auto branch_node = std::make_shared<BranchNode<json>>(branch_config);
        graph->AddNode("smart_router", branch_node);
        
        // 5. 连接节点（数据流）
        graph->AddEdge(START_NODE, "order_processor");
        graph->AddEdge("order_processor", "user_lookup");
        graph->AddEdge("user_lookup", "smart_router");
        
        // 6. 使用 AddBranchEdge 定义分支路由 ✨
        graph->AddBranchEdge("smart_router", 0, "vip_service");       // VIP分支
        graph->AddBranchEdge("smart_router", 1, "regular_service");   // 普通分支
        graph->AddBranchEdge("smart_router", 2, "new_user_service");  // 新用户分支
        
        // 7. 连接到终点
        graph->AddEdge("vip_service", END_NODE);
        graph->AddEdge("regular_service", END_NODE);
        graph->AddEdge("new_user_service", END_NODE);
        
        // 8. 编译图
        std::cout << "\n编译 Graph..." << std::endl;
        auto compile_result = graph->Compile(GraphCompileOptions());
        if (!compile_result.empty()) {
            std::cerr << "编译失败: " << compile_result << std::endl;
            return;
        }
        std::cout << "✓ 编译成功" << std::endl;
        
        // 9. 准备输入数据
        json input;
        input["user_id"] = user_id;
        input["order_id"] = order_id;
        input["amount"] = amount;
        
        std::cout << "\n输入数据:" << std::endl;
        std::cout << "  用户ID: " << user_id << std::endl;
        std::cout << "  订单ID: " << order_id << std::endl;
        std::cout << "  金额: " << amount << std::endl;
        
        // 10. 执行图
        std::cout << "\n开始执行 Graph..." << std::endl;
        auto ctx = Context::Background();
        auto result = graph->Invoke(ctx, input);
        
        // 11. 输出结果
        std::cout << "\n" << std::string(60, '-') << std::endl;
        std::cout << "执行结果:" << std::endl;
        if (result.contains("service_type")) {
            std::cout << "  服务类型: " << result["service_type"].get<std::string>() << std::endl;
        }
        if (result.contains("discount")) {
            std::cout << "  折扣: " << (result["discount"].get<double>() * 100) << "%" << std::endl;
        }
        if (result.contains("priority")) {
            std::cout << "  优先级: " << result["priority"].get<std::string>() << std::endl;
        }
        if (result.contains("welcome_gift") && result["welcome_gift"].get<bool>()) {
            std::cout << "  新人礼包: 是" << std::endl;
        }
        std::cout << std::string(60, '-') << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     BranchNode 多节点引用完整示例 (C++14)                ║" << std::endl;
    std::cout << "║                                                            ║" << std::endl;
    std::cout << "║  图结构: order_processor -> user_lookup -> smart_router  ║" << std::endl;
    std::cout << "║                                              ├─ vip       ║" << std::endl;
    std::cout << "║                                              ├─ regular   ║" << std::endl;
    std::cout << "║                                              └─ new_user  ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    
    // 场景 1: VIP用户大额订单
    RunExample("VIP用户大额订单", "user_vip", "ORDER-001", 1000.0);
    
    // 场景 2: 普通老用户小额订单
    RunExample("普通老用户小额订单", "user_regular", "ORDER-002", 100.0);
    
    // 场景 3: 新用户订单
    RunExample("新用户订单", "user_new", "ORDER-003", 200.0);
    
    std::cout << "\n✅ 所有场景执行完成！\n" << std::endl;
    
    return 0;
}
