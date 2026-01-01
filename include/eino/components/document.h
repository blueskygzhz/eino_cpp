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

#ifndef EINO_CPP_COMPONENTS_DOCUMENT_H_
#define EINO_CPP_COMPONENTS_DOCUMENT_H_

#include "../compose/runnable.h"
#include "../schema/types.h"
#include <vector>
#include <memory>
#include <map>

namespace eino {
namespace components {

// Loader loads documents from a source
// Input: Source, Output: vector of Document
class Loader : public compose::Runnable<schema::Source, std::vector<schema::Document>> {
public:
    virtual ~Loader() = default;
};

// Transformer transforms documents (e.g., split, filter)
// Input: vector of Document, Output: vector of Document
class Transformer : public compose::Runnable<std::vector<schema::Document>, std::vector<schema::Document>> {
public:
    virtual ~Transformer() = default;
};

// Parser parses documents
// Input: vector of Document, Output: vector of Document
class Parser : public compose::Runnable<std::vector<schema::Document>, std::vector<schema::Document>> {
public:
    virtual ~Parser() = default;
};

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_DOCUMENT_H_
