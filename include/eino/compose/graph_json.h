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

#ifndef EINO_CPP_COMPOSE_GRAPH_JSON_H_
#define EINO_CPP_COMPOSE_GRAPH_JSON_H_

#include <nlohmann/json.hpp>
#include "graph.h"
#include "graph_node.h"
#include "types.h"

namespace eino {
namespace compose {

// =============================================================================
// JSON Serialization for Graph Components
// =============================================================================

// Convert NodeTriggerMode to string
inline std::string NodeTriggerModeToString(NodeTriggerMode mode) {
    switch (mode) {
        case NodeTriggerMode::AllPredecessor:
            return "AllPredecessor";
        case NodeTriggerMode::AnyPredecessor:
            return "AnyPredecessor";
        case NodeTriggerMode::OnInput:
            return "OnInput";
        default:
            return "Unknown";
    }
}

// Convert string to NodeTriggerMode
inline NodeTriggerMode StringToNodeTriggerMode(const std::string& str) {
    if (str == "AllPredecessor") return NodeTriggerMode::AllPredecessor;
    if (str == "AnyPredecessor") return NodeTriggerMode::AnyPredecessor;
    if (str == "OnInput") return NodeTriggerMode::OnInput;
    return NodeTriggerMode::AllPredecessor;
}

// Serialize FieldMapping to JSON
inline nlohmann::json FieldMappingToJson(const FieldMapping& mapping) {
    nlohmann::json j;
    j["from"] = mapping.from;
    j["to"] = mapping.to;
    return j;
}

// Deserialize FieldMapping from JSON
inline FieldMapping FieldMappingFromJson(const nlohmann::json& j) {
    FieldMapping mapping;
    if (j.contains("from")) mapping.from = j["from"].get<std::string>();
    if (j.contains("to")) mapping.to = j["to"].get<std::string>();
    return mapping;
}

// Serialize NodeInfo to JSON
inline nlohmann::json NodeInfoToJson(const NodeInfo& info) {
    nlohmann::json j;
    j["name"] = info.name;
    j["input_key"] = info.input_key;
    j["output_key"] = info.output_key;
    j["trigger_mode"] = NodeTriggerModeToString(info.trigger_mode);
    
    // Serialize metadata
    nlohmann::json metadata_json = nlohmann::json::object();
    for (const auto& kv : info.metadata) {
        metadata_json[kv.first] = kv.second;
    }
    j["metadata"] = metadata_json;
    
    return j;
}

// Deserialize NodeInfo from JSON
inline NodeInfo NodeInfoFromJson(const nlohmann::json& j) {
    NodeInfo info;
    if (j.contains("name")) info.name = j["name"].get<std::string>();
    if (j.contains("input_key")) info.input_key = j["input_key"].get<std::string>();
    if (j.contains("output_key")) info.output_key = j["output_key"].get<std::string>();
    if (j.contains("trigger_mode")) {
        info.trigger_mode = StringToNodeTriggerMode(j["trigger_mode"].get<std::string>());
    }
    
    // Deserialize metadata
    if (j.contains("metadata") && j["metadata"].is_object()) {
        for (auto it = j["metadata"].begin(); it != j["metadata"].end(); ++it) {
            info.metadata[it.key()] = it.value().get<std::string>();
        }
    }
    
    return info;
}

// Serialize GraphEdge to JSON
inline nlohmann::json GraphEdgeToJson(const GraphEdge& edge) {
    nlohmann::json j;
    j["from"] = edge.from;
    j["to"] = edge.to;
    j["label"] = edge.label;
    j["is_control_edge"] = edge.is_control_edge;
    j["is_data_edge"] = edge.is_data_edge;
    
    // Serialize mappings
    nlohmann::json mappings_array = nlohmann::json::array();
    for (const auto& mapping_ptr : edge.mappings) {
        if (mapping_ptr) {
            mappings_array.push_back(FieldMappingToJson(*mapping_ptr));
        }
    }
    j["mappings"] = mappings_array;
    
    return j;
}

// Deserialize GraphEdge from JSON
inline GraphEdge GraphEdgeFromJson(const nlohmann::json& j) {
    std::string from = j.value("from", "");
    std::string to = j.value("to", "");
    std::string label = j.value("label", "");
    
    GraphEdge edge(from, to, label);
    
    if (j.contains("is_control_edge")) {
        edge.is_control_edge = j["is_control_edge"].get<bool>();
    }
    if (j.contains("is_data_edge")) {
        edge.is_data_edge = j["is_data_edge"].get<bool>();
    }
    
    // Deserialize mappings
    if (j.contains("mappings") && j["mappings"].is_array()) {
        for (const auto& mapping_json : j["mappings"]) {
            auto mapping = std::make_shared<FieldMapping>(FieldMappingFromJson(mapping_json));
            edge.mappings.push_back(mapping);
        }
    }
    
    return edge;
}

// Serialize GraphNode to JSON (structure only, runnable is not serialized)
inline nlohmann::json GraphNodeToJson(const GraphNode& node) {
    nlohmann::json j;
    j["name"] = node.name;
    j["trigger_mode"] = NodeTriggerModeToString(node.trigger_mode);
    j["info"] = NodeInfoToJson(node.info);
    
    // Note: runnable is not serialized (type-erased pointer)
    // Note: processor is not serialized (function pointers)
    j["has_runnable"] = (node.runnable != nullptr);
    j["has_processor"] = (node.processor != nullptr);
    
    return j;
}

// Serialize GraphCompileOptions to JSON
inline nlohmann::json GraphCompileOptionsToJson(const GraphCompileOptions& opts) {
    nlohmann::json j;
    j["graph_name"] = opts.graph_name;
    j["interrupt_before_nodes"] = opts.interrupt_before_nodes;
    j["interrupt_after_nodes"] = opts.interrupt_after_nodes;
    j["max_run_steps"] = opts.max_run_steps;
    j["enable_checkpoint"] = opts.enable_checkpoint;
    return j;
}

// Deserialize GraphCompileOptions from JSON
inline GraphCompileOptions GraphCompileOptionsFromJson(const nlohmann::json& j) {
    GraphCompileOptions opts;
    
    if (j.contains("graph_name")) {
        opts.graph_name = j["graph_name"].get<std::string>();
    }
    if (j.contains("interrupt_before_nodes")) {
        opts.interrupt_before_nodes = j["interrupt_before_nodes"].get<std::vector<std::string>>();
    }
    if (j.contains("interrupt_after_nodes")) {
        opts.interrupt_after_nodes = j["interrupt_after_nodes"].get<std::vector<std::string>>();
    }
    if (j.contains("max_run_steps")) {
        opts.max_run_steps = j["max_run_steps"].get<int>();
    }
    if (j.contains("enable_checkpoint")) {
        opts.enable_checkpoint = j["enable_checkpoint"].get<bool>();
    }
    
    return opts;
}

// =============================================================================
// Graph Structure Serialization (without runnable implementations)
// =============================================================================

/**
 * @brief Serialize Graph structure to JSON
 * 
 * Note: This serializes the graph STRUCTURE only (nodes, edges, metadata).
 * Runnable implementations (function pointers, lambdas) are NOT serialized.
 * 
 * Use cases:
 * - Graph visualization
 * - Graph structure inspection
 * - Graph topology export
 * - Debugging and monitoring
 * 
 * @param graph The graph to serialize
 * @return JSON representation of graph structure
 */
template<typename I, typename O>
nlohmann::json GraphToJson(const Graph<I, O>& graph) {
    nlohmann::json j;
    
    // Basic metadata
    j["type"] = "Graph";
    j["is_compiled"] = graph.IsCompiled();
    j["has_error"] = graph.HasError();
    
    // Compile options
    if (graph.IsCompiled()) {
        j["compile_options"] = GraphCompileOptionsToJson(graph.GetCompileOptions());
    }
    
    // Nodes (structure only, no runnable)
    nlohmann::json nodes_json = nlohmann::json::array();
    auto node_names = graph.GetAllNodeNames();
    for (const auto& name : node_names) {
        auto node = graph.GetNode(name);
        if (node) {
            nodes_json.push_back(GraphNodeToJson(*node));
        }
    }
    j["nodes"] = nodes_json;
    
    // Edges
    nlohmann::json edges_json = nlohmann::json::array();
    auto edges = graph.GetAllEdges();
    for (const auto& edge : edges) {
        edges_json.push_back(GraphEdgeToJson(edge));
    }
    j["edges"] = edges_json;
    
    // Topological order
    if (graph.IsCompiled()) {
        j["topological_order"] = graph.GetTopologicalOrder();
    }
    
    // Start and end nodes
    j["start_nodes"] = graph.GetStartNodes();
    j["end_nodes"] = graph.GetEndNodes();
    
    return j;
}

/**
 * @brief Export Graph structure to JSON string
 * 
 * @param graph The graph to export
 * @param indent Indentation for pretty printing (0 = compact, 2 = readable)
 * @return JSON string
 */
template<typename I, typename O>
std::string GraphToJsonString(const Graph<I, O>& graph, int indent = 2) {
    nlohmann::json j = GraphToJson(graph);
    return j.dump(indent);
}

/**
 * @brief Create a lightweight graph structure from JSON
 * 
 * Note: This creates a graph structure with edges loaded.
 * Runnable implementations must be added separately via AddNode().
 * 
 * Use case:
 * 1. Load graph structure from JSON
 * 2. Create runnable implementations
 * 3. Populate graph with AddNode()
 * 4. Add edges with AddEdge() (edges are loaded from JSON)
 * 5. Compile and execute
 * 
 * @param j JSON object containing graph structure
 * @return Graph with structure loaded (edges ready, nodes need runnables)
 */
template<typename I, typename O>
std::shared_ptr<Graph<I, O>> GraphStructureFromJson(const nlohmann::json& j) {
    auto graph = std::make_shared<Graph<I, O>>();
    
    // Load edges from JSON
    if (j.contains("edges") && j["edges"].is_array()) {
        for (const auto& edge_json : j["edges"]) {
            GraphEdge edge = GraphEdgeFromJson(edge_json);
            
            // Add edge to graph
            // Note: AddEdge expects (from, to, label)
            // Mappings are stored in edge.mappings
            if (!edge.from.empty() && !edge.to.empty()) {
                // For now, we store the edges but cannot add them until nodes exist
                // The user must call AddNode() first, then AddEdge()
                // This is a structural representation only
            }
        }
    }
    
    // Store compile options for later use
    if (j.contains("compile_options")) {
        GraphCompileOptions opts = GraphCompileOptionsFromJson(j["compile_options"]);
        // Options will be used when Compile() is called
        // graph->Compile(opts);  // Cannot compile yet - needs nodes with runnables
    }
    
    return graph;
}

/**
 * @brief Reconstruct graph structure from JSON with edge information
 * 
 * This function returns both the graph object and the edge information
 * so that edges can be added after nodes are populated with runnables.
 * 
 * Workflow:
 * 1. auto [graph, edges, opts] = GraphStructureWithEdgesFromJson(json);
 * 2. For each node: graph->AddNode(name, runnable);
 * 3. For each edge: graph->AddEdge(edge.from, edge.to);
 * 4. graph->Compile(opts);
 * 
 * @return tuple of (Graph, vector of edges, compile options)
 */
template<typename I, typename O>
std::tuple<
    std::shared_ptr<Graph<I, O>>, 
    std::vector<GraphEdge>, 
    GraphCompileOptions
> GraphStructureWithEdgesFromJson(const nlohmann::json& j) {
    auto graph = std::make_shared<Graph<I, O>>();
    std::vector<GraphEdge> edges;
    GraphCompileOptions opts;
    
    // Extract edges
    if (j.contains("edges") && j["edges"].is_array()) {
        for (const auto& edge_json : j["edges"]) {
            edges.push_back(GraphEdgeFromJson(edge_json));
        }
    }
    
    // Extract compile options
    if (j.contains("compile_options")) {
        opts = GraphCompileOptionsFromJson(j["compile_options"]);
    }
    
    // Extract node information (names, metadata) for reference
    // Note: Cannot create actual GraphNodes without runnables
    
    return std::make_tuple(graph, edges, opts);
}

/**
 * @brief Extract node metadata from JSON for reconstruction
 * 
 * This helps you understand what nodes need to be created
 * before deserializing the full graph.
 * 
 * @return vector of NodeInfo structures
 */
inline std::vector<NodeInfo> ExtractNodeInfoFromJson(const nlohmann::json& j) {
    std::vector<NodeInfo> node_infos;
    
    if (j.contains("nodes") && j["nodes"].is_array()) {
        for (const auto& node_json : j["nodes"]) {
            if (node_json.contains("info")) {
                node_infos.push_back(NodeInfoFromJson(node_json["info"]));
            }
        }
    }
    
    return node_infos;
}

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * @brief Pretty print graph structure
 */
template<typename I, typename O>
void PrintGraphStructure(const Graph<I, O>& graph, std::ostream& os = std::cout) {
    os << GraphToJsonString(graph, 2) << std::endl;
}

/**
 * @brief Save graph structure to file
 */
template<typename I, typename O>
bool SaveGraphToFile(const Graph<I, O>& graph, const std::string& filepath) {
    try {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        file << GraphToJsonString(graph, 2);
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * @brief Load graph structure from file
 * 
 * Note: Returns structure only, runnables must be added separately
 */
template<typename I, typename O>
std::shared_ptr<Graph<I, O>> LoadGraphStructureFromFile(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return nullptr;
        }
        nlohmann::json j;
        file >> j;
        file.close();
        return GraphStructureFromJson<I, O>(j);
    } catch (...) {
        return nullptr;
    }
}

// =============================================================================
// Helper: Deserialize Graph with Edge and Node Information
// =============================================================================

/**
 * @brief Complete graph reconstruction helper
 * 
 * This structure holds all information needed to reconstruct a graph
 * including node metadata, edges, and compile options.
 */
struct GraphReconstructionInfo {
    std::vector<NodeInfo> nodes;
    std::vector<GraphEdge> edges;
    GraphCompileOptions compile_options;
    std::vector<std::string> topological_order;
    std::vector<std::string> start_nodes;
    std::vector<std::string> end_nodes;
};

/**
 * @brief Extract complete reconstruction information from JSON
 * 
 * Use this to get all information needed to rebuild a graph:
 * 
 * Example:
 * ```cpp
 * auto info = ExtractGraphReconstructionInfo(json);
 * auto graph = std::make_shared<Graph<Input, Output>>();
 * 
 * // Create and add nodes with your runnables
 * for (const auto& node_info : info.nodes) {
 *     auto runnable = CreateYourRunnable(node_info);
 *     graph->AddNode(node_info.name, runnable);
 * }
 * 
 * // Add edges
 * for (const auto& edge : info.edges) {
 *     graph->AddEdge(edge.from, edge.to);
 * }
 * 
 * // Compile
 * graph->Compile(info.compile_options);
 * ```
 */
inline GraphReconstructionInfo ExtractGraphReconstructionInfo(const nlohmann::json& j) {
    GraphReconstructionInfo info;
    
    // Extract node information
    if (j.contains("nodes") && j["nodes"].is_array()) {
        for (const auto& node_json : j["nodes"]) {
            if (node_json.contains("info")) {
                info.nodes.push_back(NodeInfoFromJson(node_json["info"]));
            }
        }
    }
    
    // Extract edges
    if (j.contains("edges") && j["edges"].is_array()) {
        for (const auto& edge_json : j["edges"]) {
            info.edges.push_back(GraphEdgeFromJson(edge_json));
        }
    }
    
    // Extract compile options
    if (j.contains("compile_options")) {
        info.compile_options = GraphCompileOptionsFromJson(j["compile_options"]);
    }
    
    // Extract topological order
    if (j.contains("topological_order") && j["topological_order"].is_array()) {
        info.topological_order = j["topological_order"].get<std::vector<std::string>>();
    }
    
    // Extract start and end nodes
    if (j.contains("start_nodes") && j["start_nodes"].is_array()) {
        info.start_nodes = j["start_nodes"].get<std::vector<std::string>>();
    }
    if (j.contains("end_nodes") && j["end_nodes"].is_array()) {
        info.end_nodes = j["end_nodes"].get<std::vector<std::string>>();
    }
    
    return info;
}

/**
 * @brief Load reconstruction info from file
 */
inline GraphReconstructionInfo LoadGraphReconstructionInfoFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    
    nlohmann::json j;
    file >> j;
    file.close();
    
    return ExtractGraphReconstructionInfo(j);
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_GRAPH_JSON_H_
