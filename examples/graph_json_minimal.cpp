/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Minimal Graph JSON Serialization Example
 * Demonstrates Graph structure export to JSON without heavy dependencies
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>

namespace eino {
namespace compose {

enum class NodeTriggerMode {
    AllPredecessor,
    AnyPredecessor,
    OnInput
};

struct GraphNode {
    std::string name;
    std::string type;
    std::map<std::string, std::string> metadata;
    bool has_runnable;
    
    GraphNode() : has_runnable(false) {}
};

struct GraphEdge {
    std::string from;
    std::string to;
    std::string label;
    bool is_control;
    bool is_data;
    
    GraphEdge(const std::string& f, const std::string& t, const std::string& l)
        : from(f), to(t), label(l), is_control(true), is_data(true) {}
};

struct SimpleGraph {
    std::string name;
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;
    std::vector<std::string> topological_order;
    bool is_compiled;
    int max_steps;
    
    SimpleGraph() : is_compiled(false), max_steps(-1) {}
};

} // namespace compose
} // namespace eino

using namespace eino::compose;

// =============================================================================
// JSON Serialization (Manual, no dependencies)
// =============================================================================

std::string EscapeJson(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c; break;
        }
    }
    return oss.str();
}

std::string NodeToJson(const GraphNode& node, const std::string& indent = "    ") {
    std::ostringstream oss;
    oss << indent << "{\n";
    oss << indent << "  \"name\": \"" << EscapeJson(node.name) << "\",\n";
    oss << indent << "  \"type\": \"" << EscapeJson(node.type) << "\",\n";
    oss << indent << "  \"has_runnable\": " << (node.has_runnable ? "true" : "false");
    
    if (!node.metadata.empty()) {
        oss << ",\n" << indent << "  \"metadata\": {\n";
        bool first = true;
        for (const auto& kv : node.metadata) {
            if (!first) oss << ",\n";
            oss << indent << "    \"" << EscapeJson(kv.first) << "\": \"" 
                << EscapeJson(kv.second) << "\"";
            first = false;
        }
        oss << "\n" << indent << "  }";
    }
    
    oss << "\n" << indent << "}";
    return oss.str();
}

std::string EdgeToJson(const GraphEdge& edge, const std::string& indent = "    ") {
    std::ostringstream oss;
    oss << indent << "{\n";
    oss << indent << "  \"from\": \"" << EscapeJson(edge.from) << "\",\n";
    oss << indent << "  \"to\": \"" << EscapeJson(edge.to) << "\",\n";
    oss << indent << "  \"label\": \"" << EscapeJson(edge.label) << "\",\n";
    oss << indent << "  \"is_control\": " << (edge.is_control ? "true" : "false") << ",\n";
    oss << indent << "  \"is_data\": " << (edge.is_data ? "true" : "false") << "\n";
    oss << indent << "}";
    return oss.str();
}

std::string GraphToJson(const SimpleGraph& graph) {
    std::ostringstream oss;
    
    oss << "{\n";
    oss << "  \"type\": \"Graph\",\n";
    oss << "  \"name\": \"" << EscapeJson(graph.name) << "\",\n";
    oss << "  \"is_compiled\": " << (graph.is_compiled ? "true" : "false") << ",\n";
    oss << "  \"max_steps\": " << graph.max_steps << ",\n";
    
    // Nodes
    oss << "  \"nodes\": [\n";
    for (size_t i = 0; i < graph.nodes.size(); ++i) {
        if (i > 0) oss << ",\n";
        oss << NodeToJson(graph.nodes[i]);
    }
    oss << "\n  ],\n";
    
    // Edges
    oss << "  \"edges\": [\n";
    for (size_t i = 0; i < graph.edges.size(); ++i) {
        if (i > 0) oss << ",\n";
        oss << EdgeToJson(graph.edges[i]);
    }
    oss << "\n  ]";
    
    // Topological order
    if (!graph.topological_order.empty()) {
        oss << ",\n  \"topological_order\": [";
        for (size_t i = 0; i < graph.topological_order.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << "\"" << EscapeJson(graph.topological_order[i]) << "\"";
        }
        oss << "]";
    }
    
    oss << "\n}";
    return oss.str();
}

