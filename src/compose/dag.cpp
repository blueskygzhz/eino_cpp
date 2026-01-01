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

#include "eino/compose/dag.h"
#include "eino/compose/values_merge.h"

#include <algorithm>
#include <queue>
#include <set>

namespace eino {
namespace compose {

// 对齐 eino/compose/dag.go

std::shared_ptr<Channel> DAGChannelBuilder(
    const std::vector<std::string>& control_dependencies,
    const std::vector<std::string>& data_dependencies,
    std::function<std::shared_ptr<void>()> zero_value,
    std::function<std::shared_ptr<IStreamReader>()> empty_stream) {
    
    return std::make_shared<DAGChannel>(
        control_dependencies, data_dependencies, 
        std::move(zero_value), std::move(empty_stream));
}

DAGChannel::DAGChannel(
    const std::vector<std::string>& control_deps,
    const std::vector<std::string>& data_deps,
    std::function<std::shared_ptr<void>()> zero_value_fn,
    std::function<std::shared_ptr<IStreamReader>()> empty_stream_fn)
    : zero_value_fn_(std::move(zero_value_fn))
    , empty_stream_fn_(std::move(empty_stream_fn)) {
    
    // 初始化控制依赖为 Waiting 状态
    for (const auto& dep : control_deps) {
        control_predecessors_[dep] = DependencyState::Waiting;
    }
    
    // 初始化数据依赖为 false (未接收数据)
    for (const auto& dep : data_deps) {
        data_predecessors_[dep] = false;
    }
}

void DAGChannel::ReportValues(const std::map<std::string, std::shared_ptr<void>>& values) {
    if (skipped_) {
        return;
    }
    
    for (const auto& [key, value] : values) {
        // 只处理声明的数据依赖
        if (data_predecessors_.count(key)) {
            data_predecessors_[key] = true;
            values_[key] = value;
        }
    }
}

void DAGChannel::ReportDependencies(const std::vector<std::string>& dependencies) {
    if (skipped_) {
        return;
    }
    
    for (const auto& dep : dependencies) {
        if (control_predecessors_.count(dep)) {
            control_predecessors_[dep] = DependencyState::Ready;
        }
    }
}

bool DAGChannel::ReportSkip(const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        // 标记控制依赖为 Skipped
        if (control_predecessors_.count(key)) {
            control_predecessors_[key] = DependencyState::Skipped;
        }
        
        // 标记数据依赖为已接收（即使被跳过）
        if (data_predecessors_.count(key)) {
            data_predecessors_[key] = true;
        }
    }
    
    // 检查是否所有控制依赖都被跳过
    bool all_skipped = true;
    for (const auto& [_, state] : control_predecessors_) {
        if (state != DependencyState::Skipped) {
            all_skipped = false;
            break;
        }
    }
    
