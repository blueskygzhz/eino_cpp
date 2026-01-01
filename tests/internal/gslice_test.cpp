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

#include "eino/internal/gslice.h"
#include <gtest/gtest.h>
#include <string>

using namespace eino::internal::gslice;

struct Foo {
    int id;
    std::string name;
};

TEST(GSliceTest, ToMapEmpty) {
    std::vector<Foo> empty;
    auto mapper = [](const Foo& f) -> std::pair<int, std::string> {
        return {f.id, f.name};
    };
    
    auto result = ToMap<Foo, int, std::string>(empty, mapper);
    EXPECT_TRUE(result.empty());
}

TEST(GSliceTest, ToMapBasic) {
    std::vector<Foo> foos = {{1, "one"}, {2, "two"}, {3, "three"}};
    
    auto mapper = [](const Foo& f) -> std::pair<int, std::string> {
        return {f.id, f.name};
    };
    
    auto result = ToMap<Foo, int, std::string>(foos, mapper);
    
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[1], "one");
    EXPECT_EQ(result[2], "two");
    EXPECT_EQ(result[3], "three");
}

TEST(GSliceTest, FilterBasic) {
    std::vector<int> nums = {1, 2, 3, 4, 5};
    
    auto is_even = [](int n) { return n % 2 == 0; };
    auto result = Filter(nums, is_even);
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], 2);
    EXPECT_EQ(result[1], 4);
}

TEST(GSliceTest, MapBasic) {
    std::vector<int> nums = {1, 2, 3};
    
    auto to_string = [](int n) { return std::to_string(n); };
    auto result = Map<int, std::string>(nums, to_string);
    
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "1");
    EXPECT_EQ(result[1], "2");
    EXPECT_EQ(result[2], "3");
}

TEST(GSliceTest, ContainsTrue) {
    std::vector<int> nums = {1, 2, 3, 4, 5};
    EXPECT_TRUE(Contains(nums, 3));
}

TEST(GSliceTest, ContainsFalse) {
    std::vector<int> nums = {1, 2, 3, 4, 5};
    EXPECT_FALSE(Contains(nums, 10));
}

TEST(GSliceTest, UniqueBasic) {
    std::vector<int> nums = {1, 2, 2, 3, 3, 3, 4, 5, 5};
    auto result = Unique(nums);
    
    EXPECT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[1], 2);
    EXPECT_EQ(result[2], 3);
    EXPECT_EQ(result[3], 4);
    EXPECT_EQ(result[4], 5);
}