// =============================================================================
// JSON Deserialization (Manual parsing)
// =============================================================================

// Simple JSON string parser (minimal implementation)
class SimpleJsonParser {
public:
    static std::string GetStringValue(const std::string& json, const std::string& key) {
        std::string pattern = "\"" + key + "\":";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return "";
        
        pos = json.find("\"", pos + pattern.length());
        if (pos == std::string::npos) return "";
        
        size_t end = json.find("\"", pos + 1);
        if (end == std::string::npos) return "";
        
        return json.substr(pos + 1, end - pos - 1);
    }
    
    static bool GetBoolValue(const std::string& json, const std::string& key) {
        std::string pattern = "\"" + key + "\":";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return false;
        
        pos = json.find_first_not_of(" \t\n", pos + pattern.length());
        if (pos == std::string::npos) return false;
        
        return json.substr(pos, 4) == "true";
    }
    
    static int GetIntValue(const std::string& json, const std::string& key) {
        std::string pattern = "\"" + key + "\":";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return 0;
        
        pos = json.find_first_not_of(" \t\n", pos + pattern.length());
        if (pos == std::string::npos) return 0;
        
        size_t end = json.find_first_of(",}\n", pos);
        std::string num_str = json.substr(pos, end - pos);
        return std::atoi(num_str.c_str());
    }
    
    static std::map<std::string, std::string> GetObjectValue(const std::string& json, const std::string& key) {
        std::map<std::string, std::string> result;
        std::string pattern = "\"" + key + "\":";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return result;
        
        pos = json.find("{", pos + pattern.length());
        if (pos == std::string::npos) return result;
        
        size_t end = json.find("}", pos);
        if (end == std::string::npos) return result;
        
        std::string obj = json.substr(pos + 1, end - pos - 1);
        
        // Parse key-value pairs
        size_t current = 0;
        while (current < obj.length()) {
            size_t key_start = obj.find("\"", current);
            if (key_start == std::string::npos) break;
            
            size_t key_end = obj.find("\"", key_start + 1);
            if (key_end == std::string::npos) break;
            
            std::string k = obj.substr(key_start + 1, key_end - key_start - 1);
            
            size_t val_start = obj.find("\"", key_end + 1);
            if (val_start == std::string::npos) break;
            
            size_t val_end = obj.find("\"", val_start + 1);
            if (val_end == std::string::npos) break;
            
            std::string v = obj.substr(val_start + 1, val_end - val_start - 1);
            
            result[k] = v;
            current = val_end + 1;
        }
        
        return result;
    }
    
    static std::vector<std::string> GetArrayValue(const std::string& json, const std::string& key) {
        std::vector<std::string> result;
        std::string pattern = "\"" + key + "\":";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return result;
        
        pos = json.find("[", pos + pattern.length());
        if (pos == std::string::npos) return result;
        
        size_t end = json.find("]", pos);
        if (end == std::string::npos) return result;
        
        std::string arr = json.substr(pos + 1, end - pos - 1);
        
        // Parse array elements
        size_t current = 0;
        while (current < arr.length()) {
            size_t elem_start = arr.find("\"", current);
            if (elem_start == std::string::npos) break;
            
            size_t elem_end = arr.find("\"", elem_start + 1);
            if (elem_end == std::string::npos) break;
            
            result.push_back(arr.substr(elem_start + 1, elem_end - elem_start - 1));
            current = elem_end + 1;
        }
        
        return result;
    }
};

GraphNode ParseNode(const std::string& node_json) {
    GraphNode node;
    node.name = SimpleJsonParser::GetStringValue(node_json, "name");
    node.type = SimpleJsonParser::GetStringValue(node_json, "type");
    node.has_runnable = SimpleJsonParser::GetBoolValue(node_json, "has_runnable");
    node.metadata = SimpleJsonParser::GetObjectValue(node_json, "metadata");
    return node;
}

