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

#ifndef EINO_CPP_COMPOSE_CHAIN_BRANCH_H_
#define EINO_CPP_COMPOSE_CHAIN_BRANCH_H_

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include "runnable.h"
#include "graph_call_options.h"

namespace eino {
namespace compose {

/**
 * @brief ChainBranch represents a conditional branch within a chain
 * 
 * Aligns with: eino/compose/chain_branch.go:29-39
 * 
 * ChainBranch allows dynamic routing based on a condition function.
 * All branches must either end the chain or converge to another node.
 * 
 * Usage:
 *   auto branch = NewChainBranch<Message>();
 *   branch->SetCondition([](Context ctx, Message msg) -> string {
 *       return msg.role == "user" ? "user_path" : "default_path";
 *   });
 *   branch->AddChatModel("user_path", user_model);
 *   branch->AddLambda("default_path", default_lambda);
 */
template<typename T>
class ChainBranch : public ComposableRunnable<T, T> {
public:
    /**
     * @brief Condition function type
     * Returns branch key to route to (or empty for default/end)
     * Aligns with: eino/compose/chain_branch.go:42-43
     */
    using ConditionFunc = std::function<std::string(std::shared_ptr<Context>, const T&)>;
    
    ChainBranch() = default;
    virtual ~ChainBranch() = default;
    
    // ========================================================================
    // Configuration Methods
    // ========================================================================
    
    /**
     * @brief SetCondition sets the routing condition function
     * Aligns with: eino/compose/chain_branch.go:45-51
     */
    std::shared_ptr<ChainBranch<T>> SetCondition(ConditionFunc condition) {
        if (!error_.empty()) {
            return std::shared_ptr<ChainBranch<T>>(this);
        }
        
        if (is_compiled_) {
            error_ = "ChainBranch already compiled, cannot set condition";
            return std::shared_ptr<ChainBranch<T>>(this);
        }
        
        condition_ = condition;
        return std::shared_ptr<ChainBranch<T>>(this);
    }
    
    // ========================================================================
    // Builder Methods - Add Nodes to Branch
    // Aligns with: eino/compose/chain_branch.go:53-241
    // ========================================================================
    
    /**
     * @brief AddChatTemplate adds a ChatTemplate node
     * Aligns with: eino/compose/chain_branch.go:53-71
     */
    std::shared_ptr<ChainBranch<T>> AddChatTemplate(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> chat_template) {
        return addNode(key, chat_template, "ChatTemplate");
    }
    
    /**
     * @brief AddChatModel adds a ChatModel node
     * Aligns with: eino/compose/chain_branch.go:73-91
     */
    std::shared_ptr<ChainBranch<T>> AddChatModel(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> chat_model) {
        return addNode(key, chat_model, "ChatModel");
    }
    
    /**
     * @brief AddToolsNode adds a ToolsNode
     * Aligns with: eino/compose/chain_branch.go:93-111
     */
    std::shared_ptr<ChainBranch<T>> AddToolsNode(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> tools_node) {
        return addNode(key, tools_node, "ToolsNode");
    }
    
    /**
     * @brief AddLambda adds a Lambda node
     * Aligns with: eino/compose/chain_branch.go:113-131
     */
    std::shared_ptr<ChainBranch<T>> AddLambda(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> lambda) {
        return addNode(key, lambda, "Lambda");
    }
    
    /**
     * @brief AddRetriever adds a Retriever node
     * Aligns with: eino/compose/chain_branch.go:133-151
     */
    std::shared_ptr<ChainBranch<T>> AddRetriever(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> retriever) {
        return addNode(key, retriever, "Retriever");
    }
    
    /**
     * @brief AddEmbedding adds an Embedding node
     * Aligns with: eino/compose/chain_branch.go:153-171
     */
    std::shared_ptr<ChainBranch<T>> AddEmbedding(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> embedding) {
        return addNode(key, embedding, "Embedding");
    }
    
    /**
     * @brief AddDocumentTransformer adds a DocumentTransformer node
     * Aligns with: eino/compose/chain_branch.go:173-191
     */
    std::shared_ptr<ChainBranch<T>> AddDocumentTransformer(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> transformer) {
        return addNode(key, transformer, "DocumentTransformer");
    }
    
    /**
     * @brief AddIndexer adds an Indexer node
     * Aligns with: eino/compose/chain_branch.go:193-211
     */
    std::shared_ptr<ChainBranch<T>> AddIndexer(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> indexer) {
        return addNode(key, indexer, "Indexer");
    }
    
    /**
     * @brief AddLoader adds a Loader node
     * Aligns with: eino/compose/chain_branch.go:213-231
     */
    std::shared_ptr<ChainBranch<T>> AddLoader(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> loader) {
        return addNode(key, loader, "Loader");
    }
    
    /**
     * @brief AddGraph adds a nested Graph node
     * Aligns with: eino/compose/chain_branch.go:233-251
     */
    std::shared_ptr<ChainBranch<T>> AddGraph(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> graph) {
        return addNode(key, graph, "Graph");
    }
    
    /**
     * @brief AddPassthrough adds a Passthrough node
     * Aligns with: eino/compose/chain_branch.go:253-271
     */
    std::shared_ptr<ChainBranch<T>> AddPassthrough(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> passthrough) {
        return addNode(key, passthrough, "Passthrough");
    }
    
