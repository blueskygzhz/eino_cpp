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

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>

#include "eino/compose/stream_reader.h"

namespace eino {
namespace compose {

// Merge function type for a specific type T
template <typename T>
using MergeFunc = std::function<T(const std::vector<T>&)>;

// Merge options for fan-in operations
struct MergeOptions {
    // Whether to merge streams with source EOF
    bool stream_merge_with_source_eof = false;
    
    // Names for named merge
    std::vector<std::string> names;
    
    MergeOptions() = default;
};

// Registry for value merge functions
class ValuesMergeRegistry {
public:
    static ValuesMergeRegistry& Instance() {
        static ValuesMergeRegistry instance;
        return instance;
    }
    
    // Register a merge function for type T
    template <typename T>
    void RegisterMergeFunc(MergeFunc<T> fn) {
        std::type_index idx(typeid(T));
        merge_funcs_[idx] = [fn](const std::vector<Any>& values) -> Any {
            std::vector<T> typed_values;
            typed_values.reserve(values.size());
            for (const auto& v : values) {
                typed_values.push_back(std::any_cast<T>(v));
            }
            return fn(typed_values);
        };
    }
    
    // Get merge function for a type
    std::function<Any(const std::vector<Any>&)> GetMergeFunc(std::type_index idx) const {
        auto it = merge_funcs_.find(idx);
        if (it != merge_funcs_.end()) {
            return it->second;
        }
        return nullptr;
    }

private:
    ValuesMergeRegistry() {
        // Register default merge functions for common types
        RegisterDefaultMergeFuncs();
    }
    
    void RegisterDefaultMergeFuncs();
    
    std::map<std::type_index, std::function<Any(const std::vector<Any>&)>> merge_funcs_;
};

// Register a merge function for type T
template <typename T>
void RegisterValuesMergeFunc(MergeFunc<T> fn) {
    ValuesMergeRegistry::Instance().RegisterMergeFunc<T>(std::move(fn));
}

// Merge values from multiple sources
// Caller should ensure vs.size() > 1
Any MergeValues(const std::vector<Any>& vs, const MergeOptions* opts = nullptr);

// Default merge function for maps
std::map<std::string, Any> MergeMaps(const std::vector<std::map<std::string, Any>>& maps);

// Merge streams
std::shared_ptr<IStreamReader> MergeStreams(
    const std::vector<std::shared_ptr<IStreamReader>>& streams,
    const MergeOptions* opts = nullptr);

} // namespace compose
} // namespace eino
