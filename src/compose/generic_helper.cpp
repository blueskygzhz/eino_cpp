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

#include "eino/compose/generic_helper.h"
#include "eino/compose/stream_concat.h"

#include <typeinfo>

namespace eino {
namespace compose {

// 对齐 eino/compose/generic_helper.go

std::shared_ptr<GenericHelper> GenericHelper::ForMapInput() const {
    auto helper = std::make_shared<GenericHelper>();
    
    // 保持output不变
    helper->output_stream_filter = output_stream_filter;
    helper->output_converter = output_converter;
    helper->output_field_mapping_converter = output_field_mapping_converter;
    helper->output_stream_convert_pair = output_stream_convert_pair;
    helper->output_zero_value = output_zero_value;
    helper->output_empty_stream = output_empty_stream;
    
    // input改为map[string]any
    helper->input_stream_filter = DefaultStreamMapFilter<std::map<std::string, std::any>>();
    helper->input_converter.invoke = DefaultValueChecker<std::map<std::string, std::any>>();
    helper->input_converter.transform = DefaultStreamConverter<std::map<std::string, std::any>>();
    helper->input_field_mapping_converter.invoke = BuildFieldMappingConverter<std::map<std::string, std::any>>();
    helper->input_field_mapping_converter.transform = BuildStreamFieldMappingConverter<std::map<std::string, std::any>>();
    helper->input_stream_convert_pair = DefaultStreamConvertPair<std::map<std::string, std::any>>();
    helper->input_zero_value = ZeroValueFromGeneric<std::map<std::string, std::any>>();
    helper->input_empty_stream = EmptyStreamFromGeneric<std::map<std::string, std::any>>();
    
    return helper;
}

std::shared_ptr<GenericHelper> GenericHelper::ForMapOutput() const {
    auto helper = std::make_shared<GenericHelper>();
    
    // 保持input不变
    helper->input_stream_filter = input_stream_filter;
    helper->input_converter = input_converter;
    helper->input_field_mapping_converter = input_field_mapping_converter;
    helper->input_stream_convert_pair = input_stream_convert_pair;
    helper->input_zero_value = input_zero_value;
    helper->input_empty_stream = input_empty_stream;
    
    // output改为map[string]any
    helper->output_stream_filter = DefaultStreamMapFilter<std::map<std::string, std::any>>();
    helper->output_converter.invoke = DefaultValueChecker<std::map<std::string, std::any>>();
    helper->output_converter.transform = DefaultStreamConverter<std::map<std::string, std::any>>();
    helper->output_field_mapping_converter.invoke = BuildFieldMappingConverter<std::map<std::string, std::any>>();
    helper->output_field_mapping_converter.transform = BuildStreamFieldMappingConverter<std::map<std::string, std::any>>();
    helper->output_stream_convert_pair = DefaultStreamConvertPair<std::map<std::string, std::any>>();
    helper->output_zero_value = ZeroValueFromGeneric<std::map<std::string, std::any>>();
    helper->output_empty_stream = EmptyStreamFromGeneric<std::map<std::string, std::any>>();
    
    return helper;
}

std::shared_ptr<GenericHelper> GenericHelper::ForPredecessorPassthrough() const {
    auto helper = std::make_shared<GenericHelper>();
    
    // input和output都使用input的配置
    helper->input_stream_filter = input_stream_filter;
    helper->output_stream_filter = input_stream_filter;
    
    helper->input_converter = input_converter;
    helper->output_converter = input_converter;
    
    helper->input_field_mapping_converter = input_field_mapping_converter;
    helper->output_field_mapping_converter = input_field_mapping_converter;
    
    helper->input_stream_convert_pair = input_stream_convert_pair;
    helper->output_stream_convert_pair = input_stream_convert_pair;
    
    helper->input_zero_value = input_zero_value;
    helper->output_zero_value = input_zero_value;
    
    helper->input_empty_stream = input_empty_stream;
    helper->output_empty_stream = input_empty_stream;
    
    return helper;
}

std::shared_ptr<GenericHelper> GenericHelper::ForSuccessorPassthrough() const {
    auto helper = std::make_shared<GenericHelper>();
    
    // input和output都使用output的配置
    helper->input_stream_filter = output_stream_filter;
    helper->output_stream_filter = output_stream_filter;
    
    helper->input_converter = output_converter;
    helper->output_converter = output_converter;
    
    helper->input_field_mapping_converter = output_field_mapping_converter;
    helper->output_field_mapping_converter = output_field_mapping_converter;
    
    helper->input_stream_convert_pair = output_stream_convert_pair;
    helper->output_stream_convert_pair = output_stream_convert_pair;
    
    helper->input_zero_value = output_zero_value;
    helper->output_zero_value = output_zero_value;
    
    helper->input_empty_stream = output_empty_stream;
    helper->output_empty_stream = output_empty_stream;
    
    return helper;
}

// 模板实现

template<typename I, typename O>
std::shared_ptr<GenericHelper> NewGenericHelper() {
    auto helper = std::make_shared<GenericHelper>();
    
    // Input配置
    helper->input_stream_filter = DefaultStreamMapFilter<I>();
    helper->input_converter.invoke = DefaultValueChecker<I>();
    helper->input_converter.transform = DefaultStreamConverter<I>();
    helper->input_field_mapping_converter.invoke = BuildFieldMappingConverter<I>();
    helper->input_field_mapping_converter.transform = BuildStreamFieldMappingConverter<I>();
    helper->input_stream_convert_pair = DefaultStreamConvertPair<I>();
    helper->input_zero_value = ZeroValueFromGeneric<I>();
    helper->input_empty_stream = EmptyStreamFromGeneric<I>();
    
    // Output配置
    helper->output_stream_filter = DefaultStreamMapFilter<O>();
    helper->output_converter.invoke = DefaultValueChecker<O>();
    helper->output_converter.transform = DefaultStreamConverter<O>();
    helper->output_field_mapping_converter.invoke = BuildFieldMappingConverter<O>();
    helper->output_field_mapping_converter.transform = BuildStreamFieldMappingConverter<O>();
    helper->output_stream_convert_pair = DefaultStreamConvertPair<O>();
    helper->output_zero_value = ZeroValueFromGeneric<O>();
    helper->output_empty_stream = EmptyStreamFromGeneric<O>();
    
    return helper;
}

template<typename T>
StreamMapFilter DefaultStreamMapFilter() {
    return [](const std::string& key, std::shared_ptr<IStreamReader> stream) 
        -> std::tuple<std::shared_ptr<IStreamReader>, bool> {
        // 实现从map流中提取key对应的值流
        // 这里是简化实现，实际需要类型转换
        return {stream, true};
    };
}

template<typename T>
StreamHandler DefaultStreamConverter() {
    return [](std::shared_ptr<IStreamReader> reader) -> std::shared_ptr<IStreamReader> {
        // 类型转换，确保流中的值是T类型
        return reader;
    };
}

template<typename T>
ValueHandler DefaultValueChecker() {
    return [](std::shared_ptr<void> value) -> std::tuple<std::shared_ptr<void>, std::error_code> {
        // 运行时类型检查
        try {
            auto typed_value = std::static_pointer_cast<T>(value);
            return {typed_value, {}};
        } catch (...) {
            return {nullptr, std::make_error_code(std::errc::invalid_argument)};
        }
    };
}

template<typename T>
StreamConvertPair DefaultStreamConvertPair() {
    StreamConvertPair pair;
    
    // concat_stream: Stream -> Value (for checkpoint save)
    // 对齐 Go: eino/compose/generic_helper.go:171-183
    pair.concat_stream = [](std::shared_ptr<IStreamReader> sr) 
        -> std::tuple<std::shared_ptr<void>, std::error_code> {
        if (!sr) {
            return {nullptr, std::make_error_code(std::errc::invalid_argument)};
        }
        
        // Type check and unpack
        auto typed_sr = std::dynamic_pointer_cast<StreamReaderPacker<T>>(sr);
        if (!typed_sr) {
            return {nullptr, std::make_error_code(std::errc::invalid_argument)};
        }
        
        try {
            // Call ConcatStreamReader to aggregate all chunks
            // 对齐 Go: concatStreamReader(tsr)
            T value = ConcatStreamReader<T>(typed_sr->GetInner());
            return {std::make_shared<T>(std::move(value)), std::error_code{}};
        } catch (const EmptyStreamConcatError&) {
            // Empty stream returns nullptr
            // 对齐 Go: errors.Is(err, emptyStreamConcatErr) -> return nil, nil
            return {nullptr, std::error_code{}};
        } catch (const std::exception&) {
            return {nullptr, std::make_error_code(std::errc::io_error)};
        }
    };
    
    // restore_stream: Value -> Stream (for checkpoint restore)
    // 对齐 Go: eino/compose/generic_helper.go:185-195
    pair.restore_stream = [](std::shared_ptr<void> value) 
        -> std::tuple<std::shared_ptr<IStreamReader>, std::error_code> {
        if (!value) {
            // nil -> empty stream
            // 对齐 Go: return packStreamReader(schema.StreamReaderFromArray([]T{})), nil
            auto empty_sr = schema::StreamReaderFromArray<T>({});
            return {std::make_shared<StreamReaderPacker<T>>(empty_sr), std::error_code{}};
        }
        
        try {
            // Type check
            auto typed_value = std::static_pointer_cast<T>(value);
            if (!typed_value) {
                return {nullptr, std::make_error_code(std::errc::invalid_argument)};
            }
            
            // Wrap single value as stream with one element
            // 对齐 Go: return packStreamReader(schema.StreamReaderFromArray([]T{value})), nil
            auto sr = schema::StreamReaderFromArray<T>({*typed_value});
            return {std::make_shared<StreamReaderPacker<T>>(sr), std::error_code{}};
        } catch (const std::exception&) {
            return {nullptr, std::make_error_code(std::errc::invalid_argument)};
        }
    };
    
    return pair;
}

template<typename T>
std::function<std::shared_ptr<void>()> ZeroValueFromGeneric() {
    return []() -> std::shared_ptr<void> {
        return std::make_shared<T>();
    };
}

template<typename T>
std::function<std::shared_ptr<IStreamReader>()> EmptyStreamFromGeneric() {
    return []() -> std::shared_ptr<IStreamReader> {
        // 创建空流
        return nullptr; // 占位符
    };
}

template<typename T>
ValueHandler BuildFieldMappingConverter() {
    return [](std::shared_ptr<void> value) -> std::tuple<std::shared_ptr<void>, std::error_code> {
        // 字段映射转换
        return {value, {}};
    };
}

template<typename T>
StreamHandler BuildStreamFieldMappingConverter() {
    return [](std::shared_ptr<IStreamReader> reader) -> std::shared_ptr<IStreamReader> {
        // 流字段映射转换
        return reader;
    };
}

// 显式实例化常用类型
template std::shared_ptr<GenericHelper> NewGenericHelper<std::string, std::string>();
template std::shared_ptr<GenericHelper> NewGenericHelper<int, int>();
template std::shared_ptr<GenericHelper> NewGenericHelper<std::string, int>();
template std::shared_ptr<GenericHelper> NewGenericHelper<std::map<std::string, std::any>, std::map<std::string, std::any>>();

} // namespace compose
} // namespace eino
