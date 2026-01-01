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
#include "eino/flow/retriever/utils.h"
#include <set>
#include <sstream>
#include <algorithm>

namespace eino {
namespace flow {
namespace retriever {

// Default rewrite prompt template
// Aligns with: eino/flow/retriever/multiquery/multi_query.go:defaultRewritePrompt
constexpr const char* kDefaultRewritePrompt = R"(You are an helpful assistant.
Your role is to create three different versions of the user query to retrieve relevant documents from store.
Your goal is to improve the performance of similarity search by generating text from different perspectives based on the user query.
Only provide the generated queries and separate them by newlines. 
user query: {{query}})";

// Default query variable name
// Aligns with: eino/flow/retriever/multiquery/multi_query.go:defaultQueryVariable
constexpr const char* kDefaultQueryVariable = "query";

// Default max queries number
// Aligns with: eino/flow/retriever/multiquery/multi_query.go:defaultMaxQueriesNum
constexpr int kDefaultMaxQueriesNum = 5;

// DefaultFusion implements deduplication by document ID
// Aligns with: eino/flow/retriever/multiquery/multi_query.go:deduplicateFusion
std::vector<schema::Document> MultiQueryRetriever::DefaultFusion(
    std::shared_ptr<compose::Context> ctx,
    const std::vector<std::vector<schema::Document>>& docs_list) {
    
    std::set<std::string> seen_ids;
    std::vector<schema::Document> result;
    
    for (const auto& docs : docs_list) {
        for (const auto& doc : docs) {
            // Only add if not seen before
            if (seen_ids.find(doc.id) == seen_ids.end()) {
                seen_ids.insert(doc.id);
                result.push_back(doc);
            }
        }
    }
    
    return result;
}

// Default LLM output parser - split by newlines
// Aligns with: eino/flow/retriever/multiquery/multi_query.go:parser
std::vector<std::string> DefaultLLMOutputParser(
    std::shared_ptr<compose::Context> ctx,
    const schema::Message& message) {
    
    std::vector<std::string> queries;
    std::istringstream stream(message.content);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // Add non-empty lines
        if (!line.empty()) {
            queries.push_back(line);
        }
    }
    
    return queries;
}

// GenerateQueries generates multiple query variations
std::vector<std::string> MultiQueryRetriever::GenerateQueries(
    std::shared_ptr<compose::Context> ctx,
    const std::string& query) {
    
    std::vector<std::string> queries;
    
    if (rewrite_handler_) {
        // Use custom rewrite handler
        queries = rewrite_handler_(ctx, query);
    } else {
        // Use LLM-based approach (placeholder - requires full chain implementation)
        // In full implementation, this would:
        // 1. Format query with template
        // 2. Call LLM
        // 3. Parse output
        
        // For now, just return the original query
        queries.push_back(query);
    }
    
    // Limit to max_queries_num
    if (queries.size() > static_cast<size_t>(max_queries_num_)) {
        queries.resize(max_queries_num_);
    }
    
    return queries;
}

// Retrieve implementation
// Aligns with: eino/flow/retriever/multiquery/multi_query.go:multiQueryRetriever.Retrieve
std::vector<schema::Document> MultiQueryRetriever::Retrieve(
    std::shared_ptr<compose::Context> ctx,
    const std::string& query,
    const std::vector<compose::Option>& opts) {
    
    // Step 1: Generate multiple queries
    auto queries = GenerateQueries(ctx, query);
    
    if (queries.empty()) {
        throw std::runtime_error("no queries generated");
    }
    
    // Step 2: Prepare concurrent retrieve tasks
    std::vector<std::shared_ptr<utils::RetrieveTask>> tasks;
    tasks.reserve(queries.size());
    
    for (const auto& q : queries) {
        auto task = std::make_shared<utils::RetrieveTask>(
            "", retriever_, q, opts);
        tasks.push_back(task);
    }
    
    // Step 3: Execute retrievals concurrently
    utils::ConcurrentRetrieveWithCallback(ctx, tasks);
    
    // Step 4: Collect results
    std::vector<std::vector<schema::Document>> results;
    results.reserve(tasks.size());
    
    for (const auto& task : tasks) {
        if (task->has_error) {
            throw std::runtime_error(task->error);
        }
        results.push_back(task->result);
    }
    
    // Step 5: Fuse results
    FusionFunc fusion = fusion_func_ ? fusion_func_ : DefaultFusion;
    
    return fusion(ctx, results);
}

// Factory method
// Aligns with: eino/flow/retriever/multiquery/multi_query.go:NewRetriever
std::shared_ptr<MultiQueryRetriever> MultiQueryRetriever::Create(
    std::shared_ptr<compose::Context> ctx,
    const Config& config) {
    
    // Validate config
    if (!config.retriever) {
        throw std::runtime_error("OrigRetriever is required");
    }
    
    if (!config.rewrite_handler && !config.rewrite_llm) {
        throw std::runtime_error("at least one of RewriteHandler and RewriteLLM must not be empty");
    }
    
    auto retriever = std::make_shared<MultiQueryRetriever>();
    retriever->retriever_ = config.retriever;
    retriever->rewrite_handler_ = config.rewrite_handler;
    retriever->max_queries_num_ = config.max_queries_num > 0 ? 
        config.max_queries_num : kDefaultMaxQueriesNum;
    retriever->fusion_func_ = config.fusion_func;
    
    // TODO: Build rewrite chain if using LLM
    // This requires full Chain implementation with ChatTemplate and ChatModel
    
    return retriever;
}

} // namespace retriever
} // namespace flow
} // namespace eino
