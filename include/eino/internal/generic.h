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

#ifndef EINO_CPP_INTERNAL_GENERIC_H_
#define EINO_CPP_INTERNAL_GENERIC_H_

#include <memory>
#include <vector>
#include <map>
#include <typeinfo>
#include <algorithm>

namespace eino {
namespace internal {

// ============================================================================
// Generic Utilities - Aligns with eino internal/generic
// ============================================================================

// NewInstance creates an instance of type T
// Aligns with eino internal/generic.NewInstance (generic.go:18-51)
template<typename T>
T NewInstance() {
    return T();
}

// Specialization for pointers
template<typename T>
T* NewInstance() {
    return new T();
}

// TypeName returns a string representation of the type
// Aligns with eino internal/generic.TypeOf (generic.go:53-56)
template<typename T>
const char* TypeName() {
    return typeid(T).name();
}

// PtrOf returns a pointer to the value
// Aligns with eino internal/generic.PtrOf (generic.go:60-63)
template<typename T>
std::shared_ptr<T> PtrOf(const T& value) {
    return std::make_shared<T>(value);
}

// Pair represents a pair of values
// Aligns with eino internal/generic.Pair (generic.go:65-68)
template<typename F, typename S>
struct Pair {
    F first;
    S second;
    
    Pair() = default;
    Pair(const F& f, const S& s) : first(f), second(s) {}
};

// Reverse returns a new vector with elements in reversed order
// Aligns with eino internal/generic.Reverse (generic.go:70-77)
template<typename T>
std::vector<T> Reverse(const std::vector<T>& vec) {
    std::vector<T> result(vec.rbegin(), vec.rend());
    return result;
}

// CopyMap copies a map to a new map
// Aligns with eino internal/generic.CopyMap (generic.go:79-85)
template<typename K, typename V>
std::map<K, V> CopyMap(const std::map<K, V>& src) {
    return std::map<K, V>(src);
}

} // namespace internal
} // namespace eino

#endif // EINO_CPP_INTERNAL_GENERIC_H_