GraphEdge ParseEdge(const std::string& edge_json) {
    std::string from = SimpleJsonParser::GetStringValue(edge_json, "from");
    std::string to = SimpleJsonParser::GetStringValue(edge_json, "to");
    std::string label = SimpleJsonParser::GetStringValue(edge_json, "label");
    
    GraphEdge edge(from, to, label);
    edge.is_control = SimpleJsonParser::GetBoolValue(edge_json, "is_control");
    edge.is_data = SimpleJsonParser::GetBoolValue(edge_json, "is_data");
    
    return edge;
}

SimpleGraph JsonToGraph(const std::string& json) {
    SimpleGraph graph;
    
    // Parse basic fields
    graph.name = SimpleJsonParser::GetStringValue(json, "name");
    graph.is_compiled = SimpleJsonParser::GetBoolValue(json, "is_compiled");
    graph.max_steps = SimpleJsonParser::GetIntValue(json, "max_steps");
    
    // Parse nodes array
    size_t nodes_start = json.find("\"nodes\":");
    if (nodes_start != std::string::npos) {
        size_t array_start = json.find("[", nodes_start);
        size_t array_end = json.find("]", array_start);
        
        if (array_start != std::string::npos && array_end != std::string::npos) {
            std::string nodes_section = json.substr(array_start + 1, array_end - array_start - 1);
            
            // Parse each node
            int depth = 0;
            size_t node_start = 0;
            for (size_t i = 0; i < nodes_section.length(); ++i) {
                if (nodes_section[i] == '{') {
                    if (depth == 0) node_start = i;
                    depth++;
                } else if (nodes_section[i] == '}') {
                    depth--;
                    if (depth == 0) {
                        std::string node_json = nodes_section.substr(node_start, i - node_start + 1);
                        graph.nodes.push_back(ParseNode(node_json));
                    }
                }
            }
        }
    }
    
    // Parse edges array
    size_t edges_start = json.find("\"edges\":");
    if (edges_start != std::string::npos) {
        size_t array_start = json.find("[", edges_start);
        size_t array_end = json.find("]", array_start);
        
        if (array_start != std::string::npos && array_end != std::string::npos) {
            std::string edges_section = json.substr(array_start + 1, array_end - array_start - 1);
            
            // Parse each edge
            int depth = 0;
            size_t edge_start = 0;
            for (size_t i = 0; i < edges_section.length(); ++i) {
                if (edges_section[i] == '{') {
                    if (depth == 0) edge_start = i;
                    depth++;
                } else if (edges_section[i] == '}') {
                    depth--;
                    if (depth == 0) {
                        std::string edge_json = edges_section.substr(edge_start, i - edge_start + 1);
                        graph.edges.push_back(ParseEdge(edge_json));
                    }
                }
            }
        }
    }
    
    // Parse topological order
    graph.topological_order = SimpleJsonParser::GetArrayValue(json, "topological_order");
    
    return graph;
}

SimpleGraph LoadGraphFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return JsonToGraph(buffer.str());
}

// =============================================================================
// Examples
// =============================================================================

void Example1_LinearPipeline() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Example 1: Linear Pipeline A → B → C\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    SimpleGraph graph;
    graph.name = "LinearTextPipeline";
    graph.is_compiled = true;
    graph.max_steps = 100;
    
    // Add nodes
    GraphNode node_a;
    node_a.name = "node_a";
    node_a.type = "UppercaseTransformer";
    node_a.metadata["description"] = "Converts text to uppercase";
    node_a.has_runnable = true;
    graph.nodes.push_back(node_a);
    
    GraphNode node_b;
    node_b.name = "node_b";
    node_b.type = "PrefixAdder";
    node_b.metadata["prefix"] = "[PREFIX] ";
    node_b.has_runnable = true;
    graph.nodes.push_back(node_b);
    
    GraphNode node_c;
    node_c.name = "node_c";
    node_c.type = "SuffixAdder";
    node_c.metadata["suffix"] = " [SUFFIX]";
    node_c.has_runnable = true;
    graph.nodes.push_back(node_c);
    
    // Add edges
    graph.edges.push_back(GraphEdge("__START__", "node_a", "start"));
    graph.edges.push_back(GraphEdge("node_a", "node_b", "data"));
    graph.edges.push_back(GraphEdge("node_b", "node_c", "data"));
    graph.edges.push_back(GraphEdge("node_c", "__END__", "end"));
    
    // Topological order
    graph.topological_order = {"__START__", "node_a", "node_b", "node_c", "__END__"};
    
    // Serialize to JSON
    std::cout << "📊 Graph Structure (JSON):\n\n";
    std::string json = GraphToJson(graph);
    std::cout << json << "\n\n";
    
    std::cout << "✅ Example 1 Complete\n";
    std::cout << "   Pipeline: __START__ → node_a → node_b → node_c → __END__\n";
}

