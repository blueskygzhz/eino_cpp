/*
 * Copyright 2024 CloudWeGo Authors
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

#ifndef EINO_CPP_COMPOSE_CHANNEL_H_
#define EINO_CPP_COMPOSE_CHANNEL_H_

#include <map>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <nlohmann/json.hpp>

namespace eino {
namespace compose {

using json = nlohmann::json;

/**
 * @brief Channel抽象接口
 * 
 * Aligned with: eino/compose/graph_manager.go channel interface
 */
class Channel {
public:
    virtual ~Channel() = default;
    
    /**
     * @brief 报告节点的输出值
     * Aligned with: eino/compose/graph_manager.go reportValues(ins map[string]any) error
     */
    virtual std::string ReportValues(const std::map<std::string, json>& values) = 0;
    
    /**
     * @brief 报告依赖关系就绪
     * Aligned with: eino/compose/graph_manager.go reportDependencies(dependencies []string)
     */
    virtual void ReportDependencies(const std::vector<std::string>& dependencies) = 0;
    
    /**
     * @brief 报告节点跳过
     * Aligned with: eino/compose/graph_manager.go reportSkip(keys []string) bool
     * @return true if this channel should also be skipped
     */
    virtual bool ReportSkip(const std::vector<std::string>& keys) = 0;
    
    /**
     * @brief 获取合并后的值
     * Aligned with: eino/compose/graph_manager.go get(isStream bool, name string, edgeHandler *edgeHandlerManager) (any, bool, error)
     * @return tuple<value, has_value, error>
     */
    virtual std::tuple<json, bool, std::string> Get(
        bool is_stream,
        const std::string& name,
        void* edge_handler) = 0;
    
    /**
     * @brief ⭐ CRITICAL: 转换channel中的值
     * 
     * Aligned with: eino/compose/dag.go:193 (dagChannel.convertValues)
     *               eino/compose/pregel.go:44 (pregelChannel.convertValues)
     * 
     * 该方法接收一个函数，该函数会操作channel内部的Values map。
     * 用于在checkpoint时将StreamReader转换为可序列化的值，
     * 或者从checkpoint恢复时将值转换回StreamReader。
     * 
     * Go implementation:
     * func (ch *dagChannel) convertValues(fn func(map[string]any) error) error {
     *     return fn(ch.Values)
     * }
     * 
     * @param fn 转换函数，接收Values map引用，返回错误信息
     * @return 错误信息（空字符串表示成功）
     */
    virtual std::string ConvertValues(
        std::function<std::string(std::map<std::string, json>&)> fn) = 0;
    
    /**
     * @brief 加载另一个channel的状态
     * Aligned with: eino/compose/graph_manager.go load(c channel) error
     */
    virtual std::string Load(std::shared_ptr<Channel> other) = 0;
    
    /**
     * @brief 设置合并配置
     * Aligned with: eino/compose/graph_manager.go setMergeConfig(cfg FanInMergeConfig)
     */
    virtual void SetMergeConfig(const json& config) = 0;
    
    /**
     * @brief 序列化为JSON
     * 用于checkpoint持久化
     */
    virtual json ToJSON() const = 0;
    
    /**
     * @brief 从JSON反序列化
     * 用于checkpoint恢复
     */
    virtual std::string FromJSON(const json& j) = 0;
    
    /**
     * @brief 获取channel类型名称
     * 用于类型识别和反序列化
     */
    virtual std::string GetTypeName() const = 0;
};

/**
 * @brief Pregel风格的Channel实现
 * 
 * Aligned with: eino/compose/pregel.go pregelChannel
 */
class PregelChannel : public Channel {
public:
    PregelChannel() = default;
    
    std::string ReportValues(const std::map<std::string, json>& values) override;
    void ReportDependencies(const std::vector<std::string>& dependencies) override;
    bool ReportSkip(const std::vector<std::string>& keys) override;
    
    std::tuple<json, bool, std::string> Get(
        bool is_stream,
        const std::string& name,
        void* edge_handler) override;
    
    // ⭐ ConvertValues实现（关键）
    std::string ConvertValues(
        std::function<std::string(std::map<std::string, json>&)> fn) override {
        // Aligned with: eino/compose/pregel.go:44-46
        // func (ch *pregelChannel) convertValues(fn func(map[string]any) error) error {
        //     return fn(ch.Values)
        // }
        return fn(values_);
    }
    
    std::string Load(std::shared_ptr<Channel> other) override;
    void SetMergeConfig(const json& config) override;
    json ToJSON() const override;
    std::string FromJSON(const json& j) override;
    std::string GetTypeName() const override { return "pregel"; }
    
private:
    std::map<std::string, json> values_;
    json merge_config_;
};

/**
 * @brief DAG风格的Channel实现
 * 
 * Aligned with: eino/compose/dag.go dagChannel
 */
class DAGChannel : public Channel {
public:
    DAGChannel(
        const std::vector<std::string>& control_deps,
        const std::vector<std::string>& data_deps);
    
    std::string ReportValues(const std::map<std::string, json>& values) override;
    void ReportDependencies(const std::vector<std::string>& dependencies) override;
    bool ReportSkip(const std::vector<std::string>& keys) override;
    
    std::tuple<json, bool, std::string> Get(
        bool is_stream,
        const std::string& name,
        void* edge_handler) override;
    
    // ⭐ ConvertValues实现（关键）
    std::string ConvertValues(
        std::function<std::string(std::map<std::string, json>&)> fn) override {
        // Aligned with: eino/compose/dag.go:193-195
        // func (ch *dagChannel) convertValues(fn func(map[string]any) error) error {
        //     return fn(ch.Values)
        // }
        return fn(values_);
    }
    
    std::string Load(std::shared_ptr<Channel> other) override;
    void SetMergeConfig(const json& config) override;
    json ToJSON() const override;
    std::string FromJSON(const json& j) override;
    std::string GetTypeName() const override { return "dag"; }
    
private:
    enum class DependencyState {
        Pending = 0,
        Ready = 1,
        Skipped = 2
    };
    
    std::map<std::string, DependencyState> control_predecessors_;
    std::map<std::string, bool> data_predecessors_;
    std::map<std::string, json> values_;
    bool skipped_ = false;
    json merge_config_;
};

/**
 * @brief Channel工厂函数
 */
std::shared_ptr<Channel> CreateChannelFromJSON(const json& j);

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_CHANNEL_H_
