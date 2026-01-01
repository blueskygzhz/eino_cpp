/*
 * Copyright 2025 CloudWeGo Authors
 *
 * 复杂条件逻辑的序列化/反序列化解决方案
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>

// 简化的数据结构
namespace eino {
namespace compose {

enum class ConditionOperator {
    EQUAL = 0,
    NOT_EQUAL = 1,
    GREATER_THAN = 2,
    GREATER_EQUAL = 3,
    LESS_THAN = 4,
    LESS_EQUAL = 5,
    CONTAINS = 6,
    STARTS_WITH = 7,
    ENDS_WITH = 8
};

struct ConditionRule {
    std::string field;
    ConditionOperator op;
    std::string value;
    std::string target_node;
    int priority = 0;
};

struct ConditionRuleGroup {
    std::string logic;  // "AND" or "OR"
    std::vector<ConditionRule> rules;
    std::string target_node;
    int priority = 0;
};

} // namespace compose
} // namespace eino

using namespace eino::compose;

// =============================================================================
// 序列化函数
// =============================================================================

std::string EscapeJson(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n"; break;
            default: oss << c; break;
        }
    }
    return oss.str();
}

std::string ConditionOperatorToString(ConditionOperator op) {
    switch (op) {
        case ConditionOperator::EQUAL: return "==";
        case ConditionOperator::NOT_EQUAL: return "!=";
        case ConditionOperator::GREATER_THAN: return ">";
        case ConditionOperator::GREATER_EQUAL: return ">=";
        case ConditionOperator::LESS_THAN: return "<";
        case ConditionOperator::LESS_EQUAL: return "<=";
        case ConditionOperator::CONTAINS: return "contains";
        case ConditionOperator::STARTS_WITH: return "starts_with";
        case ConditionOperator::ENDS_WITH: return "ends_with";
        default: return "unknown";
    }
}

std::string ConditionRuleToJson(const ConditionRule& rule, const std::string& indent = "        ") {
    std::ostringstream oss;
    oss << indent << "{\n";
    oss << indent << "  \"field\": \"" << EscapeJson(rule.field) << "\",\n";
    oss << indent << "  \"operator\": \"" << ConditionOperatorToString(rule.op) << "\",\n";
    oss << indent << "  \"value\": \"" << EscapeJson(rule.value) << "\",\n";
    oss << indent << "  \"target_node\": \"" << EscapeJson(rule.target_node) << "\",\n";
    oss << indent << "  \"priority\": " << rule.priority << "\n";
    oss << indent << "}";
    return oss.str();
}

std::string ConditionRuleGroupToJson(const ConditionRuleGroup& group, const std::string& indent = "    ") {
    std::ostringstream oss;
    oss << indent << "{\n";
    oss << indent << "  \"logic\": \"" << group.logic << "\",\n";
    oss << indent << "  \"target_node\": \"" << group.target_node << "\",\n";
    oss << indent << "  \"priority\": " << group.priority << ",\n";
    oss << indent << "  \"rules\": [\n";
    
    for (size_t i = 0; i < group.rules.size(); ++i) {
        if (i > 0) oss << ",\n";
        oss << ConditionRuleToJson(group.rules[i]);
    }
    
    oss << "\n" << indent << "  ]\n";
    oss << indent << "}";
    return oss.str();
}

// =============================================================================
// 模拟输入数据结构
// =============================================================================

struct UserRequest {
    std::string text;
    double confidence_score;
    int user_level;
    int history_count;
    std::string user_type;
};

// =============================================================================
// 示例 1: 简单规则（单个条件）
// =============================================================================

void Example1_SimpleRule() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "示例 1: 简单规则 - 基于置信度分数路由\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "❌ 原始代码（不可序列化）:\n";
    std::cout << "   auto condition = [](Context* ctx, const Input& input) {\n";
    std::cout << "       if (input.confidence_score > 0.8) {\n";
    std::cout << "           return \"high_confidence_handler\";\n";
    std::cout << "       }\n";
    std::cout << "       return \"low_confidence_handler\";\n";
    std::cout << "   };\n\n";
    
    std::cout << "✅ 转换为规则表示:\n\n";
    
    ConditionRuleGroup rule_group;
    rule_group.logic = "AND";
    rule_group.target_node = "high_confidence_handler";
    rule_group.priority = 10;
    
    ConditionRule rule;
    rule.field = "confidence_score";
    rule.op = ConditionOperator::GREATER_THAN;
    rule.value = "0.8";
    rule.target_node = "high_confidence_handler";
    rule.priority = 10;
    
    rule_group.rules.push_back(rule);
    
    std::string json = ConditionRuleGroupToJson(rule_group);
    std::cout << "JSON 表示:\n" << json << "\n\n";
    
    std::cout << "📋 规则说明:\n";
    std::cout << "   - 字段: confidence_score\n";
    std::cout << "   - 操作: > 0.8\n";
    std::cout << "   - 目标: high_confidence_handler\n";
    std::cout << "   - 默认: low_confidence_handler（未匹配时）\n\n";
}

// =============================================================================
// 示例 2: 复合条件（AND 逻辑）
// =============================================================================

void Example2_AndCondition() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "示例 2: 复合条件 - AND 逻辑\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "❌ 原始代码:\n";
    std::cout << "   auto condition = [](Context* ctx, const Input& input) {\n";
    std::cout << "       if (input.confidence_score > 0.8 && input.user_level >= 5) {\n";
    std::cout << "           return \"vip_handler\";\n";
    std::cout << "       }\n";
    std::cout << "       return \"normal_handler\";\n";
    std::cout << "   };\n\n";
    
    std::cout << "✅ 转换为规则表示:\n\n";
    
    ConditionRuleGroup rule_group;
    rule_group.logic = "AND";  // 所有规则都必须满足
    rule_group.target_node = "vip_handler";
    rule_group.priority = 20;
    
    // 规则 1: score > 0.8
    ConditionRule rule1;
    rule1.field = "confidence_score";
    rule1.op = ConditionOperator::GREATER_THAN;
    rule1.value = "0.8";
    rule1.target_node = "vip_handler";
    
    // 规则 2: user_level >= 5
    ConditionRule rule2;
    rule2.field = "user_level";
    rule2.op = ConditionOperator::GREATER_EQUAL;
    rule2.value = "5";
    rule2.target_node = "vip_handler";
    
    rule_group.rules.push_back(rule1);
    rule_group.rules.push_back(rule2);
    
    std::string json = ConditionRuleGroupToJson(rule_group);
    std::cout << "JSON 表示:\n" << json << "\n\n";
    
    std::cout << "📋 规则说明:\n";
    std::cout << "   逻辑: AND（所有条件必须满足）\n";
    std::cout << "   - confidence_score > 0.8\n";
    std::cout << "   - user_level >= 5\n";
    std::cout << "   → vip_handler\n\n";
}

// =============================================================================
// 示例 3: 复合条件（OR 逻辑）
// =============================================================================

void Example3_OrCondition() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "示例 3: 复合条件 - OR 逻辑\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "❌ 原始代码:\n";
    std::cout << "   auto condition = [](Context* ctx, const Input& input) {\n";
    std::cout << "       if (input.confidence_score > 0.5 || input.history_count > 100) {\n";
    std::cout << "           return \"experienced_handler\";\n";
    std::cout << "       }\n";
    std::cout << "       return \"newbie_handler\";\n";
    std::cout << "   };\n\n";
    
    std::cout << "✅ 转换为规则表示:\n\n";
    
    ConditionRuleGroup rule_group;
    rule_group.logic = "OR";  // 任意规则满足即可
    rule_group.target_node = "experienced_handler";
    rule_group.priority = 15;
    
    ConditionRule rule1;
    rule1.field = "confidence_score";
    rule1.op = ConditionOperator::GREATER_THAN;
    rule1.value = "0.5";
    rule1.target_node = "experienced_handler";
    
    ConditionRule rule2;
    rule2.field = "history_count";
    rule2.op = ConditionOperator::GREATER_THAN;
    rule2.value = "100";
    rule2.target_node = "experienced_handler";
    
    rule_group.rules.push_back(rule1);
    rule_group.rules.push_back(rule2);
    
    std::string json = ConditionRuleGroupToJson(rule_group);
    std::cout << "JSON 表示:\n" << json << "\n\n";
    
    std::cout << "📋 规则说明:\n";
    std::cout << "   逻辑: OR（任意条件满足即可）\n";
    std::cout << "   - confidence_score > 0.5\n";
    std::cout << "   OR\n";
    std::cout << "   - history_count > 100\n";
    std::cout << "   → experienced_handler\n\n";
}

// =============================================================================
// 示例 4: 多分支复杂条件（优先级）
// =============================================================================

void Example4_MultiBranchWithPriority() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "示例 4: 多分支复杂条件（带优先级）\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "❌ 原始代码:\n";
    std::cout << "   auto condition = [](Context* ctx, const Input& input) {\n";
    std::cout << "       // 优先级 1: VIP 用户 + 高分\n";
    std::cout << "       if (input.user_level >= 10 && input.confidence_score > 0.9) {\n";
    std::cout << "           return \"premium_handler\";\n";
    std::cout << "       }\n";
    std::cout << "       // 优先级 2: 高分用户\n";
    std::cout << "       if (input.confidence_score > 0.8) {\n";
    std::cout << "           return \"high_quality_handler\";\n";
    std::cout << "       }\n";
    std::cout << "       // 优先级 3: 新用户\n";
    std::cout << "       if (input.history_count < 10) {\n";
    std::cout << "           return \"newbie_handler\";\n";
    std::cout << "       }\n";
    std::cout << "       return \"default_handler\";\n";
    std::cout << "   };\n\n";
    
    std::cout << "✅ 转换为规则表示:\n\n";
    
    std::vector<ConditionRuleGroup> rule_groups;
    
    // 规则组 1: Premium 用户（优先级最高）
    {
        ConditionRuleGroup group;
        group.logic = "AND";
        group.target_node = "premium_handler";
        group.priority = 100;  // 最高优先级
        
        ConditionRule rule1;
        rule1.field = "user_level";
        rule1.op = ConditionOperator::GREATER_EQUAL;
        rule1.value = "10";
        
        ConditionRule rule2;
        rule2.field = "confidence_score";
        rule2.op = ConditionOperator::GREATER_THAN;
        rule2.value = "0.9";
        
        group.rules.push_back(rule1);
        group.rules.push_back(rule2);
        rule_groups.push_back(group);
    }
    
    // 规则组 2: 高质量用户
    {
        ConditionRuleGroup group;
        group.logic = "AND";
        group.target_node = "high_quality_handler";
        group.priority = 50;
        
        ConditionRule rule;
        rule.field = "confidence_score";
        rule.op = ConditionOperator::GREATER_THAN;
        rule.value = "0.8";
        
        group.rules.push_back(rule);
        rule_groups.push_back(group);
    }
    
    // 规则组 3: 新用户
    {
        ConditionRuleGroup group;
        group.logic = "AND";
        group.target_node = "newbie_handler";
        group.priority = 30;
        
        ConditionRule rule;
        rule.field = "history_count";
        rule.op = ConditionOperator::LESS_THAN;
        rule.value = "10";
        
        group.rules.push_back(rule);
        rule_groups.push_back(group);
    }
    
    std::ostringstream json_oss;
    json_oss << "{\n";
    json_oss << "  \"condition_type\": \"rule_based\",\n";
    json_oss << "  \"default_target\": \"default_handler\",\n";
    json_oss << "  \"rule_groups\": [\n";
    
    for (size_t i = 0; i < rule_groups.size(); ++i) {
        if (i > 0) json_oss << ",\n";
        json_oss << ConditionRuleGroupToJson(rule_groups[i]);
    }
    
    json_oss << "\n  ]\n";
    json_oss << "}";
    
    std::string json = json_oss.str();
    std::cout << "JSON 表示:\n" << json << "\n\n";
    
    std::cout << "📋 规则执行顺序（按优先级）:\n";
    std::cout << "   1. [优先级 100] Premium 用户检查\n";
    std::cout << "      条件: user_level >= 10 AND confidence_score > 0.9\n";
    std::cout << "      → premium_handler\n\n";
    
    std::cout << "   2. [优先级 50] 高质量用户检查\n";
    std::cout << "      条件: confidence_score > 0.8\n";
    std::cout << "      → high_quality_handler\n\n";
    
    std::cout << "   3. [优先级 30] 新用户检查\n";
    std::cout << "      条件: history_count < 10\n";
    std::cout << "      → newbie_handler\n\n";
    
    std::cout << "   4. 默认: default_handler\n\n";
    
    // 保存到文件
    std::string filepath = "/tmp/complex_condition_rules.json";
    std::ofstream file(filepath);
    file << json;
    file.close();
    
    std::cout << "💾 已保存到: " << filepath << "\n";
}

// =============================================================================
// 示例 5: 字符串匹配条件
// =============================================================================

void Example5_StringMatching() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "示例 5: 字符串匹配条件\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "❌ 原始代码:\n";
    std::cout << "   auto condition = [](Context* ctx, const Input& input) {\n";
    std::cout << "       if (input.text.find(\"weather\") != std::string::npos) {\n";
    std::cout << "           return \"weather_handler\";\n";
    std::cout << "       }\n";
    std::cout << "       if (input.user_type.starts_with(\"vip\")) {\n";
    std::cout << "           return \"vip_handler\";\n";
    std::cout << "       }\n";
    std::cout << "       return \"default_handler\";\n";
    std::cout << "   };\n\n";
    
    std::cout << "✅ 转换为规则表示:\n\n";
    
    std::vector<ConditionRuleGroup> rule_groups;
    
    // 规则组 1: 包含关键词
    {
        ConditionRuleGroup group;
        group.logic = "AND";
        group.target_node = "weather_handler";
        group.priority = 20;
        
        ConditionRule rule;
        rule.field = "text";
        rule.op = ConditionOperator::CONTAINS;
        rule.value = "weather";
        
        group.rules.push_back(rule);
        rule_groups.push_back(group);
    }
    
    // 规则组 2: 前缀匹配
    {
        ConditionRuleGroup group;
        group.logic = "AND";
        group.target_node = "vip_handler";
        group.priority = 15;
        
        ConditionRule rule;
        rule.field = "user_type";
        rule.op = ConditionOperator::STARTS_WITH;
        rule.value = "vip";
        
        group.rules.push_back(rule);
        rule_groups.push_back(group);
    }
    
    std::ostringstream json_oss;
    json_oss << "{\n";
    json_oss << "  \"rule_groups\": [\n";
    
    for (size_t i = 0; i < rule_groups.size(); ++i) {
        if (i > 0) json_oss << ",\n";
        json_oss << ConditionRuleGroupToJson(rule_groups[i]);
    }
    
    json_oss << "\n  ]\n";
    json_oss << "}";
    
    std::cout << "JSON 表示:\n" << json_oss.str() << "\n\n";
    
    std::cout << "📋 支持的字符串操作:\n";
    std::cout << "   - CONTAINS: 包含子串\n";
    std::cout << "   - STARTS_WITH: 前缀匹配\n";
    std::cout << "   - ENDS_WITH: 后缀匹配\n";
    std::cout << "   - EQUAL: 精确匹配\n";
    std::cout << "   - NOT_EQUAL: 不等于\n\n";
}

// =============================================================================
// 示例 6: 方案对比总结
// =============================================================================

void Example6_SolutionComparison() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "示例 6: 三种方案对比\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "📊 方案对比:\n\n";
    
    std::cout << "┌────────────────────┬──────────────┬──────────┬────────────┐\n";
    std::cout << "│ 方案               │ 灵活性       │ 复杂度   │ 适用场景   │\n";
    std::cout << "├────────────────────┼──────────────┼──────────┼────────────┤\n";
    std::cout << "│ 1. 规则引擎        │ ★★★★☆       │ ★★☆☆☆   │ 中等复杂度 │\n";
    std::cout << "│ (Rule-Based)       │ 可配置化     │ 易实现   │ 业务规则   │\n";
    std::cout << "├────────────────────┼──────────────┼──────────┼────────────┤\n";
    std::cout << "│ 2. 脚本嵌入        │ ★★★★★       │ ★★★★☆   │ 高度动态   │\n";
    std::cout << "│ (Lua/JS/Python)    │ 完全自由     │ 需依赖   │ 复杂逻辑   │\n";
    std::cout << "├────────────────────┼──────────────┼──────────┼────────────┤\n";
    std::cout << "│ 3. 表达式树        │ ★★★★☆       │ ★★★☆☆   │ 数学表达式 │\n";
    std::cout << "│ (Expression Tree)  │ 递归计算     │ 中等     │ 计算密集   │\n";
    std::cout << "└────────────────────┴──────────────┴──────────┴────────────┘\n\n";
    
    std::cout << "💡 推荐选择:\n\n";
    
    std::cout << "1️⃣  **规则引擎（推荐 80% 场景）**\n";
    std::cout << "   ✅ 优点:\n";
    std::cout << "      - 零依赖，纯 C++ 实现\n";
    std::cout << "      - 易于理解和维护\n";
    std::cout << "      - JSON 格式清晰可读\n";
    std::cout << "      - 支持优先级排序\n";
    std::cout << "      - 支持 AND/OR 组合\n";
    std::cout << "   ❌ 限制:\n";
    std::cout << "      - 无法表达任意复杂逻辑\n";
    std::cout << "      - 需要预定义字段和操作符\n\n";
    
    std::cout << "2️⃣  **脚本嵌入（适用于极端复杂场景）**\n";
    std::cout << "   ✅ 优点:\n";
    std::cout << "      - 完全灵活，支持任意逻辑\n";
    std::cout << "      - 可以在运行时修改\n";
    std::cout << "      - 支持复杂数学计算\n";
    std::cout << "   ❌ 限制:\n";
    std::cout << "      - 需要嵌入脚本引擎（Lua/V8）\n";
    std::cout << "      - 性能略低\n";
    std::cout << "      - 安全风险（需要沙箱）\n\n";
    
    std::cout << "3️⃣  **表达式树（适用于数学密集场景）**\n";
    std::cout << "   ✅ 优点:\n";
    std::cout << "      - 支持复杂数学表达式\n";
    std::cout << "      - 可以优化计算\n";
    std::cout << "      - 结构化存储\n";
    std::cout << "   ❌ 限制:\n";
    std::cout << "      - 实现复杂\n";
    std::cout << "      - 主要适用于数学计算\n\n";
}

// =============================================================================
// 示例 7: 实际反序列化代码
// =============================================================================

void Example7_DeserializationCode() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "示例 7: 反序列化实现代码\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "📝 完整的反序列化流程:\n\n";
    
    std::cout << "// Step 1: 定义值提取器\n";
    std::cout << "auto value_extractor = [](const UserRequest& req, const std::string& field) {\n";
    std::cout << "    if (field == \"confidence_score\") {\n";
    std::cout << "        return std::to_string(req.confidence_score);\n";
    std::cout << "    } else if (field == \"user_level\") {\n";
    std::cout << "        return std::to_string(req.user_level);\n";
    std::cout << "    } else if (field == \"history_count\") {\n";
    std::cout << "        return std::to_string(req.history_count);\n";
    std::cout << "    } else if (field == \"text\") {\n";
    std::cout << "        return req.text;\n";
    std::cout << "    } else if (field == \"user_type\") {\n";
    std::cout << "        return req.user_type;\n";
    std::cout << "    }\n";
    std::cout << "    return \"\";\n";
    std::cout << "};\n\n";
    
    std::cout << "// Step 2: 创建规则引擎\n";
    std::cout << "RuleBasedConditionEngine<UserRequest> engine;\n";
    std::cout << "engine.SetValueExtractor(value_extractor);\n\n";
    
    std::cout << "// Step 3: 从 JSON 加载规则组\n";
    std::cout << "auto json = LoadJsonFile(\"complex_condition_rules.json\");\n";
    std::cout << "std::vector<ConditionRuleGroup> rule_groups;\n";
    std::cout << "for (const auto& group_json : json[\"rule_groups\"]) {\n";
    std::cout << "    rule_groups.push_back(ConditionRuleGroupFromJson(group_json));\n";
    std::cout << "}\n\n";
    
    std::cout << "// Step 4: 创建条件函数\n";
    std::cout << "auto condition_func = engine.CreateConditionFromRules(\n";
    std::cout << "    rule_groups,\n";
    std::cout << "    \"default_handler\"  // 默认目标\n";
    std::cout << ");\n\n";
    
    std::cout << "// Step 5: 使用条件函数创建 Branch\n";
    std::cout << "auto branch = NewGraphBranch(condition_func, end_nodes);\n\n";
    
    std::cout << "// Step 6: 添加到 Graph\n";
    std::cout << "graph->AddBranch(\"intelligent_router\", branch);\n\n";
    
    std::cout << "✅ 完成！现在 Graph 可以根据 JSON 定义的规则进行动态路由\n\n";
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   复杂条件逻辑的序列化/反序列化完整解决方案                   ║\n";
    std::cout << "║   解决 Branch 中包含复杂代码逻辑的序列化问题                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
    
    try {
        Example1_SimpleRule();
        Example2_AndCondition();
        Example3_OrCondition();
        Example4_MultiBranchWithPriority();
        Example5_StringMatching();
        Example6_SolutionComparison();
        Example7_DeserializationCode();
        
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "✅ 所有示例运行完成！\n";
        std::cout << std::string(70, '=') << "\n\n";
        
        std::cout << "📚 核心要点:\n\n";
        
        std::cout << "1️⃣  将代码逻辑转换为数据结构\n";
        std::cout << "   ✅ if (score > 0.8) → {field: \"score\", op: \">\", value: \"0.8\"}\n\n";
        
        std::cout << "2️⃣  使用规则引擎动态执行\n";
        std::cout << "   ✅ 根据 JSON 规则在运行时进行判断\n\n";
        
        std::cout << "3️⃣  支持复杂逻辑组合\n";
        std::cout << "   ✅ AND/OR 逻辑\n";
        std::cout << "   ✅ 优先级排序\n";
        std::cout << "   ✅ 多分支路由\n\n";
        
        std::cout << "4️⃣  完全可序列化\n";
        std::cout << "   ✅ JSON 格式存储\n";
        std::cout << "   ✅ 可读性强\n";
        std::cout << "   ✅ 易于调试\n\n";
        
        std::cout << "💡 使用建议:\n";
        std::cout << "   - 80% 的业务场景推荐使用规则引擎\n";
        std::cout << "   - 极端复杂场景考虑嵌入脚本语言\n";
        std::cout << "   - 提前设计好字段名和操作符\n";
        std::cout << "   - 为每个规则添加清晰的描述\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
}
