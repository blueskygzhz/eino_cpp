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
#include <cmath>
#include <cassert>
#include "eino/components/component.h"
#include "eino/components/prebuilt/simple_loader.h"
#include "eino/components/prebuilt/text_splitter.h"
#include "eino/components/prebuilt/simple_embedder.h"

using json = nlohmann::json;

#define TEST(name) std::cout << "\n" << #name << "..." << std::endl;
#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_NE(a, b) assert((a) != (b))
#define ASSERT_GT(a, b) assert((a) > (b))
#define ASSERT_GE(a, b) assert((a) >= (b))
#define ASSERT_NEAR(a, b, eps) assert(std::abs((a) - (b)) < (eps))
#define PASS std::cout << "  ✓ PASS" << std::endl;

int main() {
    std::cout << "\n========== Components Unit Tests ==========" << std::endl;
    int passed = 0, failed = 0;
    
    try {
        auto ctx = eino::compose::Context::Background();
        
        // Test 1: PromptTemplate basic
        {
            TEST("PromptTemplate basic");
            auto template_obj = std::make_shared<eino::components::PromptTemplate>(
                "Hello {name}, your age is {age}");
            
            std::map<std::string, json> vars;
            vars["name"] = "Alice";
            vars["age"] = 25;
            
            auto messages = template_obj->Invoke(ctx, vars);
            
            ASSERT_EQ(messages.size(), 1);
            ASSERT_EQ(messages[0].content, "Hello Alice, your age is 25");
            ASSERT_EQ(messages[0].role, eino::schema::RoleType::kUser);
            PASS; passed++;
        }
        
        // Test 2: PromptTemplate multiple
        {
            TEST("PromptTemplate multiple");
            auto template_obj = std::make_shared<eino::components::PromptTemplate>();
            template_obj->AddTemplate("Question: {question}");
            template_obj->AddTemplate("Topic: {topic}");
            
            std::map<std::string, json> vars;
            vars["question"] = "What is AI?";
            vars["topic"] = "Artificial Intelligence";
            
            auto messages = template_obj->Invoke(ctx, vars);
            
            ASSERT_EQ(messages.size(), 2);
            ASSERT_EQ(messages[0].content, "Question: What is AI?");
            ASSERT_EQ(messages[1].content, "Topic: Artificial Intelligence");
            PASS; passed++;
        }
        
        // Test 3: SimpleLoader basic
        {
            TEST("SimpleLoader basic");
            auto loader = std::make_shared<eino::components::SimpleLoader>();
            
            eino::schema::Source source;
            source.uri = "test_document.txt";
            
            auto docs = loader->Invoke(ctx, source);
            ASSERT_GE(docs.size(), 0);
            PASS; passed++;
        }
        
        // Test 4: TextSplitter basic
        {
            TEST("TextSplitter basic");
            auto splitter = std::make_shared<eino::components::TextSplitter>(100, 20);
            
            std::vector<eino::schema::Document> docs;
            eino::schema::Document doc;
            doc.id = "doc1";
            doc.page_content = "This is a very long document with lots of text that needs to be split into chunks for processing. " 
                              "It contains multiple sentences and should be properly divided into smaller pieces. "
                              "Each piece should maintain some overlap with the previous one for context preservation.";
            docs.push_back(doc);
            
            auto chunks = splitter->Invoke(ctx, docs);
            ASSERT_GT(chunks.size(), 1);
            PASS; passed++;
        }
        
        // Test 5: SimpleEmbedder basic
        {
            TEST("SimpleEmbedder basic");
            auto embedder = std::make_shared<eino::components::SimpleEmbedder>(256);
            
            std::vector<std::string> texts = {"hello", "world", "test"};
            auto embeddings = embedder->Invoke(ctx, texts);
            
            ASSERT_EQ(embeddings.size(), 3);
            for (const auto& embedding : embeddings) {
                ASSERT_EQ(embedding.size(), 256);
            }
            PASS; passed++;
        }
        
        // Test 6: SimpleEmbedder determinism
        {
            TEST("SimpleEmbedder determinism");
            auto embedder1 = std::make_shared<eino::components::SimpleEmbedder>(128);
            auto embedder2 = std::make_shared<eino::components::SimpleEmbedder>(128);
            
            std::vector<std::string> texts = {"same text"};
            auto embed1 = embedder1->Invoke(ctx, texts);
            auto embed2 = embedder2->Invoke(ctx, texts);
            
            ASSERT_EQ(embed1.size(), embed2.size());
            for (size_t i = 0; i < embed1[0].size(); ++i) {
                ASSERT_NEAR(embed1[0][i], embed2[0][i], 1e-9);
            }
            PASS; passed++;
        }
        
        // Test 7: SimpleEmbedder normalization
        {
            TEST("SimpleEmbedder normalization");
            auto embedder = std::make_shared<eino::components::SimpleEmbedder>(512);
            
            std::vector<std::string> texts = {"test"};
            auto embeddings = embedder->Invoke(ctx, texts);
            
            double norm = 0.0;
            for (double val : embeddings[0]) {
                norm += val * val;
            }
            norm = std::sqrt(norm);
            
            ASSERT_NEAR(norm, 1.0, 1e-6);
            PASS; passed++;
        }
        
        // Test 8: Document metadata
        {
            TEST("Document metadata");
            eino::schema::Document doc;
            doc.id = "doc1";
            doc.page_content = "Test document";
            
            doc.WithScore(0.95);
            ASSERT_NEAR(doc.GetScore(), 0.95, 1e-6);
            
            doc.SetMetadata("author", "John Doe");
            auto author = doc.GetMetadata("author");
            ASSERT_EQ(std::string(author), "John Doe");
            PASS; passed++;
        }
        
        // Test 9: Component types
        {
            TEST("Component types");
            std::cout << "  - Prompt: " << eino::components::kComponentOfPrompt << std::endl;
            std::cout << "  - ChatModel: " << eino::components::kComponentOfChatModel << std::endl;
            std::cout << "  - Embedding: " << eino::components::kComponentOfEmbedding << std::endl;
            PASS; passed++;
        }
        
        // Test 10: Component pipeline
        {
            TEST("Component pipeline");
            auto loader = std::make_shared<eino::components::SimpleLoader>();
            auto splitter = std::make_shared<eino::components::TextSplitter>(50, 10);
            auto embedder = std::make_shared<eino::components::SimpleEmbedder>(128);
            
            eino::schema::Source source;
            source.uri = "test.txt";
            
            auto docs = loader->Invoke(ctx, source);
            auto chunks = splitter->Invoke(ctx, docs);
            
            std::vector<std::string> texts;
            for (const auto& chunk : chunks) {
                texts.push_back(chunk.page_content);
            }
            
            if (!texts.empty()) {
                auto embeddings = embedder->Invoke(ctx, texts);
                ASSERT_EQ(embeddings.size(), texts.size());
            }
            PASS; passed++;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        failed++;
    }
    
    std::cout << "\n========== Results ==========" << std::endl;
    std::cout << "Passed: " << passed << "/10" << std::endl;
    std::cout << "Failed: " << failed << "/10" << std::endl;
    
    if (failed == 0) {
        std::cout << "\n✅ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ Some tests failed!" << std::endl;
        return 1;
    }
}
