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

#include "eino/schema/document.h"

namespace eino {
namespace schema {

Document& Document::WithSubIndexes(const std::vector<std::string>& indexes) {
    meta_data[kDocMetaDataKeySubIndexes] = indexes;
    return *this;
}

std::vector<std::string> Document::SubIndexes() const {
    auto it = meta_data.find(kDocMetaDataKeySubIndexes);
    if (it != meta_data.end() && it->second.is_array()) {
        return it->second.get<std::vector<std::string>>();
    }
    return std::vector<std::string>();
}

Document& Document::WithScore(double score) {
    meta_data[kDocMetaDataKeyScore] = score;
    return *this;
}

double Document::Score() const {
    auto it = meta_data.find(kDocMetaDataKeyScore);
    if (it != meta_data.end() && it->second.is_number()) {
        return it->second.get<double>();
    }
    return 0.0;
}

Document& Document::WithExtraInfo(const std::string& extra_info) {
    meta_data[kDocMetaDataKeyExtraInfo] = extra_info;
    return *this;
}

std::string Document::ExtraInfo() const {
    auto it = meta_data.find(kDocMetaDataKeyExtraInfo);
    if (it != meta_data.end() && it->second.is_string()) {
        return it->second.get<std::string>();
    }
    return "";
}

Document& Document::WithDSLInfo(const std::map<std::string, json>& dsl_info) {
    meta_data[kDocMetaDataKeyDSL] = dsl_info;
    return *this;
}

std::map<std::string, json> Document::DSLInfo() const {
    auto it = meta_data.find(kDocMetaDataKeyDSL);
    if (it != meta_data.end() && it->second.is_object()) {
        return it->second.get<std::map<std::string, json>>();
    }
    return std::map<std::string, json>();
}

Document& Document::WithDenseVector(const std::vector<double>& vector) {
    meta_data[kDocMetaDataKeyDenseVector] = vector;
    return *this;
}

std::vector<double> Document::DenseVector() const {
    auto it = meta_data.find(kDocMetaDataKeyDenseVector);
    if (it != meta_data.end() && it->second.is_array()) {
        return it->second.get<std::vector<double>>();
    }
    return std::vector<double>();
}

Document& Document::WithSparseVector(const std::map<int, double>& sparse) {
    meta_data[kDocMetaDataKeySparseVector] = sparse;
    return *this;
}

std::map<int, double> Document::SparseVector() const {
    auto it = meta_data.find(kDocMetaDataKeySparseVector);
    if (it != meta_data.end() && it->second.is_object()) {
        return it->second.get<std::map<int, double>>();
    }
    return std::map<int, double>();
}

} // namespace schema
} // namespace eino
