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

#ifndef EINO_CPP_COMPOSE_GRAPH_JSON_ADVANCED_H_
#define EINO_CPP_COMPOSE_GRAPH_JSON_ADVANCED_H_

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>

#include "graph_json.h"
#include "branch.h"
#include "tool_node.h"

namespace eino {
namespace compose {

// =============================================================================
// Branch 序列化
// =============================================================================

/**
 * @brief Branch 节点的序列化信息
 * 
 * 由于 Branch 包含函数指针（条件逻辑），无法直接序列化。
 * 我们保存必要的元数据，用于反序列化时重建。
 */
struct BranchNodeInfo {
    std::string name;                    // Branch 节点名称
    std::string branch_type;             // 类型: "single" | "multi" | "stream_single" | "stream_multi"
    std::set<std::string> end_nodes;     // 可能的目标节点
    std::string condition_key;           // 条件逻辑标识符（用于查找注册的条件函数）
    std::map<std::string, std::string> metadata;  // 额外元数据
    
    // 条件逻辑的描述（可选，用于文档/调试）
    std::string condition_description;
    
    // 示例路由规则（JSON 格式）
    // 例如: {"input_value": "A", "target_node": "node_a"}
    std::vector<std::map<std::string, std::string>> example_routes;
};

/**
 * @brief 将 BranchNodeInfo 序列化为 JSON
 */
inline nlohmann::json BranchNodeInfoToJson(const BranchNodeInfo& info) {
    nlohmann::json j;
    
    j["name"] = info.name;
    j["branch_type"] = info.branch_type;
    j["end_nodes"] = info.end_nodes;
    j["condition_key"] = info.condition_key;
    j["metadata"] = info.metadata;
    
    if (!info.condition_description.empty()) {
        j["condition_description"] = info.condition_description;
    }
    
    if (!info.example_routes.empty()) {
        nlohmann::json routes_array = nlohmann::json::array();
        for (const auto& route : info.example_routes) {
            nlohmann::json route_obj;
            for (const auto& kv : route) {
                route_obj[kv.first] = kv.second;
            }
            routes_array.push_back(route_obj);
        }
        j["example_routes"] = routes_array;
    }
    
    return j;
}

/**
 * @brief 从 JSON 反序列化 BranchNodeInfo
 */
inline BranchNodeInfo BranchNodeInfoFromJson(const nlohmann::json& j) {
    BranchNodeInfo info;
    
    if (j.contains("name")) {
        info.name = j["name"].get<std::string>();
    }
    if (j.contains("branch_type")) {
        info.branch_type = j["branch_type"].get<std::string>();
    }
    if (j.contains("end_nodes")) {
        info.end_nodes = j["end_nodes"].get<std::set<std::string>>();
    }
    if (j.contains("condition_key")) {
        info.condition_key = j["condition_key"].get<std::string>();
    }
    if (j.contains("metadata") && j["metadata"].is_object()) {
        for (auto it = j["metadata"].begin(); it != j["metadata"].end(); ++it) {
            info.metadata[it.key()] = it.value().get<std::string>();
        }
    }
    if (j.contains("condition_description")) {
        info.condition_description = j["condition_description"].get<std::string>();
    }
    if (j.contains("example_routes") && j["example_routes"].is_array()) {
        for (const auto& route_json : j["example_routes"]) {
            std::map<std::string, std::string> route;
            for (auto it = route_json.begin(); it != route_json.end(); ++it) {
                route[it.key()] = it.value().get<std::string>();
            }
            info.example_routes.push_back(route);
        }
    }
    
    return info;
}

// =============================================================================
// ToolsNode 序列化
// =============================================================================

/**
 * @brief Tool 定义的序列化信息
 */
struct ToolDefinitionInfo {
    std::string name;                    // Tool 名称
    std::string description;             // Tool 描述
    std::string parameters_schema;       // 参数 JSON Schema
    std::string type;                    // Tool 类型: "invokable" | "streamable" | "both"
    std::map<std::string, std::string> metadata;
};

/**
 * @brief ToolsNode 的序列化信息
 */
struct ToolsNodeInfo {
    std::string name;                              // ToolsNode 名称
    std::vector<ToolDefinitionInfo> tools;         // Tool 列表
    bool execute_sequentially = false;             // 是否顺序执行
    bool has_unknown_tools_handler = false;        // 是否有未知 Tool 处理器
    bool has_arguments_handler = false;            // 是否有参数预处理器
    int middleware_count = 0;                      // Middleware 数量
    std::map<std::string, std::string> metadata;
};

/**
 * @brief 将 ToolDefinitionInfo 序列化为 JSON
 */
inline nlohmann::json ToolDefinitionInfoToJson(const ToolDefinitionInfo& info) {
    nlohmann::json j;
    j["name"] = info.name;
    j["description"] = info.description;
    j["parameters_schema"] = info.parameters_schema;
    j["type"] = info.type;
    j["metadata"] = info.metadata;
    return j;
}

/**
 * @brief 从 JSON 反序列化 ToolDefinitionInfo
 */
inline ToolDefinitionInfo ToolDefinitionInfoFromJson(const nlohmann::json& j) {
    ToolDefinitionInfo info;
    
    if (j.contains("name")) info.name = j["name"].get<std::string>();
    if (j.contains("description")) info.description = j["description"].get<std::string>();
    if (j.contains("parameters_schema")) info.parameters_schema = j["parameters_schema"].get<std::string>();
    if (j.contains("type")) info.type = j["type"].get<std::string>();
    if (j.contains("metadata") && j["metadata"].is_object()) {
        for (auto it = j["metadata"].begin(); it != j["metadata"].end(); ++it) {
            info.metadata[it.key()] = it.value().get<std::string>();
        }
    }
    
    return info;
}

/**
 * @brief 将 ToolsNodeInfo 序列化为 JSON
 */
inline nlohmann::json ToolsNodeInfoToJson(const ToolsNodeInfo& info) {
    nlohmann::json j;
    
    j["name"] = info.name;
    j["execute_sequentially"] = info.execute_sequentially;
    j["has_unknown_tools_handler"] = info.has_unknown_tools_handler;
    j["has_arguments_handler"] = info.has_arguments_handler;
    j["middleware_count"] = info.middleware_count;
    j["metadata"] = info.metadata;
    
    nlohmann::json tools_array = nlohmann::json::array();
    for (const auto& tool : info.tools) {
        tools_array.push_back(ToolDefinitionInfoToJson(tool));
    }
    j["tools"] = tools_array;
    
    return j;
}

/**
 * @brief 从 JSON 反序列化 ToolsNodeInfo
 */
inline ToolsNodeInfo ToolsNodeInfoFromJson(const nlohmann::json& j) {
    ToolsNodeInfo info;
    
    if (j.contains("name")) info.name = j["name"].get<std::string>();
    if (j.contains("execute_sequentially")) info.execute_sequentially = j["execute_sequentially"].get<bool>();
    if (j.contains("has_unknown_tools_handler")) info.has_unknown_tools_handler = j["has_unknown_tools_handler"].get<bool>();
    if (j.contains("has_arguments_handler")) info.has_arguments_handler = j["has_arguments_handler"].get<bool>();
    if (j.contains("middleware_count")) info.middleware_count = j["middleware_count"].get<int>();
    if (j.contains("metadata") && j["metadata"].is_object()) {
        for (auto it = j["metadata"].begin(); it != j["metadata"].end(); ++it) {
            info.metadata[it.key()] = it.value().get<std::string>();
        }
    }
    
    if (j.contains("tools") && j["tools"].is_array()) {
        for (const auto& tool_json : j["tools"]) {
            info.tools.push_back(ToolDefinitionInfoFromJson(tool_json));
        }
    }
    
    return info;
}

// =============================================================================
// Branch 条件函数注册机制
// =============================================================================

/**
 * @brief Branch 条件函数注册表
 * 
 * 由于函数指针无法序列化，我们使用注册表模式：
 * 1. 序列化时保存 condition_key
 * 2. 反序列化时通过 condition_key 查找注册的函数
 */
template<typename T>
class BranchConditionRegistry {
public:
    using SingleCondition = GraphBranchCondition<T>;
    using MultiCondition = GraphMultiBranchCondition<T>;
    using StreamSingleCondition = StreamGraphBranchCondition<T>;
    using StreamMultiCondition = StreamGraphMultiBranchCondition<T>;
    
