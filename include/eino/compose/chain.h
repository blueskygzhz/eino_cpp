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

#ifndef EINO_CPP_COMPOSE_CHAIN_H_
#define EINO_CPP_COMPOSE_CHAIN_H_

#include <memory>
#include <vector>
#include <string>
#include <typeinfo>
#include <stdexcept>
#include "runnable.h"
#include "graph_call_options.h"

namespace eino {
namespace compose {

// Forward declarations
class Parallel;
class ChainBranch;
template<typename I, typename O> class Graph;

/**
 * @brief ChainNodeType represents the type of node in a chain
 * 
 * Aligned with: eino/compose/chain.go node types
 */
enum class ChainNodeType {
    Unknown,
    ChatModel,
    ChatTemplate,
    ToolsNode,
    Lambda,
    Embedding,
    Retriever,
    Loader,
    Indexer,
    DocumentTransformer,
    Graph,
    Passthrough,
    Parallel,
    Branch
};

/**
 * @brief Convert ChainNodeType to string
 */
std::string ChainNodeTypeToString(ChainNodeType type);

/**
 * @brief Convert string to ChainNodeType
 */
ChainNodeType StringToChainNodeType(const std::string& str);

/**
 * @brief ChainNodeInfo stores metadata about a node in the chain
 */
struct ChainNodeInfo {
    ChainNodeType node_type;
    std::string node_key;
    std::shared_ptr<void> node;  // Type-erased node pointer
    std::vector<std::string> options;
    
    ChainNodeInfo();
    ChainNodeInfo(ChainNodeType type, 
                  const std::string& key,
                  std::shared_ptr<void> node_ptr);
};

/**
 * @brief ChainBase provides common functionality for all chain types
 * 
 * This is a non-template base class that handles chain state management,
 * error tracking, and node key generation.
 */
class ChainBase {
public:
    ChainBase();
    virtual ~ChainBase();
    
    /**
     * @brief Compile prepares the chain for execution
     */
    virtual void Compile();
    
    /**
     * @brief IsCompiled returns whether the chain is compiled
     */
    bool IsCompiled() const;
    
    /**
     * @brief HasError returns whether the chain has errors
     */
    bool HasError() const;
    
    /**
     * @brief GetError returns the error message
     */
    const std::string& GetError() const;
    
    /**
     * @brief ReportError records an error (only the first one is kept)
     */
    void ReportError(const std::string& err);
    
    /**
     * @brief NextNodeKey generates the next node key
     */
    std::string NextNodeKey();
    
    /**
     * @brief GetPreNodeKeys returns the previous node keys
     */
    const std::vector<std::string>& GetPreNodeKeys() const;
    
    /**
     * @brief SetPreNodeKeys sets the previous node keys
     */
    void SetPreNodeKeys(const std::vector<std::string>& keys);
    
    /**
     * @brief AddPreNodeKey adds a previous node key
     */
    void AddPreNodeKey(const std::string& key);
    
    /**
     * @brief ClearPreNodeKeys clears all previous node keys
     */
    void ClearPreNodeKeys();
    
    /**
     * @brief GetNodeCount returns the number of nodes added
     */
    size_t GetNodeCount() const;
    
protected:
    /**
     * @brief ValidateNotCompiled throws if chain is already compiled
     */
    void ValidateNotCompiled() const;
    
    /**
     * @brief CheckError throws if chain has errors
     */
    void CheckError() const;
    
    bool is_compiled_;
    bool has_end_;
    std::string error_;
    int node_idx_;
    std::vector<std::string> pre_node_keys_;
};

/**
 * @brief SequentialChainBase provides base functionality for sequential chains
 * 
 * Sequential chains execute nodes one after another in a linear fashion.
 */
class SequentialChainBase : public ChainBase {
public:
    SequentialChainBase();
    virtual ~SequentialChainBase();
    
    /**
     * @brief AddNodeInfo adds a node to the chain
     */
    void AddNodeInfo(const ChainNodeInfo& info);
    
    /**
     * @brief GetNodes returns all nodes in the chain
     */
    const std::vector<ChainNodeInfo>& GetNodes() const;
    
