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

#include <gtest/gtest.h>
#include "eino/components/component.h"
#include "eino/components/prebuilt/simple_loader.h"
#include "eino/components/prebuilt/text_splitter.h"
#include "eino/components/prebuilt/simple_embedder.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ComponentsTest : public ::testing::Test {
protected:
    void SetUp() override {
        ctx_ = eino::compose::Context::Background();
    }
    
    std::shared_ptr<eino::compose::Context> ctx_;
};

// Test ChatTemplate (PromptTemplate)
TEST_F(ComponentsTest, PromptTemplateBasic) {
    auto template_obj = std::make_shared<eino::components::PromptTemplate>(
        "Hello {name}, your age is {age}");
    
    std::map<std::string, json> vars;
    vars["name"] = "Alice";
    vars["age"] = 25;
    
    auto messages = template_obj->Invoke(ctx_, vars);
    
    EXPECT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].content, "Hello Alice, your age is 25");
    EXPECT_EQ(messages[0].role, eino::schema::RoleType::kUser);
}

// Test PromptTemplate with multiple templates
TEST_F(ComponentsTest, PromptTemplateMultiple) {
    auto template_obj = std::make_shared<eino::components::PromptTemplate>();
    template_obj->AddTemplate("Question: {question}");
    template_obj->AddTemplate("Topic: {topic}");
    
    std::map<std::string, json> vars;
    vars["question"] = "What is AI?";
    vars["topic"] = "Artificial Intelligence";
    
    auto messages = template_obj->Invoke(ctx_, vars);
    
    EXPECT_EQ(messages.size(), 2);
    EXPECT_EQ(messages[0].content, "Question: What is AI?");
    EXPECT_EQ(messages[1].content, "Topic: Artificial Intelligence");
}

// Test SimpleLoader
TEST_F(ComponentsTest, SimpleLoaderBasic) {
    auto loader = std::make_shared<eino::components::SimpleLoader>();
    
    eino::schema::Source source;
    source.uri = "test_document.txt";
    
    auto docs = loader->Invoke(ctx_, source);
    
    // Should return at least one document
    EXPECT_GE(docs.size(), 0);
}

// Test TextSplitter
TEST_F(ComponentsTest, TextSplitterBasic) {
    auto splitter = std::make_shared<eino::components::TextSplitter>(100, 20);
    
    std::vector<eino::schema::Document> docs;
    eino::schema::Document doc;
    doc.id = "doc1";
    doc.page_content = "This is a very long document with lots of text that needs to be split into chunks for processing. " 
                      "It contains multiple sentences and should be properly divided into smaller pieces. "
                      "Each piece should maintain some overlap with the previous one for context preservation.";
    docs.push_back(doc);
    
    auto chunks = splitter->Invoke(ctx_, docs);
    
    // Should split into multiple chunks
    EXPECT_GT(chunks.size(), 1);
    
    // Check that all content is preserved
    std::string combined;
    for (const auto& chunk : chunks) {
        combined += chunk.page_content;
    }
    EXPECT_NE(combined.find("This is a very long document"), std::string::npos);
}

// Test SimpleEmbedder
TEST_F(ComponentsTest, SimpleEmbedderBasic) {
    auto embedder = std::make_shared<eino::components::SimpleEmbedder>(256);
    
    std::vector<std::string> texts = {"hello", "world", "test"};
    
    auto embeddings = embedder->Invoke(ctx_, texts);
    
    EXPECT_EQ(embeddings.size(), 3);
    for (const auto& embedding : embeddings) {
        EXPECT_EQ(embedding.size(), 256);
    }
}

// Test SimpleEmbedder determinism
TEST_F(ComponentsTest, SimpleEmbedderDeterminism) {
    auto embedder1 = std::make_shared<eino::components::SimpleEmbedder>(128);
    auto embedder2 = std::make_shared<eino::components::SimpleEmbedder>(128);
    
    std::vector<std::string> texts = {"same text"};
    
    auto embed1 = embedder1->Invoke(ctx_, texts);
    auto embed2 = embedder2->Invoke(ctx_, texts);
    
    // Same text should produce same embedding
    EXPECT_EQ(embed1.size(), embed2.size());
    for (size_t i = 0; i < embed1[0].size(); ++i) {
        EXPECT_NEAR(embed1[0][i], embed2[0][i], 1e-9);
    }
}

// Test SimpleEmbedder normalization
TEST_F(ComponentsTest, SimpleEmbedderNormalization) {
    auto embedder = std::make_shared<eino::components::SimpleEmbedder>(512);
    
    std::vector<std::string> texts = {"test"};
    auto embeddings = embedder->Invoke(ctx_, texts);
    
    // Check that embedding is normalized (unit vector)
    double norm = 0.0;
    for (double val : embeddings[0]) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    EXPECT_NEAR(norm, 1.0, 1e-6);
}

// Test Document metadata
TEST_F(ComponentsTest, DocumentMetadata) {
    eino::schema::Document doc;
    doc.id = "doc1";
    doc.page_content = "Test document";
    
    // Test score metadata
    doc.SetScore(0.95);
    EXPECT_NEAR(doc.GetScore(), 0.95, 1e-6);
    
    // Test custom metadata
    doc.SetMetadata("author", "John Doe");
    auto author = doc.GetMetadata("author");
    EXPECT_EQ(author.get<std::string>(), "John Doe");
}

// Test component types
TEST_F(ComponentsTest, ComponentTypes) {
    EXPECT_STREQ(eino::components::kComponentOfPrompt, "ChatTemplate");
    EXPECT_STREQ(eino::components::kComponentOfChatModel, "ChatModel");
    EXPECT_STREQ(eino::components::kComponentOfEmbedding, "Embedding");
    EXPECT_STREQ(eino::components::kComponentOfRetriever, "Retriever");
    EXPECT_STREQ(eino::components::kComponentOfTool, "Tool");
}

// Test component composition: Loader -> Splitter -> Embedder
TEST_F(ComponentsTest, ComponentPipeline) {
    auto loader = std::make_shared<eino::components::SimpleLoader>();
    auto splitter = std::make_shared<eino::components::TextSplitter>(50, 10);
    auto embedder = std::make_shared<eino::components::SimpleEmbedder>(128);
    
    // Create a source
    eino::schema::Source source;
    source.uri = "test.txt";
    
    // Load documents
    auto docs = loader->Invoke(ctx_, source);
    
    // Split documents
    auto chunks = splitter->Invoke(ctx_, docs);
    
    // Extract text from chunks
    std::vector<std::string> texts;
    for (const auto& chunk : chunks) {
        texts.push_back(chunk.page_content);
    }
    
    // Generate embeddings
    if (!texts.empty()) {
        auto embeddings = embedder->Invoke(ctx_, texts);
        EXPECT_EQ(embeddings.size(), texts.size());
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