    static BranchConditionRegistry& Instance() {
        static BranchConditionRegistry instance;
        return instance;
    }
    
    // 注册单选条件
    void RegisterSingleCondition(const std::string& key, SingleCondition condition) {
        single_conditions_[key] = condition;
    }
    
    // 注册多选条件
    void RegisterMultiCondition(const std::string& key, MultiCondition condition) {
        multi_conditions_[key] = condition;
    }
    
    // 注册流式单选条件
    void RegisterStreamSingleCondition(const std::string& key, StreamSingleCondition condition) {
        stream_single_conditions_[key] = condition;
    }
    
    // 注册流式多选条件
    void RegisterStreamMultiCondition(const std::string& key, StreamMultiCondition condition) {
        stream_multi_conditions_[key] = condition;
    }
    
    // 获取条件函数
    SingleCondition GetSingleCondition(const std::string& key) const {
        auto it = single_conditions_.find(key);
        if (it == single_conditions_.end()) {
            throw std::runtime_error("Branch condition not found: " + key);
        }
        return it->second;
    }
    
    MultiCondition GetMultiCondition(const std::string& key) const {
        auto it = multi_conditions_.find(key);
        if (it == multi_conditions_.end()) {
            throw std::runtime_error("Branch multi-condition not found: " + key);
        }
        return it->second;
    }
    
