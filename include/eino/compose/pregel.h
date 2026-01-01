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

#ifndef EINO_COMPOSE_PREGEL_H_
#define EINO_COMPOSE_PREGEL_H_

#include <map>
#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "eino/compose/graph_manager.h"

namespace eino {
namespace compose {

// 对齐 eino/compose/pregel.go

/**
 * @brief Pregel通道构建器
 * 
 * 对应 Go: pregelChannelBuilder
 * 创建Pregel风格的通道，简化的依赖模型（不区分控制/数据依赖）
 */
std::shared_ptr<Channel> PregelChannelBuilder(
    const std::vector<std::string>& control_dependencies,
    const std::vector<std::string>& data_dependencies,
    std::function<std::shared_ptr<void>()> zero_value,
    std::function<std::shared_ptr<IStreamReader>()> empty_stream);

/**
 * @brief Pregel通道实现
 * 
 * 对应 Go: pregelChannel
 * 
 * Pregel模型特性:
 * - 不区分控制依赖和数据依赖
 * - 简化的就绪条件: 只要有值就立即就绪
 * - 适用于迭代式图算法（如PageRank）
 * - 不支持跳过传播
 */
class PregelChannel : public Channel {
public:
    PregelChannel(
        std::function<std::shared_ptr<void>()> zero_value_fn,
        std::function<std::shared_ptr<IStreamReader>()> empty_stream_fn);

    ~PregelChannel() override = default;

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

    // Pregel specific methods
    
    /**
     * @brief 检查是否有值可以获取
     * @return true 如果有值
     */
    bool HasValues() const { return !values_.empty(); }
    
    /**
     * @brief 获取当前存储的值数量
     */
    size_t GetValueCount() const { return values_.size(); }
    
    /**
     * @brief 清空所有值
     */
    void Clear();

private:
    // 值存储（不区分依赖类型）
    std::map<std::string, std::shared_ptr<void>> values_;
    
    // 合并配置
    FanInMergeConfig merge_config_;
    
    // 零值和空流工厂（虽然Pregel通常不需要，但保持接口一致）
    std::function<std::shared_ptr<void>()> zero_value_fn_;
    std::function<std::shared_ptr<IStreamReader>()> empty_stream_fn_;
};

/**
 * @brief Pregel迭代器
 * 
 * 辅助管理Pregel风格的迭代执行
 */
class PregelIterator {
public:
    PregelIterator() = default;
    
    /**
     * @brief 是否应该继续迭代
     * @param iteration 当前迭代次数
     * @param max_iterations 最大迭代次数 (0表示无限制)
     * @return true 如果应该继续
     */
    bool ShouldContinue(int iteration, int max_iterations = 0) const;
    
    /**
     * @brief 检查是否收敛
     * @param current_values 当前值
     * @param previous_values 上一轮值
     * @param threshold 收敛阈值
     * @return true 如果已收敛
     */
    bool HasConverged(
        const std::map<std::string, std::shared_ptr<void>>& current_values,
        const std::map<std::string, std::shared_ptr<void>>& previous_values,
        double threshold = 1e-6) const;
    
    /**
     * @brief 记录迭代
     */
    void RecordIteration() { iteration_count_++; }
    
    /**
     * @brief 获取当前迭代次数
     */
    int GetIterationCount() const { return iteration_count_; }
    
    /**
     * @brief 重置迭代器
     */
    void Reset() { iteration_count_ = 0; }

private:
    int iteration_count_ = 0;
};

} // namespace compose
} // namespace eino

#endif // EINO_COMPOSE_PREGEL_H_
