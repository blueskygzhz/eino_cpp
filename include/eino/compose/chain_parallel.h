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

#ifndef EINO_CPP_COMPOSE_CHAIN_PARALLEL_H_
#define EINO_CPP_COMPOSE_CHAIN_PARALLEL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

#include "runnable.h"
#include "graph.h"

namespace eino {
namespace compose {

// Forward declarations
template<typename I, typename O> class Graph;
template<typename I, typename O> class Chain;

/**
 * @brief Parallel represents a collection of nodes that execute in parallel
 * 
 * This class allows running multiple nodes concurrently and collecting their
 * outputs with specific output keys. It's useful when you want to run multiple
 * operations on the same input in parallel within a chain.
 * 
 * Aligned with: eino/compose/chain_parallel.go
 * 
 * Example:
 * ```cpp
 * auto parallel = NewParallel();
 * parallel->AddChatModel("model1_output", chat_model_1);
 * parallel->AddChatModel("model2_output", chat_model_2);
 * parallel->AddLambda("processed_output", lambda_func);
 * 
 * auto chain = NewChain<Input, map<string, any>>();
 * chain->AppendParallel(parallel);
 * ```
 */
class Parallel {
public:
    struct NodeEntry {
        std::string output_key;
        std::shared_ptr<void> node; // Type-erased runnable
        std::string node_type;
        std::vector<GraphAddNodeOpt> opts;
        
        NodeEntry(const std::string& key, 
                 std::shared_ptr<void> n,
                 const std::string& type,
                 const std::vector<GraphAddNodeOpt>& options = {})
            : output_key(key), node(n), node_type(type), opts(options) {}
    };

    Parallel() = default;
    
    /**
     * @brief AddChatModel adds a chat model to the parallel execution
     * @param output_key The key for this node's output in the result map
     * @param node The chat model runnable
     * @param opts Optional graph node options
     */
    template<typename I, typename O>
    Parallel* AddChatModel(
        const std::string& output_key,
        std::shared_ptr<Runnable<I, O>> node,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        return AddNode(output_key, node, "ChatModel", opts);
    }
    
    /**
     * @brief AddChatTemplate adds a chat template to the parallel execution
     */
    template<typename I, typename O>
    Parallel* AddChatTemplate(
        const std::string& output_key,
        std::shared_ptr<Runnable<I, O>> node,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        return AddNode(output_key, node, "ChatTemplate", opts);
    }
    
    /**
     * @brief AddToolsNode adds a tools node to the parallel execution
     */
    template<typename I, typename O>
    Parallel* AddToolsNode(
        const std::string& output_key,
        std::shared_ptr<Runnable<I, O>> node,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        return AddNode(output_key, node, "ToolsNode", opts);
    }
    
    /**
     * @brief AddLambda adds a lambda runnable to the parallel execution
     */
    template<typename I, typename O>
    Parallel* AddLambda(
        const std::string& output_key,
        std::shared_ptr<Runnable<I, O>> node,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        return AddNode(output_key, node, "Lambda", opts);
    }
    
    /**
     * @brief AddEmbedding adds an embedding node to the parallel execution
     */
    template<typename I, typename O>
    Parallel* AddEmbedding(
        const std::string& output_key,
        std::shared_ptr<Runnable<I, O>> node,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        return AddNode(output_key, node, "Embedding", opts);
    }
    
    /**
     * @brief AddRetriever adds a retriever node to the parallel execution
     */
    template<typename I, typename O>
    Parallel* AddRetriever(
        const std::string& output_key,
        std::shared_ptr<Runnable<I, O>> node,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        return AddNode(output_key, node, "Retriever", opts);
    }
    
    /**
     * @brief AddLoader adds a document loader to the parallel execution
     */
    template<typename I, typename O>
    Parallel* AddLoader(
        const std::string& output_key,
        std::shared_ptr<Runnable<I, O>> node,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        return AddNode(output_key, node, "Loader", opts);
    }
    
