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

#include "eino/internal/gmap.h"
#include <gtest/gtest.h>
#include <string>

using namespace eino::internal::gmap;

TEST(GMapTest, ConcatEmpty) {
    std::vector<std::map<int, int>> maps;
    auto result = Concat(maps);
    EXPECT_TRUE(result.empty());
}

TEST(GMapTest, ConcatSingle) {
    std::map<int, int> m1 = {{1, 1}, {2, 2}};
    auto result = Concat({m1});
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[1], 1);
    EXPECT_EQ(result[2], 2);
}

TEST(GMapTest, ConcatMultiple) {
    std::map<int, int> m1 = {{1, 1}, {2, 2}};
    std::map<int, int> m2 = {{3, 3}};
    auto result = Concat({m1, m2});
    
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[1], 1);
    EXPECT_EQ(result[2], 2);
    EXPECT_EQ(result[3], 3);
}

TEST(GMapTest, ConcatWithOverride) {
    std::map<int, int> m1 = {{1, 1}, {2, 2}};
    std::map<int, int> m2 = {{2, -1}, {3, 3}};
    auto result = Concat({m1, m2});
    
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[1], 1);
    EXPECT_EQ(result[2], -1);  // Newer value replaces older
    EXPECT_EQ(result[3], 3);
}

TEST(GMapTest, MapFunction) {
    std::map<int, int> m = {{1, 1}, {2, 2}};
    
    auto f = [](int k, int v) -> std::pair<std::string, std::string> {
        return {std::to_string(k), std::to_string(v)};
    };
    
    auto result = Map<int, int, std::string, std::string>(m, f);
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result["1"], "1");
    EXPECT_EQ(result["2"], "2");
}

TEST(GMapTest, Values) {
    std::map<int, std::string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto result = Values(m);
    
    EXPECT_EQ(result.size(), 3);
    // Note: order is not guaranteed in maps
    EXPECT_TRUE(std::find(result.begin(), result.end(), "one") != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(), "two") != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(), "three") != result.end());
}

TEST(GMapTest, Keys) {
    std::map<int, std::string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto result = Keys(m);
    
    EXPECT_EQ(result.size(), 3);
    EXPECT_TRUE(std::find(result.begin(), result.end(), 1) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(), 2) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(), 3) != result.end());
}

TEST(GMapTest, Clone) {
    std::map<int, int> m = {{1, 1}, {2, 2}};
    auto cloned = Clone(m);
    
    EXPECT_EQ(cloned.size(), 2);
    EXPECT_EQ(cloned[1], 1);
    EXPECT_EQ(cloned[2], 2);
    
    // Modify original
    m[1] = 100;
    
    // Clone should be unchanged
    EXPECT_EQ(cloned[1], 1);
}
