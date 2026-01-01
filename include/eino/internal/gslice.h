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

#ifndef EINO_CPP_INTERNAL_GSLICE_H_
#define EINO_CPP_INTERNAL_GSLICE_H_

#include <vector>
#include <map>
#include <functional>

namespace eino {
namespace internal {
namespace gslice {

// ============================================================================
// Slice Utilities - Aligns with eino internal/gslice
// ============================================================================

// ToMap collects elements of slice to map, both map keys and values are produced
// by mapping function f
// Aligns with eino internal/gslice.ToMap (gslice.go:18-31)
//
// EXAMPLE:
//   struct Foo {
//       int id;
//       std::string name;
//   };
//   auto mapper = [](const Foo& f) -> std::pair<int, std::string> {
//       return {f.id, f.name};
//   };
//   ToMap(std::vector<Foo>(), mapper) => std::map<int, std::string>()
//   std::vector<Foo> s = {{1, "one"}, {2, "two"}, {3, "three"}};
//   ToMap(s, mapper) => {{1, "one"}, {2, "two"}, {3, "three"}}
template<typename T, typename K, typename V>
std::map<K, V> ToMap(const std::vector<T>& s, 
                     std::function<std::pair<K, V>(const T&)> f) {
    std::map<K, V> m;
    
    for (const auto& e : s) {
        auto pair = f(e);
        m[pair.first] = pair.second;
    }
    
    return m;
}

// Filter returns a new slice containing only elements that satisfy the predicate
// Additional utility not in Go version
template<typename T>
std::vector<T> Filter(const std::vector<T>& s, 
                      std::function<bool(const T&)> predicate) {
    std::vector<T> result;
    
    for (const auto& e : s) {
        if (predicate(e)) {
            result.push_back(e);
        }
    }
    
    return result;
}

// Map applies function f to each element of slice s
// Results of f are returned as a new slice
// Additional utility not in Go version
template<typename T, typename R>
std::vector<R> Map(const std::vector<T>& s, 
                   std::function<R(const T&)> f) {
    std::vector<R> result;
    result.reserve(s.size());
    
    for (const auto& e : s) {
        result.push_back(f(e));
    }
    
    return result;
}

// Contains checks if the slice contains the given value
// Additional utility not in Go version
template<typename T>
bool Contains(const std::vector<T>& s, const T& value) {
    for (const auto& e : s) {
        if (e == value) {
            return true;
        }
    }
    return false;
}

// Unique returns a new slice with duplicate elements removed
// Additional utility not in Go version
template<typename T>
std::vector<T> Unique(const std::vector<T>& s) {
    std::vector<T> result;
    
    for (const auto& e : s) {
        if (!Contains(result, e)) {
            result.push_back(e);
        }
    }
    
    return result;
}

} // namespace gslice
} // namespace internal
} // namespace eino

#endif // EINO_CPP_INTERNAL_GSLICE_H_
