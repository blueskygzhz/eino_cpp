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

#ifndef EINO_CPP_COMPOSE_WORKFLOW_H_
#define EINO_CPP_COMPOSE_WORKFLOW_H_

#include <memory>
#include <map>
#include <string>
#include <functional>
#include <vector>
#include "runnable.h"
#include "graph.h"
#include "graph_call_options.h"
#include "branch.h"
#include "field_mapping.h"

namespace eino {
namespace compose {

// Import json type from field_mapping
using json = nlohmann::json;

// Forward declarations
template<typename I, typename O> class Graph;
class WorkflowNode;
class WorkflowBranch;
class GraphBranch;

// ============================================================================
// DependencyType - Type of dependency between nodes
// Aligns with: eino/compose/workflow.go dependencyType
// ============================================================================

enum class DependencyType {
    Normal,              // Normal execution dependency with data flow
    NoDirectDependency,  // Data flow without direct execution dependency
    Branch               // Dependency through a branch
};

std::string DependencyTypeToString(DependencyType type);

// ============================================================================
// WorkflowAddInputOptions - Options for AddInput
// Aligns with: eino/compose/workflow.go workflowAddInputOpts
// ============================================================================

struct WorkflowAddInputOptions {
    bool no_direct_dependency;      // Create data mapping without direct dependency
    bool dependency_without_input;   // Create execution dependency without data
    
    WorkflowAddInputOptions();
};

// Helper to create WithNoDirectDependency option
// Aligns with: eino/compose/workflow.go WithNoDirectDependency
WorkflowAddInputOptions WithNoDirectDependency();

// ============================================================================
// WorkflowInputInfo - Stores information about a workflow input
// ============================================================================

struct WorkflowInputInfo {
    std::string from_node_key;
    std::vector<FieldMapping> mappings;
    WorkflowAddInputOptions options;
    
    WorkflowInputInfo();
    ~WorkflowInputInfo();
};

// ============================================================================
// WorkflowNode - Node in the workflow
// Aligns with: eino/compose/workflow.go WorkflowNode
// ============================================================================

/**
 * @brief WorkflowNode is the node of the Workflow
 * 
 * Provides methods to define dependencies and data flow between nodes.
 */
class WorkflowNode {
public:
    explicit WorkflowNode(const std::string& key);
    ~WorkflowNode();
    
    /**
     * @brief AddInput creates both data and execution dependencies
     * Aligns with: eino/compose/workflow.go:164-183
     */
    WorkflowNode& AddInput(
        const std::string& from_node_key,
        const std::vector<FieldMapping>& inputs = {});
    
    /**
     * @brief AddInputWithOptions creates a dependency with custom configuration
     * Aligns with: eino/compose/workflow.go:263-270
     */
    WorkflowNode& AddInputWithOptions(
        const std::string& from_node_key,
        const std::vector<FieldMapping>& inputs,
        const WorkflowAddInputOptions& options);
    
    /**
     * @brief AddDependency creates an execution-only dependency
     * Aligns with: eino/compose/workflow.go:287-300
     */
    WorkflowNode& AddDependency(const std::string& from_node_key);
    
    /**
     * @brief SetStaticValue sets a static value for a field path
     * Aligns with: eino/compose/workflow.go:307-312
     */
    WorkflowNode& SetStaticValue(const FieldPath& path, const json& value);
    
    // Getters
    const std::string& GetKey() const;
    const std::vector<WorkflowInputInfo>& GetAddInputs() const;
    const std::map<std::string, json>& GetStaticValues() const;
    
    // Internal methods
    void ClearAddInputs();
    bool CheckAndAddMappedPath(const std::vector<FieldPath>& paths);
    
private:
    std::string key_;
    std::vector<WorkflowInputInfo> add_inputs_;
    std::map<std::string, json> static_values_;
    json mapped_field_path_;  // Track mapped field paths to detect conflicts
};

// ============================================================================
// WorkflowBranch - Wraps GraphBranch with workflow metadata
// Aligns with: eino/compose/workflow.go WorkflowBranch
// ============================================================================

class WorkflowBranch {
public:
    WorkflowBranch(
        const std::string& from_node_key,
        std::shared_ptr<GraphBranch> branch);
    ~WorkflowBranch();
    