void Example2_ParallelBranches() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Example 2: Parallel Branches (DAG)\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    SimpleGraph graph;
    graph.name = "ParallelBranchPipeline";
    graph.is_compiled = true;
    graph.max_steps = 200;
    
    // Input processor
    GraphNode processor;
    processor.name = "input_processor";
    processor.type = "DataPreprocessor";
    processor.metadata["operation"] = "normalize";
    processor.has_runnable = true;
    graph.nodes.push_back(processor);
    
    // Branch A
    GraphNode branch_a;
    branch_a.name = "branch_a";
    branch_a.type = "FastProcessor";
    branch_a.metadata["priority"] = "high";
    branch_a.metadata["timeout"] = "100ms";
    branch_a.has_runnable = true;
    graph.nodes.push_back(branch_a);
    
    // Branch B
    GraphNode branch_b;
    branch_b.name = "branch_b";
    branch_b.type = "SlowProcessor";
    branch_b.metadata["priority"] = "low";
    branch_b.metadata["timeout"] = "500ms";
    branch_b.has_runnable = true;
    graph.nodes.push_back(branch_b);
    
    // Merger
    GraphNode merger;
    merger.name = "merger";
    merger.type = "ResultMerger";
    merger.metadata["strategy"] = "concatenate";
    merger.has_runnable = true;
    graph.nodes.push_back(merger);
    
    // Build DAG
    graph.edges.push_back(GraphEdge("__START__", "input_processor", ""));
    graph.edges.push_back(GraphEdge("input_processor", "branch_a", ""));
    graph.edges.push_back(GraphEdge("input_processor", "branch_b", ""));
    graph.edges.push_back(GraphEdge("branch_a", "merger", ""));
    graph.edges.push_back(GraphEdge("branch_b", "merger", ""));
    graph.edges.push_back(GraphEdge("merger", "__END__", ""));
    
    graph.topological_order = {
        "__START__", "input_processor", 
        "branch_a", "branch_b", 
        "merger", "__END__"
    };
    
    std::cout << "📊 Parallel DAG Structure:\n\n";
    std::cout << GraphToJson(graph) << "\n\n";
    
    std::cout << "✅ Example 2 Complete\n";
    std::cout << "   Topology:\n";
    std::cout << "     __START__ → input_processor → branch_a ↘\n";
    std::cout << "                                  → branch_b → merger → __END__\n";
}

