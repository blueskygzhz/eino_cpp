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

#include <map>
#include <string>
#include <vector>
#include <typeinfo>

#include "eino/compose/field_mapping.h"

namespace eino {
namespace compose {

/**
 * Edge validation entry for delayed type inference
 * Aligns with eino/compose/graph.go:524-540 (toValidateMap)
 */
struct EdgeToValidate {
    std::string end_node;
    std::vector<FieldMapping> mappings;
    
    EdgeToValidate(const std::string& end, const std::vector<FieldMapping>& maps)
        : end_node(end), mappings(maps) {}
};

/**
 * Graph validation state
 * Manages delayed type inference for passthrough nodes
 * Aligns with eino/compose/graph.go:525-560 (updateToValidateMap)
 */
class GraphValidator {
public:
    /**
     * Add edge to validation queue
     * @param start_node Source node
     * @param end_node Target node
     * @param mappings Field mappings (empty for direct connection)
     */
    void AddToValidateMap(
        const std::string& start_node,
        const std::string& end_node,
        const std::vector<FieldMapping>& mappings);
    
    /**
     * Update toValidateMap after node type changes
     * Performs iterative type inference for passthrough chains
     * Aligns with eino/compose/graph.go:533-630
     * 
     * @return Error message if validation fails, empty string otherwise
     */
    std::string UpdateToValidateMap(
        std::map<std::string, const std::type_info*>& node_input_types,
        std::map<std::string, const std::type_info*>& node_output_types,
        bool is_passthrough_node(const std::string&));
    
    /**
     * Check if validation queue is empty
     */
    bool IsEmpty() const { return to_validate_map_.empty(); }
    
    /**
     * Get number of pending validations
     */
    size_t GetPendingCount() const;
    
    /**
     * Clear validation queue
     */
    void Clear() { to_validate_map_.clear(); }

private:
    // Map from start node to list of edges to validate
    // Aligns with Go: toValidateMap map[string][]struct{endNode string, mappings []*FieldMapping}
    std::map<std::string, std::vector<EdgeToValidate>> to_validate_map_;
};

/**
 * Type inference helper for passthrough nodes
 * Passthrough nodes inherit types from their neighbors
 */
class PassthroughTypeInference {
public:
    /**
     * Infer types for passthrough chains
     * Supports forward, backward, and multi-round inference
     * 
     * @param start_node Source node
     * @param end_node Target node
     * @param start_output_type Source node output type (may be null)
     * @param end_input_type Target node input type (may be null)
     * @param node_types In/out parameter for node types
     * @return true if inference was performed
     */
    static bool InferTypes(
        const std::string& start_node,
        const std::string& end_node,
        const std::type_info* start_output_type,
        const std::type_info* end_input_type,
        std::map<std::string, const std::type_info*>& node_input_types,
        std::map<std::string, const std::type_info*>& node_output_types);
    
    /**
     * Forward inference: passthrough inherits from predecessor
     * Aligns with Go: g.nodes[endNode].cr.inputType = startNodeOutputType
     */
    static void InferForward(
        const std::string& target_node,
        const std::type_info& source_type,
        std::map<std::string, const std::type_info*>& node_types);
    
    /**
     * Backward inference: passthrough inherits from successor
     * Aligns with Go: g.nodes[startNode].cr.inputType = endNodeInputType
     */
    static void InferBackward(
        const std::string& target_node,
        const std::type_info& source_type,
        std::map<std::string, const std::type_info*>& node_types);
};

/**
 * Edge type validator
 * Checks type compatibility between connected nodes
 */
class EdgeTypeValidator {
public:
    /**
     * Validate edge type compatibility
     * Aligns with eino/compose/graph.go:570-585
     * 
     * @param start_output Source node output type
     * @param end_input Target node input type
     * @param mappings Field mappings (empty for direct connection)
     * @return Error message if validation fails, empty string otherwise
     */
    static std::string ValidateEdge(
        const std::type_info& start_output,
        const std::type_info& end_input,
        const std::vector<FieldMapping>& mappings);
    
    /**
     * Check if runtime type conversion is needed
     * Returns true for AssignableType::May case
     */
    static bool NeedsRuntimeCheck(
        const std::type_info& start_output,
        const std::type_info& end_input);
};

} // namespace compose
} // namespace eino
