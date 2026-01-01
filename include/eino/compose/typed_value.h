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

#ifndef EINO_CPP_COMPOSE_TYPED_VALUE_H_
#define EINO_CPP_COMPOSE_TYPED_VALUE_H_

#include <memory>
#include <any>
#include <typeindex>
#include <string>
#include <stdexcept>
#include <vector>

namespace eino {
namespace compose {

// Forward declaration
template<typename T> class StreamReader;

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ IStreamReader - 流式接口（对齐 eino streamReader interface）
//
// Aligns with: eino/compose/stream_reader.go:26-35
//
// type streamReader interface {
//     copy(n int) []streamReader
//     getType() reflect.Type
//     getChunkType() reflect.Type
//     merge([]streamReader) streamReader
//     withKey(string) streamReader
//     close()
//     toAnyStreamReader() *schema.StreamReader[any]
//     mergeWithNames([]streamReader, []string) streamReader
// }
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

class IStreamReader {
public:
    virtual ~IStreamReader() = default;
    
    // Get the type of the stream (e.g., StreamReader<int>)
    virtual std::type_index GetType() const = 0;
    
    // Get the element type (e.g., int for StreamReader<int>)
    virtual std::type_index GetChunkType() const = 0;
    
    // Close the stream
    virtual void Close() = 0;
    
    // Copy stream (for multiple consumers)
    virtual std::vector<std::shared_ptr<IStreamReader>> Copy(int n) = 0;
    
    // Check if closed
    virtual bool IsClosed() const = 0;
};

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ StreamReaderPacker - 类型擦除包装器
//
// Aligns with: eino/compose/stream_reader.go:37-109
//
// type streamReaderPacker[T any] struct {
//     sr *schema.StreamReader[T]
// }
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

template<typename T>
class StreamReaderPacker : public IStreamReader {
public:
    explicit StreamReaderPacker(std::shared_ptr<StreamReader<T>> sr)
        : stream_reader_(sr) {}
    
    std::type_index GetType() const override {
        return typeid(StreamReader<T>);
    }
    
    std::type_index GetChunkType() const override {
        return typeid(T);
    }
    
    void Close() override {
        if (stream_reader_) {
            stream_reader_->Close();
        }
    }
    
    std::vector<std::shared_ptr<IStreamReader>> Copy(int n) override {
        // TODO: Implement stream copy
        std::vector<std::shared_ptr<IStreamReader>> result;
        result.push_back(shared_from_this());
        return result;
    }
    
    bool IsClosed() const override {
        return stream_reader_ ? stream_reader_->IsClosed() : true;
    }
    
