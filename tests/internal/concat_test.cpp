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

#include "eino/internal/concat.h"
#include <gtest/gtest.h>

using namespace eino::internal;

TEST(ConcatTest, ConcatStrings) {
    std::vector<std::string> strings = {"Hello", " ", "World", "!"};
    auto result = ConcatStrings(strings);
    EXPECT_EQ(result, "Hello World!");
}

TEST(ConcatTest, ConcatInts) {
    InitDefaultConcatFuncs();
    
    std::vector<int> ints = {1, 2, 3, 4, 5};
    auto result = ConcatItems(ints);
    EXPECT_EQ(result, 5);  // Should use last
}

TEST(ConcatTest, RegisterCustomFunc) {
    // Register a custom concat function for int (sum instead of last)
    RegisterStreamChunkConcatFunc<int>([](const std::vector<int>& items) {
        int sum = 0;
        for (int i : items) {
            sum += i;
        }
        return sum;
    });
    
    std::vector<int> ints = {1, 2, 3, 4, 5};
    auto result = ConcatItems(ints);
    EXPECT_EQ(result, 15);  // Sum: 1+2+3+4+5
}

TEST(ConcatTest, EmptyVector) {
    std::vector<std::string> empty;
    EXPECT_THROW(ConcatItems(empty), std::runtime_error);
}

TEST(ConcatTest, SingleElement) {
    std::vector<std::string> single = {"Only"};
    auto result = ConcatItems(single);
    EXPECT_EQ(result, "Only");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
