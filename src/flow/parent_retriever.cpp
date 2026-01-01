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

#include "eino/flow/retriever/parent_retriever.h"
#include <algorithm>

namespace eino {
namespace flow {
namespace retriever {

std::shared_ptr<ParentRetriever> ParentRetriever::Create(
    std::shared_ptr<compose::Context> ctx,
    const Config& config) {
    auto pr = std::make_shared<ParentRetriever>();
    pr->SetRetriever(config.retriever);
    pr->SetParentIDKey(config.parent_id_key);
    pr->SetOrigDocGetter(config.orig_doc_getter);
    return pr;
}

std::vector<schema::Document> ParentRetriever::Retrieve(
    std::shared_ptr<compose::Context> ctx,
    const std::string& query,
    const std::vector<compose::Option>& opts) {
    
    if (!retriever_) {
        return std::vector<schema::Document>();
    }
    
    // Retrieve sub-documents
    auto sub_docs = retriever_->Retrieve(ctx, query, opts);
    
    // Extract parent IDs
    auto parent_ids = ExtractParentIDs(sub_docs);
    
    if (parent_ids.empty() || !orig_doc_getter_) {
        return std::vector<schema::Document>();
    }
    
    // Retrieve original documents using callback
    return orig_doc_getter_(ctx, parent_ids);
}

std::vector<std::string> ParentRetriever::ExtractParentIDs(
    const std::vector<schema::Document>& sub_docs) {
    std::vector<std::string> parent_ids;
    std::vector<bool> seen;  // Track seen IDs for deduplication
    
    for (const auto& doc : sub_docs) {
        auto it = doc.metadata.find(parent_id_key_);
        if (it != doc.metadata.end() && it->second.is_string()) {
            std::string parent_id = it->second.dump();
            // Remove quotes from JSON string
            if (parent_id.length() >= 2 && parent_id[0] == '"' && parent_id[parent_id.length()-1] == '"') {
                parent_id = parent_id.substr(1, parent_id.length() - 2);
            }
            
            // Check for duplicates
            if (std::find(parent_ids.begin(), parent_ids.end(), parent_id) == parent_ids.end()) {
                parent_ids.push_back(parent_id);
            }
        }
    }
    
    return parent_ids;
}

} // namespace retriever
} // namespace flow
} // namespace eino
