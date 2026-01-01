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

#ifndef EINO_CPP_INTERNAL_GMAP_H_
#define EINO_CPP_INTERNAL_GMAP_H_

#include <map>
#include <vector>
#include <functional>

namespace eino {
namespace internal {
namespace gmap {

// ============================================================================
// Map Utilities - Aligns with eino internal/gmap
// ============================================================================

// Concat returns the unions of maps as a new map
// Aligns with eino internal/gmap.Concat (gmap.go:17-59)
// 
// NOTE:
// - Once the key conflicts, the newer value always replace the older one
// - If the result is an empty set, always return an empty map
// 
// EXAMPLE:
//   std::map<int, int> m = {{1, 1}, {2, 2}};
//   Concat(m, {})              => {{1, 1}, {2, 2}}
//   Concat(m, {{3, 3}})        => {{1, 1}, {2, 2}, {3, 3}}
//   Concat(m, {{2, -1}})       => {{1, 1}, {2, -1}}  // "2:2" is replaced by "2:-1"
//
// AKA: Merge, Union, Combine
template<typename K, typename V>
std::map<K, V> Concat(const std::vector<std::map<K, V>>& maps) {
    // FastPath: no map or only one map given
    if (maps.empty()) {
        return std::map<K, V>();
    }
    if (maps.size() == 1) {
        return maps[0];
    }
    
    // Find max size for reservation hint
    size_t max_len = 0;
    for (const auto& m : maps) {
        if (m.size() > max_len) {
            max_len = m.size();
        }
    }
    
    std::map<K, V> result;
    
    // FastPath: all maps are empty
    if (max_len == 0) {
        return result;
    }
    
    // Concat all maps
    for (const auto& m : maps) {
        for (const auto& pair : m) {
            result[pair.first] = pair.second;
        }
    }
    
    return result;
}

// Variadic version for convenience
template<typename K, typename V>
std::map<K, V> Concat(std::initializer_list<std::map<K, V>> maps) {
    return Concat(std::vector<std::map<K, V>>(maps.begin(), maps.end()));
}

// Map applies function f to each key and value of map m
// Results of f are returned as a new map
// Aligns with eino internal/gmap.Map (gmap.go:61-72)
//
// EXAMPLE:
//   auto f = [](int k, int v) -> std::pair<std::string, std::string> {
//       return {std::to_string(k), std::to_string(v)};
//   };
//   Map({{1, 1}}, f) => {{"1", "1"}}
//   Map({}, f)       => {}
template<typename K1, typename V1, typename K2, typename V2>
std::map<K2, V2> Map(const std::map<K1, V1>& m, 
                     std::function<std::pair<K2, V2>(K1, V1)> f) {
    std::map<K2, V2> result;
    
    for (const auto& pair : m) {
        auto new_pair = f(pair.first, pair.second);
        result[new_pair.first] = new_pair.second;
    }
    
    return result;
}

// Values returns the values of the map m
// Aligns with eino internal/gmap.Values (gmap.go:74-85)
//
// EXAMPLE:
//   std::map<int, std::string> m = {{1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}};
//   Values(m) => {"1", "2", "3", "4"} // ⚠️ INDETERMINATE ORDER ⚠️
//
// WARNING: The values will be in an indeterminate order
template<typename K, typename V>
std::vector<V> Values(const std::map<K, V>& m) {
    std::vector<V> result;
    result.reserve(m.size());
    
    for (const auto& pair : m) {
        result.push_back(pair.second);
    }
    
    return result;
}

// Keys returns the keys of the map m
// Additional utility not in Go version
template<typename K, typename V>
std::vector<K> Keys(const std::map<K, V>& m) {
    std::vector<K> result;
    result.reserve(m.size());
    
    for (const auto& pair : m) {
        result.push_back(pair.first);
    }
    
    return result;
}

// Clone returns a shallow copy of map
// If the given map is empty, empty map is returned
// Aligns with eino internal/gmap.Clone (gmap.go:87-103)
//
// EXAMPLE:
//   Clone({{1, 1}, {2, 2}}) => {{1, 1}, {2, 2}}
//   Clone({})               => {}
//
// HINT: Both keys and values are copied using assignment (=), so this is a shallow clone
// AKA: Copy
template<typename K, typename V>
std::map<K, V> Clone(const std::map<K, V>& m) {
    return std::map<K, V>(m);
}

} // namespace gmap
} // namespace internal
} // namespace eino

#endif // EINO_CPP_INTERNAL_GMAP_H_
