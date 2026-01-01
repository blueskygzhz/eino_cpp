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

#ifndef EINO_CPP_COMPOSE_GRAPH_JSON_CONDITION_ENGINE_H_
#define EINO_CPP_COMPOSE_GRAPH_JSON_CONDITION_ENGINE_H_

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <stdexcept>

namespace eino {
namespace compose {

// =============================================================================
// 方案 1: 参数化条件表达式（Rule-Based System）
// =============================================================================

/**
 * @brief 条件规则类型
 */
enum class ConditionOperator {
    EQUAL,              // ==
    NOT_EQUAL,          // !=
    GREATER_THAN,       // >
    GREATER_EQUAL,      // >=
    LESS_THAN,          // <
    LESS_EQUAL,         // <=
    CONTAINS,           // string contains
    STARTS_WITH,        // string starts with
    ENDS_WITH,          // string ends with
    REGEX_MATCH,        // regex match
    IN_LIST,            // value in [list]
    NOT_IN_LIST         // value not in [list]
};

/**
 * @brief 单个条件规则
 */
struct ConditionRule {
    std::string field;              // 字段名（如 "input.score", "context.user_level"）
    ConditionOperator op;           // 操作符
    std::string value;              // 比较值
    std::string target_node;        // 满足条件时路由到的节点
    
    // 可选：优先级（数字越大优先级越高）
    int priority = 0;
    
    // 可选：额外元数据
    std::map<std::string, std::string> metadata;
};

/**
 * @brief 条件规则组（支持 AND/OR 逻辑）
 */
struct ConditionRuleGroup {
    std::string logic;              // "AND" | "OR"
    std::vector<ConditionRule> rules;
    std::string target_node;        // 满足整组条件时路由到的节点
    int priority = 0;
};

/**
 * @brief 将 ConditionRule 序列化为 JSON
 */
inline nlohmann::json ConditionRuleToJson(const ConditionRule& rule) {
    nlohmann::json j;
    j["field"] = rule.field;
    j["operator"] = static_cast<int>(rule.op);
    j["value"] = rule.value;
    j["target_node"] = rule.target_node;
    j["priority"] = rule.priority;
    j["metadata"] = rule.metadata;
    return j;
}

/**
 * @brief 从 JSON 反序列化 ConditionRule
 */
inline ConditionRule ConditionRuleFromJson(const nlohmann::json& j) {
    ConditionRule rule;
    if (j.contains("field")) rule.field = j["field"].get<std::string>();
    if (j.contains("operator")) rule.op = static_cast<ConditionOperator>(j["operator"].get<int>());
    if (j.contains("value")) rule.value = j["value"].get<std::string>();
    if (j.contains("target_node")) rule.target_node = j["target_node"].get<std::string>();
    if (j.contains("priority")) rule.priority = j["priority"].get<int>();
    if (j.contains("metadata") && j["metadata"].is_object()) {
        for (auto it = j["metadata"].begin(); it != j["metadata"].end(); ++it) {
            rule.metadata[it.key()] = it.value().get<std::string>();
        }
    }
    return rule;
}

/**
 * @brief 将 ConditionRuleGroup 序列化为 JSON
 */
inline nlohmann::json ConditionRuleGroupToJson(const ConditionRuleGroup& group) {
    nlohmann::json j;
    j["logic"] = group.logic;
    j["target_node"] = group.target_node;
    j["priority"] = group.priority;
    
    nlohmann::json rules_array = nlohmann::json::array();
    for (const auto& rule : group.rules) {
        rules_array.push_back(ConditionRuleToJson(rule));
    }
    j["rules"] = rules_array;
    
    return j;
}

/**
 * @brief 从 JSON 反序列化 ConditionRuleGroup
 */
inline ConditionRuleGroup ConditionRuleGroupFromJson(const nlohmann::json& j) {
    ConditionRuleGroup group;
    if (j.contains("logic")) group.logic = j["logic"].get<std::string>();
    if (j.contains("target_node")) group.target_node = j["target_node"].get<std::string>();
    if (j.contains("priority")) group.priority = j["priority"].get<int>();
    
    if (j.contains("rules") && j["rules"].is_array()) {
        for (const auto& rule_json : j["rules"]) {
            group.rules.push_back(ConditionRuleFromJson(rule_json));
        }
    }
    
    return group;
}

/**
 * @brief 规则引擎：根据规则集动态执行条件判断
 */
template<typename T>
class RuleBasedConditionEngine {
public:
    using ValueExtractor = std::function<std::string(const T&, const std::string& field)>;
    
