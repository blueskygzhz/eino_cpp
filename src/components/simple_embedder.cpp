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

#include "eino/components/prebuilt/simple_embedder.h"
#include <cmath>
#include <numeric>
#include <functional>

namespace eino {
namespace components {

SimpleEmbedder::SimpleEmbedder(size_t embedding_dim)
    : embedding_dim_(embedding_dim) {
}

void SimpleEmbedder::SetEmbeddingDim(size_t dim) {
    embedding_dim_ = dim;
}

std::vector<double> SimpleEmbedder::GenerateEmbedding(const std::string& text) {
    std::vector<double> embedding(embedding_dim_, 0.0);
    
    // Simple embedding: hash-based with text content
    std::hash<std::string> hash_fn;
    size_t hash_val = hash_fn(text);
    
    // Generate embedding from hash
    for (size_t i = 0; i < embedding_dim_; ++i) {
        // Use deterministic pseudo-random generation
        size_t seed = hash_val ^ (i * 2654435761UL);
        // Simple LCG pseudo-random number generator
        seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL;
        embedding[i] = (static_cast<double>(seed) / 0x7fffffffUL) * 2.0 - 1.0;
    }
    
    // Normalize to unit vector
    double norm = 0.0;
    for (double val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    if (norm > 0.0) {
        for (double& val : embedding) {
            val /= norm;
        }
    }
    
    return embedding;
}

std::vector<std::vector<double>> SimpleEmbedder::Invoke(
    std::shared_ptr<compose::Context> ctx,
    const std::vector<std::string>& input,
    const std::vector<compose::Option>& opts) {
    std::vector<std::vector<double>> result;
    
    for (const auto& text : input) {
        result.push_back(GenerateEmbedding(text));
    }
    
    return result;
}

std::shared_ptr<compose::StreamReader<std::vector<std::vector<double>>>> SimpleEmbedder::Stream(
    std::shared_ptr<compose::Context> ctx,
    const std::vector<std::string>& input,
    const std::vector<compose::Option>& opts) {
    auto embeddings = Invoke(ctx, input, opts);
    auto reader = std::make_shared<compose::SimpleStreamReader<std::vector<std::vector<double>>>>();
    reader->Add(embeddings);
    return reader;
}

std::vector<std::vector<double>> SimpleEmbedder::Collect(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<compose::StreamReader<std::vector<std::string>>> input,
    const std::vector<compose::Option>& opts) {
    std::vector<std::vector<double>> result;
    std::vector<std::string> texts;
    
    while (input->Read(texts)) {
        auto embeddings = Invoke(ctx, texts, opts);
        result.insert(result.end(), embeddings.begin(), embeddings.end());
    }
    
    return result;
}

std::shared_ptr<compose::StreamReader<std::vector<std::vector<double>>>> SimpleEmbedder::Transform(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<compose::StreamReader<std::vector<std::string>>> input,
    const std::vector<compose::Option>& opts) {
    auto reader = std::make_shared<compose::SimpleStreamReader<std::vector<std::vector<double>>>>();
    std::vector<std::string> texts;
    
    while (input->Read(texts)) {
        auto embeddings = Invoke(ctx, texts, opts);
        reader->Add(embeddings);
    }
    
    return reader;
}

} // namespace components
} // namespace eino