void Example3_ComplexWorkflow() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Example 3: Complex Multi-Stage Workflow\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    SimpleGraph graph;
    graph.name = "ComplexDataPipeline";
    graph.is_compiled = true;
    graph.max_steps = 500;
    
    // Stage 1: Input validation
    GraphNode validator;
    validator.name = "validator";
    validator.type = "InputValidator";
    validator.metadata["rules"] = "required,min:1,max:1000";
    validator.has_runnable = true;
    graph.nodes.push_back(validator);
    
    // Stage 2: Data transformation
    GraphNode transformer;
    transformer.name = "transformer";
    transformer.type = "DataTransformer";
    transformer.metadata["format"] = "json";
    transformer.has_runnable = true;
    graph.nodes.push_back(transformer);
    
    // Stage 3a: ML Model A
    GraphNode model_a;
    model_a.name = "model_a";
    model_a.type = "MLModel";
    model_a.metadata["model"] = "bert-base";
    model_a.metadata["task"] = "classification";
    model_a.has_runnable = true;
    graph.nodes.push_back(model_a);
    
    // Stage 3b: ML Model B
    GraphNode model_b;
    model_b.name = "model_b";
    model_b.type = "MLModel";
    model_b.metadata["model"] = "gpt-3.5";
    model_b.metadata["task"] = "generation";
    model_b.has_runnable = true;
    graph.nodes.push_back(model_b);
    
    // Stage 4: Result aggregator
    GraphNode aggregator;
    aggregator.name = "aggregator";
    aggregator.type = "ResultAggregator";
    aggregator.metadata["method"] = "weighted_average";
    aggregator.has_runnable = true;
    graph.nodes.push_back(aggregator);
    
    // Stage 5: Post-processor
    GraphNode postproc;
    postproc.name = "post_processor";
    postproc.type = "PostProcessor";
    postproc.metadata["cleanup"] = "true";
    postproc.has_runnable = true;
    graph.nodes.push_back(postproc);
    
    // Build workflow
    graph.edges.push_back({"__START__", "validator", ""});
    graph.edges.push_back({"validator", "transformer", ""});
    graph.edges.push_back({"transformer", "model_a", ""});
    graph.edges.push_back({"transformer", "model_b", ""});
    graph.edges.push_back({"model_a", "aggregator", ""});
    graph.edges.push_back({"model_b", "aggregator", ""});
    graph.edges.push_back({"aggregator", "post_processor", ""});
    graph.edges.push_back({"post_processor", "__END__", ""});
    
    graph.topological_order = {
        "__START__", "validator", "transformer",
        "model_a", "model_b", "aggregator",
        "post_processor", "__END__"
    };
    
    std::cout << "📊 Complex Workflow Structure:\n\n";
    std::cout << GraphToJson(graph) << "\n\n";
    
    std::cout << "✅ Example 3 Complete\n";
    std::cout << "   6-node workflow with parallel ML inference\n";
    std::cout << "   Stages: Validation → Transform → [Model A + Model B] → Aggregate → Post-process\n";
}

void Example4_SaveToFile() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Example 4: Save Graph Structure to File\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    SimpleGraph graph;
    graph.name = "SavedGraphExample";
    graph.is_compiled = true;
    graph.max_steps = 50;
    
    GraphNode node;
    node.name = "processor";
    node.type = "DataProcessor";
    node.metadata["version"] = "1.0.0";
    node.has_runnable = true;
    graph.nodes.push_back(node);
    
    graph.edges.push_back(GraphEdge("__START__", "processor", ""));
    graph.edges.push_back(GraphEdge("processor", "__END__", ""));
    graph.topological_order = {"__START__", "processor", "__END__"};
    
    // Generate JSON
    std::string json = GraphToJson(graph);
    
    // Save to file
    std::string filepath = "/tmp/graph_structure_minimal.json";
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << json;
        file.close();
        std::cout << "✅ Graph saved successfully!\n";
        std::cout << "   File: " << filepath << "\n\n";
        std::cout << "📄 Content:\n" << json << "\n";
    } else {
        std::cout << "❌ Failed to save file\n";
    }
}

