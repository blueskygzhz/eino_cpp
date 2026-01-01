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

#ifndef EINO_CPP_INTERNAL_TYPES_H_
#define EINO_CPP_INTERNAL_TYPES_H_

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace eino {
namespace internal {

// OrderedMap maintains insertion order while providing map-like access
template<typename T>
class OrderedMap {
public:
    OrderedMap() = default;
    
    void Insert(const std::string& key, const T& value) {
        if (map_.find(key) == map_.end()) {
            keys_.push_back(key);
        }
        map_[key] = value;
    }
    
    T& Get(const std::string& key) {
        return map_[key];
    }
    
    const T& Get(const std::string& key) const {
        auto it = map_.find(key);
        if (it != map_.end()) {
            return it->second;
        }
        throw std::runtime_error("Key not found: " + key);
    }
    
    const std::vector<std::string>& GetKeys() const {
        return keys_;
    }
    
    bool Contains(const std::string& key) const {
        return map_.find(key) != map_.end();
    }
    
    size_t Size() const {
        return map_.size();
    }
    
    void Clear() {
        keys_.clear();
        map_.clear();
    }
    
private:
    std::vector<std::string> keys_;
    std::map<std::string, T> map_;
};

} // namespace internal
} // namespace eino

#endif // EINO_CPP_INTERNAL_TYPES_H_
