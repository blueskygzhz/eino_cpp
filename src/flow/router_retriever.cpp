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
#include <algorithm>
#include <cmath>

namespace eino {
namespace flow {
namespace retriever {

std::shared_ptr<RouterRetriever> RouterRetriever::Create(
    std::shared_ptr<compose::Context> ctx,
    const Config& config) {
    auto rr = std::make_shared<RouterRetriever>();
    rr->retrievers_ = config.retrievers;
    rr->router_ = config.router;
    
    // Set fusion function (use provided or default)
    if (config.fusion_func) {
        rr->fusion_func_ = config.fusion_func;
    } else {
        rr->fusion_func_ = [](std::shared_ptr<compose::Context> ctx,
                               const std::map<std::string, std::vector<schema::Document>>& results) {
            return RouterRetriever::DefaultRRFFusion(ctx, results);
        };
    }
    
    return rr;
}

std::vector<schema::Document> RouterRetriever::Retrieve(
    std::shared_ptr<compose::Context> ctx,
    const std::string& query,
    const std::vector<compose::Option>& opts) {
    
    // Route the query
    auto retriever_names = RouteQuery(ctx, query);
    
    if (retriever_names.empty()) {
        return std::vector<schema::Document>();
    }
    
    // Retrieve documents from selected retrievers
    std::map<std::string, std::vector<schema::Document>> results;
    for (const auto& name : retriever_names) {
        auto it = retrievers_.find(name);
        if (it != retrievers_.end() && it->second) {
            auto docs = it->second->Retrieve(ctx, query, opts);
            results[name] = docs;
        }
    }
    
    // Fusion
    if (!fusion_func_) {
        return DefaultRRFFusion(ctx, results);
    }
    return fusion_func_(ctx, results);
}

std::vector<schema::Document> RouterRetriever::DefaultRRFFusion(
    std::shared_ptr<compose::Context> ctx,
    const std::map<std::string, std::vector<schema::Document>>& results) {
    
    if (results.empty()) {
        return std::vector<schema::Document>();
    }
    
    // If only one retriever returned results
    if (results.size() == 1) {
        return results.begin()->second;
    }
    
    // Reciprocal Rank Fusion (RRF)
    std::map<std::string, double> doc_rank_map;  // doc_id -> rank score
    std::map<std::string, schema::Document> doc_map;  // doc_id -> document
    
    for (const auto& pair : results) {
        const auto& docs = pair.second;
        for (size_t i = 0; i < docs.size(); ++i) {
            doc_map[docs[i].id] = docs[i];
            
            if (doc_rank_map.find(docs[i].id) == doc_rank_map.end()) {
                // 1 / (i + 60) is the RRF formula
                doc_rank_map[docs[i].id] = 1.0 / (i + 60.0);
            } else {
                doc_rank_map[docs[i].id] += 1.0 / (i + 60.0);
            }
        }
    }
    
    // Sort by rank score - convert to vector of (score, doc) pairs
    std::vector<std::pair<double, schema::Document>> sorted_docs;
    for (const auto& pair : doc_map) {
        sorted_docs.push_back({doc_rank_map[pair.first], pair.second});
    }
    
    // Custom sort using simple comparison
    for (size_t i = 0; i < sorted_docs.size(); ++i) {
        for (size_t j = i + 1; j < sorted_docs.size(); ++j) {
            if (sorted_docs[j].first > sorted_docs[i].first) {
                std::swap(sorted_docs[i], sorted_docs[j]);
            }
        }
    }
    
    std::vector<schema::Document> result;
    for (const auto& pair : sorted_docs) {
        result.push_back(pair.second);  // pair.first is score, pair.second is document
    }
    
    return result;
}

std::vector<std::string> RouterRetriever::RouteQuery(
    std::shared_ptr<compose::Context> ctx,
    const std::string& query) {
    
    // If custom router is provided, use it
    if (router_) {
        return router_(ctx, query);
    }
    
    // Otherwise use all retrievers
    std::vector<std::string> all_retrievers;
    for (const auto& pair : retrievers_) {
        all_retrievers.push_back(pair.first);
    }
    return all_retrievers;
}

} // namespace retriever
} // namespace flow
} // namespace eino
