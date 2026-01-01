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

#ifndef EINO_CPP_COMPONENTS_PREBUILT_TEXT_SPLITTER_H_
#define EINO_CPP_COMPONENTS_PREBUILT_TEXT_SPLITTER_H_

#include "../document.h"
#include <string>
#include <memory>

namespace eino {
namespace components {

// TextSplitter splits documents into chunks
class TextSplitter : public Transformer {
public:
    explicit TextSplitter(size_t chunk_size = 1000, size_t overlap = 200);
    virtual ~TextSplitter() = default;
    
    // Set chunk size
    void SetChunkSize(size_t size);
    
    // Set overlap between chunks
    void SetOverlap(size_t overlap);
    
    // Invoke splits documents
    std::vector<schema::Document> Invoke(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Document>& input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    // Stream splits documents with streaming
    std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> Stream(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Document>& input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    std::vector<schema::Document> Collect(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> Transform(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;

private:
    size_t chunk_size_;
    size_t overlap_;
    
    // Split a single document into chunks
    std::vector<schema::Document> SplitDocument(const schema::Document& doc);
};

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_PREBUILT_TEXT_SPLITTER_H_
