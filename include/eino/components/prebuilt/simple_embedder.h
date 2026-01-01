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

#ifndef EINO_CPP_COMPONENTS_PREBUILT_SIMPLE_EMBEDDER_H_
#define EINO_CPP_COMPONENTS_PREBUILT_SIMPLE_EMBEDDER_H_

#include "../embedding.h"
#include <string>
#include <vector>
#include <memory>

namespace eino {
namespace components {

// SimpleEmbedder generates simple embeddings for text
// This is a mock implementation that generates random embeddings
// In production, this would call an actual embedding model
class SimpleEmbedder : public Embedder {
public:
    explicit SimpleEmbedder(size_t embedding_dim = 384);
    virtual ~SimpleEmbedder() = default;
    
    // Set embedding dimension
    void SetEmbeddingDim(size_t dim);
    
    // Invoke embeds texts
    std::vector<std::vector<double>> Invoke(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<std::string>& input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    // Stream embeds texts with streaming
    std::shared_ptr<compose::StreamReader<std::vector<std::vector<double>>>> Stream(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<std::string>& input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    std::vector<std::vector<double>> Collect(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<std::vector<std::string>>> input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    std::shared_ptr<compose::StreamReader<std::vector<std::vector<double>>>> Transform(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<std::vector<std::string>>> input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;

private:
    size_t embedding_dim_;
    
    // Generate a simple embedding for a text
    std::vector<double> GenerateEmbedding(const std::string& text);
};

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_PREBUILT_SIMPLE_EMBEDDER_H_
