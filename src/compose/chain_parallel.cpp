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

#include "eino/compose/chain_parallel.h"
#include <stdexcept>
#include <sstream>

namespace eino {
namespace compose {

// Parallel implementation

Parallel::Parallel() {}

Parallel::~Parallel() = default;

Parallel* Parallel::AddPassthrough(
    const std::string& output_key,
    const std::vector<GraphAddNodeOpt>& opts) {
    
    if (!error_.empty()) {
        return this;
    }
    
    if (output_key.empty()) {
        error_ = "Parallel AddPassthrough: output_key cannot be empty";
        return this;
    }
    
    if (output_keys_.find(output_key) != output_keys_.end()) {
        error_ = "Parallel AddPassthrough: duplicate output_key=" + output_key;
        return this;
    }
    
    output_keys_[output_key] = true;
    nodes_.emplace_back(output_key, nullptr, "Passthrough", opts);
    
    return this;
}

const std::vector<Parallel::NodeEntry>& Parallel::GetNodes() const {
    return nodes_;
}

bool Parallel::HasError() const {
    return !error_.empty();
}

const std::string& Parallel::GetError() const {
    return error_;
}

void Parallel::Clear() {
    nodes_.clear();
    output_keys_.clear();
    error_.clear();
}

size_t Parallel::GetNodeCount() const {
    return nodes_.size();
}

bool Parallel::HasOutputKey(const std::string& key) const {
    return output_keys_.find(key) != output_keys_.end();
}

std::vector<std::string> Parallel::GetOutputKeys() const {
    std::vector<std::string> keys;
    keys.reserve(output_keys_.size());
    for (const auto& pair : output_keys_) {
        keys.push_back(pair.first);
    }
    return keys;
}

void Parallel::Validate() const {
    if (HasError()) {
        throw std::runtime_error("Parallel validation failed: " + error_);
    }
    
    if (nodes_.empty()) {
        throw std::runtime_error("Parallel validation failed: no nodes added");
    }
    
    if (nodes_.size() < 2) {
        std::stringstream ss;
        ss << "Parallel validation failed: need at least 2 nodes, got " 
           << nodes_.size();
        throw std::runtime_error(ss.str());
    }
}

// Factory function
std::shared_ptr<Parallel> NewParallel() {
    return std::make_shared<Parallel>();
}

} // namespace compose
} // namespace eino
