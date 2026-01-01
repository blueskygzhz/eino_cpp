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
#include "eino/compose/component_to_graph_node.h"
#include "eino/compose/tool_node.h"
#include "eino/compose/branch.h"
#include "eino/components/model.h"
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <functional>

namespace eino {
namespace compose {

// Helper functions for cycle detection

std::vector<std::vector<std::string>> FindCycles(
    const std::string& start_node,
    const std::map<std::string, std::vector<std::string>>& adjacency) {
    
    std::vector<std::vector<std::string>> cycles;
    std::set<std::string> visited;
    
    // DFS for cycle detection
    std::function<void(const std::string&, std::vector<std::string>&, std::map<std::string, int>&)> dfs;
    dfs = [&](const std::string& node, std::vector<std::string>& path, std::map<std::string, int>& color) {
        color[node] = 1;
        path.push_back(node);
        
        if (adjacency.count(node)) {
            for (const auto& next : adjacency.at(node)) {
                if (color[next] == 1) {
                    auto cycle_start = std::find(path.begin(), path.end(), next);
                    if (cycle_start != path.end()) {
                        std::vector<std::string> cycle(cycle_start, path.end());
                        cycle.push_back(next);
                        cycles.push_back(cycle);
                    }
                } else if (color[next] == 0) {
                    dfs(next, path, color);
                }
            }
        }
        
        path.pop_back();
        color[node] = 2;
    };
    
    std::map<std::string, int> color;
    std::vector<std::string> path;
    dfs(start_node, path, color);
    
    return cycles;
}

std::string FormatCycleError(const std::vector<std::vector<std::string>>& cycles) {
    std::stringstream ss;
    ss << "Graph contains cycles: ";
    for (size_t i = 0; i < cycles.size(); ++i) {
        if (i > 0) ss << "; ";
        ss << "[";
        for (size_t j = 0; j < cycles[i].size(); ++j) {
            if (j > 0) ss << " -> ";
            ss << cycles[i][j];
        }
        ss << "]";
    }
    return ss.str();
}

// GraphImpl - Internal implementation class for Graph
// Aligns with eino/compose/graph.go:146-150 (newGraph)
class GraphImpl {
public:
    std::map<std::string, std::shared_ptr<GraphNode>> nodes;
    std::map<std::string, std::vector<std::string>> control_edges;
    std::map<std::string, std::vector<std::string>> data_edges;
    std::map<std::string, std::vector<std::shared_ptr<GraphBranch>>> branches;
    std::vector<std::string> start_nodes;
    std::vector<std::string> end_nodes;
    
    bool compiled = false;
    bool has_error = false;
    std::string build_error;
    
    // Add node to graph
    // Aligns with eino/compose/graph.go:561-590 (addNode)
    void AddNode(const std::string& key, 
                 std::shared_ptr<GraphNode> node,
                 const GraphAddNodeOpts& opts) {
        if (compiled) {
            throw std::runtime_error("Graph already compiled, cannot add node");
        }
        
        if (key.empty()) {
            throw std::invalid_argument("Node key cannot be empty");
        }
        
        if (key == START || key == END) {
            throw std::invalid_argument("Cannot use reserved node name: " + key);
        }
        
        if (nodes.count(key)) {
            throw std::runtime_error("Node already exists: " + key);
        }
        
        if (!node) {
            throw std::invalid_argument("Node cannot be null");
        }
        
        nodes[key] = node;
        control_edges[key] = {};
        data_edges[key] = {};
    }
    
    // Add branch to graph
    // Aligns with eino/compose/graph.go:444-470 (AddBranch/addBranch)
    void AddBranch(const std::string& start_node,
                   std::shared_ptr<GraphBranch> branch) {
        if (compiled) {
            throw std::runtime_error("Graph already compiled, cannot add branch");
        }
        
        if (start_node.empty()) {
            throw std::invalid_argument("Start node cannot be empty");
        }
        
        if (!nodes.count(start_node) && start_node != START) {
            throw std::runtime_error("Start node not found: " + start_node);
        }
        
        if (!branch) {
            throw std::invalid_argument("Branch cannot be null");
        }
        
        // Validate branch end nodes exist
        for (const auto& end_node : branch->GetEndNodes()) {
            if (!nodes.count(end_node) && end_node != END) {
                throw std::runtime_error("Branch end node not found: " + end_node);
            }
        }
        
        branches[start_node].push_back(branch);
    }
};

// Global constants
// Aligns with eino/compose/graph.go:35-38
const std::string START = "__START__";
const std::string END = "__END__";

} // namespace compose
} // namespace eino
