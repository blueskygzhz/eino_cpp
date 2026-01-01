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

// Aligns with: eino/compose/values_merge.go, eino/internal/merge.go
// Implementation of value merge functionality

#include "eino/compose/value_merge.h"
#include <sstream>
#include <cxxabi.h>

namespace eino {
namespace compose {

// =============================================================================
// MergeRegistry Implementation
// Aligns with: eino/internal/merge.go
// =============================================================================

MergeRegistry::MergeRegistry() {
    RegisterBuiltinMergeFuncs();
}

MergeRegistry& MergeRegistry::Instance() {
    static MergeRegistry instance;
    return instance;
}

MergeFunc MergeRegistry::GetMergeFunc(std::type_index type) const {
    auto it = merge_funcs_.find(type);
    if (it != merge_funcs_.end()) {
        return it->second;
    }
    return nullptr;
}

bool MergeRegistry::HasMergeFunc(std::type_index type) const {
    return merge_funcs_.find(type) != merge_funcs_.end();
}

bool MergeRegistry::IsMapType(const std::type_info& type) {
    // Simple heuristic: check if type name contains "map"
    // This is a simplified version; more robust type checking can be added
    std::string type_name = type.name();
    return type_name.find("map") != std::string::npos;
}

void MergeRegistry::RegisterBuiltinMergeFuncs() {
    // Register common map types merge functions
    // Aligns with: eino/internal/merge.go:53-58 (Map kind check)
    
    // std::map<std::string, std::string>
    RegisterMergeFunc<std::map<std::string, std::string>>(
        [](const std::vector<std::map<std::string, std::string>>& values) {
            return MergeMapValues(values);
        });
    
    // std::map<std::string, int>
    RegisterMergeFunc<std::map<std::string, int>>(
        [](const std::vector<std::map<std::string, int>>& values) {
            return MergeMapValues(values);
        });
    
    // std::map<std::string, double>
    RegisterMergeFunc<std::map<std::string, double>>(
        [](const std::vector<std::map<std::string, double>>& values) {
            return MergeMapValues(values);
        });
    
    // std::map<std::string, std::any>
    RegisterMergeFunc<std::map<std::string, std::any>>(
        [](const std::vector<std::map<std::string, std::any>>& values) {
            return MergeMapValues(values);
        });
    
    // Add more common map types as needed
}

// =============================================================================
// MergeValues Implementation
// Aligns with: eino/compose/values_merge.go:39-81
// =============================================================================

std::any MergeValues(
    const std::vector<std::any>& values,
    const MergeOptions* opts) {
    
    // Precondition check
    if (values.empty()) {
        throw std::runtime_error("(mergeValues) empty values list");
    }
    
    // If only one value, return it directly
    // Aligns with: eino/compose/dag.go:178-180
    if (values.size() == 1) {
        return values[0];
    }
    
    // Check if all values have the same type
    if (!AllSameType(values)) {
        throw std::runtime_error(
            "(mergeValues) all values must have the same type");
    }
    
    // Get type of first value
    std::type_index type = GetTypeIndex(values[0]);
    
    // 1. Look for registered custom merge function
    // Aligns with: eino/compose/values_merge.go:42-45
    auto& registry = MergeRegistry::Instance();
    auto merge_func = registry.GetMergeFunc(type);
    
    if (merge_func) {
        try {
            return merge_func(values);
        } catch (const std::exception& e) {
            throw std::runtime_error(
                std::string("(mergeValues) merge function failed: ") + e.what());
        }
    }
    
    // 2. Handle StreamReader (if implemented)
    // Aligns with: eino/compose/values_merge.go:47-78
    // TODO: Implement StreamReader merge when stream support is added
    
    // 3. Type not supported - throw error
    // Aligns with: eino/compose/values_merge.go:80-81
    throw std::runtime_error(
        "(mergeValues) unsupported type: " + GetTypeName(type) + 
        ". Please register a merge function using RegisterValuesMergeFunc<T>()");
}

// =============================================================================
// Helper Functions
// =============================================================================

bool AllSameType(const std::vector<std::any>& values) {
    if (values.empty()) {
        return true;
    }
    
    auto first_type = values[0].type();
    for (size_t i = 1; i < values.size(); ++i) {
        if (values[i].type() != first_type) {
            return false;
        }
    }
    return true;
}

std::string GetTypeName(std::type_index type) {
    const char* mangled = type.name();
    
    // Try to demangle the name
    int status;
    char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    
    std::string result;
    if (status == 0 && demangled) {
        result = demangled;
        free(demangled);
    } else {
        result = mangled;
    }
    
    return result;
}

} // namespace compose
} // namespace eino
