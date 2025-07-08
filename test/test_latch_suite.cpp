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



TEST(test_latch_basic_suite, test_count_down_and_try_wait) {
    ks_latch latch(5);
    latch.count_down(2);
    EXPECT_EQ(latch.try_wait(), false) << "Latch should not be ready yet.";
    latch.count_down(3);
    EXPECT_EQ(latch.try_wait(), true) << "Latch should be ready now.";
}

TEST(test_latch_basic_suite, test_wait) {
    ks_latch latch(5);
    auto start_time = std::chrono::steady_clock::now();
    std::thread t([&latch]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        latch.count_down(5);
    });
    latch.wait();
    t.join();
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    EXPECT_GE(elapsed_time, 100) << "Latch wait time should be at least 100ms.";
}

TEST(test_latch_basic_suite, test_add) {
    ks_latch latch(0);
    auto start_time = std::chrono::steady_clock::now();
    latch.wait();
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    EXPECT_LT(elapsed_time, 5) << "Latch wait time should be less than 5ms.";

    latch.add(5);
    EXPECT_EQ(latch.try_wait(), false) << "Latch should not be ready yet.";
    start_time = std::chrono::steady_clock::now();
    std::thread t([&latch]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        latch.count_down(5);
    });
    latch.wait();
    t.join();
    end_time = std::chrono::steady_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    EXPECT_GE(elapsed_time, 100) << "Latch wait time should be at least 100ms.";
}

TEST(test_latch_mutil_thread_suite, test_count_down_and_wait) {
    ks_latch latch(5);
    std::atomic<int> count(0);
    int thread_count = 10;
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&latch, &count]() {
            count.fetch_add(1);
            latch.wait();
            count.fetch_sub(1);
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(count.load(), thread_count) << "All threads should be waiting.";
    latch.count_down(5);
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_EQ(count.load(), 0) << "All threads should have finished.";
}


TEST(test_latch_mutil_thread_suite, test_add) {
    ks_latch latch(1);
    std::atomic<int> count(0);
    int thread_count = 10;
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&latch, &count]() {
            latch.add();
            count.fetch_add(1);
            latch.count_down();
            latch.wait();
            count.fetch_sub(1);
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(count.load(), thread_count) << "All threads should be waiting.";
    latch.count_down(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(count.load(), 0) << "All threads should have finished.";
    
    for (auto& t : threads) {
        t.join();
    }
}

TEST(test_latch_mutil_thread_suite, test_count_down_add) {
    int loop_count = 100;
    for (int i = 0; i < loop_count; ++i) {
        ks_latch latch(0);
        std::thread t1([&latch]() {
            while(latch.try_wait()) {}
            latch.count_down();
        });
        std::thread t2([&latch]() {
            latch.add(1);
        });
        t1.join();
        t2.join();
        EXPECT_EQ(latch.try_wait(), true) << "Latch should not be ready after count_down and add.";
    }
}