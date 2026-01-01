/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Simple Graph JSON Serialization Example
 * 
 * This example demonstrates Graph structure to JSON export
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <fstream>

// Minimal includes to avoid C++17 requirements
#include <nlohmann/json.hpp>

// Forward declarations to avoid pulling in heavy headers
namespace eino {
namespace compose {

enum class NodeTriggerMode {
    AllPredecessor,
    AnyPredecessor,
    OnInput
};

struct FieldMapping {
    std::string from;
    std::string to;
    FieldMapping() = default;
    FieldMapping(const std::string& f, const std::string& t) : from(f), to(t) {}
};

struct NodeInfo {
    std::string name;
    std::string input_key;
    std::string output_key;
    NodeTriggerMode trigger_mode;
    std::map<std::string, std::string> metadata;
    NodeInfo() : trigger_mode(NodeTriggerMode::AllPredecessor) {}
};

struct GraphEdge {
    std::string from;
    std::string to;
    std::string label;
    bool is_control_edge;
    bool is_data_edge;
    std::vector<std::shared_ptr<FieldMapping>> mappings;
    
    GraphEdge(const std::string& f, const std::string& t, const std::string& l = "")
        : from(f), to(t), label(l), is_control_edge(true), is_data_edge(true) {}
};

struct GraphNode {
    std::string name;
    std::shared_ptr<void> runnable;
    NodeTriggerMode trigger_mode;
    NodeInfo info;
    
    GraphNode() : trigger_mode(NodeTriggerMode::AllPredecessor) {}
};

struct GraphCompileOptions {
    std::string graph_name;
    std::vector<std::string> interrupt_before_nodes;
    std::vector<std::string> interrupt_after_nodes;
    int max_run_steps = -1;
    bool enable_checkpoint = false;
};

} // namespace compose
} // namespace eino

using namespace eino::compose;

// =============================================================================
// JSON Serialization Functions
// =============================================================================

std::string NodeTriggerModeToString(NodeTriggerMode mode) {
    switch (mode) {
        case NodeTriggerMode::AllPredecessor: return "AllPredecessor";
        case NodeTriggerMode::AnyPredecessor: return "AnyPredecessor";
        case NodeTriggerMode::OnInput: return "OnInput";
        default: return "Unknown";
    }
}

nlohmann::json FieldMappingToJson(const FieldMapping& mapping) {
    nlohmann::json j;
    j[std::string("from")] = mapping.from;
    j[std::string("to")] = mapping.to;
    return j;
}

nlohmann::json NodeInfoToJson(const NodeInfo& info) {
    nlohmann::json j;
    j[std::string("name")] = info.name;
    j[std::string("input_key")] = info.input_key;
    j[std::string("output_key")] = info.output_key;
    j[std::string("trigger_mode")] = NodeTriggerModeToString(info.trigger_mode);
    
    nlohmann::json metadata_json = nlohmann::json::object();
    for (const auto& kv : info.metadata) {
        metadata_json[kv.first] = kv.second;
    }
    j[std::string("metadata")] = metadata_json;
    
    return j;
}

nlohmann::json GraphEdgeToJson(const GraphEdge& edge) {
    nlohmann::json j;
    j[std::string("from")] = edge.from;
    j[std::string("to")] = edge.to;
    j[std::string("label")] = edge.label;
    j[std::string("is_control_edge")] = edge.is_control_edge;
    j[std::string("is_data_edge")] = edge.is_data_edge;
    
    nlohmann::json mappings_array = nlohmann::json::array();
    for (const auto& mapping_ptr : edge.mappings) {
        if (mapping_ptr) {
            mappings_array.push_back(FieldMappingToJson(*mapping_ptr));
        }
    }
    j[std::string("mappings")] = mappings_array;
    
    return j;
}

nlohmann::json GraphNodeToJson(const GraphNode& node) {
    nlohmann::json j;
    j[std::string("name")] = node.name;
    j[std::string("trigger_mode")] = NodeTriggerModeToString(node.trigger_mode);
    j[std::string("info")] = NodeInfoToJson(node.info);
    j[std::string("has_runnable")] = (node.runnable != nullptr);
    return j;
}

