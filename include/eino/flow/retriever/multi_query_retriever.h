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

#ifndef EINO_CPP_FLOW_RETRIEVER_MULTI_QUERY_RETRIEVER_H_
#define EINO_CPP_FLOW_RETRIEVER_MULTI_QUERY_RETRIEVER_H_

#include "../../components/retriever.h"
#include "../../components/model.h"
#include "../../components/prompt.h"
#include "../../schema/types.h"
#include <functional>
#include <memory>
#include <vector>

namespace eino {
namespace flow {
namespace retriever {

// MultiQueryRetriever generates multiple query variations from a single user query
// and retrieves documents using each query, then fuses the results.
class MultiQueryRetriever : public components::Retriever {
public:
    // QueryRewriter function for generating multiple queries
    using QueryRewriter = std::function<std::vector<std::string>(
        std::shared_ptr<compose::Context>,
        const std::string&)>;
    
    // FusionFunc for combining results from multiple retrievals
    using FusionFunc = std::function<std::vector<schema::Document>(
        std::shared_ptr<compose::Context>,
        const std::vector<std::vector<schema::Document>>&)>;
    
    // Configuration for MultiQueryRetriever
    struct Config {
        // Original retriever used for all queries
        std::shared_ptr<components::Retriever> retriever;
        
        // Query rewriting options:
        // Either provide a custom QueryRewriter or use LLM-based approach
        
        // Custom query rewriter function (takes precedence over LLM)
        QueryRewriter rewrite_handler;
        
        // LLM for query rewriting (used if rewrite_handler is not set)
        std::shared_ptr<components::ChatModel> rewrite_llm;
        
        // Prompt template for LLM query rewriting
        std::shared_ptr<components::ChatTemplate> rewrite_template;
        
        // Variable name for query in prompt template
        std::string query_var = "query";
        
        // Parser for LLM output (converts message to queries)
        std::function<std::vector<std::string>(
            std::shared_ptr<compose::Context>,
            const schema::Message&)> llm_output_parser;
        
        // Maximum number of generated queries to use
        int max_queries_num = 5;
        
        // Fusion function for combining results (deduplication by default)
        FusionFunc fusion_func;
    };
    
    // Constructor
    MultiQueryRetriever() = default;
    virtual ~MultiQueryRetriever() = default;
    
    // Initialize the MultiQueryRetriever with configuration
    static std::shared_ptr<MultiQueryRetriever> Create(
        std::shared_ptr<compose::Context> ctx,
        const Config& config);
    
    // Retrieve documents - rewrites query, retrieves with all versions, fuses results
    std::vector<schema::Document> Retrieve(
        std::shared_ptr<compose::Context> ctx,
        const std::string& query,
        const std::vector<compose::Option>& opts = {}) override;
    
    // Runnable interface implementation
    std::vector<schema::Document> Invoke(
        std::shared_ptr<compose::Context> ctx,
        const std::string& input,
        const std::vector<compose::Option>& opts = {}) override {
        return Retrieve(ctx, input, opts);
    }
    
    std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> Stream(
        std::shared_ptr<compose::Context> ctx,
        const std::string& input,
        const std::vector<compose::Option>& opts = {}) override {
        auto result = Retrieve(ctx, input, opts);
        auto reader = std::make_shared<compose::SimpleStreamReader<std::vector<schema::Document>>>();
        reader->Add(result);
        return reader;
    }
    
    std::vector<schema::Document> Collect(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<std::string>> input,
        const std::vector<compose::Option>& opts = {}) override {
        std::vector<schema::Document> result;
        std::string query;
        while (input->Read(query)) {
            auto docs = Retrieve(ctx, query, opts);
            result.insert(result.end(), docs.begin(), docs.end());
        }
        return result;
    }
    
    std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> Transform(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<std::string>> input,
        const std::vector<compose::Option>& opts = {}) override {
        auto reader = std::make_shared<compose::SimpleStreamReader<std::vector<schema::Document>>>();
        std::string query;
        while (input->Read(query)) {
            auto docs = Retrieve(ctx, query, opts);
            reader->Add(docs);
        }
        return reader;
    }
    
    // Set the rewrite handler
    void SetRewriteHandler(QueryRewriter handler) {
        rewrite_handler_ = handler;
    }
    
    // Set maximum queries
    void SetMaxQueriesNum(int max_num) {
        max_queries_num_ = max_num;
    }

private:
    std::shared_ptr<components::Retriever> retriever_;
    QueryRewriter rewrite_handler_;
    int max_queries_num_;
    FusionFunc fusion_func_;
    
    // Default fusion function (deduplication)
    static std::vector<schema::Document> DefaultFusion(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<std::vector<schema::Document>>& docs_list);
    
    // Generate multiple queries
    std::vector<std::string> GenerateQueries(
        std::shared_ptr<compose::Context> ctx,
        const std::string& query);
};

} // namespace retriever
} // namespace flow
} // namespace eino

#endif // EINO_CPP_FLOW_RETRIEVER_MULTI_QUERY_RETRIEVER_H_