    StreamSingleCondition GetStreamSingleCondition(const std::string& key) const {
        auto it = stream_single_conditions_.find(key);
        if (it == stream_single_conditions_.end()) {
            throw std::runtime_error("Stream branch condition not found: " + key);
        }
        return it->second;
    }
    
    StreamMultiCondition GetStreamMultiCondition(const std::string& key) const {
        auto it = stream_multi_conditions_.find(key);
        if (it == stream_multi_conditions_.end()) {
            throw std::runtime_error("Stream branch multi-condition not found: " + key);
        }
        return it->second;
    }
    
    // 检查是否已注册
    bool HasSingleCondition(const std::string& key) const {
        return single_conditions_.find(key) != single_conditions_.end();
    }
    
    bool HasMultiCondition(const std::string& key) const {
        return multi_conditions_.find(key) != multi_conditions_.end();
    }
    
private:
    BranchConditionRegistry() = default;
    
    std::map<std::string, SingleCondition> single_conditions_;
    std::map<std::string, MultiCondition> multi_conditions_;
    std::map<std::string, StreamSingleCondition> stream_single_conditions_;
    std::map<std::string, StreamMultiCondition> stream_multi_conditions_;
};

// =============================================================================
// Tool 工厂注册机制
// =============================================================================

/**
 * @brief Tool 工厂注册表
 * 
 * 用于从 ToolDefinitionInfo 重建实际的 Tool 对象
 */
class ToolFactoryRegistry {
public:
    using ToolFactory = std::function<std::shared_ptr<tool::BaseTool>(const ToolDefinitionInfo&)>;
    
    static ToolFactoryRegistry& Instance() {
        static ToolFactoryRegistry instance;
        return instance;
    }
    
    // 注册 Tool 工厂
    void RegisterToolFactory(const std::string& tool_name, ToolFactory factory) {
        factories_[tool_name] = factory;
    }
    
