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

#include "eino/compose/pregel.h"
#include "eino/compose/values_merge.h"

namespace eino {
namespace compose {

// 对齐 eino/compose/pregel.go

std::shared_ptr<Channel> PregelChannelBuilder(
    const std::vector<std::string>& control_dependencies,
    const std::vector<std::string>& data_dependencies,
    std::function<std::shared_ptr<void>()> zero_value,
    std::function<std::shared_ptr<IStreamReader>()> empty_stream) {
    
    // Pregel不使用依赖参数，但保持接口一致
    return std::make_shared<PregelChannel>(
        std::move(zero_value), std::move(empty_stream));
}

PregelChannel::PregelChannel(
    std::function<std::shared_ptr<void>()> zero_value_fn,
    std::function<std::shared_ptr<IStreamReader>()> empty_stream_fn)
    : zero_value_fn_(std::move(zero_value_fn))
    , empty_stream_fn_(std::move(empty_stream_fn)) {
}

void PregelChannel::ReportValues(const std::map<std::string, std::shared_ptr<void>>& values) {
    // Pregel简单地接收所有值
    for (const auto& [key, value] : values) {
        values_[key] = value;
    }
}

void PregelChannel::ReportDependencies(const std::vector<std::string>& dependencies) {
    // Pregel不使用显式的依赖报告，忽略
    return;
}

bool PregelChannel::ReportSkip(const std::vector<std::string>& keys) {
    // Pregel不支持跳过传播，总是返回false
    return false;
}

std::tuple<std::shared_ptr<void>, bool, std::error_code> PregelChannel::Get(
    bool is_stream,
    const std::string& name,
    EdgeHandlerManager* edge_handler) {
    
    // 如果没有值，返回未就绪
    if (values_.empty()) {
        return {nullptr, false, {}};
    }
    
    // 收集所有值
    std::vector<std::shared_ptr<void>> value_list;
    std::vector<std::string> names;
    
    for (const auto& [key, value] : values_) {
        // 应用边处理器
        auto [resolved_value, err] = edge_handler->Handle(key, name, value, is_stream);
        if (err) {
            Clear();
            return {nullptr, false, err};
        }
        
        value_list.push_back(resolved_value);
        names.push_back(key);
    }
    
    // 清空值（准备下次迭代）
    Clear();
    
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

std::error_code PregelChannel::ConvertValues(
    std::function<std::error_code(std::map<std::string, std::shared_ptr<void>>&)> fn) {
    return fn(values_);
}

std::error_code PregelChannel::Load(std::shared_ptr<Channel> other) {
    auto pregel_channel = std::dynamic_pointer_cast<PregelChannel>(other);
    if (!pregel_channel) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    
    values_ = pregel_channel->values_;
    return {};
}

void PregelChannel::SetMergeConfig(const FanInMergeConfig& config) {
    merge_config_ = config;
}

void PregelChannel::Clear() {
    values_.clear();
}

// PregelIterator implementation

bool PregelIterator::ShouldContinue(int iteration, int max_iterations) const {
    if (max_iterations <= 0) {
        return true; // 无限制
    }
    return iteration < max_iterations;
}

bool PregelIterator::HasConverged(
    const std::map<std::string, std::shared_ptr<void>>& current_values,
    const std::map<std::string, std::shared_ptr<void>>& previous_values,
    double threshold) const {
    
    // 简单的收敛检查（需要根据具体类型实现）
    // 这里提供基本框架
    
    if (current_values.size() != previous_values.size()) {
        return false;
    }
    
    // 注意：实际实现需要根据值的类型进行比较
    // 这里只是占位符逻辑
    for (const auto& [key, current_value] : current_values) {
        if (!previous_values.count(key)) {
            return false;
        }
        
        // 实际比较逻辑需要类型特化
        // 例如：如果是double，比较差值是否小于threshold
        // 这里简化为指针比较
        if (current_value != previous_values.at(key)) {
            return false;
        }
    }
    
    return true;
}

} // namespace compose
} // namespace eino
