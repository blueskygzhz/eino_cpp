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

#include "eino/compose/introspect.h"
#include "eino/compose/dag.h"

#include <sstream>
#include <algorithm>
#include <iostream>
#include <mutex>

namespace eino {
namespace compose {

// 对齐 eino/compose/introspect.go

// 全局回调存储
static std::vector<std::shared_ptr<GraphCompileCallback>> g_global_callbacks;
static std::mutex g_callbacks_mutex;

std::string FieldMappingInfo::ToString() const {
    std::ostringstream oss;
    oss << "[from ";
    
    if (!from_field.empty()) {
        oss << from_field << "(field) of ";
    }
    
    oss << from_node_key;
    
    if (!to_field.empty()) {
        oss << " to " << to_field << "(field)";
    }
    
    oss << "]";
    return oss.str();
}

std::vector<std::string> GraphInfo::GetPredecessors(const std::string& node_key) const {
    std::vector<std::string> predecessors;
    
    // 检查控制边
    for (const auto& [from, to_list] : edges) {
        if (std::find(to_list.begin(), to_list.end(), node_key) != to_list.end()) {
            predecessors.push_back(from);
        }
    }
    
    // 检查数据边
    for (const auto& [from, to_list] : data_edges) {
        if (std::find(to_list.begin(), to_list.end(), node_key) != to_list.end()) {
            if (std::find(predecessors.begin(), predecessors.end(), from) == predecessors.end()) {
                predecessors.push_back(from);
            }
        }
    }
    
    return predecessors;
}

std::vector<std::string> GraphInfo::GetSuccessors(const std::string& node_key) const {
    std::vector<std::string> successors;
    
    // 控制边
    if (edges.count(node_key)) {
        const auto& control_succs = edges.at(node_key);
        successors.insert(successors.end(), control_succs.begin(), control_succs.end());
    }
    
    // 数据边
    if (data_edges.count(node_key)) {
        const auto& data_succs = data_edges.at(node_key);
        for (const auto& succ : data_succs) {
            if (std::find(successors.begin(), successors.end(), succ) == successors.end()) {
                successors.push_back(succ);
            }
        }
    }
    
    return successors;
}

bool GraphInfo::HasCycle() const {
    // 构建邻接表
    std::map<std::string, std::vector<std::string>> adjacency;
    for (const auto& [from, to_list] : edges) {
        adjacency[from] = to_list;
    }
    for (const auto& [from, to_list] : data_edges) {
        for (const auto& to : to_list) {
            adjacency[from].push_back(to);
        }
    }
    
    auto cycles = DAGChannelHelper::DetectCycles(adjacency);
    return !cycles.empty();
}

std::vector<std::string> GraphInfo::TopologicalSort() const {
    // 构建邻接表
    std::map<std::string, std::vector<std::string>> adjacency;
    for (const auto& [from, to_list] : edges) {
        adjacency[from] = to_list;
    }
    for (const auto& [from, to_list] : data_edges) {
        for (const auto& to : to_list) {
            adjacency[from].push_back(to);
        }
    }
    
    return DAGChannelHelper::TopologicalSort(adjacency);
}

std::string GraphInfo::ToJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"name\": \"" << name << "\",\n";
    oss << "  \"input_type\": \"" << input_type << "\",\n";
    oss << "  \"output_type\": \"" << output_type << "\",\n";
    
    // Nodes
    oss << "  \"nodes\": {\n";
    bool first = true;
    for (const auto& [key, node] : nodes) {
        if (!first) oss << ",\n";
        first = false;
        oss << "    \"" << key << "\": {\n";
        oss << "      \"name\": \"" << node.name << "\",\n";
        oss << "      \"input_key\": \"" << node.input_key << "\",\n";
        oss << "      \"output_key\": \"" << node.output_key << "\"\n";
        oss << "    }";
    }
    oss << "\n  },\n";
    
    // Edges
    oss << "  \"edges\": {\n";
    first = true;
    for (const auto& [from, to_list] : edges) {
        if (!first) oss << ",\n";
        first = false;
        oss << "    \"" << from << "\": [";
        for (size_t i = 0; i < to_list.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << "\"" << to_list[i] << "\"";
        }
        oss << "]";
    }
    oss << "\n  }\n";
    