    /**
     * @brief 设置值提取器（从输入对象中提取字段值）
     */
    void SetValueExtractor(ValueExtractor extractor) {
        value_extractor_ = extractor;
    }
    
    /**
     * @brief 从规则集合创建条件函数
     */
    std::function<std::string(void*, const T&)> CreateConditionFromRules(
        const std::vector<ConditionRuleGroup>& rule_groups,
        const std::string& default_target
    ) {
        return [this, rule_groups, default_target](void* ctx, const T& input) -> std::string {
            // 按优先级排序规则组
            auto sorted_groups = rule_groups;
            std::sort(sorted_groups.begin(), sorted_groups.end(), 
                [](const ConditionRuleGroup& a, const ConditionRuleGroup& b) {
                    return a.priority > b.priority;
                });
            
            // 依次评估规则组
            for (const auto& group : sorted_groups) {
                if (EvaluateRuleGroup(group, input)) {
                    return group.target_node;
                }
            }
            
            return default_target;
        };
    }
    
private:
    ValueExtractor value_extractor_;
    
    /**
     * @brief 评估规则组
     */
    bool EvaluateRuleGroup(const ConditionRuleGroup& group, const T& input) {
        if (group.logic == "AND") {
            // 所有规则都必须满足
            for (const auto& rule : group.rules) {
                if (!EvaluateRule(rule, input)) {
                    return false;
                }
            }
            return true;
        } else if (group.logic == "OR") {
            // 至少一个规则满足
            for (const auto& rule : group.rules) {
                if (EvaluateRule(rule, input)) {
                    return true;
                }
            }
            return false;
        }
        return false;
    }
    
    /**
     * @brief 评估单个规则
     */
    bool EvaluateRule(const ConditionRule& rule, const T& input) {
        if (!value_extractor_) {
            throw std::runtime_error("ValueExtractor not set");
        }
        
        std::string actual_value = value_extractor_(input, rule.field);
        
        switch (rule.op) {
            case ConditionOperator::EQUAL:
                return actual_value == rule.value;
            
            case ConditionOperator::NOT_EQUAL:
                return actual_value != rule.value;
            
            case ConditionOperator::GREATER_THAN:
                return std::stod(actual_value) > std::stod(rule.value);
            
            case ConditionOperator::GREATER_EQUAL:
                return std::stod(actual_value) >= std::stod(rule.value);
            
            case ConditionOperator::LESS_THAN:
                return std::stod(actual_value) < std::stod(rule.value);
            
            case ConditionOperator::LESS_EQUAL:
                return std::stod(actual_value) <= std::stod(rule.value);
            
            case ConditionOperator::CONTAINS:
                return actual_value.find(rule.value) != std::string::npos;
            
            case ConditionOperator::STARTS_WITH:
                return actual_value.rfind(rule.value, 0) == 0;
            
            case ConditionOperator::ENDS_WITH: {
                if (rule.value.size() > actual_value.size()) return false;
                return actual_value.compare(
                    actual_value.size() - rule.value.size(), 
                    rule.value.size(), 
                    rule.value
                ) == 0;
            }
            
            default:
                return false;
        }
    }
};

// =============================================================================
// 方案 2: 脚本化条件（使用嵌入式脚本语言）
// =============================================================================

/**
 * @brief 脚本化条件信息
 */
struct ScriptConditionInfo {
    std::string script_type;        // "lua" | "javascript" | "python"
    std::string script_code;        // 脚本代码
    std::string entry_function;     // 入口函数名
    std::map<std::string, std::string> metadata;
};

/**
 * @brief 将 ScriptConditionInfo 序列化为 JSON
 */
inline nlohmann::json ScriptConditionInfoToJson(const ScriptConditionInfo& info) {
    nlohmann::json j;
    j["script_type"] = info.script_type;
    j["script_code"] = info.script_code;
    j["entry_function"] = info.entry_function;
    j["metadata"] = info.metadata;
    return j;
}

/**
 * @brief 从 JSON 反序列化 ScriptConditionInfo
 */
inline ScriptConditionInfo ScriptConditionInfoFromJson(const nlohmann::json& j) {
    ScriptConditionInfo info;
    if (j.contains("script_type")) info.script_type = j["script_type"].get<std::string>();
    if (j.contains("script_code")) info.script_code = j["script_code"].get<std::string>();
    if (j.contains("entry_function")) info.entry_function = j["entry_function"].get<std::string>();
    if (j.contains("metadata") && j["metadata"].is_object()) {
        for (auto it = j["metadata"].begin(); it != j["metadata"].end(); ++it) {
            info.metadata[it.key()] = it.value().get<std::string>();
        }
    }
    return info;
}

// =============================================================================
// 方案 3: 表达式树（Expression Tree）
// =============================================================================

/**
 * @brief 表达式树节点类型
 */
enum class ExprNodeType {
    LITERAL,            // 字面量
    VARIABLE,           // 变量
    BINARY_OP,          // 二元操作符
    UNARY_OP,           // 一元操作符
    FUNCTION_CALL       // 函数调用
};

/**
 * @brief 表达式树节点
 */
struct ExpressionNode {
    ExprNodeType type;
    std::string value;              // 对于 LITERAL/VARIABLE/操作符
    std::vector<ExpressionNode> children;
    
