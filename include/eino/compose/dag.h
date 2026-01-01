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

#ifndef EINO_COMPOSE_DAG_H_
#define EINO_COMPOSE_DAG_H_

#include <map>
#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "eino/compose/graph_manager.h"

namespace eino {
namespace compose {

// 对齐 eino/compose/dag.go

/**
 * @brief DAG通道构建器
 * 
 * 对应 Go: dagChannelBuilder
 * 创建DAG风格的通道，支持控制依赖和数据依赖
 */
std::shared_ptr<Channel> DAGChannelBuilder(
    const std::vector<std::string>& control_dependencies,
    const std::vector<std::string>& data_dependencies,
    std::function<std::shared_ptr<void>()> zero_value,
    std::function<std::shared_ptr<IStreamReader>()> empty_stream);

/**
 * @brief DAG通道实现
 * 
 * 对应 Go: dagChannel
 * 
 * 特性:
 * - 控制依赖: 必须全部完成才能执行
 * - 数据依赖: 提供输入数据
 * - 跳过传播: 如果所有控制依赖都跳过，则节点跳过
 */
class DAGChannel : public Channel {
public:
    DAGChannel(
        const std::vector<std::string>& control_deps,
        const std::vector<std::string>& data_deps,
        std::function<std::shared_ptr<void>()> zero_value_fn,
        std::function<std::shared_ptr<IStreamReader>()> empty_stream_fn);

    ~DAGChannel() override = default;

    // Channel interface implementation
    void ReportValues(const std::map<std::string, std::shared_ptr<void>>& values) override;
    
    void ReportDependencies(const std::vector<std::string>& dependencies) override;
    
    bool ReportSkip(const std::vector<std::string>& keys) override;
    
    std::tuple<std::shared_ptr<void>, bool, std::error_code> Get(
        bool is_stream,
        const std::string& name,
        EdgeHandlerManager* edge_handler) override;
    
    std::error_code ConvertValues(
        std::function<std::error_code(std::map<std::string, std::shared_ptr<void>>&)> fn) override;
    
    std::error_code Load(std::shared_ptr<Channel> other) override;
    
    void SetMergeConfig(const FanInMergeConfig& config) override;

    // DAG specific methods
    
    /**
     * @brief 检查是否所有依赖都已就绪
     * @return true 如果可以执行
     */
    bool IsReady() const;
    
    /**
     * @brief 检查是否应该跳过
     * @return true 如果所有控制依赖都跳过
     */
    bool IsSkipped() const { return skipped_; }
    
    /**
     * @brief 重置通道状态（用于下次迭代）
     */
    void Reset();

private:
    // 依赖状态管理
    std::map<std::string, DependencyState> control_predecessors_;
    std::map<std::string, bool> data_predecessors_; // key -> has_value
    
    // 值存储
    std::map<std::string, std::shared_ptr<void>> values_;
    
    // 跳过标志
    bool skipped_ = false;
    
    // 合并配置
    FanInMergeConfig merge_config_;
    
    // 零值和空流工厂
    std::function<std::shared_ptr<void>()> zero_value_fn_;
    std::function<std::shared_ptr<IStreamReader>()> empty_stream_fn_;
};

/**
 * @brief DAG通道管理器辅助类
 * 
 * 提供DAG图相关的辅助功能
 */
class DAGChannelHelper {
public:
    /**
     * @brief 检测依赖关系中的循环
     * @param adjacency 邻接表
     * @return 发现的循环列表
     */
    static std::vector<std::vector<std::string>> DetectCycles(
        const std::map<std::string, std::vector<std::string>>& adjacency);
    
    /**
     * @brief 拓扑排序
     * @param adjacency 邻接表
     * @return 拓扑排序结果（如果有环则返回空）
     */
    static std::vector<std::string> TopologicalSort(
        const std::map<std::string, std::vector<std::string>>& adjacency);
    
    /**
     * @brief 计算节点的所有前驱
     * @param node 目标节点
     * @param adjacency 邻接表
     * @return 所有前驱节点集合
     */
    static std::set<std::string> GetAllPredecessors(
        const std::string& node,
        const std::map<std::string, std::vector<std::string>>& adjacency);
};

} // namespace compose
} // namespace eino

#endif // EINO_COMPOSE_DAG_H_
