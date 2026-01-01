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

#include "eino/internal/channel.h"
#include <gtest/gtest.h>
#include <thread>

using namespace eino::internal;

TEST(UnboundedChanTest, BasicSendReceive) {
    auto chan = NewUnboundedChan<int>();
    
    // Send some values
    chan->Send(1);
    chan->Send(2);
    chan->Send(3);
    
    // Receive values
    int val;
    ASSERT_TRUE(chan->Receive(val));
    EXPECT_EQ(val, 1);
    
    ASSERT_TRUE(chan->Receive(val));
    EXPECT_EQ(val, 2);
    
    ASSERT_TRUE(chan->Receive(val));
    EXPECT_EQ(val, 3);
    
    chan->Close();
}

TEST(UnboundedChanTest, CloseChannel) {
    auto chan = NewUnboundedChan<int>();
    
    chan->Send(1);
    chan->Close();
    
    int val;
    ASSERT_TRUE(chan->Receive(val));
    EXPECT_EQ(val, 1);
    
    // Receiving from closed empty channel should return false
    ASSERT_FALSE(chan->Receive(val));
}

TEST(UnboundedChanTest, SendOnClosedChannel) {
    auto chan = NewUnboundedChan<int>();
    chan->Close();
    
    EXPECT_THROW(chan->Send(1), std::runtime_error);
}

TEST(UnboundedChanTest, ThreadedSendReceive) {
    auto chan = NewUnboundedChan<int>();
    
    std::thread sender([chan]() {
        for (int i = 1; i <= 100; i++) {
            chan->Send(i);
        }
        chan->Close();
    });
    
    std::thread receiver([chan]() {
        int val;
        int count = 0;
        while (chan->Receive(val)) {
            count++;
            EXPECT_EQ(val, count);
        }
        EXPECT_EQ(count, 100);
    });
    
    sender.join();
    receiver.join();
}

TEST(UnboundedChanTest, Size) {
    auto chan = NewUnboundedChan<std::string>();
    
    EXPECT_EQ(chan->Size(), 0);
    
    chan->Send("A");
    chan->Send("B");
    EXPECT_EQ(chan->Size(), 2);
    
    std::string val;
    chan->Receive(val);
    EXPECT_EQ(chan->Size(), 1);
    
    chan->Close();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