    // 序列化为 JSON
    nlohmann::json ToJson() const {
        nlohmann::json j;
        j["type"] = static_cast<int>(type);
        j["value"] = value;
        
        if (!children.empty()) {
            nlohmann::json children_array = nlohmann::json::array();
            for (const auto& child : children) {
                children_array.push_back(child.ToJson());
            }
            j["children"] = children_array;
        }
        
        return j;
    }
    
    // 从 JSON 反序列化
    static ExpressionNode FromJson(const nlohmann::json& j) {
        ExpressionNode node;
        if (j.contains("type")) node.type = static_cast<ExprNodeType>(j["type"].get<int>());
        if (j.contains("value")) node.value = j["value"].get<std::string>();
        
        if (j.contains("children") && j["children"].is_array()) {
            for (const auto& child_json : j["children"]) {
                node.children.push_back(FromJson(child_json));
            }
        }
        
        return node;
    }
};

/**
 * @brief 条件表达式信息
 */
struct ExpressionConditionInfo {
    ExpressionNode root;            // 表达式树根节点
    std::string description;        // 表达式描述
    std::map<std::string, std::string> variable_mappings;  // 变量映射
};

/**
 * @brief 将 ExpressionConditionInfo 序列化为 JSON
 */
inline nlohmann::json ExpressionConditionInfoToJson(const ExpressionConditionInfo& info) {
    nlohmann::json j;
    j["root"] = info.root.ToJson();
    j["description"] = info.description;
    j["variable_mappings"] = info.variable_mappings;
    return j;
}

/**
 * @brief 从 JSON 反序列化 ExpressionConditionInfo
 */
inline ExpressionConditionInfo ExpressionConditionInfoFromJson(const nlohmann::json& j) {
    ExpressionConditionInfo info;
    if (j.contains("root")) info.root = ExpressionNode::FromJson(j["root"]);
    if (j.contains("description")) info.description = j["description"].get<std::string>();
    if (j.contains("variable_mappings") && j["variable_mappings"].is_object()) {
        for (auto it = j["variable_mappings"].begin(); it != j["variable_mappings"].end(); ++it) {
            info.variable_mappings[it.key()] = it.value().get<std::string>();
        }
    }
    return info;
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_GRAPH_JSON_CONDITION_ENGINE_H_