    /**
     * @brief Compile prepares the chain for execution
     */
    void Compile() override;
    
protected:
    std::vector<ChainNodeInfo> nodes_;
};

/**
 * @brief Chain represents a linear chain of runnables
 * 
 * Chain is the main abstraction for building sequential pipelines.
 * All components must be compatible (output of one = input of next).
 *
 * Aligned with: eino/compose/chain.go:33-47 Chain[I, O]
 * 
 * Key Design:
 * - Chain wraps an internal Graph[I, O] (gg_)
 * - Builder pattern: AppendXXX methods add nodes to graph
 * - Compile() builds the graph and creates __END__ node
 * - Execution delegates to the internal graph
 * 
 * Usage:
 *   auto chain = NewChain<InputType, OutputType>();
 *   chain->AppendChatTemplate(template);
 *   chain->AppendChatModel(model);
 *   chain->AppendLambda(lambda);
 *   chain->Compile(ctx);
 *   auto output = chain->Invoke(ctx, input);
 */
template<typename I, typename O>
class Chain : public ComposableRunnable<I, O> {
public:
    /**
     * @brief Constructor - creates internal graph
     * Aligns with: eino/compose/chain.go:90-95
     */
    Chain();
    
    virtual ~Chain() = default;
    
    // ========================================================================
    // Builder Methods - Aligns with: eino/compose/chain.go:97-314
    // ========================================================================
    
    /**
     * @brief AppendChatTemplate adds a ChatTemplate node
     * Aligns with: eino/compose/chain.go:97-106
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendChatTemplate(
        std::shared_ptr<Runnable<M, M>> chat_template,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendChatModel adds a ChatModel node
     * Aligns with: eino/compose/chain.go:108-117
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendChatModel(
        std::shared_ptr<Runnable<M, M>> chat_model,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendToolsNode adds a ToolsNode
     * Aligns with: eino/compose/chain.go:119-128
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendToolsNode(
        std::shared_ptr<Runnable<M, M>> tools_node,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendLambda adds a Lambda node
     * Aligns with: eino/compose/chain.go:130-139
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendLambda(
        std::shared_ptr<Runnable<M, M>> lambda,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendRetriever adds a Retriever node
     * Aligns with: eino/compose/chain.go:141-150
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendRetriever(
        std::shared_ptr<Runnable<M, M>> retriever,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendEmbedding adds an Embedding node
     * Aligns with: eino/compose/chain.go:152-161
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendEmbedding(
        std::shared_ptr<Runnable<M, M>> embedding,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendDocumentTransformer adds a DocumentTransformer node
     * Aligns with: eino/compose/chain.go:163-172
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendDocumentTransformer(
        std::shared_ptr<Runnable<M, M>> transformer,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendIndexer adds an Indexer node
     * Aligns with: eino/compose/chain.go:174-183
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendIndexer(
        std::shared_ptr<Runnable<M, M>> indexer,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendLoader adds a Loader node
     * Aligns with: eino/compose/chain.go:185-194
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendLoader(
        std::shared_ptr<Runnable<M, M>> loader,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendGraph adds a nested Graph node
     * Aligns with: eino/compose/chain.go:196-205
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendGraph(
        std::shared_ptr<Runnable<M, M>> graph,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendPassthrough adds a Passthrough node
     * Aligns with: eino/compose/chain.go:207-216
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendPassthrough(
        std::shared_ptr<Runnable<M, M>> passthrough,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendParallel adds a Parallel node
     * Aligns with: eino/compose/chain.go:218-240
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendParallel(
        std::shared_ptr<Parallel> parallel,
        const std::vector<Option>& opts = {});
    
    /**
     * @brief AppendBranch adds a conditional Branch node
     * Aligns with: eino/compose/chain.go:242-314
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> AppendBranch(
        std::shared_ptr<ChainBranch> branch,
        const std::vector<Option>& opts = {});
    
    // ========================================================================
    // Compilation and Execution - Aligns with: eino/compose/chain.go:316-395
    // ========================================================================
    
    /**
     * @brief Compile prepares the chain for execution
     * Aligns with: eino/compose/chain.go:316-341
     * 
     * Process:
     * 1. Check if already compiled
     * 2. Add __END__ node if needed
     * 3. Compile internal graph
     */
    void Compile(std::shared_ptr<Context> ctx);
    