    skipped_ = all_skipped;
    return all_skipped;
}

std::tuple<std::shared_ptr<void>, bool, std::error_code> DAGChannel::Get(
    bool is_stream,
    const std::string& name,
    EdgeHandlerManager* edge_handler) {
    
    // 如果被跳过，返回空
    if (skipped_) {
        return {nullptr, false, {}};
    }
    
    // 如果没有任何依赖，返回空
    if (control_predecessors_.empty() && data_predecessors_.empty()) {
        return {nullptr, false, {}};
    }
    
    // 检查所有控制依赖是否就绪
    for (const auto& [_, state] : control_predecessors_) {
        if (state == DependencyState::Waiting) {
            return {nullptr, false, {}}; // 还未就绪
        }
    }
    
    // 检查所有数据依赖是否到达
    for (const auto& [_, ready] : data_predecessors_) {
        if (!ready) {
            return {nullptr, false, {}}; // 数据未到达
        }
    }
    
    // 所有依赖就绪，收集值
    std::vector<std::shared_ptr<void>> value_list;
    std::vector<std::string> names;
    
    for (const auto& [key, value] : values_) {
        // 应用边处理器
        auto [resolved_value, err] = edge_handler->Handle(key, name, value, is_stream);
        if (err) {
            Reset();
            return {nullptr, false, err};
        }
        
        value_list.push_back(resolved_value);
        names.push_back(key);
    }
    
    // 清理状态（准备下次使用）
    Reset();
    
    // 如果没有值
    if (value_list.empty()) {
        if (is_stream) {
            return {empty_stream_fn_(), true, {}};
        }
        return {zero_value_fn_(), true, {}};
    }
    
    // 只有一个值，直接返回
    if (value_list.size() == 1) {
        return {value_list[0], true, {}};
    }
    
    // 多个值需要合并
    MergeOptions opts;
    opts.stream_merge_with_source_eof = merge_config_.stream_merge_with_source_eof;
    opts.names = names;
    
    auto [merged_value, merge_err] = MergeValues(value_list, opts);
    if (merge_err) {
        return {nullptr, false, merge_err};
    }
    
    return {merged_value, true, {}};
}

std::error_code DAGChannel::ConvertValues(
    std::function<std::error_code(std::map<std::string, std::shared_ptr<void>>&)> fn) {
    return fn(values_);
}

std::error_code DAGChannel::Load(std::shared_ptr<Channel> other) {
    auto dag_channel = std::dynamic_pointer_cast<DAGChannel>(other);
    if (!dag_channel) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    
    control_predecessors_ = dag_channel->control_predecessors_;
    data_predecessors_ = dag_channel->data_predecessors_;
    skipped_ = dag_channel->skipped_;
    values_ = dag_channel->values_;
    
    return {};
}

void DAGChannel::SetMergeConfig(const FanInMergeConfig& config) {
    merge_config_ = config;
}

bool DAGChannel::IsReady() const {
    if (skipped_) {
        return false;
    }
    
    // 检查控制依赖
    for (const auto& [_, state] : control_predecessors_) {
        if (state == DependencyState::Waiting) {
            return false;
        }
    }
    
    // 检查数据依赖
    for (const auto& [_, ready] : data_predecessors_) {
        if (!ready) {
            return false;
        }
    }
    
    return true;
}

void DAGChannel::Reset() {
    values_.clear();
    
    for (auto& [_, state] : control_predecessors_) {
        state = DependencyState::Waiting;
    }
    
    for (auto& [_, ready] : data_predecessors_) {
        ready = false;
    }
}

// DAGChannelHelper implementation

std::vector<std::vector<std::string>> DAGChannelHelper::DetectCycles(
    const std::map<std::string, std::vector<std::string>>& adjacency) {
    
    std::vector<std::vector<std::string>> cycles;
    std::map<std::string, int> color; // 0: white, 1: gray, 2: black
    
    std::function<void(const std::string&, std::vector<std::string>&)> dfs;
    dfs = [&](const std::string& node, std::vector<std::string>& path) {
        color[node] = 1; // gray
        path.push_back(node);
        
        if (adjacency.count(node)) {
            for (const auto& next : adjacency.at(node)) {
                if (color[next] == 1) {
                    // Found a cycle
                    auto it = std::find(path.begin(), path.end(), next);
                    if (it != path.end()) {
                        std::vector<std::string> cycle(it, path.end());
                        cycle.push_back(next);
                        cycles.push_back(cycle);
                    }
                } else if (color[next] == 0) {
                    dfs(next, path);
                }
            }
        }
        
        path.pop_back();
        color[node] = 2; // black
    };
    
    // 遍历所有节点
    for (const auto& [node, _] : adjacency) {
        if (color[node] == 0) {
            std::vector<std::string> path;
            dfs(node, path);
        }
    }
    
    return cycles;
}

std::vector<std::string> DAGChannelHelper::TopologicalSort(
    const std::map<std::string, std::vector<std::string>>& adjacency) {
    
    // 计算入度
    std::map<std::string, int> in_degree;
    std::set<std::string> all_nodes;
    
    for (const auto& [node, neighbors] : adjacency) {
        all_nodes.insert(node);
        if (!in_degree.count(node)) {
            in_degree[node] = 0;
        }
        for (const auto& neighbor : neighbors) {
            all_nodes.insert(neighbor);
            in_degree[neighbor]++;
        }
    }
    
    // 找到所有入度为0的节点
    std::queue<std::string> queue;
    for (const auto& node : all_nodes) {
        if (in_degree[node] == 0) {
            queue.push(node);
        }
    }
    
    std::vector<std::string> result;
    
    while (!queue.empty()) {
        auto node = queue.front();
        queue.pop();
        result.push_back(node);
        
        if (adjacency.count(node)) {
            for (const auto& neighbor : adjacency.at(node)) {
                in_degree[neighbor]--;
                if (in_degree[neighbor] == 0) {
                    queue.push(neighbor);
                }
            }
        }
    }
    
    // 如果结果数量不等于所有节点数，说明有环
    if (result.size() != all_nodes.size()) {
        return {}; // 有环，返回空
    }
    
    return result;
}

std::set<std::string> DAGChannelHelper::GetAllPredecessors(
    const std::string& node,
    const std::map<std::string, std::vector<std::string>>& adjacency) {
    
    std::set<std::string> predecessors;
    std::set<std::string> visited;
    
    std::function<void(const std::string&)> dfs;
    dfs = [&](const std::string& current) {
        if (visited.count(current)) {
            return;
        }
        visited.insert(current);
        
        // 查找指向当前节点的所有节点
        for (const auto& [pred, neighbors] : adjacency) {
            for (const auto& neighbor : neighbors) {
                if (neighbor == current) {
                    predecessors.insert(pred);
                    dfs(pred);
                }
            }
        }
    };
    
    dfs(node);
    return predecessors;
}

} // namespace compose
} // namespace eino