    const std::string& GetFromNodeKey() const;
    std::shared_ptr<GraphBranch> GetBranch() const;
    
private:
    std::string from_node_key_;
    std::shared_ptr<GraphBranch> branch_;
};

// ============================================================================
// Workflow - Main workflow class
// Aligns with: eino/compose/workflow.go Workflow[I, O]
// ============================================================================

/**
 * @brief Workflow is a wrapper of Graph, replacing AddEdge with dependency declarations
 * 
 * Aligns with: eino/compose/workflow.go:40-46
 * 
 * Key Design:
 * - Uses internal Graph[I, O] for execution
 * - Declares dependencies via WorkflowNode.AddInput/AddDependency
 * - Uses NodeTriggerMode(AllPredecessor), does not support cycles
 * 
 * Usage:
 *   auto workflow = NewWorkflow<InputType, OutputType>();
 *   auto node1 = workflow->AddLambdaNode("node1", lambda1);
 *   auto node2 = workflow->AddLambdaNode("node2", lambda2);
 *   node2->AddInput("node1", {MapFields({"output"}, {"input"})});
 *   workflow->Compile(ctx);
 *   auto result = workflow->Invoke(ctx, input);
 */
template<typename I, typename O>
class Workflow : public ComposableRunnable<I, O> {
public:
    /**
     * @brief Constructor
     * Aligns with: eino/compose/workflow.go:60-75
     */
    Workflow();
    
    virtual ~Workflow() = default;
    
    // ========================================================================
    // Builder Methods - Add Nodes
    // Aligns with: eino/compose/workflow.go:77-147
    // ========================================================================
    