    /**
     * @brief IsCompiled returns whether the chain is compiled
     */
    bool IsCompiled() const;
    
    /**
     * @brief HasError returns whether the chain has errors
     */
    bool HasError() const;
    
    /**
     * @brief GetError returns the error message
     */
    const std::string& GetError() const;
    
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
        return "Chain";
    }
    
private:
    /**
     * @brief appendNode is the core method for adding nodes
     * Aligns with: eino/compose/chain.go:343-395
     */
    template<typename M>
    std::shared_ptr<Chain<I, O>> appendNode(
        ChainNodeType node_type,
        std::shared_ptr<Runnable<M, M>> node,
        const std::vector<Option>& opts);
    
    /**
     * @brief addEndIfNeeded adds __END__ node to complete the chain
     * Aligns with: eino/compose/chain.go:397-418
     */
    void addEndIfNeeded();
    
    /**
     * @brief NextNodeKey generates the next unique node key
     */
    std::string NextNodeKey();
    
    /**
     * @brief ValidateNotCompiled throws if already compiled
     */
    void ValidateNotCompiled() const;
    
    // Internal graph that executes the chain
    // Aligns with: eino/compose/chain.go:33 gg *Graph[I, O]
    std::shared_ptr<Graph<I, O>> gg_;
    
    // Chain state
    bool is_compiled_;
    bool has_end_;
    std::string error_;
    int node_idx_;
    std::vector<std::string> pre_node_keys_;
};

// SimpleChain is a 2-step chain for common use cases
// Chains two runnables with compatible types
template<typename I, typename M, typename O>
class SimpleChain : public Chain<I, O> {
public:
    SimpleChain(std::shared_ptr<Runnable<I, M>> first,
                std::shared_ptr<Runnable<M, O>> second)
        : first_(first), second_(second) {
        if (!first || !second) {
            throw std::runtime_error("Chain steps cannot be null");
        }
    }
    
    virtual ~SimpleChain() = default;
    
    O Invoke(
        std::shared_ptr<Context> ctx,
        const I& input, 
        const std::vector<Option>& opts = std::vector<Option>()) override {
        if (!this->is_compiled_) {
            throw std::runtime_error("SimpleChain not compiled");
        }
        M intermediate = first_->Invoke(ctx, input, opts);
        return second_->Invoke(ctx, intermediate, opts);
    }
    
    std::shared_ptr<StreamReader<O>> Stream(
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        if (!this->is_compiled_) {
            throw std::runtime_error("SimpleChain not compiled");
        }
        auto stream = first_->Stream(ctx, input, opts);
        return second_->Transform(ctx, stream, opts);
    }
    
    O Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        if (!this->is_compiled_) {
            throw std::runtime_error("SimpleChain not compiled");
        }
        M intermediate = first_->Collect(ctx, input, opts);
        return second_->Invoke(ctx, intermediate, opts);
    }
    
    std::shared_ptr<StreamReader<O>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        if (!this->is_compiled_) {
            throw std::runtime_error("SimpleChain not compiled");
        }
        auto stream = first_->Transform(ctx, input, opts);
        return second_->Transform(ctx, stream, opts);
    }
    
private:
    std::shared_ptr<Runnable<I, M>> first_;
    std::shared_ptr<Runnable<M, O>> second_;
};

// ============================================================================
// Factory Functions
// ============================================================================

/**
 * @brief NewChain creates a new Chain instance
 * Aligns with: eino/compose/chain.go:56-59
 */
template<typename I, typename O>
std::shared_ptr<Chain<I, O>> NewChain() {
    return std::make_shared<Chain<I, O>>();
}

/**
 * @brief NewSimpleChain creates a 2-step chain (legacy compatibility)
 */
template<typename I, typename M, typename O>
std::shared_ptr<SimpleChain<I, M, O>> NewSimpleChain(
    std::shared_ptr<Runnable<I, M>> first,
    std::shared_ptr<Runnable<M, O>> second) {
    return std::make_shared<SimpleChain<I, M, O>>(first, second);
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_CHAIN_H_
