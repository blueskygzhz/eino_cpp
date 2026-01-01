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

#include "eino/compose/graph_validation.h"
#include "eino/compose/utils.h"

#include <sstream>

namespace eino {
namespace compose {

// GraphValidator implementation

void GraphValidator::AddToValidateMap(
    const std::string& start_node,
    const std::string& end_node,
    const std::vector<FieldMapping>& mappings) {
    
    // Aligns with eino/compose/graph.go:524-527
    to_validate_map_[start_node].emplace_back(end_node, mappings);
}

std::string GraphValidator::UpdateToValidateMap(
    std::map<std::string, const std::type_info*>& node_input_types,
    std::map<std::string, const std::type_info*>& node_output_types,
    bool (*is_passthrough_node)(const std::string&)) {
    
    // Aligns with eino/compose/graph.go:533-630
    // Iterative type inference for passthrough chains
    
    bool has_changed = true;
    int max_iterations = 100;  // Prevent infinite loops
    int iteration = 0;
    
    while (has_changed && iteration < max_iterations) {
        has_changed = false;
        iteration++;
        
        // Process all edges in validation queue
        for (auto& [start_node, edges] : to_validate_map_) {
            const std::type_info* start_output_type = 
                node_output_types.count(start_node) ? node_output_types[start_node] : nullptr;
            
            for (auto it = edges.begin(); it != edges.end(); ) {
                const auto& edge = *it;
                const std::string& end_node = edge.end_node;
                const auto& mappings = edge.mappings;
                
                const std::type_info* end_input_type = 
                    node_input_types.count(end_node) ? node_input_types[end_node] : nullptr;
                
                // Skip if both types are unknown
                if (!start_output_type && !end_input_type) {
                    ++it;
                    continue;
                }
                
                // Remove from validation queue
                it = edges.erase(it);
                has_changed = true;
                
                // Type inference for passthrough nodes
                // Aligns with Go: lines 545-558
                if (start_output_type && !end_input_type) {
                    // Forward inference: end node is passthrough
                    if (is_passthrough_node && is_passthrough_node(end_node)) {
                        node_input_types[end_node] = start_output_type;
                        node_output_types[end_node] = start_output_type;
                    }
                } else if (!start_output_type && end_input_type) {
                    // Backward inference: start node is passthrough
                    if (is_passthrough_node && is_passthrough_node(start_node)) {
                        node_input_types[start_node] = end_input_type;
                        node_output_types[start_node] = end_input_type;
                    }
                } else if (start_output_type && end_input_type) {
                    // Both types known - validate
                    
                    if (mappings.empty()) {
                        // Direct connection - check type compatibility
                        // Aligns with Go: lines 560-575
                        AssignableType result = CheckAssignable(*start_output_type, *end_input_type);
                        
                        if (result == AssignableType::MustNot) {
                            std::stringstream ss;
                            ss << "Graph edge [" << start_node << "]->[" << end_node 
                               << "]: start node's output type [" << start_output_type->name()
                               << "] and end node's input type [" << end_input_type->name()
                               << "] mismatch";
                            return ss.str();
                        }
                        // For AssignableType::May, runtime check will be added
                    } else {
                        // Field mapping connection - validate mappings
                        // Aligns with Go: lines 578-595
                        std::string err = ValidateFieldMapping(
                            *start_output_type, 
                            *end_input_type, 
                            mappings);
                        
                        if (!err.empty()) {
                            std::stringstream ss;
                            ss << "Graph edge [" << start_node << "]->[" << end_node 
                               << "]: field mapping validation failed: " << err;
                            return ss.str();
                        }
                    }
                }
            }
        }
        
        // Remove empty entries
        for (auto it = to_validate_map_.begin(); it != to_validate_map_.end(); ) {
            if (it->second.empty()) {
                it = to_validate_map_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Check for unresolved edges (should not happen in valid graphs)
    if (!to_validate_map_.empty()) {
        std::stringstream ss;
        ss << "Failed to infer types for " << GetPendingCount() 
           << " edges after " << iteration << " iterations. "
           << "This may indicate circular passthrough chains or disconnected nodes.";
        return ss.str();
    }
    
    return "";  // Success
}

size_t GraphValidator::GetPendingCount() const {
    size_t count = 0;
    for (const auto& [_, edges] : to_validate_map_) {
        count += edges.size();
    }
    return count;
}

// PassthroughTypeInference implementation

bool PassthroughTypeInference::InferTypes(
    const std::string& start_node,
    const std::string& end_node,
    const std::type_info* start_output_type,
    const std::type_info* end_input_type,
    std::map<std::string, const std::type_info*>& node_input_types,
    std::map<std::string, const std::type_info*>& node_output_types) {
    
    if (start_output_type && !end_input_type) {
        // Forward inference
        InferForward(end_node, *start_output_type, node_input_types);
        node_output_types[end_node] = start_output_type;
        return true;
    }
    
    if (!start_output_type && end_input_type) {
        // Backward inference
        InferBackward(start_node, *end_input_type, node_output_types);
        node_input_types[start_node] = end_input_type;
        return true;
    }
    
    return false;
}

void PassthroughTypeInference::InferForward(
    const std::string& target_node,
    const std::type_info& source_type,
    std::map<std::string, const std::type_info*>& node_types) {
    
    node_types[target_node] = &source_type;
}

void PassthroughTypeInference::InferBackward(
    const std::string& target_node,
    const std::type_info& source_type,
    std::map<std::string, const std::type_info*>& node_types) {
    
    node_types[target_node] = &source_type;
}

// EdgeTypeValidator implementation

std::string EdgeTypeValidator::ValidateEdge(
    const std::type_info& start_output,
    const std::type_info& end_input,
    const std::vector<FieldMapping>& mappings) {
    
    if (mappings.empty()) {
        // Direct connection
        AssignableType result = CheckAssignable(start_output, end_input);
        
        if (result == AssignableType::MustNot) {
            std::stringstream ss;
            ss << "Type mismatch: cannot assign " 
               << start_output.name() << " to " << end_input.name();
            return ss.str();
        }
        
        return "";  // Valid
    }
    
    // Field mapping connection
    return ValidateFieldMapping(start_output, end_input, mappings);
}

bool EdgeTypeValidator::NeedsRuntimeCheck(
    const std::type_info& start_output,
    const std::type_info& end_input) {
    
    AssignableType result = CheckAssignable(start_output, end_input);
    return result == AssignableType::May;
}

} // namespace compose
} // namespace eino
