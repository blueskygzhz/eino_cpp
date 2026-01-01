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

#ifndef EINO_CPP_INTERNAL_CONCAT_H_
#define EINO_CPP_INTERNAL_CONCAT_H_

#include <vector>
#include <map>
#include <string>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <typeindex>

namespace eino {
namespace internal {

// ============================================================================
// Stream Chunk Concatenation - Aligns with eino internal/concat.go
// ============================================================================

// ConcatFunc is a function that concatenates multiple chunks into one
template<typename T>
using ConcatFunc = std::function<T(const std::vector<T>&)>;

// Global concat function registry
template<typename T>
class ConcatFuncRegistry {
public:
    static ConcatFuncRegistry& Instance() {
        static ConcatFuncRegistry instance;
        return instance;
    }

    void Register(ConcatFunc<T> func) {
        func_ = func;
    }

    bool HasFunc() const {
        return static_cast<bool>(func_);
    }

    ConcatFunc<T> GetFunc() const {
        return func_;
    }

private:
    ConcatFuncRegistry() = default;
    ConcatFunc<T> func_;
};

// RegisterStreamChunkConcatFunc registers a concat function for type T
// Aligns with eino internal.RegisterStreamChunkConcatFunc (concat.go:66-68)
template<typename T>
void RegisterStreamChunkConcatFunc(ConcatFunc<T> func) {
    ConcatFuncRegistry<T>::Instance().Register(func);
}

// UseLast returns the last element in the vector
// Aligns with eino internal.useLast (concat.go:48-50)
template<typename T>
T UseLast(const std::vector<T>& items) {
    if (items.empty()) {
        throw std::runtime_error("cannot use last on empty vector");
    }
    return items.back();
}

// ConcatStrings concatenates multiple strings
// Aligns with eino internal.concatStrings (concat.go:52-64)
inline std::string ConcatStrings(const std::vector<std::string>& strings) {
    size_t total_size = 0;
    for (const auto& s : strings) {
        total_size += s.size();
    }
    
    std::string result;
    result.reserve(total_size);
    
    for (const auto& s : strings) {
        result += s;
    }
    
    return result;
}

// ConcatItems concatenates multiple items
// Aligns with eino internal.ConcatItems (concat.go:87-108)
template<typename T>
T ConcatItems(const std::vector<T>& items) {
    if (items.empty()) {
        throw std::runtime_error("cannot concat empty items");
    }
    
    if (items.size() == 1) {
        return items[0];
    }
    
    // Check if there's a registered concat function
    auto& registry = ConcatFuncRegistry<T>::Instance();
    if (registry.HasFunc()) {
        return registry.GetFunc()(items);
    }
    
    // Default: use last
    return UseLast(items);
}

// Initialize default concat functions
// Aligns with eino internal concat.go lines 29-46
inline void InitDefaultConcatFuncs() {
    RegisterStreamChunkConcatFunc<std::string>(ConcatStrings);
    RegisterStreamChunkConcatFunc<int>(UseLast<int>);
    RegisterStreamChunkConcatFunc<int8_t>(UseLast<int8_t>);
    RegisterStreamChunkConcatFunc<int16_t>(UseLast<int16_t>);
    RegisterStreamChunkConcatFunc<int32_t>(UseLast<int32_t>);
    RegisterStreamChunkConcatFunc<int64_t>(UseLast<int64_t>);
    RegisterStreamChunkConcatFunc<uint8_t>(UseLast<uint8_t>);
    RegisterStreamChunkConcatFunc<uint16_t>(UseLast<uint16_t>);
    RegisterStreamChunkConcatFunc<uint32_t>(UseLast<uint32_t>);
    RegisterStreamChunkConcatFunc<uint64_t>(UseLast<uint64_t>);
    RegisterStreamChunkConcatFunc<bool>(UseLast<bool>);
    RegisterStreamChunkConcatFunc<float>(UseLast<float>);
    RegisterStreamChunkConcatFunc<double>(UseLast<double>);
}

} // namespace internal
} // namespace eino

#endif // EINO_CPP_INTERNAL_CONCAT_H_
