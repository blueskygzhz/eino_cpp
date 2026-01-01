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

#include "eino/compose/field_mapping.h"
#include "eino/compose/utils.h"

#include <sstream>
#include <algorithm>

namespace eino {
namespace compose {

// FieldPath implementation

FieldPath::FieldPath(const std::string& path) {
    if (path.empty()) {
        return;
    }
    
    std::stringstream ss(path);
    std::string segment;
    
    while (std::getline(ss, segment, '.')) {
        if (!segment.empty()) {
            segments_.push_back(segment);
        }
    }
}

std::string FieldPath::ToString() const {
    if (segments_.empty()) {
        return "";
    }
    
    std::stringstream ss;
    for (size_t i = 0; i < segments_.size(); ++i) {
        if (i > 0) {
            ss << ".";
        }
        ss << segments_[i];
    }
    return ss.str();
}

// ValidateFieldMapping implementation
// Aligns with eino/compose/graph.go:690-750

std::string ValidateFieldMapping(
    const std::type_info& from_type,
    const std::type_info& to_type,
    const std::vector<FieldMapping>& mappings) {
    
    // Basic validation
    for (const auto& mapping : mappings) {
        // Check that at least one key is specified
        if (mapping.from_key.empty() && mapping.to_key.empty() && !mapping.map_entire_input) {
            return "FieldMapping: both from_key and to_key are empty";
        }
        
        // Check transformer consistency
        if (mapping.transformer && !mapping.transformer_func) {
            return "FieldMapping: transformer flag set but function is null";
        }
    }
    
    // Type-based validation
    // Go version uses reflect to check field types
    // In C++, we rely on TypeRegistry for type checking
    
    for (const auto& mapping : mappings) {
        // Skip validation if transformer is present (it handles conversion)
        if (mapping.transformer) {
            continue;
        }
        
        // Extract source field type
        FieldTypeInfo from_field_info;
        if (mapping.from_key.empty() || mapping.map_entire_input) {
            from_field_info = FieldTypeInfo(from_type);
        } else {
            from_field_info = CheckAndExtractFieldType(from_type, mapping.from_key);
            if (!from_field_info.type) {
                std::stringstream ss;
                ss << "FieldMapping: cannot find field '" << mapping.from_key 
                   << "' in source type";
                return ss.str();
            }
        }
        
        // Extract target field type
        FieldTypeInfo to_field_info;
        if (mapping.to_key.empty()) {
            to_field_info = FieldTypeInfo(to_type);
        } else {
            to_field_info = CheckAndExtractFieldType(to_type, mapping.to_key);
            if (!to_field_info.type) {
                std::stringstream ss;
                ss << "FieldMapping: cannot find field '" << mapping.to_key 
                   << "' in target type";
                return ss.str();
            }
        }
        
        // Check type compatibility
        if (from_field_info.type && to_field_info.type) {
            AssignableType result = CheckAssignable(*from_field_info.type, *to_field_info.type);
            
            if (result == AssignableType::MustNot) {
                std::stringstream ss;
                ss << "FieldMapping: type mismatch - cannot assign "
                   << from_field_info.type->name() << " to " << to_field_info.type->name()
                   << " (from: " << mapping.from_key << ", to: " << mapping.to_key << ")";
                return ss.str();
            }
        }
    }
    
    return "";  // Validation passed
}

// CheckAndExtractFieldType implementation

FieldTypeInfo CheckAndExtractFieldType(
    const std::type_info& base_type,
    const std::string& field_path) {
    
    FieldPath path(field_path);
    
    if (path.IsEmpty()) {
        return FieldTypeInfo(base_type);
    }
    
    // Check if base type is map[string]any
    if (base_type == typeid(std::map<std::string, std::any>)) {
        // For maps, we can't determine field type at compile time
        // Return std::any as field type
        FieldTypeInfo info(typeid(std::any));
        info.is_map = true;
        info.path = field_path;
        return info;
    }
    
    // For struct types, we would need reflection or type traits
    // This is a limitation in C++ without additional metadata
    // For now, return null to indicate unknown type
    
    return FieldTypeInfo();
}

// Helper function implementations

FieldMapping MapField(const std::string& from, const std::string& to) {
    FieldMapping mapping;
    mapping.from_key = from;
    mapping.to_key = to;
    return mapping;
}

FieldMapping MapFieldWithTransform(
    const std::string& from,
    const std::string& to,
    std::function<std::any(void*, const std::any&)> transformer) {
    
    FieldMapping mapping;
    mapping.from_key = from;
    mapping.to_key = to;
    mapping.transformer = true;
    mapping.transformer_func = transformer;
    return mapping;
}

FieldMapping MapEntireInput(const std::string& to) {
    FieldMapping mapping;
    mapping.from_key = "";
    mapping.to_key = to;
    mapping.map_entire_input = true;
    return mapping;
}

FieldMapping MapFieldToEntire(const std::string& from) {
    FieldMapping mapping;
    mapping.from_key = from;
    mapping.to_key = "";
    return mapping;
}

// FieldTypeChecker implementation

bool FieldTypeChecker::IsAssignable(
    const std::type_info& source_type,
    const std::type_info& target_type,
    bool allow_transform) {
    
    AssignableType result = CheckAssignable(source_type, target_type);
    
    if (result == AssignableType::Must) {
        return true;
    }
    
    if (result == AssignableType::May && allow_transform) {
        return true;
    }
    
    return false;
}

const std::type_info* FieldTypeChecker::ExtractFromMap(
    const std::string& field_path) {
    
    // map[string]any can contain any type
    // Return std::any as the field type
    return &typeid(std::any);
}

const std::type_info* FieldTypeChecker::ExtractFromStruct(
    const std::type_info& struct_type,
    const std::string& field_path) {
    
    // Without reflection, we cannot extract struct field types at runtime
    // This would require compile-time metadata or external type database
    
    return nullptr;
}

} // namespace compose
} // namespace eino
