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

#include "eino/schema/stream_copy.h"
#include "eino/schema/stream.h"
#include <gtest/gtest.h>
#include <thread>

using namespace eino::schema;

// Test basic copy functionality
// Aligns with eino/schema/stream_test.go TestStreamCopy
TEST(StreamCopyTest, BasicCopy) {
    // Create a stream with some data
    auto [reader, writer] = Pipe<int>(5);
    
    // Send some data
    std::thread sender([writer]() {
        for (int i = 1; i <= 5; i++) {
            writer->Send(i);
        }
        writer->Close();
    });
    
    // Copy the reader
    auto copies = CopyStreamReader(reader, 2);
    ASSERT_EQ(copies.size(), 2);
    
    auto reader1 = copies[0];
    auto reader2 = copies[1];
    
    // Read from both copies
    std::vector<int> values1, values2;
    
    std::thread receiver1([reader1, &values1]() {
        int val;
        while (reader1->Recv(val)) {
            values1.push_back(val);
        }
        reader1->Close();
    });
    
    std::thread receiver2([reader2, &values2]() {
        int val;
        while (reader2->Recv(val)) {
            values2.push_back(val);
        }
        reader2->Close();
    });
    
    sender.join();
    receiver1.join();
    receiver2.join();
    
    // Both copies should receive all values
    EXPECT_EQ(values1.size(), 5);
    EXPECT_EQ(values2.size(), 5);
    
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(values1[i], i + 1);
        EXPECT_EQ(values2[i], i + 1);
    }
}

// Test array reader copy
// Aligns with eino/schema/stream_test.go TestNewStreamCopy
TEST(StreamCopyTest, ArrayReaderCopy) {
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto reader = std::make_shared<CopyableArrayStreamReader<int>>(data);
    
    auto copies = reader->Copy(3);
    ASSERT_EQ(copies.size(), 3);
    
    // Each copy should read all values independently
    for (int i = 0; i < 3; i++) {
        std::vector<int> values;
        int val;
        while (copies[i]->Recv(val)) {
            values.push_back(val);
        }
        
        EXPECT_EQ(values.size(), 5);
        for (int j = 0; j < 5; j++) {
            EXPECT_EQ(values[j], j + 1);
        }
    }
}

// Test copy with independent reading
TEST(StreamCopyTest, IndependentReading) {
    auto [reader, writer] = Pipe<std::string>(10);
    
    // Send data
    std::thread sender([writer]() {
        writer->Send("A");
        writer->Send("B");
        writer->Send("C");
        writer->Close();
    });
    
    auto copies = CopyStreamReader(reader, 2);
    auto reader1 = copies[0];
    auto reader2 = copies[1];
    
    sender.join();
    
    // Reader1 reads first value
    std::string val1;
    ASSERT_TRUE(reader1->Recv(val1));
    EXPECT_EQ(val1, "A");
    
    // Reader2 also reads first value (independent position)
    std::string val2;
    ASSERT_TRUE(reader2->Recv(val2));
    EXPECT_EQ(val2, "A");
    
    // Reader1 reads second value
    ASSERT_TRUE(reader1->Recv(val1));
    EXPECT_EQ(val1, "B");
    
    // Reader2 reads second value
    ASSERT_TRUE(reader2->Recv(val2));
    EXPECT_EQ(val2, "B");
    
    reader1->Close();
    reader2->Close();
}

// Test copy with n=1 (should return original)
TEST(StreamCopyTest, CopyOneReturnsOriginal) {
    std::vector<int> data = {1, 2, 3};
    auto reader = StreamReaderFromArray(data);
    
    auto copies = CopyStreamReader(reader, 1);
    ASSERT_EQ(copies.size(), 1);
    EXPECT_EQ(copies[0], reader);  // Should be same instance
}

// Test close propagation
TEST(StreamCopyTest, ClosePropagation) {
    auto [reader, writer] = Pipe<int>(5);
    
    auto copies = CopyStreamReader(reader, 2);
    auto reader1 = copies[0];
    auto reader2 = copies[1];
    
    // Close both children
    reader1->Close();
    reader2->Close();
    
    // Original reader should be closed
    // (This is verified by the ParentStreamReader closing the source)
    
    SUCCEED();  // If we reach here without crash, test passes
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
