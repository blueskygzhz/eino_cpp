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

#ifndef EINO_COMPOSE_GENERIC_HELPER_H_
#define EINO_COMPOSE_GENERIC_HELPER_H_

#include <functional>
#include <memory>
#include <string>
#include <map>

#include "eino/compose/stream_reader.h"

namespace eino {
namespace compose {

// 对齐 eino/compose/generic_helper.go

/**
 * @brief 流Map过滤器
 * 
 * 对应 Go: streamMapFilter
 * 从map[string]any流中过滤指定key的流
 */
using StreamMapFilter = std::function<
    std::tuple<std::shared_ptr<IStreamReader>, bool>(
        const std::string& key,
        std::shared_ptr<IStreamReader> stream)>;

/**
 * @brief 值处理器
 * 
 * 对应 Go: valueHandler
 * 处理和验证值类型
 */
using ValueHandler = std::function<
    std::tuple<std::shared_ptr<void>, std::error_code>(std::shared_ptr<void>)>;

/**
 * @brief 流处理器
 * 
 * 对应 Go: streamHandler
 * 处理流转换
 */
using StreamHandler = std::function<
    std::shared_ptr<IStreamReader>(std::shared_ptr<IStreamReader>)>;

/**
 * @brief 处理器对
 * 
 * 对应 Go: handlerPair
 */
struct HandlerPair {
    ValueHandler invoke;      // Invoke时使用
    StreamHandler transform;  // Stream时使用
};

/**
 * @brief 流转换对
 * 
 * 对应 Go: streamConvertPair
 * 用于checkpoint时在流和非流间转换
 */
struct StreamConvertPair {
    // 拼接流为单个值
    std::function<std::tuple<std::shared_ptr<void>, std::error_code>(
        std::shared_ptr<IStreamReader>)> concat_stream;
    
    // 恢复值为流
    std::function<std::tuple<std::shared_ptr<IStreamReader>, std::error_code>(
        std::shared_ptr<void>)> restore_stream;
};

/**
 * @brief 泛型辅助类
 * 
 * 对应 Go: genericHelper
 * 
 * 为特定类型提供:
 * - 类型检查和转换
 * - 流处理
 * - 字段映射
 * - 零值和空流生成
 */
class GenericHelper {
public:
    GenericHelper() = default;
    ~GenericHelper() = default;

    // 流过滤器
    StreamMapFilter input_stream_filter;
    StreamMapFilter output_stream_filter;
    
    // 类型转换器（用于前驱输出可能赋值给当前节点输入时）
    HandlerPair input_converter;
    HandlerPair output_converter;
    
    // 字段映射转换器
    HandlerPair input_field_mapping_converter;
    HandlerPair output_field_mapping_converter;
    
    // 流转换对（用于checkpoint）
    StreamConvertPair input_stream_convert_pair;
    StreamConvertPair output_stream_convert_pair;
    
    // 零值和空流工厂
    std::function<std::shared_ptr<void>()> input_zero_value;
    std::function<std::shared_ptr<void>()> output_zero_value;
    std::function<std::shared_ptr<IStreamReader>()> input_empty_stream;
    std::function<std::shared_ptr<IStreamReader>()> output_empty_stream;

    /**
     * @brief 为Map输入创建helper
     * 
     * 对应 Go: forMapInput() *genericHelper
     */
    std::shared_ptr<GenericHelper> ForMapInput() const;
    
    /**
     * @brief 为Map输出创建helper
     * 
     * 对应 Go: forMapOutput() *genericHelper
     */
    std::shared_ptr<GenericHelper> ForMapOutput() const;
    
    /**
     * @brief 为前驱Passthrough节点创建helper
     * 
     * 对应 Go: forPredecessorPassthrough() *genericHelper
     */
    std::shared_ptr<GenericHelper> ForPredecessorPassthrough() const;
    
    /**
     * @brief 为后继Passthrough节点创建helper
     * 
     * 对应 Go: forSuccessorPassthrough() *genericHelper
     */
    std::shared_ptr<GenericHelper> ForSuccessorPassthrough() const;
};

/**
 * @brief 创建泛型辅助类
 * 
 * 对应 Go: newGenericHelper[I, O any]() *genericHelper
 */
template<typename I, typename O>
std::shared_ptr<GenericHelper> NewGenericHelper();

/**
 * @brief 默认流Map过滤器
 * 
 * 对应 Go: defaultStreamMapFilter[T any]
 */
template<typename T>
StreamMapFilter DefaultStreamMapFilter();

/**
 * @brief 默认流转换器
 * 
 * 对应 Go: defaultStreamConverter[T any]
 */
template<typename T>
StreamHandler DefaultStreamConverter();

/**
 * @brief 默认值检查器
 * 
 * 对应 Go: defaultValueChecker[T any]
 */
template<typename T>
ValueHandler DefaultValueChecker();

/**
 * @brief 默认流转换对
 * 
 * 对应 Go: defaultStreamConvertPair[T any]
 */
template<typename T>
StreamConvertPair DefaultStreamConvertPair();

/**
 * @brief 零值工厂
 * 
 * 对应 Go: zeroValueFromGeneric[T any]
 */
template<typename T>
std::function<std::shared_ptr<void>()> ZeroValueFromGeneric();

/**
 * @brief 空流工厂
 * 
 * 对应 Go: emptyStreamFromGeneric[T any]
 */
template<typename T>
std::function<std::shared_ptr<IStreamReader>()> EmptyStreamFromGeneric();

/**
 * @brief 构建字段映射转换器
 * 
 * 对应 Go: buildFieldMappingConverter[T any]
 */
template<typename T>
ValueHandler BuildFieldMappingConverter();

/**
 * @brief 构建流字段映射转换器
 * 
 * 对应 Go: buildStreamFieldMappingConverter[T any]
 */
template<typename T>
StreamHandler BuildStreamFieldMappingConverter();

} // namespace compose
} // namespace eino

#endif // EINO_COMPOSE_GENERIC_HELPER_H_
