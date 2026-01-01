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

#ifndef EINO_CPP_COMPOSE_GRAPH_H_
#define EINO_CPP_COMPOSE_GRAPH_H_

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <set>
#include <queue>
#include <algorithm>
#include <stdexcept>
#include <typeinfo>
#include <functional>
#include "runnable.h"
#include "types.h"
#include "graph_validation.h"

namespace eino {

// Forward declarations for components
namespace components {
class BaseChatModel;
class ToolCallingChatModel;
} // namespace components

namespace compose {

// Forward declarations
struct NodeInfo;
struct FieldMapping;
struct NodeProcessor;
struct GraphNode;
struct GraphEdge;
struct GraphAddNodeOpts;
struct GraphAddNodeOpt;
class ToolsNode;
class GraphBranch;
struct GraphEdge;

// FieldMapping aligns with eino compose field mapping for node composition
// Aligns with eino compose.FieldMapping
struct FieldMapping {
    std::string from;  // Source field
    std::string to;    // Target field
    
    FieldMapping() = default;
    FieldMapping(const std::string& f, const std::string& t)
        : from(f), to(t) {}
};

// NodeProcessor for pre/post handlers
// Aligns with eino compose node processor pattern
struct NodeProcessor {
    std::function<void(std::shared_ptr<void>&)> pre_handler;
    std::function<void(std::shared_ptr<void>&)> post_handler;
    
    NodeProcessor() = default;
};

// NodeInfo represents metadata about a graph node
// Aligns with eino compose.GraphNodeInfo
struct NodeInfo {
    std::string name;
    std::string input_key;
    std::string output_key;
    NodeTriggerMode trigger_mode;
    std::map<std::string, std::string> metadata;
    
    NodeInfo() : trigger_mode(NodeTriggerMode::AllPredecessor) {}
};

// GraphEdge represents an edge between two nodes
// Aligns with eino compose edge handling with control/data distinction
struct GraphEdge {
    std::string from;
    std::string to;
    std::string label;
    bool is_control_edge;
    bool is_data_edge;
    std::vector<std::shared_ptr<FieldMapping>> mappings;
    
    GraphEdge(const std::string& f, const std::string& t, 
              const std::string& l = "")
        : from(f), to(t), label(l), is_control_edge(true), is_data_edge(true) {}
    
    GraphEdge(const std::string& f, const std::string& t,
              bool control, bool data)
        : from(f), to(t), label(""), is_control_edge(control), is_data_edge(data) {}
};

// GraphNode represents a node in the graph with its configuration
// Aligns with eino compose.graphNode
// Note: runnable is stored as void* for type-erasure
struct GraphNode {
    std::string name;
    std::shared_ptr<void> runnable;  // Type-erased runnable stored as void*
    NodeTriggerMode trigger_mode;
    NodeInfo info;
    std::shared_ptr<NodeProcessor> processor;
    
    GraphNode() 
        : trigger_mode(NodeTriggerMode::AllPredecessor),
          processor(nullptr) {}
};

// GraphCompileOptions aligns with eino compose.graphCompileOptions
struct GraphCompileOptions {
    std::string graph_name;
    std::vector<std::string> interrupt_before_nodes;
    std::vector<std::string> interrupt_after_nodes;
    int max_run_steps = -1;
    bool enable_checkpoint = false;
    
    GraphCompileOptions() = default;
};

// Graph represents a directed graph orchestration of runnables
// Aligns with eino compose.graph for DAG and Pregel execution
template<typename I, typename O>
class Graph : public ComposableRunnable<I, O> {
public:
    static constexpr const char* START_NODE = "__START__";
    static constexpr const char* END_NODE = "__END__";
    
    Graph() : is_compiled_(false), has_error_(false) {}
    
    virtual ~Graph() = default;
    
