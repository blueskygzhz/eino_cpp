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

#ifndef EINO_CPP_COMPONENTS_INDEXER_H_
#define EINO_CPP_COMPONENTS_INDEXER_H_

#include "../compose/runnable.h"
#include "../schema/types.h"
#include <vector>
#include <memory>

namespace eino {
namespace components {

// Indexer indexes documents for later retrieval
// Input: vector of Document, Output: vector of Document (indexed)
class Indexer : public compose::Runnable<std::vector<schema::Document>, std::vector<schema::Document>> {
public:
    virtual ~Indexer() = default;
    
    // Index indexes documents
    virtual std::vector<schema::Document> Index(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Document>& documents,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) = 0;
};

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_INDEXER_H_
