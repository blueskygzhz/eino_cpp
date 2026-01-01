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

#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include "eino/compose/graph.h"

using json = nlohmann::json;

// Example node implementation
template<typename I, typename O>
class SimpleNode : public eino::compose::Runnable<I, O> {
public:
    SimpleNode(const std::string& name) : name_(name) {}
    
    O Invoke(
        std::shared_ptr<eino::compose::Context> ctx,
        const I& input, 
        const std::vector<eino::compose::Option>& opts = std::vector<eino::compose::Option>()) override {
        std::cout << "[" << name_ << "] Processing input" << std::endl;
        return O();
    }
    
    std::shared_ptr<eino::compose::StreamReader<O>> Stream(
        std::shared_ptr<eino::compose::Context> ctx,
        const I& input,
        const std::vector<eino::compose::Option>& opts = std::vector<eino::compose::Option>()) override {
        std::vector<O> result;
        result.push_back(O());
        return std::make_shared<eino::compose::SimpleStreamReader<O>>(result);
    }
    
    O Collect(
        std::shared_ptr<eino::compose::Context> ctx,
        std::shared_ptr<eino::compose::StreamReader<I>> input,
        const std::vector<eino::compose::Option>& opts = std::vector<eino::compose::Option>()) override {
        return O();
    }
    
    std::shared_ptr<eino::compose::StreamReader<O>> Transform(
        std::shared_ptr<eino::compose::Context> ctx,
        std::shared_ptr<eino::compose::StreamReader<I>> input,
        const std::vector<eino::compose::Option>& opts = std::vector<eino::compose::Option>()) override {
        std::vector<O> result;
        result.push_back(O());
        return std::make_shared<eino::compose::SimpleStreamReader<O>>(result);
    }
    
private:
    std::string name_;
};

int main() {
    std::cout << "=== Eino C++ Simple Graph Example ===" << std::endl;
    
    try {
        // Create a simple graph
        typedef eino::compose::Graph<json, json> JsonGraph;
        auto graph = std::make_shared<JsonGraph>();
        
        // Create nodes
        auto node1 = std::make_shared<SimpleNode<json, json>>("Node1");
        auto node2 = std::make_shared<SimpleNode<json, json>>("Node2");
        
        // Add nodes to graph
        graph->AddNode("node1", node1);
        graph->AddNode("node2", node2);
        
        // Connect edges from START to node1, then node1 to node2, then node2 to END
        graph->AddEdge(eino::compose::Graph<json, json>::START_NODE, "node1");
        graph->AddEdge("node1", "node2");
        graph->AddEdge("node2", eino::compose::Graph<json, json>::END_NODE);
        
        // Get node names
        auto names = graph->GetNodeNames();
        std::cout << "\nGraph nodes (" << names.size() << "): ";
        for (const auto& name : names) {
            std::cout << name << " ";
        }
        std::cout << std::endl;
        
        std::cout << "Graph edges: " << graph->GetEdgeCount() << std::endl;
        
        // Compile graph
        graph->Compile();
        std::cout << "\nGraph compiled successfully!" << std::endl;
        
        // Create context and options
        auto ctx = eino::compose::Context::Background();
        auto empty_opts = std::vector<eino::compose::Option>();
        
        // Execute graph
        json input = json::value_t::object;
        json output = graph->Invoke(ctx, input, empty_opts);
        std::cout << "\nGraph executed successfully!" << std::endl;
        
        std::cout << "\nExample completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