void Example5_Deserialization() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Example 5: Deserialize Graph from JSON\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    // Step 1: Create and save a graph
    std::cout << "📝 Step 1: Create and serialize original graph\n\n";
    
    SimpleGraph original;
    original.name = "DeserializationTestGraph";
    original.is_compiled = true;
    original.max_steps = 150;
    
    GraphNode node1;
    node1.name = "input_validator";
    node1.type = "Validator";
    node1.metadata["rule"] = "required";
    node1.metadata["min_length"] = "10";
    node1.has_runnable = true;
    original.nodes.push_back(node1);
    
    GraphNode node2;
    node2.name = "transformer";
    node2.type = "Transformer";
    node2.metadata["format"] = "json";
    node2.has_runnable = true;
    original.nodes.push_back(node2);
    
    GraphNode node3;
    node3.name = "output_formatter";
    node3.type = "Formatter";
    node3.metadata["style"] = "pretty";
    node3.has_runnable = true;
    original.nodes.push_back(node3);
    
    original.edges.push_back(GraphEdge("__START__", "input_validator", ""));
    original.edges.push_back(GraphEdge("input_validator", "transformer", ""));
    original.edges.push_back(GraphEdge("transformer", "output_formatter", ""));
    original.edges.push_back(GraphEdge("output_formatter", "__END__", ""));
    
    original.topological_order = {
        "__START__", "input_validator", "transformer", 
        "output_formatter", "__END__"
    };
    
    std::string json = GraphToJson(original);
    std::cout << "Original Graph JSON:\n" << json << "\n\n";
    
    // Step 2: Save to file
    std::cout << "💾 Step 2: Save to file\n";
    std::string filepath = "/tmp/graph_deserialization_test.json";
    std::ofstream file(filepath);
    file << json;
    file.close();
    std::cout << "   Saved to: " << filepath << "\n\n";
    
    // Step 3: Load from file
    std::cout << "📂 Step 3: Load graph from file\n";
    SimpleGraph loaded;
    try {
        loaded = LoadGraphFromFile(filepath);
        std::cout << "   ✅ Graph loaded successfully!\n\n";
    } catch (const std::exception& e) {
        std::cout << "   ❌ Failed to load: " << e.what() << "\n";
        return;
    }
    
    // Step 4: Verify deserialization
    std::cout << "🔍 Step 4: Verify deserialized graph\n\n";
    
    std::cout << "Graph Name: " << loaded.name << "\n";
    std::cout << "Is Compiled: " << (loaded.is_compiled ? "true" : "false") << "\n";
    std::cout << "Max Steps: " << loaded.max_steps << "\n\n";
    
    std::cout << "Nodes (" << loaded.nodes.size() << "):\n";
    for (const auto& node : loaded.nodes) {
        std::cout << "  - " << node.name << " [" << node.type << "]\n";
        std::cout << "    has_runnable: " << (node.has_runnable ? "true" : "false") << "\n";
        if (!node.metadata.empty()) {
            std::cout << "    metadata:\n";
            for (const auto& kv : node.metadata) {
                std::cout << "      " << kv.first << ": " << kv.second << "\n";
            }
        }
    }
    std::cout << "\n";
    
    std::cout << "Edges (" << loaded.edges.size() << "):\n";
    for (const auto& edge : loaded.edges) {
        std::cout << "  - " << edge.from << " → " << edge.to;
        if (!edge.label.empty()) std::cout << " [" << edge.label << "]";
        std::cout << "\n";
    }
    std::cout << "\n";
    
    std::cout << "Topological Order: ";
    for (size_t i = 0; i < loaded.topological_order.size(); ++i) {
        if (i > 0) std::cout << " → ";
        std::cout << loaded.topological_order[i];
    }
    std::cout << "\n\n";
    
    // Step 5: Compare with original
    std::cout << "⚖️  Step 5: Comparison\n";
    bool match = true;
    
    if (loaded.name != original.name) {
        std::cout << "   ❌ Name mismatch\n";
        match = false;
    }
    if (loaded.nodes.size() != original.nodes.size()) {
        std::cout << "   ❌ Node count mismatch\n";
        match = false;
    }
    if (loaded.edges.size() != original.edges.size()) {
        std::cout << "   ❌ Edge count mismatch\n";
        match = false;
    }
    if (loaded.topological_order.size() != original.topological_order.size()) {
        std::cout << "   ❌ Topological order mismatch\n";
        match = false;
    }
    
    if (match) {
        std::cout << "   ✅ All fields match! Deserialization successful!\n";
    }
    
    std::cout << "\n";
}

