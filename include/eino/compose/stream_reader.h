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

#pragma once

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "eino/schema/stream.h"

namespace eino {
namespace compose {

// Abstract stream reader interface for type-erased stream handling
class IStreamReader {
public:
    virtual ~IStreamReader() = default;
    
    // Copy the stream reader n times
    virtual std::vector<std::shared_ptr<IStreamReader>> Copy(int n) = 0;
    
    // Get the type of the stream reader
    virtual const std::type_info& GetType() const = 0;
    
    // Get the chunk type
    virtual const std::type_info& GetChunkType() const = 0;
    
    // Merge multiple stream readers
    virtual std::shared_ptr<IStreamReader> Merge(
        const std::vector<std::shared_ptr<IStreamReader>>& others) = 0;
    
    // Merge with names
    virtual std::shared_ptr<IStreamReader> MergeWithNames(
        const std::vector<std::shared_ptr<IStreamReader>>& others,
        const std::vector<std::string>& names) = 0;
    
    // Wrap with output key
    virtual std::shared_ptr<IStreamReader> WithKey(const std::string& key) = 0;
    
    // Close the stream
    virtual void Close() = 0;
    
    // Convert to any stream reader
    virtual std::shared_ptr<schema::StreamReader<Any>> ToAnyStreamReader() = 0;
};

// Template implementation of stream reader
template <typename T>
class StreamReaderPacker : public IStreamReader {
public:
    explicit StreamReaderPacker(std::shared_ptr<schema::StreamReader<T>> sr)
        : sr_(std::move(sr)) {}
    
    std::vector<std::shared_ptr<IStreamReader>> Copy(int n) override {
        auto srs = sr_->Copy(n);
        std::vector<std::shared_ptr<IStreamReader>> result;
        result.reserve(srs.size());
        for (auto& sr : srs) {
            result.push_back(std::make_shared<StreamReaderPacker<T>>(std::move(sr)));
        }
        return result;
    }
    
    const std::type_info& GetType() const override {
        return typeid(schema::StreamReader<T>);
    }
    
    const std::type_info& GetChunkType() const override {
        return typeid(T);
    }
    
    std::shared_ptr<IStreamReader> Merge(
        const std::vector<std::shared_ptr<IStreamReader>>& others) override {
        std::vector<std::shared_ptr<schema::StreamReader<T>>> srs;
        srs.push_back(sr_);
        
        for (const auto& other : others) {
            auto sr = UnpackStreamReader<T>(other);
            if (!sr) {
                throw std::runtime_error("type mismatch in merge");
            }
            srs.push_back(sr);
        }
        
        auto merged = schema::MergeStreamReaders(srs);
        return std::make_shared<StreamReaderPacker<T>>(merged);
    }
    
    std::shared_ptr<IStreamReader> MergeWithNames(
        const std::vector<std::shared_ptr<IStreamReader>>& others,
        const std::vector<std::string>& names) override {
        std::vector<std::shared_ptr<schema::StreamReader<T>>> srs;
        srs.push_back(sr_);
        
        for (const auto& other : others) {
            auto sr = UnpackStreamReader<T>(other);
            if (!sr) {
                throw std::runtime_error("type mismatch in merge with names");
            }
            srs.push_back(sr);
        }
        
        auto merged = schema::MergeNamedStreamReaders(srs, names);
        return std::make_shared<StreamReaderPacker<T>>(merged);
    }
    
    std::shared_ptr<IStreamReader> WithKey(const std::string& key) override {
        auto convert = [key](const T& v) -> std::map<std::string, Any> {
            return {{key, v}};
        };
        
        auto converted = sr_->template Convert<std::map<std::string, Any>>(convert);
        return std::make_shared<StreamReaderPacker<std::map<std::string, Any>>>(converted);
    }
    
    void Close() override {
        sr_->Close();
    }
    
    std::shared_ptr<schema::StreamReader<Any>> ToAnyStreamReader() override {
        auto convert = [](const T& v) -> Any {
            return v;
        };
        return sr_->template Convert<Any>(convert);
    }
    
    std::shared_ptr<schema::StreamReader<T>> GetStreamReader() const {
        return sr_;
    }
    
    // Get inner StreamReader (alias for GetStreamReader)
    std::shared_ptr<schema::StreamReader<T>> GetInner() const {
        return sr_;
    }

private:
    std::shared_ptr<schema::StreamReader<T>> sr_;
};

// Helper functions

// Pack a typed stream reader into generic interface
template <typename T>
std::shared_ptr<IStreamReader> PackStreamReader(std::shared_ptr<schema::StreamReader<T>> sr) {
    return std::make_shared<StreamReaderPacker<T>>(std::move(sr));
}

// Unpack a generic stream reader to typed version
template <typename T>
std::shared_ptr<schema::StreamReader<T>> UnpackStreamReader(std::shared_ptr<IStreamReader> isr) {
    auto packer = std::dynamic_pointer_cast<StreamReaderPacker<T>>(isr);
    if (packer) {
        return packer->GetStreamReader();
    }
    
    // Try to convert through any type if T is interface-like
    // This requires additional conversion logic
    return nullptr;
}

} // namespace compose
} // namespace eino
