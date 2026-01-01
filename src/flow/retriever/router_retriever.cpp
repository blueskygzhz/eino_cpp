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

#include "eino/flow/retriever/router_retriever.h"
#include "eino/flow/retriever/utils.h"
#include <algorithm>
#include <map>

namespace eino {
namespace flow {
namespace retriever {

// DefaultRRFFusion implements Reciprocal Rank Fusion algorithm
// RRF formula: score = 1 / (rank + k), where k=60 is the standard constant
// Aligns with: eino/flow/retriever/router/router.go:rrf
std::vector<schema::Document> RouterRetriever::DefaultRRFFusion(
    std::shared_ptr<compose::Context> ctx,
    const std::map<std::string, std::vector<schema::Document>>& results) {
    
    // Handle edge cases
    if (results.empty()) {
        throw std::runtime_error("no documents");
    }
    
    if (results.size() == 1) {
        return results.begin()->second;
    }
    
    // Build document rank map using RRF
    std::map<std::string, double> doc_rank_map;
    std::map<std::string, schema::Document> doc_map;
    
    const int k = 60;  // RRF constant
    
    for (const auto& result_pair : results) {
        const auto& docs = result_pair.second;
        
        for (size_t i = 0; i < docs.size(); ++i) {
            const auto& doc = docs[i];
            const auto& doc_id = doc.id;
            
            // Store document
            doc_map[doc_id] = doc;
            
            // Calculate RRF score: 1 / (rank + k)
            // rank is 0-based (i), so we use (i + k)
            double rrf_score = 1.0 / static_cast<double>(i + k);
            
            // Accumulate scores for documents appearing in multiple results
            if (doc_rank_map.find(doc_id) == doc_rank_map.end()) {
                doc_rank_map[doc_id] = rrf_score;
            } else {
                doc_rank_map[doc_id] += rrf_score;
            }
        }
    }
    
    // Convert to vector for sorting
    std::vector<schema::Document> doc_list;
    doc_list.reserve(doc_map.size());
    
    for (const auto& pair : doc_map) {
        doc_list.push_back(pair.second);
    }
    
    // Sort by RRF score (descending)
    std::sort(doc_list.begin(), doc_list.end(),
        [&doc_rank_map](const schema::Document& a, const schema::Document& b) {
            return doc_rank_map[a.id] > doc_rank_map[b.id];
        });
    
    return doc_list;
}

// RouteQuery selects which retrievers to use
// If no router is configured, returns all retrievers
// Aligns with: eino/flow/retriever/router/router.go:NewRetriever (default router)
std::vector<std::string> RouterRetriever::RouteQuery(
    std::shared_ptr<compose::Context> ctx,
    const std::string& query) {
    
    // Use custom router if provided
    if (router_) {
        return router_(ctx, query);
    }
    
    // Default: select all retrievers
    std::vector<std::string> retriever_names;
    retriever_names.reserve(retrievers_.size());
    
    for (const auto& pair : retrievers_) {
        retriever_names.push_back(pair.first);
    }
    
    return retriever_names;
}

// Retrieve implementation
// Aligns with: eino/flow/retriever/router/router.go:routerRetriever.Retrieve
std::vector<schema::Document> RouterRetriever::Retrieve(
    std::shared_ptr<compose::Context> ctx,
    const std::string& query,
    const std::vector<compose::Option>& opts) {
    
    // Step 1: Route query to select retrievers
    auto retriever_names = RouteQuery(ctx, query);
    
    if (retriever_names.empty()) {
        throw std::runtime_error("no retriever has been selected");
    }
    
    // Step 2: Prepare concurrent retrieve tasks
    std::vector<std::shared_ptr<utils::RetrieveTask>> tasks;
    tasks.reserve(retriever_names.size());
    
    for (const auto& name : retriever_names) {
        auto it = retrievers_.find(name);
        if (it == retrievers_.end()) {
            throw std::runtime_error("router output[" + name + "] has not registered");
        }
        
        auto task = std::make_shared<utils::RetrieveTask>(
            name, it->second, query, opts);
        tasks.push_back(task);
    }
    
    // Step 3: Execute retrievals concurrently
    utils::ConcurrentRetrieveWithCallback(ctx, tasks);
    
    // Step 4: Collect results
    std::map<std::string, std::vector<schema::Document>> results;
    
    for (const auto& task : tasks) {
        if (task->has_error) {
            throw std::runtime_error(task->error);
        }
        results[task->name] = task->result;
    }
    
    // Step 5: Fuse results
    FusionFunc fusion = fusion_func_ ? fusion_func_ : DefaultRRFFusion;
    
    return fusion(ctx, results);
}

// Factory method
std::shared_ptr<RouterRetriever> RouterRetriever::Create(
    std::shared_ptr<compose::Context> ctx,
    const Config& config) {
    
    if (config.retrievers.empty()) {
        throw std::runtime_error("retrievers is empty");
    }
    
    auto retriever = std::make_shared<RouterRetriever>();
    retriever->retrievers_ = config.retrievers;
    retriever->router_ = config.router;
    retriever->fusion_func_ = config.fusion_func;
    
    return retriever;
}

} // namespace retriever
} // namespace flow
} // namespace eino
