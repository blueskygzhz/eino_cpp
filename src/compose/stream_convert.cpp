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

#include "eino/compose/stream_convert.h"

namespace eino {
namespace compose {

// StreamSerializationHelper implementation

std::tuple<json, std::string> StreamSerializationHelper::SerializeStreamReader(
    std::shared_ptr<IStreamReader> isr,
    const std::function<std::tuple<json, std::string>(std::shared_ptr<IStreamReader>)>& concat_fn) {
    
    if (!isr) {
        return {json(), "stream reader is null"};
    }
    
    if (!concat_fn) {
        return {json(), "concat function is null"};
    }
    
    // 调用concat函数合并StreamReader
    auto [value, err] = concat_fn(isr);
    if (!err.empty()) {
        return {json(), err};
    }
    
    // 创建带有标记的JSON对象
    return {CreateStreamReaderJSON(value), ""};
}

std::tuple<std::shared_ptr<IStreamReader>, std::string> StreamSerializationHelper::DeserializeStreamReader(
    const json& j,
    const std::function<std::tuple<std::shared_ptr<IStreamReader>, std::string>(const json&)>& restore_fn) {
    
    if (!restore_fn) {
        return {nullptr, "restore function is null"};
    }
    
    // 检查是否为StreamReader JSON
    if (!IsStreamReaderJSON(j)) {
        return {nullptr, "JSON does not represent a StreamReader"};
    }
    
    // 提取实际值
    json value = ExtractValueFromStreamReaderJSON(j);
    
    // 调用restore函数创建StreamReader
    return restore_fn(value);
}

bool StreamSerializationHelper::IsStreamReaderJSON(const json& j) {
    return j.is_object() && 
           j.contains("__is_stream__") && 
           j["__is_stream__"].is_boolean() && 
           j["__is_stream__"].get<bool>();
}

json StreamSerializationHelper::CreateStreamReaderJSON(const json& value) {
    json result;
    result["__is_stream__"] = true;
    result["value"] = value;
    return result;
}

json StreamSerializationHelper::ExtractValueFromStreamReaderJSON(const json& j) {
    if (j.contains("value")) {
        return j["value"];
    }
    return json();
}

} // namespace compose
} // namespace eino