    /**
     * @brief AddParallel adds a Parallel node
     * Aligns with: eino/compose/chain_branch.go:273-291
     */
    std::shared_ptr<ChainBranch<T>> AddParallel(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> parallel) {
        return addNode(key, parallel, "Parallel");
    }
    
    /**
     * @brief AddBranch adds a nested Branch node
     * Aligns with: eino/compose/chain_branch.go:293-311
     */
    std::shared_ptr<ChainBranch<T>> AddBranch(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> branch) {
        return addNode(key, branch, "Branch");
    }
    
    // ========================================================================
    // Compilation and Execution
    // ========================================================================
    
    /**
     * @brief Compile prepares the branch for execution
     */
    void Compile() {
        if (is_compiled_) {
            return;
        }
        
        if (!error_.empty()) {
            throw std::runtime_error("ChainBranch has error: " + error_);
        }
        
        if (!condition_) {
            error_ = "ChainBranch: condition function not set";
            throw std::runtime_error(error_);
        }
        
        if (branch_nodes_.empty()) {
            error_ = "ChainBranch: no branch nodes added";
            throw std::runtime_error(error_);
        }
        
        is_compiled_ = true;
    }
    
    /**
     * @brief Invoke executes the branch logic
     * Routes to appropriate branch based on condition
     */
    T Invoke(
        std::shared_ptr<Context> ctx,
        const T& input,
        const std::vector<Option>& opts = {}) override {
        
        if (!is_compiled_) {
            throw std::runtime_error("ChainBranch not compiled");
        }
        
        // Evaluate condition to get branch key
        std::string branch_key = condition_(ctx, input);
        
        // Find and execute branch
        auto it = branch_nodes_.find(branch_key);
        if (it == branch_nodes_.end()) {
            throw std::runtime_error("ChainBranch: invalid branch key '" + branch_key + "'");
        }
        
        return it->second->Invoke(ctx, input, opts);
    }
    
    std::shared_ptr<StreamReader<T>> Stream(
        std::shared_ptr<Context> ctx,
        const T& input,
        const std::vector<Option>& opts = {}) override {
        
        if (!is_compiled_) {
            throw std::runtime_error("ChainBranch not compiled");
        }
        
        std::string branch_key = condition_(ctx, input);
        auto it = branch_nodes_.find(branch_key);
        if (it == branch_nodes_.end()) {
            throw std::runtime_error("ChainBranch: invalid branch key '" + branch_key + "'");
        }
        
        return it->second->Stream(ctx, input, opts);
    }
    
    T Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<T>> input,
        const std::vector<Option>& opts = {}) override {
        throw std::runtime_error("ChainBranch::Collect not implemented");
    }
    
    std::shared_ptr<StreamReader<T>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<T>> input,
        const std::vector<Option>& opts = {}) override {
        throw std::runtime_error("ChainBranch::Transform not implemented");
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(T);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(T);
    }
    
    std::string GetComponentType() const override {
        return "ChainBranch";
    }
    
    // ========================================================================
    // Query Methods
    // ========================================================================
    
    /**
     * @brief GetBranchNodes returns all nodes in this branch
     */
    const std::map<std::string, std::shared_ptr<Runnable<T, T>>>& GetBranchNodes() const {
        return branch_nodes_;
    }
    
    /**
     * @brief HasError returns true if there was an error during construction
     */
    bool HasError() const {
        return !error_.empty();
    }
    
    /**
     * @brief GetError returns the error message
     */
    const std::string& GetError() const {
        return error_;
    }
    
    /**
     * @brief Validate checks if the branch configuration is valid
     */
    bool Validate() {
        if (!error_.empty()) {
            return false;
        }
        
        return !branch_nodes_.empty() && condition_;
    }
    
private:
    /**
     * @brief addNode is the core method for adding nodes
     * Aligns with: eino/compose/chain_branch.go addNode pattern
     */
    std::shared_ptr<ChainBranch<T>> addNode(
        const std::string& key,
        std::shared_ptr<Runnable<T, T>> node,
        const std::string& node_type) {
        
        if (!error_.empty()) {
            return std::shared_ptr<ChainBranch<T>>(this);
        }
        
        if (is_compiled_) {
            error_ = "ChainBranch already compiled, cannot add nodes";
            return std::shared_ptr<ChainBranch<T>>(this);
        }
        
        if (key.empty()) {
            error_ = "Branch key cannot be empty";
            return std::shared_ptr<ChainBranch<T>>(this);
        }
        
        if (!node) {
            error_ = node_type + " node cannot be null";
            return std::shared_ptr<ChainBranch<T>>(this);
        }
        
        if (branch_nodes_.find(key) != branch_nodes_.end()) {
            error_ = "Duplicate branch node key: " + key;
            return std::shared_ptr<ChainBranch<T>>(this);
        }
        
        branch_nodes_[key] = node;
        return std::shared_ptr<ChainBranch<T>>(this);
    }
    
    ConditionFunc condition_;
    std::map<std::string, std::shared_ptr<Runnable<T, T>>> branch_nodes_;
    std::string error_;
    bool is_compiled_ = false;
};

// Helper functions for creating ChainBranch instances
template<typename T>
std::shared_ptr<ChainBranch<T>> NewChainBranch() {
    return std::make_shared<ChainBranch<T>>();
}

template<typename T>
std::shared_ptr<ChainBranch<T>> NewChainMultiBranch() {
    return std::make_shared<ChainBranch<T>>();
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_CHAIN_BRANCH_H_
