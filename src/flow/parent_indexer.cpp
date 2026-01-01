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

#include "eino/flow/indexer/parent_indexer.h"

namespace eino {
namespace flow {
namespace indexer {

std::shared_ptr<ParentIndexer> ParentIndexer::Create(
    std::shared_ptr<compose::Context> ctx,
    const Config& config) {
    auto pi = std::make_shared<ParentIndexer>();
    pi->SetIndexer(config.indexer);
    pi->SetTransformer(config.transformer);
    pi->SetParentIDKey(config.parent_id_key);
    pi->SetSubIDGenerator(config.sub_id_generator);
    return pi;
}

std::vector<std::string> ParentIndexer::Store(
    std::shared_ptr<compose::Context> ctx,
    const std::vector<schema::Document>& documents,
    const std::vector<compose::Option>& opts) {
    
    if (!transformer_ || !indexer_ || !sub_id_generator_) {
        return std::vector<std::string>();
    }
    
    // Transform documents (split into sub-documents)
    auto sub_docs = transformer_->Transform(ctx, documents);
    
    if (sub_docs.empty()) {
        return std::vector<std::string>();
    }
    
    // Process sub-documents: set parent ID metadata and generate unique IDs
    std::string current_parent_id = sub_docs[0].id;
    size_t start_idx = 0;
    
    for (size_t i = 0; i < sub_docs.size(); ++i) {
        auto& doc = sub_docs[i];
        
        // Initialize metadata if needed
        if (doc.metadata.is_null()) {
            doc.metadata = nlohmann::json::object();
        }
        
        // Store the current ID as parent ID
        std::string parent_id = doc.id;
        doc.metadata[parent_id_key_] = parent_id;
        
        // Check if we've moved to a new parent document
        if (doc.id != current_parent_id || i == sub_docs.size() - 1) {
            // Generate new IDs for the previous batch
            int batch_size = (i == sub_docs.size() - 1) ? (i - start_idx + 1) : (i - start_idx);
            auto generated_ids = sub_id_generator_(ctx, current_parent_id, batch_size);
            
            if (generated_ids.size() == static_cast<size_t>(batch_size)) {
                for (int j = 0; j < batch_size; ++j) {
                    sub_docs[start_idx + j].id = generated_ids[j];
                }
            }
            
            // Update for next batch
            if (i < sub_docs.size() - 1) {
                current_parent_id = doc.id;
                start_idx = i;
            }
        }
    }
    
    // Generate IDs for the last batch
    if (start_idx < sub_docs.size()) {
        int batch_size = sub_docs.size() - start_idx;
        auto generated_ids = sub_id_generator_(ctx, current_parent_id, batch_size);
        
        if (generated_ids.size() == static_cast<size_t>(batch_size)) {
            for (int j = 0; j < batch_size; ++j) {
                sub_docs[start_idx + j].id = generated_ids[j];
            }
        }
    }
    
    // Store the processed documents
    return indexer_->Store(ctx, sub_docs, opts);
}

} // namespace indexer
} // namespace flow
} // namespace eino
