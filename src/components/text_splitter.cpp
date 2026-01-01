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

#include "eino/components/prebuilt/text_splitter.h"
#include <algorithm>

namespace eino {
namespace components {

TextSplitter::TextSplitter(size_t chunk_size, size_t overlap)
    : chunk_size_(chunk_size), overlap_(overlap) {
}

void TextSplitter::SetChunkSize(size_t size) {
    chunk_size_ = size;
}

void TextSplitter::SetOverlap(size_t overlap) {
    overlap_ = overlap;
}

std::vector<schema::Document> TextSplitter::SplitDocument(const schema::Document& doc) {
    std::vector<schema::Document> chunks;
    
    if (doc.page_content.empty()) {
        chunks.push_back(doc);
        return chunks;
    }
    
    // Simple character-based splitting
    size_t pos = 0;
    size_t chunk_count = 0;
    
    while (pos < doc.page_content.length()) {
        size_t chunk_start = pos;
        size_t chunk_end = std::min(pos + chunk_size_, doc.page_content.length());
        
        // Try to split at word boundary
        if (chunk_end < doc.page_content.length()) {
            size_t last_space = doc.page_content.rfind(' ', chunk_end);
            if (last_space != std::string::npos && last_space > chunk_start) {
                chunk_end = last_space;
            }
        }
        
        std::string chunk_content = doc.page_content.substr(chunk_start, chunk_end - chunk_start);
        
        schema::Document chunk;
        chunk.id = doc.id + "_chunk_" + std::to_string(chunk_count);
        chunk.page_content = chunk_content;
        chunk.metadata = doc.metadata;
        chunk.metadata["chunk_index"] = static_cast<int>(chunk_count);
        chunks.push_back(chunk);
        
        // Move position with overlap
        pos = chunk_end;
        if (overlap_ > 0 && pos < doc.page_content.length()) {
            pos = std::max(chunk_start, static_cast<size_t>(static_cast<int>(chunk_end) - static_cast<int>(overlap_)));
        }
        chunk_count++;
    }
    
    return chunks;
}

std::vector<schema::Document> TextSplitter::Invoke(
    std::shared_ptr<compose::Context> ctx,
    const std::vector<schema::Document>& input,
    const std::vector<compose::Option>& opts) {
    std::vector<schema::Document> result;
    
    for (const auto& doc : input) {
        auto chunks = SplitDocument(doc);
        result.insert(result.end(), chunks.begin(), chunks.end());
    }
    
    return result;
}

std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> TextSplitter::Stream(
    std::shared_ptr<compose::Context> ctx,
    const std::vector<schema::Document>& input,
    const std::vector<compose::Option>& opts) {
    auto result = Invoke(ctx, input, opts);
    auto reader = std::make_shared<compose::SimpleStreamReader<std::vector<schema::Document>>>();
    reader->Add(result);
    return reader;
}

std::vector<schema::Document> TextSplitter::Collect(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> input,
    const std::vector<compose::Option>& opts) {
    std::vector<schema::Document> result;
    std::vector<schema::Document> docs;
    
    while (input->Read(docs)) {
        auto chunks = Invoke(ctx, docs, opts);
        result.insert(result.end(), chunks.begin(), chunks.end());
    }
    
    return result;
}

std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> TextSplitter::Transform(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> input,
    const std::vector<compose::Option>& opts) {
    auto reader = std::make_shared<compose::SimpleStreamReader<std::vector<schema::Document>>>();
    std::vector<schema::Document> docs;
    
    while (input->Read(docs)) {
        auto chunks = Invoke(ctx, docs, opts);
        reader->Add(chunks);
    }
    
    return reader;
}

} // namespace components
} // namespace eino
