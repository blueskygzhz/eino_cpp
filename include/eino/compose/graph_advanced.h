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

#ifndef EINO_CPP_COMPOSE_GRAPH_ADVANCED_H_
#define EINO_CPP_COMPOSE_GRAPH_ADVANCED_H_

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#include "graph.h"
#include "graph_run.h"
#include "runnable.h"

namespace eino {
namespace compose {

// Advanced Graph features aligned with eino Go version

// NodeBranch represents conditional branching logic
template<typename I, typename O>
struct NodeBranch {
    std::string source_node;
    std::string target_node;
    std::function<bool(std::shared_ptr<Context>, const O&)> condition;
    
    NodeBranch(
        const std::string& src,
        const std::string& tgt,
        std::function<bool(std::shared_ptr<Context>, const O&)> cond)
        : source_node(src), target_node(tgt), condition(cond) {}
};

// StatefulGraph extends Graph with state management
template<typename I, typename O>
class StatefulGraph : public Graph<I, O> {
public:
    StatefulGraph() = default;
    
    virtual ~StatefulGraph() = default;
    
    // SetState sets a value in the graph's state
    void SetState(std::shared_ptr<Context> ctx, 
                  const std::string& key, 
                  const json& value) {
        if (ctx) {
            ctx->SetValue(key, value);
        }
    }
    
    // GetState retrieves a value from the graph's state
    bool GetState(std::shared_ptr<Context> ctx,
                  const std::string& key,
                  json& value) {
        if (ctx) {
            return ctx->GetValue(key, value);
        }
        return false;
    }
};

// ConditionalGraph adds support for conditional edges and branches
template<typename I, typename O>
class ConditionalGraph : public StatefulGraph<I, O> {
public:
    ConditionalGraph() = default;
    
    virtual ~ConditionalGraph() = default;
    
    // AddConditionalEdge adds a conditional edge between nodes
    void AddConditionalEdge(
        const std::string& from,
        const std::string& to,
        std::function<bool(std::shared_ptr<Context>, const O&)> condition) {
        branches_.emplace_back(from, to, condition);
    }
    
    // AddBranch supports multiple branches from a single node
    void AddBranch(
        const std::string& source,
        const std::vector<std::pair<std::string, std::function<bool(std::shared_ptr<Context>, const O&)>>>& branches) {
        for (const auto& branch : branches) {
            AddConditionalEdge(source, branch.first, branch.second);
        }
    }
    
    // GetBranches returns all conditional branches
    const std::vector<NodeBranch<I, O>>& GetBranches() const {
        return branches_;
    }
    
protected:
    std::vector<NodeBranch<I, O>> branches_;
};

// FanOutGraph supports parallel execution of multiple nodes
template<typename I, typename O>
class FanOutGraph : public ConditionalGraph<I, O> {
public:
    FanOutGraph() = default;
    
    virtual ~FanOutGraph() = default;
    
    // AddFanOut adds parallel nodes that execute the same input
    void AddFanOut(
        const std::string& source,
        const std::vector<std::string>& target_nodes) {
        for (const auto& target : target_nodes) {
            this->AddEdge(source, target);
        }
        fan_outs_[source] = target_nodes;
    }
    
    // GetFanOuts returns the fan-out configuration
    const std::map<std::string, std::vector<std::string>>& GetFanOuts() const {
        return fan_outs_;
    }
    
private:
    std::map<std::string, std::vector<std::string>> fan_outs_;
};

// FanInGraph supports merging results from multiple nodes
template<typename I, typename O>
class FanInGraph : public FanOutGraph<I, O> {
public:
    FanInGraph() = default;
    
    virtual ~FanInGraph() = default;
    
    // MergeStrategy defines how to merge multiple outputs
    enum class MergeStrategy {
        First,      // Take the first output
        Last,       // Take the last output
        Combine,    // Combine all outputs
        Custom      // Use custom merge function
    };
    
    // AddFanIn adds a merge node that combines results from multiple nodes
    void AddFanIn(
        const std::string& merge_node,
        const std::vector<std::string>& source_nodes,
        MergeStrategy strategy = MergeStrategy::Last) {
        
        for (const auto& source : source_nodes) {
            this->AddEdge(source, merge_node);
        }
        
        fan_ins_[merge_node] = {
            source_nodes,
            strategy
        };
    }
    
    // SetCustomMergeFunc sets a custom merge function
    void SetCustomMergeFunc(
        const std::string& merge_node,
        std::function<O(std::shared_ptr<Context>, const std::vector<O>&)> merge_func) {
        custom_merge_funcs_[merge_node] = merge_func;
    }
    
    // GetFanIns returns the fan-in configuration
    const std::map<std::string, std::pair<std::vector<std::string>, MergeStrategy>>& GetFanIns() const {
        return fan_ins_;
    }
    
private:
    struct FanInConfig {
        std::vector<std::string> source_nodes;
        MergeStrategy strategy;
    };
    
    std::map<std::string, FanInConfig> fan_ins_;
    std::map<std::string, std::function<O(std::shared_ptr<Context>, const std::vector<O>&)>> custom_merge_funcs_;
};

// CheckpointedGraph adds checkpoint/restore capability
template<typename I, typename O>
class CheckpointedGraph : public FanInGraph<I, O> {
public:
    CheckpointedGraph() = default;
    
    virtual ~CheckpointedGraph() = default;
    