    oss << "}";
    return oss.str();
}

// 全局回调管理

void RegisterGlobalGraphCompileCallback(std::shared_ptr<GraphCompileCallback> callback) {
    std::lock_guard<std::mutex> lock(g_callbacks_mutex);
    g_global_callbacks.push_back(callback);
}

void ClearGlobalGraphCompileCallbacks() {
    std::lock_guard<std::mutex> lock(g_callbacks_mutex);
    g_global_callbacks.clear();
}

std::vector<std::shared_ptr<GraphCompileCallback>> GetGlobalGraphCompileCallbacks() {
    std::lock_guard<std::mutex> lock(g_callbacks_mutex);
    return g_global_callbacks;
}

// LoggingGraphCompileCallback implementation

void LoggingGraphCompileCallback::OnFinish(
    std::shared_ptr<Context> ctx, 
    const GraphInfo& info) {
    
    std::cout << "=== Graph Compiled ===" << std::endl;
    std::cout << "Name: " << info.name << std::endl;
    std::cout << "Input Type: " << info.input_type << std::endl;
    std::cout << "Output Type: " << info.output_type << std::endl;
    std::cout << "Nodes: " << info.nodes.size() << std::endl;
    
    for (const auto& [key, node] : info.nodes) {
        std::cout << "  - " << key << ": " << node.name << std::endl;
    }
    
    std::cout << "Edges: " << info.edges.size() << std::endl;
    for (const auto& [from, to_list] : info.edges) {
        for (const auto& to : to_list) {
            std::cout << "  " << from << " -> " << to << std::endl;
        }
    }
    
    if (info.HasCycle()) {
        std::cout << "WARNING: Graph contains cycles!" << std::endl;
    }
    
    std::cout << "=====================" << std::endl;
}

// GraphPrinter implementation

std::string GraphPrinter::ToDot(const GraphInfo& info) {
    std::ostringstream oss;
    oss << "digraph \"" << info.name << "\" {\n";
    oss << "  rankdir=LR;\n";
    oss << "  node [shape=box];\n\n";
    
    // Nodes
    for (const auto& [key, node] : info.nodes) {
        oss << "  \"" << key << "\" [label=\"" << node.name << "\"];\n";
    }
    
    oss << "\n";
    
    // Control edges
    for (const auto& [from, to_list] : info.edges) {
        for (const auto& to : to_list) {
            oss << "  \"" << from << "\" -> \"" << to << "\" [color=black];\n";
        }
    }
    
    // Data edges
    for (const auto& [from, to_list] : info.data_edges) {
        for (const auto& to : to_list) {
            oss << "  \"" << from << "\" -> \"" << to << "\" [color=blue, style=dashed];\n";
        }
    }
    
    oss << "}\n";
    return oss.str();
}

std::string GraphPrinter::ToText(const GraphInfo& info) {
    std::ostringstream oss;
    oss << "Graph: " << info.name << "\n";
    oss << "Type: " << info.input_type << " -> " << info.output_type << "\n";
    oss << "\nNodes (" << info.nodes.size() << "):\n";
    
    for (const auto& [key, node] : info.nodes) {
        oss << "  " << key;
        if (!node.name.empty() && node.name != key) {
            oss << " (" << node.name << ")";
        }
        if (!node.input_key.empty()) {
            oss << " [in:" << node.input_key << "]";
        }
        if (!node.output_key.empty()) {
            oss << " [out:" << node.output_key << "]";
        }
        oss << "\n";
    }
    
    oss << "\nControl Edges:\n";
    for (const auto& [from, to_list] : info.edges) {
        for (const auto& to : to_list) {
            oss << "  " << from << " => " << to << "\n";
        }
    }
    
    if (!info.data_edges.empty()) {
        oss << "\nData Edges:\n";
        for (const auto& [from, to_list] : info.data_edges) {
            for (const auto& to : to_list) {
                oss << "  " << from << " ~> " << to << "\n";
            }
        }
    }
    
    return oss.str();
}

std::string GraphPrinter::GetStatistics(const GraphInfo& info) {
    std::ostringstream oss;
    
    oss << "Graph Statistics for: " << info.name << "\n";
    oss << "  Total Nodes: " << info.nodes.size() << "\n";
    
    size_t total_control_edges = 0;
    for (const auto& [_, to_list] : info.edges) {
        total_control_edges += to_list.size();
    }
    oss << "  Control Edges: " << total_control_edges << "\n";
    
    size_t total_data_edges = 0;
    for (const auto& [_, to_list] : info.data_edges) {
        total_data_edges += to_list.size();
    }
    oss << "  Data Edges: " << total_data_edges << "\n";
    
    oss << "  Total Branches: " << info.branches.size() << "\n";
    oss << "  Has Cycle: " << (info.HasCycle() ? "Yes" : "No") << "\n";
    
    // 计算最大深度
    auto topo = info.TopologicalSort();
    if (!topo.empty()) {
        oss << "  Max Depth: " << topo.size() << "\n";
    }
    
    return oss.str();
}

} // namespace compose
} // namespace eino
