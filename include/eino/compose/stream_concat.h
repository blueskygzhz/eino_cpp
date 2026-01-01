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

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <vector>

#include "eino/schema/stream.h"

namespace eino {
namespace compose {

// Stream chunk concatenation function type
template <typename T>
using StreamChunkConcatFunc = std::function<T(const std::vector<T>&)>;

// Registry for stream chunk concatenation functions
class StreamChunkConcatRegistry {
public:
    static StreamChunkConcatRegistry& Instance() {
        static StreamChunkConcatRegistry instance;
        return instance;
    }
    
    // Register concatenation function for type T
    template <typename T>
    void RegisterConcatFunc(StreamChunkConcatFunc<T> fn) {
        std::type_index idx(typeid(T));
        concat_funcs_[idx] = [fn](const std::vector<Any>& items) -> Any {
            std::vector<T> typed_items;
            typed_items.reserve(items.size());
            for (const auto& item : items) {
                typed_items.push_back(std::any_cast<T>(item));
            }
            return fn(typed_items);
        };
    }
    
    // Get concatenation function for type
    std::function<Any(const std::vector<Any>&)> GetConcatFunc(std::type_index idx) const {
        auto it = concat_funcs_.find(idx);
        if (it != concat_funcs_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    // Check if type has concat function
    bool HasConcatFunc(std::type_index idx) const {
        return concat_funcs_.find(idx) != concat_funcs_.end();
    }

private:
    StreamChunkConcatRegistry() {
        RegisterDefaultConcatFuncs();
    }
    
    void RegisterDefaultConcatFuncs();
    
    std::map<std::type_index, std::function<Any(const std::vector<Any>&)>> concat_funcs_;
};

// Register a stream chunk concatenation function for type T
// This is required when you want to concat stream chunks of a specific type
// For example, when you call Invoke() but the node only implements Stream()
//
// Usage:
//   RegisterStreamChunkConcatFunc<MyType>([](const std::vector<MyType>& items) {
//       MyType result;
//       for (const auto& item : items) {
//           result.merge(item);  // Your custom merge logic
//       }
//       return result;
//   });
template <typename T>
void RegisterStreamChunkConcatFunc(StreamChunkConcatFunc<T> fn) {
    StreamChunkConcatRegistry::Instance().RegisterConcatFunc<T>(std::move(fn));
}

// Error for empty stream
class EmptyStreamConcatError : public std::runtime_error {
public:
    EmptyStreamConcatError() 
        : std::runtime_error("stream reader is empty, concat fail") {}
};

// Error for stream read failure
class StreamReadError : public std::runtime_error {
public:
    explicit StreamReadError(const std::string& msg)
        : std::runtime_error("stream read error: " + msg) {}
};

// Concatenate all chunks from a stream reader into a single value
// The stream reader will be closed after concatenation
template <typename T>
T ConcatStreamReader(std::shared_ptr<schema::StreamReader<T>> sr) {
    if (!sr) {
        throw std::invalid_argument("stream reader cannot be null");
    }
    
    std::vector<T> items;
    
    try {
        while (true) {
            T chunk;
            bool ok = sr->Recv(chunk);
            if (!ok) {
                // EOF reached
                break;
            }
            items.push_back(std::move(chunk));
        }
    } catch (const std::exception& e) {
        sr->Close();
        throw StreamReadError(e.what());
    }
    
    sr->Close();
    
    if (items.empty()) {
        throw EmptyStreamConcatError();
    }
    
    if (items.size() == 1) {
        return items[0];
    }
    
    // Use registered concat function
    std::type_index idx(typeid(T));
    auto concat_fn = StreamChunkConcatRegistry::Instance().GetConcatFunc(idx);
    
    if (!concat_fn) {
        throw std::runtime_error(
            "no concat function registered for type: " + std::string(idx.name()));
    }
    
    std::vector<Any> any_items;
    any_items.reserve(items.size());
    for (const auto& item : items) {
        any_items.push_back(item);
    }
    
    return std::any_cast<T>(concat_fn(any_items));
}

// Check if a type has concatenation support
template <typename T>
bool HasConcatSupport() {
    std::type_index idx(typeid(T));
    return StreamChunkConcatRegistry::Instance().HasConcatFunc(idx);
}

} // namespace compose
} // namespace eino
