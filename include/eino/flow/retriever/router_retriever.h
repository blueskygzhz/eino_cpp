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

#ifndef EINO_CPP_FLOW_RETRIEVER_ROUTER_RETRIEVER_H_
#define EINO_CPP_FLOW_RETRIEVER_ROUTER_RETRIEVER_H_

#include "../../components/retriever.h"
#include "../../schema/types.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <string>

namespace eino {
namespace flow {
namespace retriever {

// RouterRetriever routes queries to different retrievers based on routing logic
// and fuses results from selected retrievers.
class RouterRetriever : public components::Retriever {
public:
    // Router function to select which retrievers to use for a query
    using Router = std::function<std::vector<std::string>(
        std::shared_ptr<compose::Context>,
        const std::string&)>;
    
    // FusionFunc for combining results from multiple retrievers
    using FusionFunc = std::function<std::vector<schema::Document>(
        std::shared_ptr<compose::Context>,
        const std::map<std::string, std::vector<schema::Document>>&)>;
    
    // Configuration for RouterRetriever
    struct Config {
        // Map of retriever names to retriever instances
        std::map<std::string, std::shared_ptr<components::Retriever>> retrievers;
        
        // Routing function (selects retrievers for a query)
        // If not provided, all retrievers are used
        Router router;
        
        // Fusion function for combining results
        // If not provided, Reciprocal Rank Fusion (RRF) is used
        FusionFunc fusion_func;
    };
    
    // Constructor
    RouterRetriever() = default;
    virtual ~RouterRetriever() = default;
    
    // Initialize the RouterRetriever with configuration
    static std::shared_ptr<RouterRetriever> Create(
        std::shared_ptr<compose::Context> ctx,
        const Config& config);
    
    // Retrieve documents - routes query, retrieves from selected retrievers, fuses results
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
    
    // Add or update a retriever
    void SetRetriever(const std::string& name, 
                      std::shared_ptr<components::Retriever> retriever) {
        retrievers_[name] = retriever;
    }
    
    // Set the routing function
    void SetRouter(Router router) {
        router_ = router;
    }

private:
    std::map<std::string, std::shared_ptr<components::Retriever>> retrievers_;
    Router router_;
    FusionFunc fusion_func_;
    
    // Default fusion function using Reciprocal Rank Fusion (RRF)
    static std::vector<schema::Document> DefaultRRFFusion(
        std::shared_ptr<compose::Context> ctx,
        const std::map<std::string, std::vector<schema::Document>>& results);
    
    // Route the query to select retrievers
    std::vector<std::string> RouteQuery(
        std::shared_ptr<compose::Context> ctx,
        const std::string& query);
};

} // namespace retriever
} // namespace flow
} // namespace eino

#endif // EINO_CPP_FLOW_RETRIEVER_ROUTER_RETRIEVER_H_
