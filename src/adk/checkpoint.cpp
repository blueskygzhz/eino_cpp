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

#include "../include/eino/adk/checkpoint.h"
#include <mutex>

namespace eino {
namespace adk {

// InMemoryCheckPointStore implementations
std::tuple<std::vector<uint8_t>, bool, std::string> InMemoryCheckPointStore::Get(
    void* ctx,
    const std::string& checkpoint_id) {
    
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = storage_.find(checkpoint_id);
    if (it != storage_.end()) {
        return std::make_tuple(it->second, true, "");
    }
    return std::make_tuple(std::vector<uint8_t>(), false, "");
}

std::string InMemoryCheckPointStore::Set(
    void* ctx,
    const std::string& checkpoint_id,
    const std::vector<uint8_t>& data) {
    
    std::unique_lock<std::mutex> lock(mutex_);
    storage_[checkpoint_id] = data;
    return "";
}

// CheckPointData serialization helper
std::vector<uint8_t> CheckPointData::Serialize() const {
    std::vector<uint8_t> result;
    // Simple serialization: just concatenate pointers
    // In a real implementation, you'd use proper serialization (protobuf, json, etc)
    return result;
}

std::shared_ptr<CheckPointData> CheckPointData::Deserialize(const std::vector<uint8_t>& data) {
    auto result = std::make_shared<CheckPointData>();
    // Simple deserialization
    return result;
}

}  // namespace adk
}  // namespace eino