    // Get the underlying typed stream reader
    std::shared_ptr<StreamReader<T>> GetStreamReader() const {
        return stream_reader_;
    }
    
private:
    std::shared_ptr<StreamReader<T>> stream_reader_;
};

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ Helper Functions (对齐 eino 的 pack/unpack)
//
// Aligns with:
// - eino/compose/stream_reader.go:111-129
//   func packStreamReader[T any](sr *schema.StreamReader[T]) streamReader
//   func unpackStreamReader[T any](isr streamReader) (*schema.StreamReader[T], bool)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// Pack: 将类型化的 StreamReader 包装为接口
template<typename T>
std::shared_ptr<IStreamReader> PackStreamReader(std::shared_ptr<StreamReader<T>> sr) {
    return std::make_shared<StreamReaderPacker<T>>(sr);
}

// Unpack: 从接口恢复类型化的 StreamReader
template<typename T>
std::pair<std::shared_ptr<StreamReader<T>>, bool> UnpackStreamReader(
    std::shared_ptr<IStreamReader> isr) {
    
    if (!isr) {
        return {nullptr, false};
    }
    
    // 尝试转换为 StreamReaderPacker<T>
    auto packer = std::dynamic_pointer_cast<StreamReaderPacker<T>>(isr);
    if (packer) {
        return {packer->GetStreamReader(), true};
    }
    
    // TODO: Support interface types (like Go's interface conversion)
    // if (typeid(T) == typeid(interface)) { ... }
    
    return {nullptr, false};
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ IsStreamValue - 判断是否为流式值
//
// Aligns with eino's type checking logic in composableRunnable
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

inline bool IsStreamValue(std::shared_ptr<void> value) {
    if (!value) {
        return false;
    }
    
    // 尝试转换为 IStreamReader
    try {
        auto stream_interface = std::static_pointer_cast<IStreamReader>(value);
        return stream_interface != nullptr;
    } catch (...) {
        return false;
    }
}

inline bool IsStreamValue(const std::any& value) {
    try {
        auto stream_interface = std::any_cast<std::shared_ptr<IStreamReader>>(value);
        return stream_interface != nullptr;
    } catch (const std::bad_any_cast&) {
        return false;
    }
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ TypedValue - 增强版（集成 IStreamReader）
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

class TypedValue {
public:
    TypedValue() 
        : value_(std::any()), 
          type_index_(typeid(void)), 
          is_stream_(false) {}
    
    // Create from regular value
    template<typename T>
    static TypedValue Create(T value) {
        TypedValue tv;
        tv.value_ = std::any(value);
        tv.type_index_ = typeid(T);
        tv.is_stream_ = false;
        return tv;
    }
    
    // ⭐ Create from StreamReader (使用 IStreamReader 接口)
    // Aligns with: packStreamReader in eino
    template<typename T>
    static TypedValue CreateStream(std::shared_ptr<StreamReader<T>> reader) {
        TypedValue tv;
        // 包装为 IStreamReader 接口
        auto packed = PackStreamReader(reader);
        tv.value_ = std::any(packed);
        tv.type_index_ = typeid(std::shared_ptr<IStreamReader>);
        tv.is_stream_ = true;
        return tv;
    }
    
    // Create from IStreamReader (已包装的流)
    static TypedValue CreateStreamInterface(std::shared_ptr<IStreamReader> stream_interface) {
        TypedValue tv;
        tv.value_ = std::any(stream_interface);
        tv.type_index_ = typeid(std::shared_ptr<IStreamReader>);
        tv.is_stream_ = true;
        return tv;
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Type Query
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    bool IsStream() const {
        return is_stream_;
    }
    
    std::type_index GetType() const {
        return type_index_;
    }
    
    // Get the chunk type (for streams)
    std::type_index GetChunkType() const {
        if (!is_stream_) {
            return typeid(void);
        }
        
        try {
            auto stream_interface = std::any_cast<std::shared_ptr<IStreamReader>>(value_);
            if (stream_interface) {
                return stream_interface->GetChunkType();
            }
        } catch (const std::bad_any_cast&) {}
        
        return typeid(void);
    }
    
    // Check if stream element type matches T
    template<typename T>
    bool IsStreamOf() const {
        return is_stream_ && GetChunkType() == typeid(T);
    }
    
    // Check if value type matches T
    template<typename T>
    bool IsValueOf() const {
        return !is_stream_ && type_index_ == typeid(T);
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Value Access
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Get regular value
    template<typename T>
    T Get() const {
        if (is_stream_) {
            throw std::runtime_error("Cannot get regular value from stream");
        }
        try {
            return std::any_cast<T>(value_);
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error(
                std::string("Type mismatch: expected ") + 
                typeid(T).name() + ", got " + type_index_.name());
        }
    }
    
    // ⭐ Get stream reader (对齐 unpackStreamReader)
    template<typename T>
    std::shared_ptr<StreamReader<T>> GetStream() const {
        if (!is_stream_) {
            throw std::runtime_error("Cannot get stream from regular value");
        }
        
        try {
            auto stream_interface = std::any_cast<std::shared_ptr<IStreamReader>>(value_);
            if (!stream_interface) {
                throw std::runtime_error("Stream interface is null");
            }
            
            // 使用 UnpackStreamReader 恢复类型
            auto [typed_stream, ok] = UnpackStreamReader<T>(stream_interface);
            if (!ok) {
                throw std::runtime_error(
                    std::string("Stream element type mismatch: expected ") + 
                    typeid(T).name() + ", got " + stream_interface->GetChunkType().name());
            }
            
            return typed_stream;
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Failed to cast stream interface");
        }
    }
    
    // Get stream interface (type-erased)
    std::shared_ptr<IStreamReader> GetStreamInterface() const {
        if (!is_stream_) {
            throw std::runtime_error("Cannot get stream from regular value");
        }
        
        try {
            return std::any_cast<std::shared_ptr<IStreamReader>>(value_);
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Failed to cast stream interface");
        }
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Convenience
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    bool IsEmpty() const {
        return !value_.has_value();
    }
    
    std::string GetTypeNameString() const {
        if (is_stream_) {
            return std::string("StreamReader<") + 
                   GetChunkType().name() + ">";
        }
        return type_index_.name();
    }
    
private:
    std::any value_;
    std::type_index type_index_;
    bool is_stream_;
};

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_TYPED_VALUE_H_
