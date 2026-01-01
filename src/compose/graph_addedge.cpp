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

#include "eino/compose/graph.h"
#include <stdexcept>
#include <sstream>

namespace eino {
namespace compose {

// AddEdge implementations with full field mapping support
// Aligns with eino/compose/graph.go:479-560

template<typename I, typename O>
void Graph<I, O>::AddEdge(
    const std::string& from,
    const std::string& to,
    const std::vector<FieldMapping>& mappings) {
    
    AddEdgeInternal(from, to, false, false, mappings);
}

template<typename I, typename O>
void Graph<I, O>::AddControlEdge(
    const std::string& from,
    const std::string& to) {
    
    AddEdgeInternal(from, to, true, false, {});
}

template<typename I, typename O>
void Graph<I, O>::AddEdgeInternal(
    const std::string& from,
    const std::string& to,
    bool is_control,
    bool is_data,
    const std::vector<FieldMapping>& mappings) {
    
    // Aligns with eino/compose/graph.go:515-560
    
    if (is_compiled_) {
        throw std::runtime_error("Graph already compiled, cannot add edge");
    }
    
    if (has_error_) {
        throw std::runtime_error("Graph has build error");
    }
    
    // Validate nodes exist
    // Aligns with eino/compose/graph.go:521-530
    if (from != START_NODE && from != END_NODE && !nodes_.count(from)) {
        std::stringstream ss;
        ss << "AddEdge: from node not found: " << from;
        has_error_ = true;
        build_error_ = ss.str();
        throw std::runtime_error(build_error_);
    }
    
    if (to != START_NODE && to != END_NODE && !nodes_.count(to)) {
        std::stringstream ss;
        ss << "AddEdge: to node not found: " << to;
        has_error_ = true;
        build_error_ = ss.str();
        throw std::runtime_error(build_error_);
    }
    
    // Check for self-loop
    // Aligns with eino/compose/graph.go:535
    if (from == to) {
        std::stringstream ss;
        ss << "AddEdge: self-loop not allowed: " << from;
        has_error_ = true;
        build_error_ = ss.str();
        throw std::runtime_error(build_error_);
    }
    
    // Check for duplicate edge
    // Aligns with eino/compose/graph.go:540-545
    for (const auto& edge : adjacency_list_[from]) {
        if (edge.to == to) {
            std::stringstream ss;
            ss << "AddEdge: duplicate edge: " << from << " -> " << to;
            has_error_ = true;
            build_error_ = ss.str();
            throw std::runtime_error(build_error_);
        }
    }
    
    // Create edge
    // Aligns with eino/compose/graph.go:547-555
    GraphEdge edge;
    edge.from = from;
    edge.to = to;
    edge.is_control = is_control;
    edge.is_data = is_data || !mappings.empty();
    edge.field_mappings = mappings;
    
    // Validate field mappings
    if (!mappings.empty()) {
        ValidateFieldMappings(from, to, mappings);
    }
    
    // ✅ CRITICAL FIX: Add to validation map for type checking
    // Aligns with eino/compose/graph.go:285 (AddEdge) and 487 (AddLambdaEdge)
    // This ensures type compatibility checking happens during graph construction
    if (!is_control && (is_data || !mappings.empty())) {
        // Add to validation map
        validator_.AddToValidateMap(from, to, mappings);
        
        // Immediately update validation map to detect type mismatches early
        // Aligns with eino/compose/graph.go:286-289
        auto validation_err = validator_.UpdateToValidateMap(
            node_input_types_,
            node_output_types_,
            [this](const std::string& node_name) -> bool {
                // Check if node is passthrough (Lambda nodes typically are)
                // For now, we assume non-passthrough
                // TODO: Implement proper passthrough detection
                return false;
            }
        );
        
        if (!validation_err.empty()) {
            has_error_ = true;
            build_error_ = validation_err;
            throw std::runtime_error(validation_err);
        }
    }
    
    adjacency_list_[from].push_back(edge);
    in_degree_[to]++;
    
    // Track start/end nodes
    if (from == START_NODE) {
        start_nodes_.push_back(to);
    }
    if (to == END_NODE) {
        end_nodes_.push_back(from);
    }
}

template<typename I, typename O>
void Graph<I, O>::ValidateFieldMappings(
    const std::string& from,
    const std::string& to,
    const std::vector<FieldMapping>& mappings) {
    
    // Aligns with eino/compose/graph.go:690-750
    
    // Get node types for validation
    auto from_node = nodes_.find(from);
    auto to_node = nodes_.find(to);
    
    if (from_node == nodes_.end() || to_node == nodes_.end()) {
        return;  // Validation will happen at compile time
    }
    
    // For now, just validate mapping structure
    for (const auto& mapping : mappings) {
        if (mapping.from_key.empty() && mapping.to_key.empty()) {
            throw std::invalid_argument("FieldMapping: both keys are empty");
        }
        
        // Validate transformer if present
        if (mapping.transformer && !mapping.transformer_func) {
            throw std::invalid_argument("FieldMapping: transformer set but function is null");
        }
    }
}

// FieldMapping helper functions
// Aligns with eino/compose/graph.go:760-850

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
    mapping.from_key = "";  // Empty means entire input
    mapping.to_key = to;
    mapping.map_entire_input = true;
    return mapping;
}

// Explicit template instantiations for common types
template class Graph<std::vector<schema::Message>, schema::Message>;
template class Graph<schema::Message, std::vector<schema::Message>>;
template class Graph<std::string, std::string>;
template class Graph<std::map<std::string, std::any>, std::map<std::string, std::any>>;

} // namespace compose
} // namespace eino
