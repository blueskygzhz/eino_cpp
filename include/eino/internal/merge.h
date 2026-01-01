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

#ifndef EINO_CPP_INTERNAL_MERGE_H_
#define EINO_CPP_INTERNAL_MERGE_H_

#include <vector>
#include <map>
#include <functional>
#include <stdexcept>

namespace eino {
namespace internal {

// MergeFunc type for custom merge logic
template<typename T>
using MergeFunc = std::function<T(const std::vector<T>&)>;

// Registry for custom merge functions
template<typename T>
class MergeRegistry {
public:
    static MergeRegistry& Instance() {
        static MergeRegistry instance;
        return instance;
    }
    
    void Register(MergeFunc<T> func) {
        func_ = func;
    }
    
    bool HasFunc() const {
        return func_ != nullptr;
    }
    
    T Merge(const std::vector<T>& items) const {
        if (!func_) {
            throw std::runtime_error("No merge function registered for this type");
        }
        return func_(items);
    }
    
private:
    MergeRegistry() = default;
    MergeFunc<T> func_;
};

// Register a merge function for type T
template<typename T>
void RegisterValuesMergeFunc(MergeFunc<T> func) {
    MergeRegistry<T>::Instance().Register(func);
}

// MergeMap - merge multiple maps by combining values for duplicate keys
template<typename K, typename V>
std::map<K, V> MergeMap(const std::vector<std::map<K, V>>& maps) {
    std::map<K, V> merged;
    
    for (const auto& m : maps) {
        for (const auto& pair : m) {
            const auto& key = pair.first;
            const auto& val = pair.second;
            
            if (merged.find(key) != merged.end()) {
                throw std::runtime_error("Duplicated key found during map merge");
            }
            merged[key] = val;
        }
    }
    
    return merged;
}

// MergeValues - generic merge using registered function
template<typename T>
T MergeValues(const std::vector<T>& values) {
    if (values.empty()) {
        throw std::runtime_error("Cannot merge empty vector");
    }
    
    if (values.size() == 1) {
        return values[0];
    }
    
    auto& registry = MergeRegistry<T>::Instance();
    if (registry.HasFunc()) {
        return registry.Merge(values);
    }
    
    throw std::runtime_error("No merge function available for this type");
}

} // namespace internal
} // namespace eino

#endif // EINO_CPP_INTERNAL_MERGE_H_
