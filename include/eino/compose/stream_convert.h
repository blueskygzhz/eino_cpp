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

#ifndef EINO_CPP_COMPOSE_STREAM_CONVERT_H_
#define EINO_CPP_COMPOSE_STREAM_CONVERT_H_

#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

#include "eino/compose/stream_reader.h"
#include "eino/compose/stream_concat.h"
#include "eino/schema/stream.h"

namespace eino {
namespace compose {

using json = nlohmann::json;

/**
 * @brief Stream转换对，用于checkpoint的序列化/反序列化
 * 
 * Aligned with: eino/compose/generic_helper.go:163-166 streamConvertPair
 */
struct StreamConvertPair {
    /**
     * @brief 合并StreamReader为单个值
     * 
     * Aligned with: eino/compose/generic_helper.go:164
     * concatStream func(sr streamReader) (any, error)
     * 
     * 在保存checkpoint时使用，将StreamReader读取完毕并转换为可序列化的值
     */
    std::function<std::tuple<json, std::string>(std::shared_ptr<IStreamReader>)> concat_stream;
    
    /**
     * @brief 从值恢复StreamReader
     * 
     * Aligned with: eino/compose/generic_helper.go:165
     * restoreStream func(any) (streamReader, error)
     * 
     * 在加载checkpoint时使用，将序列化的值包装成StreamReader
     */
    std::function<std::tuple<std::shared_ptr<IStreamReader>, std::string>(const json&)> restore_stream;
    
    // 默认构造函数
    StreamConvertPair() = default;
    
    StreamConvertPair(
        decltype(concat_stream) concat,
        decltype(restore_stream) restore)
        : concat_stream(concat)
        , restore_stream(restore) {}
};

/**
 * @brief 创建默认的StreamConvertPair
 * 
 * Aligned with: eino/compose/generic_helper.go:168-196 defaultStreamConvertPair[T any]()
 * 
 * @tparam T chunk类型
 * @return StreamConvertPair
 */
template<typename T>
StreamConvertPair DefaultStreamConvertPair() {
    return StreamConvertPair{
        // concatStream: 读取所有chunks并返回最后一个值
        // Aligned with: Go:171-183
        [](std::shared_ptr<IStreamReader> isr) -> std::tuple<json, std::string> {
            // 1. 解包为typed StreamReader
            auto tsr = UnpackStreamReader<T>(isr);
            if (!tsr) {
                return {json(), "cannot unpack stream reader to specified type"};
            }
            
            // 2. 使用concatStreamReader读取所有值
            // Aligned with: eino/compose/stream_concat.go:50 concatStreamReader
            T value;
            auto err = ConcatStreamReader<T>(tsr, value);
            if (!err.empty()) {
                // 如果是空流错误，返回nil（对应Go的nil）
                if (err == "empty stream") {
                    return {json(), ""};  // nil值，无错误
                }
                return {json(), err};
            }
            
            // 3. 将值转换为JSON
            json j = value;
            return {j, ""};
        },
        
        // restoreStream: 从值创建单元素StreamReader
        // Aligned with: Go:185-195
        [](const json& j) -> std::tuple<std::shared_ptr<IStreamReader>, std::string> {
            // 1. 如果是nil，创建空StreamReader
            if (j.is_null()) {
                auto empty_sr = schema::StreamReaderFromArray<T>({});
                return {PackStreamReader(empty_sr), ""};
            }
            
            // 2. 从JSON反序列化值
            T value;
            try {
                value = j.get<T>();
            } catch (const std::exception& e) {
                return {nullptr, std::string("cannot convert JSON to specified type: ") + e.what()};
            }
            
            // 3. 创建单元素StreamReader
            auto sr = schema::StreamReaderFromArray<T>({value});
            return {PackStreamReader(sr), ""};
        }
    };
}

/**
 * @brief StreamReader序列化助手
 * 
 * 用于在checkpoint中序列化/反序列化StreamReader
 */
class StreamSerializationHelper {
public:
    /**
     * @brief 将StreamReader序列化为JSON
     * 
     * @param isr StreamReader接口
     * @param concat_fn 合并函数（从StreamConvertPair获取）
     * @return tuple<json, error>
     */
    static std::tuple<json, std::string> SerializeStreamReader(
        std::shared_ptr<IStreamReader> isr,
        const std::function<std::tuple<json, std::string>(std::shared_ptr<IStreamReader>)>& concat_fn);
    
    /**
     * @brief 从JSON反序列化StreamReader
     * 
     * @param j JSON值
     * @param restore_fn 恢复函数（从StreamConvertPair获取）
     * @return tuple<IStreamReader, error>
     */
    static std::tuple<std::shared_ptr<IStreamReader>, std::string> DeserializeStreamReader(
        const json& j,
        const std::function<std::tuple<std::shared_ptr<IStreamReader>, std::string>(const json&)>& restore_fn);
    
    /**
     * @brief 检查JSON是否表示StreamReader
     * 
     * 通过检查特殊标记 "__is_stream__" 来判断
     */
    static bool IsStreamReaderJSON(const json& j);
    
    /**
     * @brief 创建StreamReader的JSON表示
     * 
     * @param value 合并后的值
     * @return JSON对象，包含 "__is_stream__": true 标记
     */
    static json CreateStreamReaderJSON(const json& value);
    
    /**
     * @brief 从StreamReader JSON中提取值
     * 
     * @param j StreamReader的JSON表示
     * @return 提取的值
     */
    static json ExtractValueFromStreamReaderJSON(const json& j);
};

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_STREAM_CONVERT_H_
