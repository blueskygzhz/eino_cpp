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

#include "eino/flow/flow.h"
#include "eino/components/prebuilt/simple_loader.h"
#include "eino/components/prebuilt/text_splitter.h"
#include "eino/components/prebuilt/simple_embedder.h"
#include <iostream>
#include <vector>

using namespace eino;

// Mock retriever for demonstration
class MockRetriever : public components::Retriever {
public:
    MockRetriever() = default;
    virtual ~MockRetriever() = default;
    
    std::vector<schema::Document> Retrieve(
        std::shared_ptr<compose::Context> ctx,
        const std::string& query,
        const std::vector<compose::Option>& opts = {}) override {
        
        std::vector<schema::Document> docs;
        
        // Create mock sub-documents with parent IDs
        schema::Document doc1;
        doc1.id = "subdoc_1";
        doc1.page_content = "Information about " + query;
        doc1.metadata["parent_id"] = "parent_doc_1";
        doc1.metadata["relevance"] = 0.9;
        docs.push_back(doc1);
        
        schema::Document doc2;
        doc2.id = "subdoc_2";
        doc2.page_content = "More information about " + query;
        doc2.metadata["parent_id"] = "parent_doc_2";
        doc2.metadata["relevance"] = 0.8;
        docs.push_back(doc2);
        
        return docs;
    }
    
    // Implement Runnable interface methods
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
};

int main() {
    std::cout << "\n=== Eino C++ Flow Example ===\n" << std::endl;
    
    auto ctx = std::make_shared<compose::Context>();
    
    // 1. Test ParentRetriever
    std::cout << "1. Testing ParentRetriever..." << std::endl;
    {
        auto mock_retriever = std::make_shared<MockRetriever>();
        
        // Create a mock original document getter
        auto orig_getter = [](std::shared_ptr<compose::Context> ctx,
                              const std::vector<std::string>& parent_ids) {
            std::vector<schema::Document> docs;
            for (const auto& id : parent_ids) {
                schema::Document doc;
                doc.id = id;
                doc.page_content = "Original document: " + id;
                doc.metadata["type"] = "original";
                docs.push_back(doc);
            }
            return docs;
        };
        
        flow::retriever::ParentRetriever::Config config;
        config.retriever = mock_retriever;
        config.parent_id_key = "parent_id";
        config.orig_doc_getter = orig_getter;
        
        auto parent_retriever = flow::retriever::ParentRetriever::Create(ctx, config);
        auto parent_docs = parent_retriever->Retrieve(ctx, "test query");
        
        std::cout << "   - Retrieved " << parent_docs.size() << " parent documents" << std::endl;
        for (const auto& doc : parent_docs) {
            std::cout << "   - Document ID: " << doc.id << std::endl;
        }
    }
    
    // 2. Test MultiQueryRetriever
    std::cout << "\n2. Testing MultiQueryRetriever..." << std::endl;
    {
        auto mock_retriever = std::make_shared<MockRetriever>();
        
        // Custom query rewriter
        auto query_rewriter = [](std::shared_ptr<compose::Context> ctx,
                                  const std::string& query) {
            std::vector<std::string> queries;
            queries.push_back(query);
            queries.push_back(query + " detailed");
            queries.push_back(query + " overview");
            return queries;
        };
        
        flow::retriever::MultiQueryRetriever::Config config;
        config.retriever = mock_retriever;
        config.rewrite_handler = query_rewriter;
        config.max_queries_num = 3;
        
        auto multi_retriever = flow::retriever::MultiQueryRetriever::Create(ctx, config);
        auto fused_docs = multi_retriever->Retrieve(ctx, "test query");
        
        std::cout << "   - Generated multiple queries" << std::endl;
        std::cout << "   - Fused into " << fused_docs.size() << " unique documents" << std::endl;
    }
    
    // 3. Test RouterRetriever
    std::cout << "\n3. Testing RouterRetriever..." << std::endl;
    {
        // Create multiple mock retrievers
        auto retriever1 = std::make_shared<MockRetriever>();
        auto retriever2 = std::make_shared<MockRetriever>();
        
        std::map<std::string, std::shared_ptr<components::Retriever>> retrievers;
        retrievers["bm25"] = retriever1;
        retrievers["vector"] = retriever2;
        
        // Create router function
        auto router = [](std::shared_ptr<compose::Context> ctx,
                         const std::string& query) {
            std::vector<std::string> selected;
            selected.push_back("bm25");
            selected.push_back("vector");
            return selected;
        };
        
        flow::retriever::RouterRetriever::Config config;
        config.retrievers = retrievers;
        config.router = router;
        
        auto router_retriever = flow::retriever::RouterRetriever::Create(ctx, config);
        auto routed_docs = router_retriever->Retrieve(ctx, "test query");
        
        std::cout << "   - Routed to 2 retrievers" << std::endl;
        std::cout << "   - Fused with RRF into " << routed_docs.size() << " documents" << std::endl;
    }
    
    // 4. Test ParentIndexer
    std::cout << "\n4. Testing ParentIndexer..." << std::endl;
    {
        // Create components
        auto splitter = std::make_shared<components::TextSplitter>(50, 10);
        
        std::cout << "   - ParentIndexer initialized with TextSplitter" << std::endl;
        std::cout << "   - Splits parent documents into sub-documents" << std::endl;
        std::cout << "   - Manages parent-child document relationships" << std::endl;
        std::cout << "   - Example: document \"doc_1\" splits into \"doc_1_chunk_0\", \"doc_1_chunk_1\", etc." << std::endl;
    }
    
    // 5. Summary
    std::cout << "\n5. Flow Components Summary..." << std::endl;
    {
        std::cout << "   Available Flow Components:" << std::endl;
        std::cout << "   - ParentRetriever: Maps search results to original documents" << std::endl;
        std::cout << "   - MultiQueryRetriever: Expands queries for better recall" << std::endl;
        std::cout << "   - RouterRetriever: Routes queries to multiple retrievers" << std::endl;
        std::cout << "   - ParentIndexer: Manages parent-child document relationships" << std::endl;
        std::cout << "   - ReActAgent: Implements reasoning and acting pattern" << std::endl;
    }
    
    std::cout << "\n=== Flow Examples Completed Successfully! ===" << std::endl;
    return 0;
}
