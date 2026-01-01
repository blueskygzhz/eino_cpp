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

#include "eino/compose/chain.h"
#include "eino/compose/graph.h"
#include <sstream>
#include <algorithm>

namespace eino {
namespace compose {

// ChainBase implementation

ChainBase::ChainBase()
    : is_compiled_(false),
      has_end_(false),
      node_idx_(0) {}

ChainBase::~ChainBase() = default;

void ChainBase::Compile() {
    if (is_compiled_) {
        return;
    }
    
    // Add END edges if needed
    if (!has_end_ && !pre_node_keys_.empty()) {
        has_end_ = true;
    }
    
    is_compiled_ = true;
}

bool ChainBase::IsCompiled() const {
    return is_compiled_;
}

bool ChainBase::HasError() const {
    return !error_.empty();
}

const std::string& ChainBase::GetError() const {
    return error_;
}

void ChainBase::ReportError(const std::string& err) {
    if (error_.empty()) {
        error_ = err;
    }
}

std::string ChainBase::NextNodeKey() {
    std::stringstream ss;
    ss << "node_" << node_idx_;
    node_idx_++;
    return ss.str();
}

const std::vector<std::string>& ChainBase::GetPreNodeKeys() const {
    return pre_node_keys_;
}

void ChainBase::SetPreNodeKeys(const std::vector<std::string>& keys) {
    pre_node_keys_ = keys;
}

void ChainBase::AddPreNodeKey(const std::string& key) {
    pre_node_keys_.push_back(key);
}

void ChainBase::ClearPreNodeKeys() {
    pre_node_keys_.clear();
}

size_t ChainBase::GetNodeCount() const {
    return node_idx_;
}

void ChainBase::ValidateNotCompiled() const {
    if (is_compiled_) {
        throw std::runtime_error("Chain has been compiled, cannot be modified");
    }
}

void ChainBase::CheckError() const {
    if (!error_.empty()) {
        throw std::runtime_error("Chain error: " + error_);
    }
}

// ChainNodeInfo implementation

ChainNodeInfo::ChainNodeInfo()
    : node_type(ChainNodeType::Unknown) {}

ChainNodeInfo::ChainNodeInfo(
    ChainNodeType type,
    const std::string& key,
    std::shared_ptr<void> node_ptr)
    : node_type(type),
      node_key(key),
      node(node_ptr) {}

// SequentialChain implementation

SequentialChainBase::SequentialChainBase()
    : ChainBase() {}

SequentialChainBase::~SequentialChainBase() = default;

void SequentialChainBase::AddNodeInfo(const ChainNodeInfo& info) {
    ValidateNotCompiled();
    
    if (HasError()) {
        return;
    }
    
    nodes_.push_back(info);
    
    // Update pre_node_keys for next node
    pre_node_keys_.clear();
    pre_node_keys_.push_back(info.node_key);
}

const std::vector<ChainNodeInfo>& SequentialChainBase::GetNodes() const {
    return nodes_;
}

void SequentialChainBase::Compile() {
    if (is_compiled_) {
        return;
    }
    
    if (nodes_.empty()) {
        ReportError("SequentialChain: no nodes added");
        return;
    }
    
    ChainBase::Compile();
}

// Utility functions

std::string ChainNodeTypeToString(ChainNodeType type) {
    switch (type) {
        case ChainNodeType::ChatModel:
            return "ChatModel";
        case ChainNodeType::ChatTemplate:
            return "ChatTemplate";
        case ChainNodeType::ToolsNode:
            return "ToolsNode";
        case ChainNodeType::Lambda:
            return "Lambda";
        case ChainNodeType::Embedding:
            return "Embedding";
        case ChainNodeType::Retriever:
            return "Retriever";
        case ChainNodeType::Loader:
            return "Loader";
        case ChainNodeType::Indexer:
            return "Indexer";
        case ChainNodeType::DocumentTransformer:
            return "DocumentTransformer";
        case ChainNodeType::Graph:
            return "Graph";
        case ChainNodeType::Passthrough:
            return "Passthrough";
        case ChainNodeType::Parallel:
            return "Parallel";
        case ChainNodeType::Branch:
            return "Branch";
        default:
            return "Unknown";
    }
}

ChainNodeType StringToChainNodeType(const std::string& str) {
    if (str == "ChatModel") return ChainNodeType::ChatModel;
    if (str == "ChatTemplate") return ChainNodeType::ChatTemplate;
    if (str == "ToolsNode") return ChainNodeType::ToolsNode;
    if (str == "Lambda") return ChainNodeType::Lambda;
    if (str == "Embedding") return ChainNodeType::Embedding;
    if (str == "Retriever") return ChainNodeType::Retriever;
    if (str == "Loader") return ChainNodeType::Loader;
    if (str == "Indexer") return ChainNodeType::Indexer;
    if (str == "DocumentTransformer") return ChainNodeType::DocumentTransformer;
    if (str == "Graph") return ChainNodeType::Graph;
    if (str == "Passthrough") return ChainNodeType::Passthrough;
    if (str == "Parallel") return ChainNodeType::Parallel;
    if (str == "Branch") return ChainNodeType::Branch;
    return ChainNodeType::Unknown;
}

// ============================================================================
// Chain Template Implementation
// Aligns with: eino/compose/chain.go
// ============================================================================

template<typename I, typename O>
Chain<I, O>::Chain()
    : is_compiled_(false),
      has_end_(false),
      node_idx_(0) {
    // Create internal graph
    // Aligns with: eino/compose/chain.go:90-95
    gg_ = std::make_shared<Graph<I, O>>();
}

template<typename I, typename O>
std::string Chain<I, O>::NextNodeKey() {
    std::stringstream ss;
    ss << "node_" << node_idx_;
    node_idx_++;
    return ss.str();
}

template<typename I, typename O>
void Chain<I, O>::ValidateNotCompiled() const {
    if (is_compiled_) {
        throw std::runtime_error("Chain has been compiled, cannot be modified");
    }
}

template<typename I, typename O>
bool Chain<I, O>::IsCompiled() const {
    return is_compiled_;
}

template<typename I, typename O>
bool Chain<I, O>::HasError() const {
    return !error_.empty();
}

template<typename I, typename O>
const std::string& Chain<I, O>::GetError() const {
    return error_;
}

// ============================================================================
// Builder Methods Implementation
// ============================================================================

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendChatTemplate(
    std::shared_ptr<Runnable<M, M>> chat_template,
    const std::vector<Option>& opts) {
    return appendNode(ChainNodeType::ChatTemplate, chat_template, opts);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendChatModel(
    std::shared_ptr<Runnable<M, M>> chat_model,
    const std::vector<Option>& opts) {
    return appendNode(ChainNodeType::ChatModel, chat_model, opts);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendToolsNode(
    std::shared_ptr<Runnable<M, M>> tools_node,
    const std::vector<Option>& opts) {
    return appendNode(ChainNodeType::ToolsNode, tools_node, opts);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendLambda(
    std::shared_ptr<Runnable<M, M>> lambda,
    const std::vector<Option>& opts) {
    return appendNode(ChainNodeType::Lambda, lambda, opts);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendRetriever(
    std::shared_ptr<Runnable<M, M>> retriever,
    const std::vector<Option>& opts) {
    return appendNode(ChainNodeType::Retriever, retriever, opts);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendEmbedding(
    std::shared_ptr<Runnable<M, M>> embedding,
    const std::vector<Option>& opts) {
    return appendNode(ChainNodeType::Embedding, embedding, opts);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendDocumentTransformer(
    std::shared_ptr<Runnable<M, M>> transformer,
    const std::vector<Option>& opts) {
    return appendNode(ChainNodeType::DocumentTransformer, transformer, opts);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendIndexer(
    std::shared_ptr<Runnable<M, M>> indexer,
    const std::vector<Option>& opts) {
    return appendNode(ChainNodeType::Indexer, indexer, opts);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendLoader(
    std::shared_ptr<Runnable<M, M>> loader,
    const std::vector<Option>& opts) {
    return appendNode(ChainNodeType::Loader, loader, opts);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendGraph(
    std::shared_ptr<Runnable<M, M>> graph,
    const std::vector<Option>& opts) {
    return appendNode(ChainNodeType::Graph, graph, opts);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendPassthrough(
    std::shared_ptr<Runnable<M, M>> passthrough,
    const std::vector<Option>& opts) {
    return appendNode(ChainNodeType::Passthrough, passthrough, opts);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendParallel(
    std::shared_ptr<Parallel> parallel,
    const std::vector<Option>& opts) {
    ValidateNotCompiled();
    
    if (HasError()) {
        return std::shared_ptr<Chain<I, O>>(this);
    }
    
    if (!parallel) {
        error_ = "Parallel node cannot be null";
        return std::shared_ptr<Chain<I, O>>(this);
    }
    
    // Add Parallel as a node to graph
    std::string node_key = NextNodeKey();
    
    // Add edges from previous nodes
    for (const auto& pre_key : pre_node_keys_) {
        gg_->AddEdge(pre_key, node_key);
    }
    
    // Add the parallel node
    gg_->AddNode(node_key, std::static_pointer_cast<Runnable<M, M>>(parallel), opts);
    
    // Update pre_node_keys
    pre_node_keys_.clear();
    pre_node_keys_.push_back(node_key);
    
    return std::shared_ptr<Chain<I, O>>(this);
}

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::AppendBranch(
    std::shared_ptr<ChainBranch> branch,
    const std::vector<Option>& opts) {
    ValidateNotCompiled();
    
    if (HasError()) {
        return std::shared_ptr<Chain<I, O>>(this);
    }
    
    if (!branch) {
        error_ = "Branch node cannot be null";
        return std::shared_ptr<Chain<I, O>>(this);
    }
    
    // Add Branch as a node to graph
    std::string node_key = NextNodeKey();
    
    // Add edges from previous nodes
    for (const auto& pre_key : pre_node_keys_) {
        gg_->AddEdge(pre_key, node_key);
    }
    
    // Add the branch node
    gg_->AddNode(node_key, std::static_pointer_cast<Runnable<M, M>>(branch), opts);
    
    // Update pre_node_keys
    pre_node_keys_.clear();
    pre_node_keys_.push_back(node_key);
    
    return std::shared_ptr<Chain<I, O>>(this);
}

// ============================================================================
// Core appendNode Implementation
// Aligns with: eino/compose/chain.go:343-395
// ============================================================================

template<typename I, typename O>
template<typename M>
std::shared_ptr<Chain<I, O>> Chain<I, O>::appendNode(
    ChainNodeType node_type,
    std::shared_ptr<Runnable<M, M>> node,
    const std::vector<Option>& opts) {
    
    ValidateNotCompiled();
    
    if (HasError()) {
        return std::shared_ptr<Chain<I, O>>(this);
    }
    
    if (!node) {
        error_ = ChainNodeTypeToString(node_type) + " node cannot be null";
        return std::shared_ptr<Chain<I, O>>(this);
    }
    
    // Generate unique node key
    std::string node_key = NextNodeKey();
    
    // First node: add START edge
    if (pre_node_keys_.empty()) {
        gg_->AddEdge("__START__", node_key);
    } else {
        // Add edges from all previous nodes
        for (const auto& pre_key : pre_node_keys_) {
            gg_->AddEdge(pre_key, node_key);
        }
    }
    
    // Add the node to graph
    gg_->AddNode(node_key, node, opts);
    
    // Update pre_node_keys for next node
    pre_node_keys_.clear();
    pre_node_keys_.push_back(node_key);
    
    return std::shared_ptr<Chain<I, O>>(this);
}

// ============================================================================
// Compilation Logic
// Aligns with: eino/compose/chain.go:316-341, 397-418
// ============================================================================

template<typename I, typename O>
void Chain<I, O>::addEndIfNeeded() {
    // Aligns with: eino/compose/chain.go:397-418
    if (has_end_) {
        return;
    }
    
    if (pre_node_keys_.empty()) {
        // No nodes added, error
        error_ = "Chain has no nodes";
        return;
    }
    
    // Add edges to __END__
    for (const auto& pre_key : pre_node_keys_) {
        gg_->AddEdge(pre_key, "__END__");
    }
    
    has_end_ = true;
}

template<typename I, typename O>
void Chain<I, O>::Compile(std::shared_ptr<Context> ctx) {
    // Aligns with: eino/compose/chain.go:316-341
    if (is_compiled_) {
        return;
    }
    
    // Add __END__ node if needed
    addEndIfNeeded();
    
    if (HasError()) {
        throw std::runtime_error("Chain compilation failed: " + error_);
    }
    
    // Compile internal graph
    gg_->Compile(ctx);
    
    is_compiled_ = true;
}

// ============================================================================
// Execution Methods - Delegate to Graph
// Aligns with: eino/compose/chain.go:343-395
// ============================================================================

template<typename I, typename O>
O Chain<I, O>::Invoke(
    std::shared_ptr<Context> ctx,
    const I& input,
    const std::vector<Option>& opts) {
    
    if (!is_compiled_) {
        throw std::runtime_error("Chain not compiled, call Compile() first");
    }
    
    return gg_->Invoke(ctx, input, opts);
}

template<typename I, typename O>
std::shared_ptr<StreamReader<O>> Chain<I, O>::Stream(
    std::shared_ptr<Context> ctx,
    const I& input,
    const std::vector<Option>& opts) {
    
    if (!is_compiled_) {
        throw std::runtime_error("Chain not compiled, call Compile() first");
    }
    
    return gg_->Stream(ctx, input, opts);
}

template<typename I, typename O>
O Chain<I, O>::Collect(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<StreamReader<I>> input,
    const std::vector<Option>& opts) {
    
    if (!is_compiled_) {
        throw std::runtime_error("Chain not compiled, call Compile() first");
    }
    
    return gg_->Collect(ctx, input, opts);
}

template<typename I, typename O>
std::shared_ptr<StreamReader<O>> Chain<I, O>::Transform(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<StreamReader<I>> input,
    const std::vector<Option>& opts) {
    
    if (!is_compiled_) {
        throw std::runtime_error("Chain not compiled, call Compile() first");
    }
    
    return gg_->Transform(ctx, input, opts);
}

// ============================================================================
// Template Instantiations
// ============================================================================

// Common type instantiations
template class Chain<std::string, std::string>;
template class Chain<int, int>;
template class Chain<double, double>;

} // namespace compose
} // namespace eino