    void AddNode(const std::string& name, 
                std::shared_ptr<Runnable<I, O>> runnable,
                NodeTriggerMode mode = NodeTriggerMode::AllPredecessor,
                std::shared_ptr<NodeProcessor> processor = nullptr) {
        if (is_compiled_) {
            throw std::runtime_error("Graph already compiled, cannot modify");
        }
        if (name == "__START__" || name == "__END__") {
            throw std::runtime_error("Cannot use reserved node name: " + name);
        }
        if (nodes_.count(name)) {
            throw std::runtime_error("Node already exists: " + name);
        }
        
        auto node = std::make_shared<GraphNode>();
        node->name = name;
        node->runnable = runnable;
        node->trigger_mode = mode;
        node->processor = processor;
        node->info.name = name;
        node->info.trigger_mode = mode;
        
        nodes_[name] = node;
        adjacency_list_[name] = std::vector<GraphEdge>();
        in_degree_[name] = 0;
        
        // ✅ Collect type information for validation
        // Aligns with eino/compose/graph.go:561-590
        if (runnable) {
            node_input_types_[name] = &runnable->GetInputType();
            node_output_types_[name] = &runnable->GetOutputType();
        }
    }
    
    void AddEdge(const std::string& from, const std::string& to,
                bool no_control = false, bool no_data = false,
                const std::vector<std::shared_ptr<FieldMapping>>& mappings = {}) {
        if (is_compiled_) {
            throw std::runtime_error("Graph already compiled, cannot modify");
        }
        
        if (no_control && no_data) {
            throw std::runtime_error("Edge must have either control or data dependency");
        }
        
        if (from != START_NODE && !nodes_.count(from)) {
            throw std::runtime_error("Source node not found: " + from);
        }
        if (to != END_NODE && !nodes_.count(to)) {
            throw std::runtime_error("Target node not found: " + to);
        }
        
        GraphEdge edge(from, to);
        edge.is_control_edge = !no_control;
        edge.is_data_edge = !no_data;
        edge.mappings = mappings;
        
        adjacency_list_[from].push_back(edge);
        
        if (!no_control && to != END_NODE) {
            if (!in_degree_.count(to)) {
                in_degree_[to] = 0;
            }
            in_degree_[to]++;
        }
        
        if (from == START_NODE) {
            start_nodes_.push_back(to);
        }
        if (to == END_NODE) {
            end_nodes_.push_back(from);
        }
    }
    
    void Compile(const GraphCompileOptions& opts = GraphCompileOptions()) {
        if (is_compiled_) {
            return;
        }
        
        if (has_error_) {
            throw std::runtime_error("Graph has build error");
        }
        
        ValidateGraphStructure();
        TopologicalSort();
        
        compile_options_ = opts;
        is_compiled_ = true;
    }
    
    bool IsCompiled() const {
        return is_compiled_;
    }
    
    const GraphCompileOptions& GetCompileOptions() const {
        return compile_options_;
    }
    
    bool HasError() const {
        return has_error_;
    }
    
    O Invoke(
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        if (!is_compiled_) {
            throw std::runtime_error("Graph not compiled, call Compile() first");
        }
        
        if (!ctx) {
            ctx = Context::Background();
        }
        
        // Store outputs from all nodes indexed by node name
        std::map<std::string, O> node_outputs;
        node_outputs[START_NODE] = input;
        O last_output = input;
        
        // Execute nodes in topological order
        for (const auto& node_name : topological_order_) {
            if (node_name == START_NODE || node_name == END_NODE) {
                continue;
            }
            
            if (!nodes_.count(node_name)) {
                continue;
            }
            
            // Check if this is a BranchNode
            auto node = nodes_[node_name];
            auto runnable = std::static_pointer_cast<Runnable<I, O>>(node->runnable);
            bool is_branch_node = (runnable && runnable->GetComponentType() == "BranchNode");
            
            O node_input;
            
            // ✅ Special handling for BranchNode: provide all node outputs for NodeReference mode
            if (is_branch_node && std::is_same_v<O, std::map<std::string, std::any>>) {
                // Build input containing all executed node outputs
                // Format: {"node_a": {...}, "node_b": {...}, ...}
                std::map<std::string, std::any> branch_input;
                
                for (const auto& [nkey, noutput] : node_outputs) {
                    if (nkey != START_NODE && nkey != END_NODE) {
                        branch_input[nkey] = noutput;
                    }
                }
                
                // Also include START node input if it's a map
                if constexpr (std::is_same_v<I, std::map<std::string, std::any>>) {
                    branch_input[START_NODE] = input;
                }
                
                node_input = branch_input;
            } else {
                // Standard input handling for regular nodes
                std::vector<O> predecessor_outputs;
                std::vector<std::string> predecessors = GetPredecessors(node_name);
                
                if (predecessors.empty()) {
                    // Node has no predecessors, use graph input
                    predecessor_outputs.push_back(input);
                } else {
                    for (const auto& pred : predecessors) {
                        if (pred == START_NODE) {
                            predecessor_outputs.push_back(input);
                        } else if (node_outputs.count(pred)) {
                            predecessor_outputs.push_back(node_outputs[pred]);
                        }
                    }
                }
                
                // Merge inputs from multiple predecessors
                node_input = MergePredecessorOutputs(predecessor_outputs);
            }
            
            // Execute the node's runnable
            if (runnable) {
                node_outputs[node_name] = runnable->Invoke(ctx, node_input, opts);
                last_output = node_outputs[node_name];
            }
        }
        
        return last_output;
    }
    
