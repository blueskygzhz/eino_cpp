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

#ifndef EINO_CPP_FLOW_RETRIEVER_PARENT_RETRIEVER_H_
#define EINO_CPP_FLOW_RETRIEVER_PARENT_RETRIEVER_H_

#include "../../components/retriever.h"
#include "../../schema/types.h"
#include <functional>
#include <memory>
#include <vector>

namespace eino {
namespace flow {
namespace retriever {

// ParentRetriever retrieves original parent documents based on sub-document search results.
// It uses a callback function to load parent documents from their IDs.
class ParentRetriever : public components::Retriever {
public:
    // OrigDocGetter function type for retrieving original documents by IDs
    using OrigDocGetter = std::function<std::vector<schema::Document>(
        std::shared_ptr<compose::Context>,
        const std::vector<std::string>&)>;
    
    // Configuration for ParentRetriever
    struct Config {
        // Original retriever that returns sub-documents with parent IDs
        std::shared_ptr<components::Retriever> retriever;
        
        // Metadata key storing parent document ID in sub-documents
        // e.g., "parent_id" or "source_doc_id"
        std::string parent_id_key = "parent_id";
        
        // Callback function to retrieve original documents by their IDs
        OrigDocGetter orig_doc_getter;
    };
    
    // Constructor
    ParentRetriever() = default;
    virtual ~ParentRetriever() = default;
    
    // Initialize the ParentRetriever with configuration
    static std::shared_ptr<ParentRetriever> Create(
        std::shared_ptr<compose::Context> ctx,
        const Config& config);
    
    // Retrieve documents - retrieves sub-documents then fetches parent documents
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
    
    // Set the underlying retriever
    void SetRetriever(std::shared_ptr<components::Retriever> retriever) {
        retriever_ = retriever;
    }
    
    // Set the parent ID key
    void SetParentIDKey(const std::string& key) {
        parent_id_key_ = key;
    }
    
    // Set the original document getter callback
    void SetOrigDocGetter(OrigDocGetter getter) {
        orig_doc_getter_ = getter;
    }

private:
    std::shared_ptr<components::Retriever> retriever_;
    std::string parent_id_key_;
    OrigDocGetter orig_doc_getter_;
    
    // Helper to extract unique parent IDs from documents
    std::vector<std::string> ExtractParentIDs(
        const std::vector<schema::Document>& sub_docs);
};

} // namespace retriever
} // namespace flow
} // namespace eino

#endif // EINO_CPP_FLOW_RETRIEVER_PARENT_RETRIEVER_H_