    /**
     * @brief AddIndexer adds an indexer node to the parallel execution
     */
    template<typename I, typename O>
    Parallel* AddIndexer(
        const std::string& output_key,
        std::shared_ptr<Runnable<I, O>> node,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        return AddNode(output_key, node, "Indexer", opts);
    }
    
    /**
     * @brief AddDocumentTransformer adds a document transformer to the parallel execution
     */
    template<typename I, typename O>
    Parallel* AddDocumentTransformer(
        const std::string& output_key,
        std::shared_ptr<Runnable<I, O>> node,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        return AddNode(output_key, node, "DocumentTransformer", opts);
    }
    
    /**
     * @brief AddGraph adds a graph as a node to the parallel execution
     * Useful when you want to use a subgraph or chain as a parallel node
     */
    template<typename I, typename O>
    Parallel* AddGraph(
        const std::string& output_key,
        std::shared_ptr<Graph<I, O>> node,
        const std::vector<GraphAddNodeOpt>& opts = {}) {
        if (!node) {
            error_ = "AddGraph: node cannot be null";
            return this;
        }
        return AddNode(output_key, std::static_pointer_cast<void>(node), 
                      "Graph", opts);
    }
    
    /**
     * @brief AddPassthrough adds a passthrough node
     * A passthrough node simply forwards its input as output
     */
    Parallel* AddPassthrough(
        const std::string& output_key,
        const std::vector<GraphAddNodeOpt>& opts = {});
    
    /**
     * @brief GetNodes returns all registered nodes
     */
    const std::vector<NodeEntry>& GetNodes() const;
    
    /**
     * @brief HasError returns whether any error occurred during node addition
     */
    bool HasError() const;
    
    /**
     * @brief GetError returns the error message if any
     */
    const std::string& GetError() const;
    
    /**
     * @brief Clear clears all nodes and errors
     */
    void Clear();
    
    /**
     * @brief GetNodeCount returns the number of nodes
     */
    size_t GetNodeCount() const;
    
    /**
     * @brief HasOutputKey checks if an output key exists
     */
    bool HasOutputKey(const std::string& key) const;
    
    /**
     * @brief GetOutputKeys returns all output keys
     */
    std::vector<std::string> GetOutputKeys() const;
    
    /**
     * @brief Validate validates the parallel configuration
     * @throws std::runtime_error if validation fails
     */
    void Validate() const;

private:
    template<typename I, typename O>
    Parallel* AddNode(
        const std::string& output_key,
        std::shared_ptr<Runnable<I, O>> node,
        const std::string& node_type,
        const std::vector<GraphAddNodeOpt>& opts) {
        
        if (!error_.empty()) {
            return this;
        }
        
        if (output_key.empty()) {
            error_ = "Parallel AddNode: output_key cannot be empty";
            return this;
        }
        
        if (output_keys_.find(output_key) != output_keys_.end()) {
            error_ = "Parallel AddNode: duplicate output_key=" + output_key;
            return this;
        }
        
        output_keys_[output_key] = true;
        nodes_.emplace_back(output_key, 
                           std::static_pointer_cast<void>(node),
                           node_type,
                           opts);
        
        return this;
    }
    
    Parallel* AddNode(
        const std::string& output_key,
        std::shared_ptr<void> node,
        const std::string& node_type,
        const std::vector<GraphAddNodeOpt>& opts) {
        
        if (!error_.empty()) {
            return this;
        }
        
        if (output_key.empty()) {
            error_ = "Parallel AddNode: output_key cannot be empty";
            return this;
        }
        
        if (output_keys_.find(output_key) != output_keys_.end()) {
            error_ = "Parallel AddNode: duplicate output_key=" + output_key;
            return this;
        }
        
        output_keys_[output_key] = true;
        nodes_.emplace_back(output_key, node, node_type, opts);
        
        return this;
    }
    
    std::vector<NodeEntry> nodes_;
    std::map<std::string, bool> output_keys_;
    std::string error_;
};

/**
 * @brief NewParallel creates a new Parallel instance
 */
std::shared_ptr<Parallel> NewParallel();

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_CHAIN_PARALLEL_H_
