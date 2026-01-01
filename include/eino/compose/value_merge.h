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

#ifndef EINO_CPP_COMPOSE_VALUE_MERGE_H_
#define EINO_CPP_COMPOSE_VALUE_MERGE_H_

// Aligns with: eino/compose/values_merge.go, eino/internal/merge.go
// Value merge functionality for fan-in scenarios

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <typeindex>
#include <functional>
#include <stdexcept>
#include <any>

namespace eino {
namespace compose {

// =============================================================================
// Merge Function Types
// Aligns with: eino/internal/merge.go
// =============================================================================

// Generic merge function that works with type-erased values
// Returns merged value or throws on error
using MergeFunc = std::function<std::any(const std::vector<std::any>&)>;

// Merge options
// Aligns with: eino/compose/values_merge.go:33-36
struct MergeOptions {
    bool stream_merge_with_source_eof = false;
    std::vector<std::string> names;
    
    MergeOptions() = default;
};

// =============================================================================
// Merge Registry
// Aligns with: eino/internal/merge.go:19-26
// =============================================================================

/**
 * @brief Registry for custom value merge functions
 * 
 * Singleton that maintains a mapping from types to their merge functions.
 * Users can register custom merge logic for their types.
 */
class MergeRegistry {
public:
    static MergeRegistry& Instance();
    
    /**
     * @brief Register a custom merge function for type T
     * 
     * Aligns with: eino/internal/merge.go:28-30 RegisterValuesMergeFunc
     * 
     * @param fn Merge function that takes vector of T and returns merged T
     * 
     * Example:
     *   RegisterMergeFunc<MyType>([](const std::vector<MyType>& values) {
     *       MyType merged;
     *       for (const auto& v : values) {
     *           // merge logic
     *       }
     *       return merged;
     *   });
     */
    template<typename T>
    void RegisterMergeFunc(std::function<T(const std::vector<T>&)> fn) {
        auto type_idx = std::type_index(typeid(T));
        
        merge_funcs_[type_idx] = [fn](const std::vector<std::any>& values) -> std::any {
            // Convert std::any to T
            std::vector<T> typed_values;
            typed_values.reserve(values.size());
            
            for (const auto& v : values) {
                try {
                    typed_values.push_back(std::any_cast<T>(v));
                } catch (const std::bad_any_cast& e) {
                    throw std::runtime_error(
                        std::string("(values merge) field type mismatch: ") + e.what());
                }
            }
            
            // Call user function
            T result = fn(typed_values);
            return std::any(result);
        };
    }
    
    /**
     * @brief Get merge function for a type
     * 
     * Aligns with: eino/internal/merge.go:32-51 GetMergeFunc
     */
    MergeFunc GetMergeFunc(std::type_index type) const;
    
    /**
     * @brief Check if a type has registered merge function
     */
    bool HasMergeFunc(std::type_index type) const;
    
    /**
     * @brief Check if type is a map type
     */
    static bool IsMapType(const std::type_info& type);
    
private:
    MergeRegistry();
    ~MergeRegistry() = default;
    
    // Prevent copy and move
    MergeRegistry(const MergeRegistry&) = delete;
    MergeRegistry& operator=(const MergeRegistry&) = delete;
    
    void RegisterBuiltinMergeFuncs();
    
    std::map<std::type_index, MergeFunc> merge_funcs_;
};

// =============================================================================
// Public API
// Aligns with: eino/compose/values_merge.go
// =============================================================================

/**
 * @brief Register a value merge function for type T
 * 
 * Aligns with: eino/compose/values_merge.go:29-31 RegisterValuesMergeFunc
 * 
 * This is the main API for users to register custom merge logic.
 * 
 * Example:
 *   RegisterValuesMergeFunc<MyStruct>([](const std::vector<MyStruct>& vs) {
 *       MyStruct result;
 *       for (const auto& v : vs) {
 *           result.Merge(v);
 *       }
 *       return result;
 *   });
 */
template<typename T>
void RegisterValuesMergeFunc(std::function<T(const std::vector<T>&)> fn) {
    MergeRegistry::Instance().RegisterMergeFunc<T>(fn);
}

/**
 * @brief Merge multiple values into one
 * 
 * Aligns with: eino/compose/values_merge.go:39-81 mergeValues
 * 
 * @param values Vector of values to merge (must have same type)
 * @param opts Merge options (optional)
 * @return Merged value as std::any
 * @throws std::runtime_error if merge fails or type not supported
 * 
 * Merge strategy:
 * 1. If only one value, return it directly
 * 2. Look for registered custom merge function
 * 3. Use built-in merge for map types
 * 4. Throw error if type not supported
 */
std::any MergeValues(
    const std::vector<std::any>& values,
    const MergeOptions* opts = nullptr);

/**
 * @brief Merge map values (built-in strategy)
 * 
 * Aligns with: eino/internal/merge.go:59-81 mergeMap
 * 
 * Merges all key-value pairs from input maps into one map.
 * Throws error if duplicate keys are found.
 * 
 * @param values Vector of map values
 * @return Merged map as std::any
 * @throws std::runtime_error on duplicate keys or type mismatch
 */
template<typename K, typename V>
std::map<K, V> MergeMapValues(const std::vector<std::map<K, V>>& values) {
    std::map<K, V> merged;
    
    for (const auto& m : values) {
        for (const auto& [key, val] : m) {
            auto it = merged.find(key);
            if (it != merged.end()) {
                throw std::runtime_error(
                    "(values merge map) duplicated key found");
            }
            merged[key] = val;
        }
    }
    
    return merged;
}

/**
 * @brief Helper to extract type index from std::any
 */
inline std::type_index GetTypeIndex(const std::any& value) {
    return std::type_index(value.type());
}

/**
 * @brief Check if all values have the same type
 */
bool AllSameType(const std::vector<std::any>& values);

/**
 * @brief Get type name for error messages
 */
std::string GetTypeName(std::type_index type);

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_VALUE_MERGE_H_