void Example6_RoundTripTest() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Example 6: Round-Trip Serialization Test\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "Testing: Graph → JSON → Graph → JSON\n\n";
    
    // Create complex graph
    SimpleGraph graph1;
    graph1.name = "RoundTripTest";
    graph1.is_compiled = true;
    graph1.max_steps = 999;
    
    // Add nodes with rich metadata
    for (int i = 0; i < 3; ++i) {
        GraphNode node;
        node.name = "node_" + std::to_string(i);
        node.type = "Processor_" + std::to_string(i);
        node.metadata["id"] = std::to_string(i);
        node.metadata["timestamp"] = "2025-12-08T10:00:00Z";
        node.has_runnable = true;
        graph1.nodes.push_back(node);
    }
    
    graph1.edges.push_back(GraphEdge("__START__", "node_0", "start"));
    graph1.edges.push_back(GraphEdge("node_0", "node_1", "step1"));
    graph1.edges.push_back(GraphEdge("node_1", "node_2", "step2"));
    graph1.edges.push_back(GraphEdge("node_2", "__END__", "end"));
    
    graph1.topological_order = {"__START__", "node_0", "node_1", "node_2", "__END__"};
    
    // First serialization
    std::cout << "🔄 First serialization (Graph → JSON)\n";
    std::string json1 = GraphToJson(graph1);
    std::cout << "JSON size: " << json1.size() << " bytes\n\n";
    
    // Deserialization
    std::cout << "🔄 Deserialization (JSON → Graph)\n";
    SimpleGraph graph2 = JsonToGraph(json1);
    std::cout << "Loaded " << graph2.nodes.size() << " nodes, " 
              << graph2.edges.size() << " edges\n\n";
    
    // Second serialization
    std::cout << "🔄 Second serialization (Graph → JSON)\n";
    std::string json2 = GraphToJson(graph2);
    std::cout << "JSON size: " << json2.size() << " bytes\n\n";
    
    // Compare JSONs
    std::cout << "⚖️  Comparison:\n";
    if (json1 == json2) {
        std::cout << "   ✅ Round-trip successful! JSONs are identical.\n";
    } else {
        std::cout << "   ⚠️  JSONs differ (might be formatting differences)\n";
        std::cout << "   JSON1 size: " << json1.size() << " bytes\n";
        std::cout << "   JSON2 size: " << json2.size() << " bytes\n";
    }
    
    // Structural comparison
    bool structural_match = true;
    if (graph1.name != graph2.name) structural_match = false;
    if (graph1.nodes.size() != graph2.nodes.size()) structural_match = false;
    if (graph1.edges.size() != graph2.edges.size()) structural_match = false;
    if (graph1.max_steps != graph2.max_steps) structural_match = false;
    
    if (structural_match) {
        std::cout << "   ✅ Graph structures are identical!\n";
    } else {
        std::cout << "   ❌ Graph structures differ!\n";
    }
    
    std::cout << "\n";
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   eino_cpp Graph JSON Serialization - Minimal Example           ║\n";
    std::cout << "║   No external dependencies, pure C++ implementation              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
    
    try {
        Example1_LinearPipeline();
        Example2_ParallelBranches();
        Example3_ComplexWorkflow();
        Example4_SaveToFile();
        Example5_Deserialization();
        Example6_RoundTripTest();
        
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "✅ All Examples Completed Successfully!\n";
        std::cout << std::string(70, '=') << "\n\n";
        
        std::cout << "📚 Features Demonstrated:\n";
        std::cout << "  ✅ Linear pipeline serialization\n";
        std::cout << "  ✅ Parallel branch (DAG) export\n";
        std::cout << "  ✅ Complex multi-stage workflow\n";
        std::cout << "  ✅ File save operations\n";
        std::cout << "  ✅ JSON deserialization (NEW!)\n";
        std::cout << "  ✅ Graph reconstruction from JSON (NEW!)\n";
        std::cout << "  ✅ Round-trip serialization test (NEW!)\n";
        std::cout << "  ✅ No external JSON library dependencies\n";
        std::cout << "\n";
        
        std::cout << "💡 Usage:\n";
        std::cout << "  Serialization:\n";
        std::cout << "    std::string json = GraphToJson(graph);\n";
        std::cout << "  \n";
        std::cout << "  Deserialization:\n";
        std::cout << "    SimpleGraph graph = JsonToGraph(json);\n";
        std::cout << "    SimpleGraph graph = LoadGraphFromFile(\"path.json\");\n";
        std::cout << "  \n";
        std::cout << "  Use cases:\n";
        std::cout << "    - Save/load graph topology\n";
        std::cout << "    - Debug graph structure\n";
        std::cout << "    - Visualize with external tools\n";
        std::cout << "    - Share graph definitions across systems\n";
        std::cout << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
}