nlohmann::json GraphCompileOptionsToJson(const GraphCompileOptions& opts) {
    nlohmann::json j;
    j[std::string("graph_name")] = opts.graph_name;
    j[std::string("interrupt_before_nodes")] = opts.interrupt_before_nodes;
    j[std::string("interrupt_after_nodes")] = opts.interrupt_after_nodes;
    j[std::string("max_run_steps")] = opts.max_run_steps;
    j[std::string("enable_checkpoint")] = opts.enable_checkpoint;
    return j;
}

// =============================================================================
// Simple Graph Structure for Demo
// =============================================================================

struct SimpleGraph {
    std::vector<std::shared_ptr<GraphNode>> nodes;
    std::vector<GraphEdge> edges;
    std::vector<std::string> topological_order;
    std::vector<std::string> start_nodes;
    std::vector<std::string> end_nodes;
    GraphCompileOptions compile_options;
    bool is_compiled = false;
};

nlohmann::json SimpleGraphToJson(const SimpleGraph& graph) {
    nlohmann::json j;
    
    j[std::string("type")] = std::string("Graph");
    j[std::string("is_compiled")] = graph.is_compiled;
    
    if (graph.is_compiled) {
        j[std::string("compile_options")] = GraphCompileOptionsToJson(graph.compile_options);
    }
    
    nlohmann::json nodes_json = nlohmann::json::array();
    for (const auto& node : graph.nodes) {
        if (node) {
            nodes_json.push_back(GraphNodeToJson(*node));
        }
    }
    j[std::string("nodes")] = nodes_json;
    
    nlohmann::json edges_json = nlohmann::json::array();
    for (const auto& edge : graph.edges) {
        edges_json.push_back(GraphEdgeToJson(edge));
    }
    j[std::string("edges")] = edges_json;
    
    if (graph.is_compiled) {
        j[std::string("topological_order")] = graph.topological_order;
    }
    
    j[std::string("start_nodes")] = graph.start_nodes;
    j[std::string("end_nodes")] = graph.end_nodes;
    
    return j;
}

// =============================================================================
// Example Functions
// =============================================================================

void Example1_BasicSerialization() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Example 1: Basic Graph Structure -> JSON\n";
    std::cout << std::string(60, '=') << "\n\n";
    
    // Create simple graph structure
    SimpleGraph graph;
    
    // Add nodes
    auto node_a = std::make_shared<GraphNode>();
    node_a->name = "node_a";
    node_a->info.name = "Uppercase Transformer";
    node_a->info.metadata["type"] = "text_processor";
    node_a->runnable = std::make_shared<int>(42);  // Dummy runnable
    
    auto node_b = std::make_shared<GraphNode>();
    node_b->name = "node_b";
    node_b->info.name = "Prefix Adder";
    node_b->info.metadata["type"] = "text_formatter";
    node_b->runnable = std::make_shared<int>(43);
    
    auto node_c = std::make_shared<GraphNode>();
    node_c->name = "node_c";
    node_c->info.name = "Suffix Adder";
    node_c->info.metadata["type"] = "text_formatter";
    node_c->runnable = std::make_shared<int>(44);
    
    graph.nodes.push_back(node_a);
    graph.nodes.push_back(node_b);
    graph.nodes.push_back(node_c);
    
    // Add edges
    graph.edges.push_back(GraphEdge("__START__", "node_a"));
    graph.edges.push_back(GraphEdge("node_a", "node_b"));
    graph.edges.push_back(GraphEdge("node_b", "node_c"));
    graph.edges.push_back(GraphEdge("node_c", "__END__"));
    
    // Compile info
    graph.is_compiled = true;
    graph.compile_options.graph_name = "SimpleTextPipeline";
    graph.compile_options.max_run_steps = 100;
    graph.topological_order = {"__START__", "node_a", "node_b", "node_c", "__END__"};
    graph.start_nodes = {"node_a"};
    graph.end_nodes = {"node_c"};
    
    // Serialize to JSON
    std::cout << "📊 Graph Structure (JSON):\n\n";
    nlohmann::json j = SimpleGraphToJson(graph);
    std::cout << j.dump(2) << "\n\n";
    
    std::cout << "✅ Serialization successful!\n";
}