    /**
     * @brief AddChatModelNode adds a chat model node
     * Aligns with: eino/compose/workflow.go:81-84
     */
    std::shared_ptr<WorkflowNode> AddChatModelNode(
        const std::string& key,
        std::shared_ptr<Runnable<json, json>> chat_model,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AddChatTemplateNode adds a chat template node
     * Aligns with: eino/compose/workflow.go:86-89
     */
    std::shared_ptr<WorkflowNode> AddChatTemplateNode(
        const std::string& key,
        std::shared_ptr<Runnable<json, json>> chat_template,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AddToolsNode adds a tools node
     * Aligns with: eino/compose/workflow.go:91-94
     */
    std::shared_ptr<WorkflowNode> AddToolsNode(
        const std::string& key,
        std::shared_ptr<Runnable<json, json>> tools_node,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AddRetrieverNode adds a retriever node
     * Aligns with: eino/compose/workflow.go:96-99
     */
    std::shared_ptr<WorkflowNode> AddRetrieverNode(
        const std::string& key,
        std::shared_ptr<Runnable<json, json>> retriever,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AddEmbeddingNode adds an embedding node
     * Aligns with: eino/compose/workflow.go:101-104
     */
    std::shared_ptr<WorkflowNode> AddEmbeddingNode(
        const std::string& key,
        std::shared_ptr<Runnable<json, json>> embedding,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AddIndexerNode adds an indexer node
     * Aligns with: eino/compose/workflow.go:106-109
     */
    std::shared_ptr<WorkflowNode> AddIndexerNode(
        const std::string& key,
        std::shared_ptr<Runnable<json, json>> indexer,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AddLoaderNode adds a document loader node
     * Aligns with: eino/compose/workflow.go:111-114
     */
    std::shared_ptr<WorkflowNode> AddLoaderNode(
        const std::string& key,
        std::shared_ptr<Runnable<json, json>> loader,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AddDocumentTransformerNode adds a document transformer node
     * Aligns with: eino/compose/workflow.go:116-119
     */
    std::shared_ptr<WorkflowNode> AddDocumentTransformerNode(
        const std::string& key,
        std::shared_ptr<Runnable<json, json>> transformer,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AddGraphNode adds a nested graph node
     * Aligns with: eino/compose/workflow.go:121-124
     */
    std::shared_ptr<WorkflowNode> AddGraphNode(
        const std::string& key,
        std::shared_ptr<Runnable<json, json>> graph,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AddLambdaNode adds a lambda node
     * Aligns with: eino/compose/workflow.go:126-129
     */
    std::shared_ptr<WorkflowNode> AddLambdaNode(
        const std::string& key,
        std::shared_ptr<Runnable<json, json>> lambda,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief End returns the WorkflowNode representing END node
     * Aligns with: eino/compose/workflow.go:132-137
     */
    std::shared_ptr<WorkflowNode> End();
    
    /**
     * @brief AddPassthroughNode adds a passthrough node
     * Aligns with: eino/compose/workflow.go:139-142
     */
    std::shared_ptr<WorkflowNode> AddPassthroughNode(
        const std::string& key,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AddBranch adds a branch to the workflow
     * Aligns with: eino/compose/workflow.go:421-433
     */
    std::shared_ptr<WorkflowBranch> AddBranch(
        const std::string& from_node_key,
        std::shared_ptr<GraphBranch> branch);
    
    // ========================================================================
    // Compilation and Execution
    // ========================================================================
    
    /**
     * @brief Compile prepares the workflow for execution
     * Aligns with: eino/compose/workflow.go:448-493
     */
    void Compile(std::shared_ptr<Context> ctx);
    
    /**
     * @brief IsCompiled returns whether the workflow is compiled
     */
    bool IsCompiled() const;
    
    // ========================================================================
    // Runnable Interface Implementation
    // ========================================================================
    
    O Invoke(
        std::shared_ptr<Context> ctx,
        const I& input, 
        const std::vector<Option>& opts = std::vector<Option>()) override;
    
    std::shared_ptr<StreamReader<O>> Stream(
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts = std::vector<Option>()) override;
    
    O Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override;
    
    std::shared_ptr<StreamReader<O>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override;
    
    const std::type_info& GetInputType() const override {
        return typeid(I);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(O);
    }
    
    std::string GetComponentType() const override {
        return "Workflow";
    }
    
    // ========================================================================
    // Query Methods
    // ========================================================================
    
    size_t GetStepCount() const {
        return workflow_nodes_.size();
    }
    
    const std::map<std::string, std::shared_ptr<WorkflowNode>>& GetNodes() const {
        return workflow_nodes_;
    }
    
    const std::map<std::string, std::map<std::string, DependencyType>>& GetDependencies() const {
        return dependencies_;
    }
    
private:
    /**
     * @brief initNode initializes a WorkflowNode
     * Aligns with: eino/compose/workflow.go:495-510
     */
    std::shared_ptr<WorkflowNode> initNode(const std::string& key);
    
    // Internal Graph for execution
    // Aligns with: eino/compose/workflow.go:40 g *graph
    std::shared_ptr<Graph<I, O>> g_;
    
    // Workflow nodes
    // Aligns with: eino/compose/workflow.go:42 workflowNodes map[string]*WorkflowNode
    std::map<std::string, std::shared_ptr<WorkflowNode>> workflow_nodes_;
    
    // Workflow branches
    // Aligns with: eino/compose/workflow.go:43 workflowBranches []*WorkflowBranch
    std::vector<std::shared_ptr<WorkflowBranch>> workflow_branches_;
    
    // Dependencies tracking
    // Aligns with: eino/compose/workflow.go:44 dependencies map[string]map[string]dependencyType
    std::map<std::string, std::map<std::string, DependencyType>> dependencies_;
    
    bool is_compiled_;
};

// ============================================================================
// Factory Function
// ============================================================================

/**
 * @brief NewWorkflow creates a new Workflow instance
 * Aligns with: eino/compose/workflow.go:60-75
 */
template<typename I, typename O>
std::shared_ptr<Workflow<I, O>> NewWorkflow() {
    return std::make_shared<Workflow<I, O>>();
}

// ============================================================================
// Template Implementation
// ============================================================================

template<typename I, typename O>
Workflow<I, O>::Workflow()
    : is_compiled_(false) {
    // Create internal graph
    // Aligns with: eino/compose/workflow.go:60-75
    g_ = std::make_shared<Graph<I, O>>();
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::AddChatModelNode(
    const std::string& key,
    std::shared_ptr<Runnable<json, json>> chat_model,
    const std::vector<Option>& opts) {
    // Aligns with: eino/compose/workflow.go:81-84
    g_->AddNode(key, chat_model, opts);
    return initNode(key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::AddChatTemplateNode(
    const std::string& key,
    std::shared_ptr<Runnable<json, json>> chat_template,
    const std::vector<Option>& opts) {
    g_->AddNode(key, chat_template, opts);
    return initNode(key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::AddToolsNode(
    const std::string& key,
    std::shared_ptr<Runnable<json, json>> tools_node,
    const std::vector<Option>& opts) {
    g_->AddNode(key, tools_node, opts);
    return initNode(key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::AddRetrieverNode(
    const std::string& key,
    std::shared_ptr<Runnable<json, json>> retriever,
    const std::vector<Option>& opts) {
    g_->AddNode(key, retriever, opts);
    return initNode(key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::AddEmbeddingNode(
    const std::string& key,
    std::shared_ptr<Runnable<json, json>> embedding,
    const std::vector<Option>& opts) {
    g_->AddNode(key, embedding, opts);
    return initNode(key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::AddIndexerNode(
    const std::string& key,
    std::shared_ptr<Runnable<json, json>> indexer,
    const std::vector<Option>& opts) {
    g_->AddNode(key, indexer, opts);
    return initNode(key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::AddLoaderNode(
    const std::string& key,
    std::shared_ptr<Runnable<json, json>> loader,
    const std::vector<Option>& opts) {
    g_->AddNode(key, loader, opts);
    return initNode(key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::AddDocumentTransformerNode(
    const std::string& key,
    std::shared_ptr<Runnable<json, json>> transformer,
    const std::vector<Option>& opts) {
    g_->AddNode(key, transformer, opts);
    return initNode(key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::AddGraphNode(
    const std::string& key,
    std::shared_ptr<Runnable<json, json>> graph,
    const std::vector<Option>& opts) {
    g_->AddNode(key, graph, opts);
    return initNode(key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::AddLambdaNode(
    const std::string& key,
    std::shared_ptr<Runnable<json, json>> lambda,
    const std::vector<Option>& opts) {
    g_->AddNode(key, lambda, opts);
    return initNode(key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::End() {
    // Aligns with: eino/compose/workflow.go:132-137
    const std::string end_key = "END";
    if (workflow_nodes_.find(end_key) != workflow_nodes_.end()) {
        return workflow_nodes_[end_key];
    }
    return initNode(end_key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::AddPassthroughNode(
    const std::string& key,
    const std::vector<Option>& opts) {
    // Aligns with: eino/compose/workflow.go:139-142
    g_->AddPassthrough(key, opts);
    return initNode(key);
}

template<typename I, typename O>
std::shared_ptr<WorkflowBranch> Workflow<I, O>::AddBranch(
    const std::string& from_node_key,
    std::shared_ptr<GraphBranch> branch) {
    // Aligns with: eino/compose/workflow.go:421-433
    auto wb = std::make_shared<WorkflowBranch>(from_node_key, branch);
    workflow_branches_.push_back(wb);
    return wb;
}

template<typename I, typename O>
void Workflow<I, O>::Compile(std::shared_ptr<Context> ctx) {
    // Aligns with: eino/compose/workflow.go:448-493
    if (is_compiled_) {
        return;
    }
    
    // Process workflow branches
    for (const auto& wb : workflow_branches_) {
        const auto& branch = wb->GetBranch();
        const auto& from_key = wb->GetFromNodeKey();
        
        // Set up branch dependencies
        for (const auto& end_pair : branch->GetEndNodes()) {
            const auto& end_node = end_pair.first;
            
            if (end_node == "END") {
                if (dependencies_.find("END") == dependencies_.end()) {
                    dependencies_["END"] = {};
                }
                dependencies_["END"][from_key] = DependencyType::Branch;
            } else {
                auto it = workflow_nodes_.find(end_node);
                if (it != workflow_nodes_.end()) {
                    if (dependencies_.find(end_node) == dependencies_.end()) {
                        dependencies_[end_node] = {};
                    }
                    dependencies_[end_node][from_key] = DependencyType::Branch;
                }
            }
        }
        
        // Add branch to graph
        g_->AddBranch(from_key, branch);
    }
    
    // Process node inputs and create edges
    for (auto& node_pair : workflow_nodes_) {
        auto& node = node_pair.second;
        const auto& inputs = node->GetAddInputs();
        
        for (const auto& input_info : inputs) {
            DependencyType dep_type = DependencyType::Normal;
            bool no_direct_dep = input_info.options.no_direct_dependency;
            bool dep_without_input = input_info.options.dependency_without_input;
            
            if (no_direct_dep) {
                dep_type = DependencyType::NoDirectDependency;
            } else if (dep_without_input) {
                dep_type = DependencyType::Normal;
                // Add edge without data mapping
                g_->AddEdge(input_info.from_node_key, node->GetKey());
            } else {
                dep_type = DependencyType::Normal;
                // Add edge with data mapping
                g_->AddEdge(input_info.from_node_key, node->GetKey());
                // TODO: Add field mappings to graph
            }
            
            // Track dependency
            if (dependencies_.find(node->GetKey()) == dependencies_.end()) {
                dependencies_[node->GetKey()] = {};
            }
            dependencies_[node->GetKey()][input_info.from_node_key] = dep_type;
        }
        
        // Process static values
        const auto& static_vals = node->GetStaticValues();
        if (!static_vals.empty()) {
            // TODO: Add static value handling to graph
        }
        
        // Clear add_inputs after processing
        node->ClearAddInputs();
    }
    
    // Compile internal graph
    g_->Compile(ctx);
    
    is_compiled_ = true;
}

template<typename I, typename O>
bool Workflow<I, O>::IsCompiled() const {
    return is_compiled_;
}

template<typename I, typename O>
O Workflow<I, O>::Invoke(
    std::shared_ptr<Context> ctx,
    const I& input,
    const std::vector<Option>& opts) {
    
    if (!is_compiled_) {
        throw std::runtime_error("Workflow not compiled, call Compile() first");
    }
    
    // Delegate to internal graph
    return g_->Invoke(ctx, input, opts);
}

template<typename I, typename O>
std::shared_ptr<StreamReader<O>> Workflow<I, O>::Stream(
    std::shared_ptr<Context> ctx,
    const I& input,
    const std::vector<Option>& opts) {
    
    if (!is_compiled_) {
        throw std::runtime_error("Workflow not compiled, call Compile() first");
    }
    
    return g_->Stream(ctx, input, opts);
}

template<typename I, typename O>
O Workflow<I, O>::Collect(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<StreamReader<I>> input,
    const std::vector<Option>& opts) {
    
    if (!is_compiled_) {
        throw std::runtime_error("Workflow not compiled, call Compile() first");
    }
    
    return g_->Collect(ctx, input, opts);
}

template<typename I, typename O>
std::shared_ptr<StreamReader<O>> Workflow<I, O>::Transform(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<StreamReader<I>> input,
    const std::vector<Option>& opts) {
    
    if (!is_compiled_) {
        throw std::runtime_error("Workflow not compiled, call Compile() first");
    }
    
    return g_->Transform(ctx, input, opts);
}

template<typename I, typename O>
std::shared_ptr<WorkflowNode> Workflow<I, O>::initNode(const std::string& key) {
    // Aligns with: eino/compose/workflow.go:495-510
    auto node = std::make_shared<WorkflowNode>(key);
    workflow_nodes_[key] = node;
    return node;
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_WORKFLOW_H_