    // Checkpoint data for node execution
    struct NodeCheckpoint {
        std::string node_name;
        O output;
        bool completed;
        long timestamp_ms;
        
        NodeCheckpoint() : completed(false), timestamp_ms(0) {}
    };
    
    // SaveCheckpoint saves the current execution state
    void SaveCheckpoint(std::shared_ptr<Context> ctx,
                       const std::string& checkpoint_id,
                       const NodeCheckpoint& checkpoint) {
        if (!ctx) return;
        
        json cp_json;
        cp_json["node_name"] = checkpoint.node_name;
        cp_json["completed"] = checkpoint.completed;
        cp_json["timestamp_ms"] = checkpoint.timestamp_ms;
        
        ctx->SetValue("checkpoint_" + checkpoint_id, cp_json);
        checkpoints_[checkpoint_id] = checkpoint;
    }
    
    // LoadCheckpoint loads a previously saved execution state
    bool LoadCheckpoint(std::shared_ptr<Context> ctx,
                       const std::string& checkpoint_id,
                       NodeCheckpoint& checkpoint) {
        auto it = checkpoints_.find(checkpoint_id);
        if (it != checkpoints_.end()) {
            checkpoint = it->second;
            return true;
        }
        return false;
    }
    
    // ClearCheckpoints removes all saved checkpoints
    void ClearCheckpoints() {
        checkpoints_.clear();
    }
    
private:
    std::map<std::string, NodeCheckpoint> checkpoints_;
};

// SubGraph represents a nested graph within a larger graph
template<typename I, typename O>
class SubGraph : public ComposableRunnable<I, O> {
public:
    explicit SubGraph(std::shared_ptr<Graph<I, O>> graph)
        : graph_(graph) {
        if (!graph_) {
            throw std::runtime_error("SubGraph: graph cannot be null");
        }
        if (!graph_->IsCompiled()) {
            graph_->Compile();
        }
    }
    
    O Invoke(
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        return graph_->Invoke(ctx, input, opts);
    }
    
    std::shared_ptr<StreamReader<O>> Stream(
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        return graph_->Stream(ctx, input, opts);
    }
    
    O Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        return graph_->Collect(ctx, input, opts);
    }
    
    std::shared_ptr<StreamReader<O>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        return graph_->Transform(ctx, input, opts);
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(I);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(O);
    }
    
    std::string GetComponentType() const override {
        return "SubGraph";
    }
    
    std::shared_ptr<Graph<I, O>> GetInnerGraph() const {
        return graph_;
    }
    
private:
    std::shared_ptr<Graph<I, O>> graph_;
};

// GraphBuilder provides fluent API for building complex graphs
template<typename I, typename O>
class GraphBuilder {
public:
    GraphBuilder() 
        : graph_(std::make_shared<CheckpointedGraph<I, O>>()) {}
    
    explicit GraphBuilder(std::shared_ptr<Graph<I, O>> graph)
        : graph_(graph) {}
    
    // Builder methods
    GraphBuilder& Node(
        const std::string& name,
        std::shared_ptr<Runnable<I, O>> runnable) {
        graph_->AddNode(name, runnable);
        return *this;
    }
    
    GraphBuilder& Edge(
        const std::string& from,
        const std::string& to) {
        graph_->AddEdge(from, to);
        return *this;
    }
    
    GraphBuilder& Start(const std::string& node) {
        graph_->AddEdge(Graph<I, O>::START_NODE, node);
        return *this;
    }
    
    GraphBuilder& End(const std::string& node) {
        graph_->AddEdge(node, Graph<I, O>::END_NODE);
        return *this;
    }
    
    GraphBuilder& Path(const std::vector<std::string>& nodes) {
        graph_->AddEdge(Graph<I, O>::START_NODE, nodes[0]);
        for (size_t i = 0; i + 1 < nodes.size(); ++i) {
            graph_->AddEdge(nodes[i], nodes[i+1]);
        }
        graph_->AddEdge(nodes.back(), Graph<I, O>::END_NODE);
        return *this;
    }
    
    // Build the graph
    std::shared_ptr<Graph<I, O>> Build() {
        graph_->Compile();
        return graph_;
    }
    
    // Build with a runner
    std::shared_ptr<GraphRunner<I, O>> BuildWithRunner(
        const GraphRunOptions& opts = GraphRunOptions()) {
        graph_->Compile();
        return NewGraphRunner(graph_, opts);
    }
    
    std::shared_ptr<Graph<I, O>> GetGraph() const {
        return graph_;
    }
    
private:
    std::shared_ptr<Graph<I, O>> graph_;
};

// Helper functions for creating advanced graphs
template<typename I, typename O>
std::shared_ptr<ConditionalGraph<I, O>> NewConditionalGraph() {
    return std::make_shared<ConditionalGraph<I, O>>();
}

template<typename I, typename O>
std::shared_ptr<FanOutGraph<I, O>> NewFanOutGraph() {
    return std::make_shared<FanOutGraph<I, O>>();
}

template<typename I, typename O>
std::shared_ptr<FanInGraph<I, O>> NewFanInGraph() {
    return std::make_shared<FanInGraph<I, O>>();
}

template<typename I, typename O>
std::shared_ptr<CheckpointedGraph<I, O>> NewCheckpointedGraph() {
    return std::make_shared<CheckpointedGraph<I, O>>();
}

template<typename I, typename O>
GraphBuilder<I, O> BuildGraph() {
    return GraphBuilder<I, O>();
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_GRAPH_ADVANCED_H_