    // 创建 Tool
    std::shared_ptr<tool::BaseTool> CreateTool(const ToolDefinitionInfo& info) const {
        auto it = factories_.find(info.name);
        if (it == factories_.end()) {
            throw std::runtime_error("Tool factory not found: " + info.name);
        }
        return it->second(info);
    }
    
    // 检查是否已注册
    bool HasFactory(const std::string& tool_name) const {
        return factories_.find(tool_name) != factories_.end();
    }
    
private:
    ToolFactoryRegistry() = default;
    
    std::map<std::string, ToolFactory> factories_;
};

// =============================================================================
// 辅助宏：简化注册
// =============================================================================

/**
 * @brief 注册 Branch 条件函数
 * 
 * 使用示例：
 * REGISTER_BRANCH_CONDITION(std::string, "route_by_intent", [](void* ctx, const std::string& input) {
 *     if (input.find("weather") != std::string::npos) return "weather_node";
 *     return "default_node";
 * });
 */
#define REGISTER_BRANCH_CONDITION(TYPE, KEY, CONDITION) \
    namespace { \
        struct BranchConditionRegistrar_##KEY { \
            BranchConditionRegistrar_##KEY() { \
                BranchConditionRegistry<TYPE>::Instance().RegisterSingleCondition(#KEY, CONDITION); \
            } \
        }; \
        static BranchConditionRegistrar_##KEY branch_condition_registrar_##KEY; \
    }

/**
 * @brief 注册 Tool 工厂
 * 
 * 使用示例：
 * REGISTER_TOOL_FACTORY("weather_tool", [](const ToolDefinitionInfo& info) {
 *     return std::make_shared<WeatherTool>();
 * });
 */
#define REGISTER_TOOL_FACTORY(TOOL_NAME, FACTORY) \
    namespace { \
        struct ToolFactoryRegistrar_##TOOL_NAME { \
            ToolFactoryRegistrar_##TOOL_NAME() { \
                ToolFactoryRegistry::Instance().RegisterToolFactory(#TOOL_NAME, FACTORY); \
            } \
        }; \
        static ToolFactoryRegistrar_##TOOL_NAME tool_factory_registrar_##TOOL_NAME; \
    }

// =============================================================================
// 完整的 Graph 序列化（包含 Branch 和 ToolsNode）
// =============================================================================

/**
 * @brief 扩展的 Graph 重建信息
 */
struct ExtendedGraphReconstructionInfo : public GraphReconstructionInfo {
    std::vector<BranchNodeInfo> branches;      // Branch 节点信息
    std::vector<ToolsNodeInfo> tools_nodes;    // ToolsNode 信息
};

/**
 * @brief 提取扩展的 Graph 重建信息
 */
inline ExtendedGraphReconstructionInfo ExtractExtendedGraphReconstructionInfo(const nlohmann::json& j) {
    ExtendedGraphReconstructionInfo info;
    
    // 基础信息
    auto base_info = ExtractGraphReconstructionInfo(j);
    info.nodes = base_info.nodes;
    info.edges = base_info.edges;
    info.compile_options = base_info.compile_options;
    info.topological_order = base_info.topological_order;
    info.start_nodes = base_info.start_nodes;
    info.end_nodes = base_info.end_nodes;
    
    // Branch 信息
    if (j.contains("branches") && j["branches"].is_array()) {
        for (const auto& branch_json : j["branches"]) {
            info.branches.push_back(BranchNodeInfoFromJson(branch_json));
        }
    }
    
    // ToolsNode 信息
    if (j.contains("tools_nodes") && j["tools_nodes"].is_array()) {
        for (const auto& tools_node_json : j["tools_nodes"]) {
            info.tools_nodes.push_back(ToolsNodeInfoFromJson(tools_node_json));
        }
    }
    
    return info;
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_GRAPH_JSON_ADVANCED_H_
