/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/daq/daq_factory.hpp"

namespace recastx::recon::test {

TEST(DaqClientTest, TestDaqBuffer) {
    using DataType = std::vector<int>;

    {
        DaqBuffer<DataType> buffer;
        ASSERT_TRUE(buffer.empty());

        DataType new_item {1, 2, 3};
        buffer.tryPush(new_item);
        ASSERT_FALSE(buffer.empty());

        DataType item;
        ASSERT_TRUE(buffer.tryPop(item));
        ASSERT_EQ(item, new_item);

        DataType another_item;
        ASSERT_FALSE(buffer.tryPop(another_item));
        ASSERT_TRUE(another_item.empty());
    }

    {
        DataType new_item1 {1, 2, 3};
        DataType new_item2 {4, 5, 6};
        DaqBuffer<DataType> buffer;
        buffer.tryPush(new_item1);
        buffer.tryPush(new_item2);

        DataType item;
        buffer.waitAndPop(item);
        ASSERT_EQ(item, new_item1);
        buffer.waitAndPop(item);
        ASSERT_EQ(item, new_item2);
    }
}

TEST(DaqClientTest, TestDataBufferBound) {
    DaqBuffer<int> buffer(2);
    ASSERT_TRUE(buffer.tryPush(1));
    ASSERT_TRUE(buffer.tryPush(1));
    ASSERT_EQ(buffer.size(), 2);
    ASSERT_FALSE(buffer.tryPush(1));
    ASSERT_EQ(buffer.size(), 2);
}

TEST(DaqClientTest, TestDataBufferMpmc) {
    DaqBuffer<int> buffer;

    int sum = 0;
    int ans = 450;
    auto p1 = std::thread([&] { 
        for (int i = 0; i < 150; ++i) buffer.tryPush(1);
    });
    auto p2 = std::thread([&] { 
        for (int i = 0; i < 150; ++i) buffer.tryPush(2);
    });
    auto c1 = std::thread([&] {
        int ret;
        for (int i = 0; i < 100; ++i) {
            if (buffer.tryPop(ret)) sum += ret;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    auto c2 = std::thread([&] {
        int ret;
        for (int i = 0; i < 100; ++i) {
            if (buffer.tryPop(ret)) sum += ret;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    auto c3 = std::thread([&] {
        int ret;
        for (int i = 0; i < 110; ++i) {
            if (buffer.waitAndPop(ret)) sum += ret;
            if (sum == ans) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    p1.join();
    p2.join();
    c1.join();
    c2.join();
    c3.join();
    ASSERT_EQ(sum, ans);
    ASSERT_EQ(buffer.size(), 0);
    ASSERT_TRUE(buffer.empty());
}

TEST(DaqClientTest, TestStdDaqClient) {
    std::string endpoint = "tcp://localhost:5555";
    EXPECT_NO_THROW(createDaqClient("default", endpoint, "pull"));
    EXPECT_NO_THROW(createDaqClient("default", endpoint, "pull", 2));
    EXPECT_NO_THROW(createDaqClient("default", endpoint, "sub"));
    EXPECT_THROW(createDaqClient("dlc", endpoint, "pull"), std::runtime_error);
    EXPECT_THROW(createDaqClient("default", endpoint, "push"), std::invalid_argument);
}

} // namespace recastx::recon::test