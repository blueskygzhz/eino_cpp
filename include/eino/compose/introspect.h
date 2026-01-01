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

#ifndef EINO_COMPOSE_INTROSPECT_H_
#define EINO_COMPOSE_INTROSPECT_H_

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "eino/components/component.h"
#include "eino/compose/graph_compile_options.h"
#include "eino/compose/graph_add_node_options.h"

namespace eino {
namespace compose {

// 对齐 eino/compose/introspect.go

/**
 * @brief 图分支信息
 * 
 * 对应 Go: GraphBranch
 */
struct GraphBranch {
    std::string condition;        // 分支条件
    std::vector<std::string> ends; // 分支结束节点
};

/**
 * @brief 字段映射信息
 * 
 * 对应 Go: FieldMapping
 */
struct FieldMappingInfo {
    std::string from_field;      // 源字段路径
    std::string to_field;        // 目标字段路径
    std::string from_node_key;   // 源节点键
    
    std::string ToString() const;
};

/**
 * @brief 图节点信息
 * 
 * 对应 Go: GraphNodeInfo
 * 用户添加节点时传入的信息
 */
struct GraphNodeInfo {
    components::Component component;              // 组件类型
    std::shared_ptr<void> instance;              // 实例对象
    std::vector<GraphAddNodeOpt> graph_add_node_opts; // 节点选项
    
    // 主要用于Lambda，其输入输出类型无法从组件类型推断
    std::string input_type;
    std::string output_type;
    
    std::string name;           // 节点名称
    std::string input_key;      // 输入键
    std::string output_key;     // 输出键
    
    std::shared_ptr<struct GraphInfo> graph_info; // 子图信息（如果是子图节点）
    std::vector<FieldMappingInfo> mappings;       // 字段映射
};

/**
 * @brief 图信息
 * 
 * 对应 Go: GraphInfo
 * 用户编译图时传入的信息，用于观察和调试
 */
struct GraphInfo {
    std::vector<GraphCompileOption> compile_options; // 编译选项
    
    std::map<std::string, GraphNodeInfo> nodes;      // 节点键 -> 节点信息
    std::map<std::string, std::vector<std::string>> edges;       // 控制边: 起始节点 -> 结束节点列表
    std::map<std::string, std::vector<std::string>> data_edges;  // 数据边: 起始节点 -> 结束节点列表
    std::map<std::string, std::vector<GraphBranch>> branches;    // 分支: 起始节点 -> 分支列表
    
    std::string input_type;     // 图输入类型
    std::string output_type;    // 图输出类型
    std::string name;           // 图名称
    
    // 新图选项（如状态生成器等）
    std::vector<std::shared_ptr<void>> new_graph_options;
    std::function<std::shared_ptr<void>(std::shared_ptr<Context>)> gen_state_fn;
    
    /**
     * @brief 获取节点的所有前驱
     */
    std::vector<std::string> GetPredecessors(const std::string& node_key) const;
    
    /**
     * @brief 获取节点的所有后继
     */
    std::vector<std::string> GetSuccessors(const std::string& node_key) const;
    
    /**
     * @brief 检查是否存在环
     */
    bool HasCycle() const;
    
    /**
     * @brief 获取拓扑排序
     * @return 拓扑排序结果（如果有环则返回空）
     */
    std::vector<std::string> TopologicalSort() const;
    
    /**
     * @brief 导出为JSON字符串（用于调试）
     */
    std::string ToJSON() const;
};

/**
 * @brief 图编译回调接口
 * 
 * 对应 Go: GraphCompileCallback
 * 当图编译完成时会被调用
 */
class GraphCompileCallback {
public:
    virtual ~GraphCompileCallback() = default;
    
    /**
     * @brief 编译完成时的回调
     * @param ctx 上下文
     * @param info 图信息
     */
    virtual void OnFinish(
        std::shared_ptr<Context> ctx, 
        const GraphInfo& info) = 0;
};

/**
 * @brief 注册全局图编译回调
 * 
 * 用于观察和监控所有图的编译
 */
void RegisterGlobalGraphCompileCallback(std::shared_ptr<GraphCompileCallback> callback);

/**
 * @brief 清除全局图编译回调
 */
void ClearGlobalGraphCompileCallbacks();

/**
 * @brief 获取所有全局图编译回调
 */
std::vector<std::shared_ptr<GraphCompileCallback>> GetGlobalGraphCompileCallbacks();

/**
 * @brief 日志回调实现（用于调试）
 */
class LoggingGraphCompileCallback : public GraphCompileCallback {
public:
    LoggingGraphCompileCallback() = default;
    
    void OnFinish(std::shared_ptr<Context> ctx, const GraphInfo& info) override;
};

/**
 * @brief 打印图结构的辅助函数
 */
class GraphPrinter {
public:
    /**
     * @brief 打印图的DOT格式（用于Graphviz可视化）
     */
    static std::string ToDot(const GraphInfo& info);
    
    /**
     * @brief 打印图的文本表示
     */
    static std::string ToText(const GraphInfo& info);
    
    /**
     * @brief 打印图的统计信息
     */
    static std::string GetStatistics(const GraphInfo& info);
};

} // namespace compose
} // namespace eino

#endif // EINO_COMPOSE_INTROSPECT_H_
