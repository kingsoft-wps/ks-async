/* Copyright 2025 The Kingsoft's ks-async Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "test_base.h"
#include "test_help.h"

#include <chrono>
#include <cmath>
#include <thread>



TEST(test_waitgroup_basic_suite, test_done_and_try_wait) {
    ks_waitgroup waitgroup(5);
    waitgroup.done();
    waitgroup.done();
    EXPECT_EQ(waitgroup.try_wait(), false) << "waitgroup should not be ready yet.";
    waitgroup.done();
    waitgroup.done();
    waitgroup.done();
    EXPECT_EQ(waitgroup.try_wait(), true) << "waitgroup should be ready now.";
}

TEST(test_waitgroup_basic_suite, test_wait) {
    ks_waitgroup waitgroup(5);
    auto start_time = std::chrono::steady_clock::now();
    std::thread t([&waitgroup]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waitgroup.done();
        waitgroup.done();
        waitgroup.done();
        waitgroup.done();
        waitgroup.done();
    });
    waitgroup.wait();
    t.join();
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    EXPECT_GE(elapsed_time, 100) << "waitgroup wait time should be at least 100ms.";
}

TEST(test_waitgroup_basic_suite, test_add) {
    ks_waitgroup waitgroup(0);
    auto start_time = std::chrono::steady_clock::now();
    waitgroup.wait();
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    EXPECT_LT(elapsed_time, 5) << "waitgroup wait time should be less than 5ms.";

    waitgroup.add(5);
    EXPECT_EQ(waitgroup.try_wait(), false) << "waitgroup should not be ready yet.";
    start_time = std::chrono::steady_clock::now();
    std::thread t([&waitgroup]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waitgroup.done();
        waitgroup.done();
        waitgroup.done();
        waitgroup.done();
        waitgroup.done();
    });
    waitgroup.wait();
    t.join();
    end_time = std::chrono::steady_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    EXPECT_GE(elapsed_time, 100) << "waitgroup wait time should be at least 100ms.";
}

TEST(test_waitgroup_mutil_thread_suite, test_done_and_wait) {
    ks_waitgroup waitgroup(5);
    std::atomic<int> count(0);
    int thread_count = 10;
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&waitgroup, &count]() {
            count.fetch_add(1);
            waitgroup.wait();
            count.fetch_sub(1);
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(count.load(), thread_count) << "All threads should be waiting.";
    waitgroup.done();
    waitgroup.done();
    waitgroup.done();
    waitgroup.done();
    waitgroup.done();
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_EQ(count.load(), 0) << "All threads should have finished.";
}

TEST(test_waitgroup_mutil_thread_suite, test_done_and_wait_for) {
    ks_waitgroup waitgroup(0);
    EXPECT_TRUE(waitgroup.wait_for(std::chrono::milliseconds(10)));
    EXPECT_TRUE(waitgroup.wait_for(std::chrono::milliseconds(10)));
    waitgroup.add(1);
    EXPECT_FALSE(waitgroup.wait_for(std::chrono::milliseconds(10)));
    EXPECT_FALSE(waitgroup.wait_for(std::chrono::milliseconds(10)));
    waitgroup.done();
    EXPECT_TRUE(waitgroup.wait_for(std::chrono::milliseconds(10)));
    EXPECT_TRUE(waitgroup.wait_for(std::chrono::milliseconds(10)));
}

TEST(test_waitgroup_mutil_thread_suite, test_done_and_wait_until) {
    ks_waitgroup waitgroup(0);
    EXPECT_TRUE(waitgroup.wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(10)));
    EXPECT_TRUE(waitgroup.wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(10)));
    waitgroup.add(1);
    EXPECT_FALSE(waitgroup.wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(10)));
    EXPECT_FALSE(waitgroup.wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(10)));
    waitgroup.done();
    EXPECT_TRUE(waitgroup.wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(10)));
    EXPECT_TRUE(waitgroup.wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(10)));
}

TEST(test_waitgroup_mutil_thread_suite, test_add) {
    ks_waitgroup waitgroup(1);
    std::atomic<int> count(0);
    int thread_count = 10;
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&waitgroup, &count]() {
            waitgroup.add(1);
            count.fetch_add(1);
            waitgroup.done();
            waitgroup.wait();
            count.fetch_sub(1);
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(count.load(), thread_count) << "All threads should be waiting.";
    waitgroup.done();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(count.load(), 0) << "All threads should have finished.";
    
    for (auto& t : threads) {
        t.join();
    }
}

TEST(test_waitgroup_mutil_thread_suite, test_done_add) {
    int loop_count = 100;
    for (int i = 0; i < loop_count; ++i) {
        ks_waitgroup waitgroup(0);
        std::thread t1([&waitgroup]() {
            while(waitgroup.try_wait()) {}
            waitgroup.done();
        });
        std::thread t2([&waitgroup]() {
            waitgroup.add(1);
        });
        t1.join();
        t2.join();
        EXPECT_EQ(waitgroup.try_wait(), true) << "waitgroup should not be ready after done and add.";
    }
}