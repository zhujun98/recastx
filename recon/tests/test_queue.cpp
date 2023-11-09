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

#include <recon/queue.hpp>

namespace recastx::recon::test {

TEST(ThreadSafeQueueTest, TestGeneral) {
    using DataType = std::vector<int>;

    {
        ThreadSafeQueue<DataType> queue;
        ASSERT_TRUE(queue.empty());

        DataType new_item {1, 2, 3};
        queue.tryPush(new_item);
        ASSERT_FALSE(queue.empty());

        DataType item;
        ASSERT_TRUE(queue.tryPop(item));
        ASSERT_EQ(item, new_item);

        DataType another_item;
        ASSERT_FALSE(queue.tryPop(another_item));
        ASSERT_TRUE(another_item.empty());
    }

    {
        DataType new_item1 {1, 2, 3};
        DataType new_item2 {4, 5, 6};
        ThreadSafeQueue<DataType> queue;
        queue.tryPush(new_item1);
        queue.tryPush(new_item2);

        DataType item;
        queue.waitAndPop(item, 10);
        ASSERT_EQ(item, new_item1);
        queue.waitAndPop(item);
        ASSERT_EQ(item, new_item2);
    }
}

TEST(ThreadSafeQueueTest, TestBound) {
    ThreadSafeQueue<int> queue(2);
    ASSERT_TRUE(queue.tryPush(1));
    ASSERT_TRUE(queue.tryPush(1));
    ASSERT_EQ(queue.size(), 2);
    ASSERT_FALSE(queue.tryPush(1));
    ASSERT_EQ(queue.size(), 2);
    queue.push(2);
    ASSERT_EQ(queue.size(), 2);
    
    int item;
    queue.tryPop(item);
    ASSERT_EQ(item, 1);
    queue.tryPop(item);
    ASSERT_EQ(item, 2);
}

TEST(ThreadSafeQueueTest, TestReset) {
    ThreadSafeQueue<float> queue(2);
    queue.push(1.f);
    queue.push(2.f);
    queue.push(3.f);
    queue.reset();
    ASSERT_TRUE(queue.empty());
}

TEST(ThreadSafeQueueTest, TestDataBufferMpmc) {
    ThreadSafeQueue<int> queue(400);

    int sum = 0;
    int ans = 450;
    auto p1 = std::thread([&] { 
        for (int i = 0; i < 150; ++i) queue.tryPush(1);
    });
    auto p2 = std::thread([&] { 
        for (int i = 0; i < 150; ++i) queue.tryPush(2);
    });
    auto c1 = std::thread([&] {
        int ret;
        for (int i = 0; i < 100; ++i) {
            if (queue.tryPop(ret)) sum += ret;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    auto c2 = std::thread([&] {
        int ret;
        for (int i = 0; i < 100; ++i) {
            if (queue.tryPop(ret)) sum += ret;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    auto c3 = std::thread([&] {
        int ret;
        for (int i = 0; i < 110; ++i) {
            if (queue.waitAndPop(ret, 100)) sum += ret;
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
    ASSERT_EQ(queue.size(), 0);
    ASSERT_TRUE(queue.empty());
}

} // namespace recastx::recon::test