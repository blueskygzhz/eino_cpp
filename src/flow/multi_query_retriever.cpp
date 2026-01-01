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

#include "eino/flow/retriever/multi_query_retriever.h"
#include <algorithm>
#include <thread>

namespace eino {
namespace flow {
namespace retriever {

std::shared_ptr<MultiQueryRetriever> MultiQueryRetriever::Create(
    std::shared_ptr<compose::Context> ctx,
    const Config& config) {
    auto mqr = std::make_shared<MultiQueryRetriever>();
    mqr->retriever_ = config.retriever;
    mqr->rewrite_handler_ = config.rewrite_handler;
    mqr->max_queries_num_ = config.max_queries_num > 0 ? config.max_queries_num : 5;
    
    // Set fusion function (use provided or default)
    if (config.fusion_func) {
        mqr->fusion_func_ = config.fusion_func;
    } else {
        mqr->fusion_func_ = [](std::shared_ptr<compose::Context> ctx, 
                                const std::vector<std::vector<schema::Document>>& docs_list) {
            return MultiQueryRetriever::DefaultFusion(ctx, docs_list);
        };
    }
    
    return mqr;
}

std::vector<schema::Document> MultiQueryRetriever::Retrieve(
    std::shared_ptr<compose::Context> ctx,
    const std::string& query,
    const std::vector<compose::Option>& opts) {
    
    if (!retriever_) {
        return std::vector<schema::Document>();
    }
    
    // Generate multiple queries
    auto queries = GenerateQueries(ctx, query);
    
    if (queries.empty()) {
        // Fallback to original query
        queries.push_back(query);
    }
    
    // Limit number of queries
    if (static_cast<int>(queries.size()) > max_queries_num_) {
        queries.resize(max_queries_num_);
    }
    
    // Retrieve documents for each query (sequentially for now)
    std::vector<std::vector<schema::Document>> results;
    for (const auto& q : queries) {
        auto docs = retriever_->Retrieve(ctx, q, opts);
        results.push_back(docs);
    }
    
    // Fusion
    if (!fusion_func_) {
        return DefaultFusion(ctx, results);
    }
    return fusion_func_(ctx, results);
}

std::vector<schema::Document> MultiQueryRetriever::DefaultFusion(
    std::shared_ptr<compose::Context> ctx,
    const std::vector<std::vector<schema::Document>>& docs_list) {
    
    std::vector<schema::Document> result;
    std::vector<std::string> seen_ids;
    
    for (const auto& docs : docs_list) {
        for (const auto& doc : docs) {
            // Deduplicate by document ID
            if (std::find(seen_ids.begin(), seen_ids.end(), doc.id) == seen_ids.end()) {
                seen_ids.push_back(doc.id);
                result.push_back(doc);
            }
        }
    }
    
    return result;
}

std::vector<std::string> MultiQueryRetriever::GenerateQueries(
    std::shared_ptr<compose::Context> ctx,
    const std::string& query) {
    
    // If custom handler is provided, use it
    if (rewrite_handler_) {
        return rewrite_handler_(ctx, query);
    }
    
    // Otherwise return the original query
    return {query};
}

} // namespace retriever
} // namespace flow
} // namespace eino
