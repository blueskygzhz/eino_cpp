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

#include <any>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <typeinfo>

#include "eino/compose/type_registry.h"

namespace eino {
namespace compose {

/**
 * FieldMapping - Maps fields from one node's output to another node's input
 * Aligns with eino/compose/field_mapping.go
 */
struct FieldMapping {
    // Source field path (dot notation: "field.subfield")
    // Empty means map entire input
    std::string from_key;
    
    // Destination field path
    // Empty means map to entire input
    std::string to_key;
    
    // Whether this mapping uses entire input as source
    bool map_entire_input = false;
    
    // Optional transformer function
    bool transformer = false;
    std::function<std::any(void*, const std::any&)> transformer_func;
};

/**
 * Field type information for validation
 */
struct FieldTypeInfo {
    const std::type_info* type = nullptr;
    bool is_map = false;
    bool is_list = false;
    bool is_pointer = false;
    std::string path;
    
    FieldTypeInfo() = default;
    
    explicit FieldTypeInfo(const std::type_info& t)
        : type(&t) {}
};

/**
 * Validate field mapping between nodes
 * Aligns with eino/compose/graph.go:690-750 (validateFieldMapping)
 * 
 * @param from_type Output type of source node
 * @param to_type Input type of target node
 * @param mappings Field mappings to validate
 * @return Error message if validation fails, empty string otherwise
 */
std::string ValidateFieldMapping(
    const std::type_info& from_type,
    const std::type_info& to_type,
    const std::vector<FieldMapping>& mappings);

/**
 * Check and extract field type from a structured type
 * Aligns with eino/compose/field_mapping.go:CheckAndExtractFieldType
 * 
 * @param base_type The base type to extract from
 * @param field_path Field path (dot notation)
 * @return Field type info or nullptr if not found
 */
FieldTypeInfo CheckAndExtractFieldType(
    const std::type_info& base_type,
    const std::string& field_path);

/**
 * Helper functions for creating field mappings
 */

// Map a single field: from -> to
FieldMapping MapField(const std::string& from, const std::string& to);

// Map a field with transformer
FieldMapping MapFieldWithTransform(
    const std::string& from,
    const std::string& to,
    std::function<std::any(void*, const std::any&)> transformer);

// Map entire input to a field
FieldMapping MapEntireInput(const std::string& to);

// Map a field to entire output
FieldMapping MapFieldToEntire(const std::string& from);

/**
 * Field path parser
 */
class FieldPath {
public:
    explicit FieldPath(const std::string& path);
    
    const std::vector<std::string>& GetSegments() const { return segments_; }
    std::string ToString() const;
    bool IsEmpty() const { return segments_.empty(); }
    
private:
    std::vector<std::string> segments_;
};

/**
 * Type checker for field mappings
 * Handles map[string]any, structs, etc.
 */
class FieldTypeChecker {
public:
    /**
     * Check if source type can be assigned to target type
     * @param source_type Source field type
     * @param target_type Target field type
     * @param allow_transform Whether to allow type transformation
     * @return true if assignable
     */
    static bool IsAssignable(
        const std::type_info& source_type,
        const std::type_info& target_type,
        bool allow_transform = false);
    
    /**
     * Extract field type from map[string]any
     * @param field_path Field path
     * @return Field type or nullptr
     */
    static const std::type_info* ExtractFromMap(
        const std::string& field_path);
    
    /**
     * Extract field type from struct
     * @param struct_type Struct type
     * @param field_path Field path
     * @return Field type or nullptr
     */
    static const std::type_info* ExtractFromStruct(
        const std::type_info& struct_type,
        const std::string& field_path);
};

} // namespace compose
} // namespace eino