void Example2_ComplexStructure() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Example 2: Complex Graph with Branches\n";
    std::cout << std::string(60, '=') << "\n\n";
    
    SimpleGraph graph;
    
    // Create nodes
    auto processor = std::make_shared<GraphNode>();
    processor->name = "input_processor";
    processor->info.name = "Input Processor";
    processor->info.metadata["description"] = "Processes input data";
    processor->runnable = std::make_shared<int>(1);
    
    auto branch_a = std::make_shared<GraphNode>();
    branch_a->name = "branch_a";
    branch_a->info.name = "Branch A";
    branch_a->info.metadata["priority"] = "high";
    branch_a->runnable = std::make_shared<int>(2);
    
    auto branch_b = std::make_shared<GraphNode>();
    branch_b->name = "branch_b";
    branch_b->info.name = "Branch B";
    branch_b->info.metadata["priority"] = "low";
    branch_b->runnable = std::make_shared<int>(3);
    
    auto merger = std::make_shared<GraphNode>();
    merger->name = "merger";
    merger->info.name = "Output Merger";
    merger->info.metadata["strategy"] = "concatenate";
    merger->runnable = std::make_shared<int>(4);
    
    graph.nodes.push_back(processor);
    graph.nodes.push_back(branch_a);
    graph.nodes.push_back(branch_b);
    graph.nodes.push_back(merger);
    
    // Create DAG structure
    graph.edges.push_back(GraphEdge("__START__", "input_processor"));
    graph.edges.push_back(GraphEdge("input_processor", "branch_a"));
    graph.edges.push_back(GraphEdge("input_processor", "branch_b"));
    graph.edges.push_back(GraphEdge("branch_a", "merger"));
    graph.edges.push_back(GraphEdge("branch_b", "merger"));
    graph.edges.push_back(GraphEdge("merger", "__END__"));
    
    graph.is_compiled = true;
    graph.compile_options.graph_name = "ParallelBranchPipeline";
    graph.compile_options.enable_checkpoint = true;
    graph.topological_order = {"__START__", "input_processor", "branch_a", "branch_b", "merger", "__END__"};
    
    std::cout << "📊 Complex Graph Structure:\n\n";
    nlohmann::json j = SimpleGraphToJson(graph);
    std::cout << j.dump(2) << "\n\n";
    
    std::cout << "📈 Statistics:\n";
    std::cout << "  - Nodes: " << graph.nodes.size() << "\n";
    std::cout << "  - Edges: " << graph.edges.size() << "\n";
    std::cout << "  - Execution Order: " << graph.topological_order.size() << " steps\n";
}

void Example3_SaveToFile() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Example 3: Save Graph to File\n";
    std::cout << std::string(60, '=') << "\n\n";
    
    SimpleGraph graph;
    
    auto node = std::make_shared<GraphNode>();
    node->name = "processor";
    node->info.name = "Data Processor";
    node->runnable = std::make_shared<int>(100);
    graph.nodes.push_back(node);
    
    graph.edges.push_back(GraphEdge("__START__", "processor"));
    graph.edges.push_back(GraphEdge("processor", "__END__"));
    
    graph.is_compiled = true;
    graph.compile_options.graph_name = "SimpleProcessor";
    
    // Save to file
    std::string filepath = "/tmp/graph_structure.json";
    nlohmann::json j = SimpleGraphToJson(graph);
    
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << j.dump(2);
        file.close();
        std::cout << "✅ Graph saved to: " << filepath << "\n\n";
        std::cout << "📄 File content:\n" << j.dump(2) << "\n";
    } else {
        std::cout << "❌ Failed to save file\n";
    }
}

// =============================================================================
// Main Function
// =============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   Graph JSON Serialization - Simple Example               ║\n";
    std::cout << "║   eino_cpp Graph Structure Export                         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    
    try {
        Example1_BasicSerialization();
        Example2_ComplexStructure();
        Example3_SaveToFile();
        
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "✅ All Examples Completed Successfully!\n";
        std::cout << std::string(60, '=') << "\n\n";
        
        std::cout << "📚 Features Demonstrated:\n";
        std::cout << "  ✅ Graph structure to JSON conversion\n";
        std::cout << "  ✅ Node metadata serialization\n";
        std::cout << "  ✅ Edge relationship export\n";
        std::cout << "  ✅ File save operations\n";
        std::cout << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
}
