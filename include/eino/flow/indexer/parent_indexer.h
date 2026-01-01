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

#ifndef EINO_CPP_FLOW_INDEXER_PARENT_INDEXER_H_
#define EINO_CPP_FLOW_INDEXER_PARENT_INDEXER_H_

#include "../../components/indexer.h"
#include "../../components/document.h"
#include "../../schema/types.h"
#include <functional>
#include <memory>
#include <vector>

namespace eino {
namespace flow {
namespace indexer {

// ParentIndexer handles indexing of documents with parent-child relationships.
// It splits parent documents into sub-documents and manages their identities.
class ParentIndexer : public components::Indexer {
public:
    // SubIDGenerator function for generating unique IDs for sub-documents
    using SubIDGenerator = std::function<std::vector<std::string>(
        std::shared_ptr<compose::Context>,
        const std::string& /* parent_id */,
        int /* num_docs */)>;
    
    // Configuration for ParentIndexer
    struct Config {
        // Underlying indexer for storing documents
        std::shared_ptr<components::Indexer> indexer;
        
        // Transformer to split parent documents into sub-documents
        std::shared_ptr<components::Transformer> transformer;
        
        // Metadata key for storing parent document ID
        std::string parent_id_key = "parent_id";
        
        // Callback for generating sub-document IDs
        SubIDGenerator sub_id_generator;
    };
    
    // Constructor
    ParentIndexer() = default;
    virtual ~ParentIndexer() = default;
    
    // Initialize the ParentIndexer with configuration
    static std::shared_ptr<ParentIndexer> Create(
        std::shared_ptr<compose::Context> ctx,
        const Config& config);
    
    // Store documents - transforms and generates IDs for sub-documents
    std::vector<std::string> Store(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Document>& documents,
        const std::vector<compose::Option>& opts = {}) override;
    
    // Runnable interface implementation
    std::vector<std::string> Invoke(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Document>& input,
        const std::vector<compose::Option>& opts = {}) override {
        return Store(ctx, input, opts);
    }
    
    std::shared_ptr<compose::StreamReader<std::vector<std::string>>> Stream(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Document>& input,
        const std::vector<compose::Option>& opts = {}) override {
        auto result = Store(ctx, input, opts);
        auto reader = std::make_shared<compose::SimpleStreamReader<std::vector<std::string>>>();
        reader->Add(result);
        return reader;
    }
    
    std::vector<std::string> Collect(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> input,
        const std::vector<compose::Option>& opts = {}) override {
        std::vector<std::string> result;
        std::vector<schema::Document> docs;
        while (input->Read(docs)) {
            auto ids = Store(ctx, docs, opts);
            result.insert(result.end(), ids.begin(), ids.end());
        }
        return result;
    }
    
    std::shared_ptr<compose::StreamReader<std::vector<std::string>>> Transform(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> input,
        const std::vector<compose::Option>& opts = {}) override {
        auto reader = std::make_shared<compose::SimpleStreamReader<std::vector<std::string>>>();
        std::vector<schema::Document> docs;
        while (input->Read(docs)) {
            auto ids = Store(ctx, docs, opts);
            reader->Add(ids);
        }
        return reader;
    }
    
    // Set the underlying indexer
    void SetIndexer(std::shared_ptr<components::Indexer> indexer) {
        indexer_ = indexer;
    }
    
    // Set the transformer
    void SetTransformer(std::shared_ptr<components::Transformer> transformer) {
        transformer_ = transformer;
    }
    
    // Set the parent ID key
    void SetParentIDKey(const std::string& key) {
        parent_id_key_ = key;
    }
    
    // Set the sub-ID generator
    void SetSubIDGenerator(SubIDGenerator generator) {
        sub_id_generator_ = generator;
    }

private:
    std::shared_ptr<components::Indexer> indexer_;
    std::shared_ptr<components::Transformer> transformer_;
    std::string parent_id_key_;
    SubIDGenerator sub_id_generator_;
};

} // namespace indexer
} // namespace flow
} // namespace eino

#endif // EINO_CPP_FLOW_INDEXER_PARENT_INDEXER_H_
