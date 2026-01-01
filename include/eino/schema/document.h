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

#ifndef EINO_CPP_SCHEMA_DOCUMENT_H_
#define EINO_CPP_SCHEMA_DOCUMENT_H_

#include <string>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>

namespace eino {
namespace schema {

using json = nlohmann::json;

// Document metadata keys
const std::string kDocMetaDataKeySubIndexes = "_sub_indexes";
const std::string kDocMetaDataKeyScore = "_score";
const std::string kDocMetaDataKeyExtraInfo = "_extra_info";
const std::string kDocMetaDataKeyDSL = "_dsl";
const std::string kDocMetaDataKeyDenseVector = "_dense_vector";
const std::string kDocMetaDataKeySparseVector = "_sparse_vector";

// Document is a piece of text with metadata
struct Document {
    std::string id;          // Unique identifier
    std::string content;     // Content of the document
    std::map<std::string, json> meta_data;  // Metadata
    
    Document() = default;
    Document(const std::string& id_, const std::string& content_)
        : id(id_), content(content_) {}
    
    // String returns the content of the document
    std::string String() const {
        return content;
    }
    
    // SubIndexes management
    Document& WithSubIndexes(const std::vector<std::string>& indexes);
    std::vector<std::string> SubIndexes() const;
    
    // Score management
    Document& WithScore(double score);
    double Score() const;
    
    // ExtraInfo management
    Document& WithExtraInfo(const std::string& extra_info);
    std::string ExtraInfo() const;
    
    // DSLInfo management
    Document& WithDSLInfo(const std::map<std::string, json>& dsl_info);
    std::map<std::string, json> DSLInfo() const;
    
    // DenseVector management
    Document& WithDenseVector(const std::vector<double>& vector);
    std::vector<double> DenseVector() const;
    
    // SparseVector management
    Document& WithSparseVector(const std::map<int, double>& sparse);
    std::map<int, double> SparseVector() const;
};

} // namespace schema
} // namespace eino

#endif // EINO_CPP_SCHEMA_DOCUMENT_H_
