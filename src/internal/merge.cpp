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

#include "eino/internal/merge.h"
#include <stdexcept>

namespace eino {
namespace internal {

// Initialize default merge functions for common types
namespace {

void InitializeDefaultMergeFuncs() {
    // Register merge function for string (concatenation)
    RegisterValuesMergeFunc<std::string>([](const std::vector<std::string>& strs) {
        std::string result;
        for (const auto& s : strs) {
            result += s;
        }
        return result;
    });
    
    // Register merge function for int (sum)
    RegisterValuesMergeFunc<int>([](const std::vector<int>& nums) {
        int sum = 0;
        for (int n : nums) {
            sum += n;
        }
        return sum;
    });
    
    // Register merge function for double (sum)
    RegisterValuesMergeFunc<double>([](const std::vector<double>& nums) {
        double sum = 0.0;
        for (double n : nums) {
            sum += n;
        }
        return sum;
    });
    
    // Register merge function for vector<string> (concatenation)
    RegisterValuesMergeFunc<std::vector<std::string>>(
        [](const std::vector<std::vector<std::string>>& vecs) {
            std::vector<std::string> result;
            for (const auto& v : vecs) {
                result.insert(result.end(), v.begin(), v.end());
            }
            return result;
        });
    
    // Register merge function for vector<int> (concatenation)
    RegisterValuesMergeFunc<std::vector<int>>(
        [](const std::vector<std::vector<int>>& vecs) {
            std::vector<int> result;
            for (const auto& v : vecs) {
                result.insert(result.end(), v.begin(), v.end());
            }
            return result;
        });
}

// Static initializer
static bool merge_initialized = (InitializeDefaultMergeFuncs(), true);

} // anonymous namespace

} // namespace internal
} // namespace eino
