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

#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include "eino/components/component.h"
#include "eino/components/prebuilt/simple_loader.h"
#include "eino/components/prebuilt/text_splitter.h"
#include "eino/components/prebuilt/simple_embedder.h"

using json = nlohmann::json;

int main() {
    std::cout << "=== Eino C++ Components Example ===" << std::endl << std::endl;
    
    try {
        auto ctx = eino::compose::Context::Background();
        
        // 1. PromptTemplate Example
        std::cout << "1. Testing PromptTemplate..." << std::endl;
        {
            auto prompt = std::make_shared<eino::components::PromptTemplate>(
                "You are a helpful assistant. User asked: {question}");
            
            std::map<std::string, json> vars;
            vars["question"] = "What is artificial intelligence?";
            
            auto messages = prompt->Invoke(ctx, vars);
            std::cout << "   - Formatted message: " << messages[0].content << std::endl;
            std::cout << "   - Role: " << messages[0].GetRoleString() << std::endl;
        }
        
        // 2. SimpleLoader Example
        std::cout << "\n2. Testing SimpleLoader..." << std::endl;
        {
            auto loader = std::make_shared<eino::components::SimpleLoader>();
            
            eino::schema::Source source;
            source.uri = "example_document.txt";
            
            auto docs = loader->Invoke(ctx, source);
            std::cout << "   - Loaded " << docs.size() << " document(s)" << std::endl;
            if (!docs.empty()) {
                std::cout << "   - First document ID: " << docs[0].id << std::endl;
                std::cout << "   - Content length: " << docs[0].page_content.length() << " bytes" << std::endl;
            }
        }
        
        // 3. TextSplitter Example
        std::cout << "\n3. Testing TextSplitter..." << std::endl;
        {
            auto splitter = std::make_shared<eino::components::TextSplitter>(100, 20);
            
            std::vector<eino::schema::Document> docs;
            eino::schema::Document doc;
            doc.id = "doc_1";
            doc.page_content = "This is a long document that contains multiple sentences. "
                              "It demonstrates how the text splitter works. "
                              "The splitter breaks large documents into manageable chunks. "
                              "Each chunk contains overlap with the previous one. "
                              "This helps preserve context across chunks. "
                              "The split documents are perfect for processing large corpora.";
            docs.push_back(doc);
            
            auto chunks = splitter->Invoke(ctx, docs);
            std::cout << "   - Original document chunks: 1" << std::endl;
            std::cout << "   - Split into " << chunks.size() << " chunks" << std::endl;
            
            for (size_t i = 0; i < chunks.size(); ++i) {
                std::cout << "   - Chunk " << i << ": " << chunks[i].page_content.substr(0, 50) << "..." << std::endl;
            }
        }
        
        // 4. SimpleEmbedder Example
        std::cout << "\n4. Testing SimpleEmbedder..." << std::endl;
        {
            auto embedder = std::make_shared<eino::components::SimpleEmbedder>(128);
            
            std::vector<std::string> texts = {
                "hello world",
                "artificial intelligence",
                "machine learning",
                "deep learning"
            };
            
            auto embeddings = embedder->Invoke(ctx, texts);
            std::cout << "   - Embedded " << texts.size() << " texts" << std::endl;
            std::cout << "   - Embedding dimension: " << embeddings[0].size() << std::endl;
            
            // Calculate cosine similarity between first two embeddings
            double dot_product = 0.0;
            for (size_t i = 0; i < embeddings[0].size(); ++i) {
                dot_product += embeddings[0][i] * embeddings[1][i];
            }
            std::cout << "   - Cosine similarity (text 1 vs text 2): " << dot_product << std::endl;
        }
        
        // 5. Component Composition Example
        std::cout << "\n5. Testing Component Composition (Loader -> Splitter -> Embedder)..." << std::endl;
        {
            auto loader = std::make_shared<eino::components::SimpleLoader>();
            auto splitter = std::make_shared<eino::components::TextSplitter>(200, 50);
            auto embedder = std::make_shared<eino::components::SimpleEmbedder>(256);
            
            // Load document
            eino::schema::Source source;
            source.uri = "document.txt";
            auto docs = loader->Invoke(ctx, source);
            std::cout << "   - Loaded " << docs.size() << " documents" << std::endl;
            
            // Split documents
            auto chunks = splitter->Invoke(ctx, docs);
            std::cout << "   - Split into " << chunks.size() << " chunks" << std::endl;
            
            // Extract text and embed
            std::vector<std::string> texts;
            for (const auto& chunk : chunks) {
                texts.push_back(chunk.page_content);
            }
            
            if (!texts.empty()) {
                auto embeddings = embedder->Invoke(ctx, texts);
                std::cout << "   - Generated " << embeddings.size() << " embeddings" << std::endl;
                std::cout << "   - Embedding dimension: " << embeddings[0].size() << std::endl;
            }
        }
        
        // 6. Component Types Information
        std::cout << "\n6. Component Type Constants..." << std::endl;
        {
            std::cout << "   - Prompt Component: " << eino::components::kComponentOfPrompt << std::endl;
            std::cout << "   - ChatModel Component: " << eino::components::kComponentOfChatModel << std::endl;
            std::cout << "   - Embedding Component: " << eino::components::kComponentOfEmbedding << std::endl;
            std::cout << "   - Retriever Component: " << eino::components::kComponentOfRetriever << std::endl;
            std::cout << "   - Tool Component: " << eino::components::kComponentOfTool << std::endl;
        }
        
        // 7. Document Metadata Example
        std::cout << "\n7. Testing Document Metadata..." << std::endl;
        {
            eino::schema::Document doc;
            doc.id = "doc_123";
            doc.page_content = "Sample document content";
            
            // Set metadata
            doc.WithScore(0.95);
            doc.SetMetadata("source", "database");
            doc.SetMetadata("author", "John Doe");
            
            std::cout << "   - Document ID: " << doc.id << std::endl;
            std::cout << "   - Score: " << doc.GetScore() << std::endl;
            auto author = doc.GetMetadata("author");
            if (author.is_string()) {
                std::cout << "   - Author: " << std::string(author) << std::endl;
            }
        }
        
        std::cout << "\n=== All examples completed successfully! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
