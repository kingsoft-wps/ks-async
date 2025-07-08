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
#include <iostream>
#include <vector>
#include <thread>
#include <algorithm>


TEST(test_semaphore_basic_suite, test_default_constructor) {
    ks_semaphore sem(0);
    EXPECT_FALSE(sem.try_acquire()) << "Default constructor should not allow acquire.";

    ks_semaphore sem2(1);
    EXPECT_TRUE(sem2.try_acquire()) << "Default constructor with count 1 should allow acquire.";
}

TEST(test_semaphore_basic_suite, test_try_acquire) {
    ks_semaphore sem(2);
    bool acquired_1 = sem.try_acquire();
    EXPECT_TRUE(acquired_1) << "First try_acquire() should succeed.";
    bool acquired_2 = sem.try_acquire();
    EXPECT_TRUE(acquired_2) << "Second try_acquire() should succeed.";
    bool acquired_3 = sem.try_acquire();
    EXPECT_FALSE(acquired_3) << "Third try_acquire() should fail.";
}


TEST(test_semaphore_basic_suite, test_release) {
    ks_semaphore sem(0);
    EXPECT_FALSE(sem.try_acquire()) << "Initial try_acquire() should fail.";
    sem.release();
    EXPECT_TRUE(sem.try_acquire()) << "Release should allow try_acquire() to succeed.";
    sem.release(2);
    EXPECT_TRUE(sem.try_acquire()) << "First try_acquire() should succeed after releasing 2.";
    EXPECT_TRUE(sem.try_acquire()) << "Second try_acquire() should succeed.";
    EXPECT_FALSE(sem.try_acquire()) << "Third try_acquire() should fail.";
}

TEST(test_semaphore_basic_suite, test_acquire) {
    ks_semaphore sem(2);
    sem.acquire();
    EXPECT_TRUE(sem.try_acquire()) << "First try_acquire() should succeed.";
    EXPECT_FALSE(sem.try_acquire()) << "Second try_acquire() should fail.";
    // 创建线程2s之后release
    auto start_time = std::chrono::steady_clock::now();
    std::thread t([&sem]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        sem.release();
    });
    sem.acquire();
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    EXPECT_GE(elapsed_time, 2000) << "Acquire should wait for at least 2s.";
    t.join();
}


TEST(test_semaphore_mutil_thread_suite, test_acquire_release) {
    ks_semaphore sem(0);
    int threads_delta = 2;
    int acquired_threads_count = 10;
    int released_threads_count = acquired_threads_count - threads_delta;
    std::atomic<int> count(0);
    std::vector<std::thread> acquire_threads;
    for (int i = 0; i < acquired_threads_count; ++i) {
        acquire_threads.emplace_back([&sem, &count]() {
            sem.acquire();
            count.fetch_add(1);
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(count.load(), 0) << "No threads should have acquired the semaphore.";
    std::vector<std::thread> release_threads;
    for (int i = 0; i < released_threads_count; ++i) {
        release_threads.emplace_back([&sem]() {
            sem.release();
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(count.load(), (std::min)(acquired_threads_count, released_threads_count)) << "All threads should have acquired the semaphore.";
    sem.release(threads_delta);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(count.load(), acquired_threads_count) << "All threads should have acquired the semaphore.";
    for (int i = 0; i < acquired_threads_count; ++i) {
        acquire_threads[i].join();
    }
    for (int i = 0; i < released_threads_count; ++i) {
        release_threads[i].join();
    }
}

TEST(test_semaphore_mutil_thread_suite, test_try_acquire_release) {
    ks_semaphore sem(0);
    int thread_count = 10;

    bool loop_flag = true;

    std::atomic<int> count(0);

    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&sem, &count, &loop_flag]() {
            while (loop_flag) {
                if (sem.try_acquire()) {
                    count.fetch_add(1);
                    // std::cout << "Thread " << std::this_thread::get_id() << " acquired semaphore. Count: " << count.load() << std::endl;
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    for (int i = 0; i < 50; ++i) {
        sem.release();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    loop_flag = false;
    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }

    EXPECT_EQ(count.load(), 50) << "All threads should have acquired the semaphore exactly 50 times.";

}

TEST(test_semaphore_mutil_thread_suite, test_random) {
    auto random_test = [](int acquired_threads_count,int released_threads_count){
        ks_semaphore sem(0);
        int allTask = 100;
        std::vector<std::thread> acquire_threads;
        std::vector<std::thread> release_threads;
        // 将所有任务平均分配给acquire和release线程
        int acquire_task_count = allTask / acquired_threads_count;
        int release_task_count = allTask / released_threads_count;
        for (int i = 0; i < acquired_threads_count; ++i) {
            acquire_threads.emplace_back([&sem,acquire_task_count]() {
                for (int j = 0; j < acquire_task_count; ++j) {
                    // 随机等待 1~10ms
                    std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));
                    sem.acquire();
                }
            });
        }
        for (int i = 0; i < released_threads_count; ++i) {
            release_threads.emplace_back([&sem,release_task_count]() {
                for (int j = 0; j < release_task_count; ++j) {
                    // 随机等待
                    std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));
                    sem.release();
                }
            });
        }
        for (int i = 0; i < acquired_threads_count; ++i) {
            acquire_threads[i].join();
        }
        for (int i = 0; i < released_threads_count; ++i) {
            release_threads[i].join();
        }
    };
    
    random_test(1,10);
    random_test(10,1);
    random_test(5,5);
}