    std::shared_ptr<StreamReader<O>> Stream(
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        if (!is_compiled_) {
            throw std::runtime_error("Graph not compiled, call Compile() first");
        }
        
        O result = Invoke(ctx, input, opts);
        std::vector<O> results{result};
        return std::make_shared<SimpleStreamReader<O>>(results);
    }
    
    O Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        if (!is_compiled_) {
            throw std::runtime_error("Graph not compiled, call Compile() first");
        }
        
        I value;
        if (input && input->Read(value)) {
            return Invoke(ctx, value, opts);
        }
        throw std::runtime_error("Graph: no input to collect");
    }
    
    std::shared_ptr<StreamReader<O>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        if (!is_compiled_) {
            throw std::runtime_error("Graph not compiled, call Compile() first");
        }
        
        std::vector<O> results;
        I value;
        while (input && input->Read(value)) {
            results.push_back(Invoke(ctx, value, opts));
        }
        return std::make_shared<SimpleStreamReader<O>>(results);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(I);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(O);
    }
    
    std::string GetComponentType() const override {
        return "Graph";
    }
    
    std::vector<std::string> GetNodeNames() const {
        std::vector<std::string> names;
        for (const auto& pair : nodes_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
    size_t GetNodeCount() const {
        return nodes_.size();
    }
    
    size_t GetEdgeCount() const {
        size_t count = 0;
        for (const auto& pair : adjacency_list_) {
            count += pair.second.size();
        }
        return count;
    }
    
    std::vector<std::string> GetStartNodes() const {
        return start_nodes_;
    }
    
    std::vector<std::string> GetEndNodes() const {
        return end_nodes_;
    }
    
    // Get topological order
    std::vector<std::string> GetTopologicalOrder() const {
        return topological_order_;
    }
    
    // Get node by name
    std::shared_ptr<GraphNode> GetNode(const std::string& node_name) const {
        auto it = nodes_.find(node_name);
        if (it != nodes_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    // Get successors of a node
    std::vector<std::string> GetSuccessors(const std::string& node_name) const {
        std::vector<std::string> successors;
        auto it = adjacency_list_.find(node_name);
        if (it != adjacency_list_.end()) {
            for (const auto& edge : it->second) {
                if (edge.is_data_edge) {
                    successors.push_back(edge.to);
                }
            }
        }
        return successors;
    }
    
    // Get edges from a specific node
    // Aligns with graph structure access pattern
    std::vector<GraphEdge> GetEdges(const std::string& node_name) const {
        auto it = adjacency_list_.find(node_name);
        if (it != adjacency_list_.end()) {
            return it->second;
        }
        return std::vector<GraphEdge>();
    }
    
    // Get all node names (for JSON serialization)
    std::vector<std::string> GetAllNodeNames() const {
        return GetNodeNames();
    }
    
    // Get all edges in the graph (for JSON serialization)
    std::vector<GraphEdge> GetAllEdges() const {
        std::vector<GraphEdge> all_edges;
        for (const auto& pair : adjacency_list_) {
            for (const auto& edge : pair.second) {
                all_edges.push_back(edge);
            }
        }
        return all_edges;
    }
    
    // Get branches from a specific node  
    // Aligns with: eino/compose/graph.go branch handling
    std::vector<std::shared_ptr<GraphBranch>> GetBranches(const std::string& node_name) const {
        auto it = branches_.find(node_name);
        if (it != branches_.end()) {
            return it->second;
        }
        return std::vector<std::shared_ptr<GraphBranch>>();
    }
    
    // Extended methods for component-based graph construction
    // These are typed methods that should be specialized for specific I/O types
    // Aligns with eino/compose/graph.go:350-447
    
    // AddChatModelNode adds a ChatModel node
    // Aligns with: eino/compose/graph.go:350-353 (AddChatModelNode)
    // This is a member method that accepts Runnable (for compatibility with react.cpp)
    std::string AddChatModelNode(
        const std::string& key,
        std::shared_ptr<Runnable<I, O>> chat_model,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        
        if (!chat_model) {
            return "ChatModel cannot be null";
        }
        
        try {
            // Directly add as a node using base AddNode method
            // This works because BindTools returns a Runnable<I, O>
            AddNode(key, chat_model, NodeTriggerMode::AllPredecessor, nullptr);
            return "";  // Success
        } catch (const std::exception& e) {
            return e.what();
        }
    }
    
    // AddToolsNode adds a ToolsNode
    // Aligns with: eino/compose/graph.go:375-378 (AddToolsNode)
    std::string AddToolsNode(
        const std::string& key,
        std::shared_ptr<Runnable<I, O>> tools_node,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        
        if (!tools_node) {
            return "ToolsNode cannot be null";
        }
        
        try {
            AddNode(key, tools_node, NodeTriggerMode::AllPredecessor, nullptr);
            return "";  // Success
        } catch (const std::exception& e) {
            return e.what();
        }
    }
    
    // AddLambdaNode adds a Lambda node
    // Aligns with: eino/compose/graph.go:398-407 (AddLambdaNode)
    std::string AddLambdaNode(
        const std::string& key,
        std::shared_ptr<Runnable<I, O>> lambda,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        
        if (!lambda) {
            return "Lambda cannot be null";
        }
        
        try {
            AddNode(key, lambda, NodeTriggerMode::AllPredecessor, nullptr);
            return "";  // Success
        } catch (const std::exception& e) {
            return e.what();
        }
    }
    
    // AddBranch adds a conditional branch
    // Aligns with: eino/compose/graph.go:444-447 (AddBranch)
    std::string AddBranch(
        const std::string& start_node,
        std::shared_ptr<GraphBranch> branch) {
        
        if (is_compiled_) {
            return "Graph already compiled, cannot add branch";
        }
        
        if (start_node.empty()) {
            return "Start node cannot be empty";
        }
        
        if (!nodes_.count(start_node) && start_node != START_NODE) {
            return "Start node not found: " + start_node;
        }
        
        if (!branch) {
            return "Branch cannot be null";
        }
        
        try {
            // Store branch
            branches_[start_node].push_back(branch);
            return "";  // Success
        } catch (const std::exception& e) {
            return e.what();
        }
    }
    
    // AddBranchEdge adds an edge from a BranchNode to a target node for a specific branch index
    // This method establishes the routing relationship between BranchNode outputs and subsequent nodes
    // 
    // Usage:
    //   graph.AddBranchEdge("branch_decision", 0, "high_value_handler");  // Branch 0 -> high_value_handler
    //   graph.AddBranchEdge("branch_decision", 1, "low_value_handler");   // Branch 1 -> low_value_handler
    //
    // Parameters:
    //   - branch_node: Name of the BranchNode
    //   - branch_index: The branch index (0, 1, 2, ... N where N is the default branch)
    //   - target_node: The node to execute when this branch is selected
    void AddBranchEdge(const std::string& branch_node,
                       int branch_index,
                       const std::string& target_node) {
        if (is_compiled_) {
            throw std::runtime_error("Graph already compiled, cannot add branch edge");
        }
        
        if (branch_node.empty()) {
            throw std::invalid_argument("Branch node name cannot be empty");
        }
        
        if (branch_index < 0) {
            throw std::invalid_argument("Branch index must be non-negative");
        }
        
        if (target_node.empty()) {
            throw std::invalid_argument("Target node name cannot be empty");
        }
        
        // Verify branch_node exists
        if (!nodes_.count(branch_node)) {
            throw std::runtime_error("Branch node not found: " + branch_node);
        }
        
        // Verify target_node exists (or is END_NODE)
        if (target_node != END_NODE && !nodes_.count(target_node)) {
            throw std::runtime_error("Target node not found: " + target_node);
        }
        
        // Store the branch edge mapping
        branch_edges_[branch_node][branch_index] = target_node;
        
        // Also add a regular edge for graph structure
        // This edge is primarily for control flow and validation
        AddEdge(branch_node, target_node, false, true);  // no_control=false, no_data=true
    }
    
    // Get the target node for a specific branch
    std::string GetBranchTarget(const std::string& branch_node, int branch_index) const {
        auto it = branch_edges_.find(branch_node);
        if (it != branch_edges_.end()) {
            auto target_it = it->second.find(branch_index);
            if (target_it != it->second.end()) {
                return target_it->second;
            }
        }
        return "";  // No target found
    }
    
    // Get all branch edges from a specific BranchNode
    std::map<int, std::string> GetBranchEdges(const std::string& branch_node) const {
        auto it = branch_edges_.find(branch_node);
        if (it != branch_edges_.end()) {
            return it->second;
        }
        return std::map<int, std::string>();
    }
    
    // AddBranchNode adds a conditional branch node (coze-studio style)
    // Aligns with: coze-studio selector node
    // This is a convenience method that creates a BranchNode from configuration
    std::string AddBranchNode(
        const std::string& key,
        const BranchNodeConfig& config,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        
        if (config.clauses.empty()) {
            return "BranchNode config clauses are empty";
        }
        
        try {
            // Create BranchNode from config
            auto branch_node = BranchNode<I, O>::New(nullptr, config);
            
            // Add as regular node
            AddNode(key, branch_node, NodeTriggerMode::AllPredecessor, nullptr);
            return "";  // Success
        } catch (const std::exception& e) {
            return e.what();
        }
    }
    
    // AddChatModelNodeTyped adds a ChatModel node (specialized per type)
    void AddChatModelNodeTyped(
        const std::string& key,
        std::shared_ptr<components::BaseChatModel> chat_model,
        const std::vector<GraphAddNodeOpt>& opts = {});
    
    // AddToolsNodeTyped adds a ToolsNode (specialized per type)
    void AddToolsNodeTyped(
        const std::string& key,
        std::shared_ptr<ToolsNode> tools_node,
        const std::vector<GraphAddNodeOpt>& opts = {});
    
    // AddBranchTyped adds a conditional branch (templated)
    void AddBranchTyped(
        const std::string& start_node,
        std::shared_ptr<GraphBranch> branch);
    
    // Internal method for adding nodes from GraphNode structure
    void AddNodeInternal(
        const std::string& key,
        std::shared_ptr<GraphNode> graph_node,
        const GraphAddNodeOpts& opts) {
        if (is_compiled_) {
            throw std::runtime_error("Graph already compiled, cannot add node");
        }
        
        if (key.empty() || key == START_NODE || key == END_NODE) {
            throw std::invalid_argument("Invalid node key: " + key);
        }
        
        if (nodes_.count(key)) {
            throw std::runtime_error("Node already exists: " + key);
        }
        
        if (!graph_node) {
            throw std::invalid_argument("Graph node cannot be null");
        }
        
        // Store the graph node (contains runnable + metadata)
        nodes_[key] = graph_node;
        adjacency_list_[key] = std::vector<GraphEdge>();
        in_degree_[key] = 0;
    }
    
    // Internal method for adding branches
    void AddBranchInternal(
        const std::string& start_node,
        std::shared_ptr<GraphBranch> branch) {
        if (is_compiled_) {
            throw std::runtime_error("Graph already compiled, cannot add branch");
        }
        
        if (start_node.empty()) {
            throw std::invalid_argument("Start node cannot be empty");
        }
        
        if (!nodes_.count(start_node) && start_node != START_NODE) {
            throw std::runtime_error("Start node not found: " + start_node);
        }
        
        if (!branch) {
            throw std::invalid_argument("Branch cannot be null");
        }
        
        branches_[start_node].push_back(branch);
    }
    
private:
    std::vector<std::string> GetPredecessors(const std::string& node_name) const {
        std::vector<std::string> preds;
        for (const auto& pair : adjacency_list_) {
            for (const auto& edge : pair.second) {
                if (edge.to == node_name && edge.is_data_edge) {
                    preds.push_back(edge.from);
                }
            }
        }
        return preds;
    }
    
    O MergePredecessorOutputs(const std::vector<O>& outputs) const {
        if (outputs.empty()) {
            return O(); // Return default-constructed value
        }
        if (outputs.size() == 1) {
            return outputs[0];
        }
        
        // For multiple outputs, merge strategy depends on type
        if constexpr (std::is_same_v<O, std::map<std::string, std::any>>) {
            // For map type, merge all outputs into one map
            // Later values overwrite earlier ones for same keys
            std::map<std::string, std::any> merged;
            for (const auto& output : outputs) {
                for (const auto& [key, value] : output) {
                    merged[key] = value;
                }
            }
            return merged;
        } else {
            // For other types, return the last output (backward compatible)
            return outputs.back();
        }
    }
    
    void ValidateGraphStructure() {
        std::set<std::string> reachable;
        std::queue<std::string> queue;
        queue.push("__START__");
        
        while (!queue.empty()) {
            std::string current = queue.front();
            queue.pop();
            
            if (reachable.count(current)) {
                continue;
            }
            reachable.insert(current);
            
            if (adjacency_list_.count(current)) {
                for (const auto& edge : adjacency_list_[current]) {
                    if (!reachable.count(edge.to)) {
                        queue.push(edge.to);
                    }
                }
            }
        }
    }
    
    void TopologicalSort() {
        topological_order_.clear();
        
        std::map<std::string, int> in_degree_copy = in_degree_;
        std::queue<std::string> queue;
        queue.push("__START__");
        
        while (!queue.empty()) {
            std::string current = queue.front();
            queue.pop();
            topological_order_.push_back(current);
            
            if (adjacency_list_.count(current)) {
                for (const auto& edge : adjacency_list_[current]) {
                    in_degree_copy[edge.to]--;
                    if (in_degree_copy[edge.to] == 0) {
                        queue.push(edge.to);
                    }
                }
            }
        }
        
        topological_order_.push_back("__END__");
    }
    
    std::map<std::string, std::shared_ptr<GraphNode>> nodes_;
    std::map<std::string, std::vector<GraphEdge>> adjacency_list_;
    std::map<std::string, std::vector<std::shared_ptr<GraphBranch>>> branches_;  // Aligns with eino/compose/graph.go:64
    std::map<std::string, int> in_degree_;
    std::vector<std::string> topological_order_;
    std::vector<std::string> start_nodes_;
    std::vector<std::string> end_nodes_;
    
    // Branch edge routing: maps branch_node -> (branch_index -> target_node)
    std::map<std::string, std::map<int, std::string>> branch_edges_;
    
    bool is_compiled_;
    bool has_error_;
    GraphCompileOptions compile_options_;
    
    // ✅ Type validation support - Aligns with eino/compose/graph.go:60-63
    GraphValidator validator_;
    std::map<std::string, const std::type_info*> node_input_types_;
    std::map<std::string, const std::type_info*> node_output_types_;
};

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_GRAPH_H_
