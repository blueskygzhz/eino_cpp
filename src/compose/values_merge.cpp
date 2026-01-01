/*
 * Copyright 2025 CloudWeGo Authors
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

#include "eino/compose/values_merge.h"

#include <stdexcept>

namespace eino {
namespace compose {

void ValuesMergeRegistry::RegisterDefaultMergeFuncs() {
    // Register merge function for map<string, Any>
    RegisterMergeFunc<std::map<std::string, Any>>(
        [](const std::vector<std::map<std::string, Any>>& maps) {
            return MergeMaps(maps);
        });
    
    // Register merge function for strings (concatenation)
    RegisterMergeFunc<std::string>(
        [](const std::vector<std::string>& strs) {
            std::string result;
            for (const auto& s : strs) {
                result += s;
            }
            return result;
        });
    
    // Register merge function for vectors
    RegisterMergeFunc<std::vector<Any>>(
        [](const std::vector<std::vector<Any>>& vecs) {
            std::vector<Any> result;
            for (const auto& v : vecs) {
                result.insert(result.end(), v.begin(), v.end());
            }
            return result;
        });
}

std::map<std::string, Any> MergeMaps(const std::vector<std::map<std::string, Any>>& maps) {
    std::map<std::string, Any> result;
    
    for (const auto& m : maps) {
        for (const auto& [key, value] : m) {
            auto it = result.find(key);
            if (it == result.end()) {
                // Key doesn't exist, just insert
                result[key] = value;
            } else {
                // Key exists, need to merge values
                std::vector<Any> values_to_merge = {it->second, value};
                
                // Try to merge using registered merge function
                std::type_index idx(value.type());
                auto merge_fn = ValuesMergeRegistry::Instance().GetMergeFunc(idx);
                if (merge_fn) {
                    result[key] = merge_fn(values_to_merge);
                } else {
                    // No merge function, just overwrite
                    result[key] = value;
                }
            }
        }
    }
    
    return result;
}

Any MergeValues(const std::vector<Any>& vs, const MergeOptions* opts) {
    if (vs.empty()) {
        throw std::runtime_error("cannot merge empty values");
    }
    
    if (vs.size() == 1) {
        return vs[0];
    }
    
    // Get the type of the first value
    std::type_index idx(vs[0].type());
    
    // Try to get merge function for this type
    auto merge_fn = ValuesMergeRegistry::Instance().GetMergeFunc(idx);
    if (merge_fn) {
        return merge_fn(vs);
    }
    
    // Check if all values are stream readers
    bool all_streams = true;
    for (const auto& v : vs) {
        try {
            std::any_cast<std::shared_ptr<IStreamReader>>(v);
        } catch (const std::bad_any_cast&) {
            all_streams = false;
            break;
        }
    }
    
    if (all_streams) {
        // Merge streams
        std::vector<std::shared_ptr<IStreamReader>> streams;
        streams.reserve(vs.size());
        for (const auto& v : vs) {
            streams.push_back(std::any_cast<std::shared_ptr<IStreamReader>>(v));
        }
        return MergeStreams(streams, opts);
    }
    
    throw std::runtime_error("unsupported type for merge: " + std::string(idx.name()));
}

std::shared_ptr<IStreamReader> MergeStreams(
    const std::vector<std::shared_ptr<IStreamReader>>& streams,
    const MergeOptions* opts) {
    
    if (streams.empty()) {
        throw std::runtime_error("cannot merge empty streams");
    }
    
    if (streams.size() == 1) {
        return streams[0];
    }
    
    // Check that all streams have the same chunk type
    auto chunk_type = streams[0]->GetChunkType();
    for (size_t i = 1; i < streams.size(); ++i) {
        if (streams[i]->GetChunkType() != chunk_type) {
            throw std::runtime_error("chunk type mismatch in stream merge");
        }
    }
    
    // Check if the chunk type has a merge function
    std::type_index idx(chunk_type);
    auto merge_fn = ValuesMergeRegistry::Instance().GetMergeFunc(idx);
    if (!merge_fn) {
        throw std::runtime_error("chunk type does not have a merge function: " + 
                                 std::string(chunk_type.name()));
    }
    
    // Merge streams
    std::vector<std::shared_ptr<IStreamReader>> others(streams.begin() + 1, streams.end());
    
    if (opts && opts->stream_merge_with_source_eof && !opts->names.empty()) {
        return streams[0]->MergeWithNames(others, opts->names);
    } else {
        return streams[0]->Merge(others);
    }
}

} // namespace compose
} // namespace eino
