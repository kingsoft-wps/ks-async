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

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
#include <algorithm>

void simTask(ks_spinlock& lock, long long duration) {
    lock.lock();
    // std::cout << "Thread " << std::this_thread::get_id() << " is doing a long task..." << std::endl;

    std::atomic<bool> keep_running{true};
    auto start_time = std::chrono::steady_clock::now();

    // 循环直到超时
    while (keep_running) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();

        if (elapsed >= duration) {
            keep_running = false;
        }

        volatile double x = 12345.6789;
        for (int i = 0; i < 1000; ++i) {
            x = std::sqrt(x);
        }
    }
    // std::cout << "Thread " << std::this_thread::get_id() << " finished long task." << std::endl;
    lock.unlock();
}

TEST(test_spinlock_basic_suite, test_lock_size) {
    ks_spinlock lock;
    EXPECT_EQ(sizeof(lock), sizeof(std::atomic<uintptr_t>)) << "Size of ks_spinlock should be 4(32bits)/8(64bits) bytes.";
}

TEST(test_spinlock_basic_suite, test_trylock) {
    ks_spinlock lock;
    bool lock_acquired_1 = lock.try_lock();
    EXPECT_TRUE(lock_acquired_1) << "First try_lock() should succeed.";
    bool lock_acquired_2 = lock.try_lock();
    EXPECT_FALSE(lock_acquired_2) << "Second try_lock() should fail.";
    lock.unlock();
}

TEST(test_spinlock_basic_suite, test_lock) {
    ks_spinlock lock;
    lock.lock();
    bool lock_acquired = lock.try_lock();
    EXPECT_FALSE(lock_acquired) << "try_lock() should fail.";
    lock.unlock();
}

TEST(test_spinlock_basic_suite, test_unlock) {
    ks_spinlock lock;
    lock.lock();
    lock.unlock();
    bool lock_acquired = lock.try_lock();
    EXPECT_TRUE(lock_acquired) << "try_lock() should succeed after unlock.";
    lock.unlock();
}

// 自旋锁有效性测试
TEST(test_spindlock_mutil_thread_suite, test_lock_and_unlock) {
    ks_spinlock lock;
    long long counter = 0;

    int threadNum = 10;
    int loopNum = 1000000;
    std::vector<std::thread> threads(threadNum);
    for (int i = 0; i < threadNum; ++i) {
        threads[i] = std::thread([&]() {
            for (int j = 0; j < loopNum; ++j) {
                lock.lock();
                counter++;
                lock.unlock();
            }
        });
    }

    for (int i = 0; i < threadNum; ++i) {
        threads[i].join();
    }

    EXPECT_EQ(counter, static_cast<long long>(threadNum) * loopNum)
        << "Counter value does not match the expected result.";
}

TEST(test_spindlock_mutil_thread_suite, test_trylock_and_unlock) {
    ks_spinlock lock;
    long long counter = 0;

    int threadNum = 10;
    int loopNum = 1000000;
    std::vector<std::thread> threads(threadNum);
    for (int i = 0; i < threadNum; ++i) {
        threads[i] = std::thread([&]() {
            for (int j = 0; j < loopNum; ++j) {
                while(!lock.try_lock()){}
                counter++;
                lock.unlock();
            }
        });
    }

    for (int i = 0; i < threadNum; ++i) {
        threads[i].join();
    }

    EXPECT_EQ(counter, static_cast<long long>(threadNum) * loopNum)
        << "Counter value does not match the expected result.";
}

TEST(test_spindlock_mutil_thread_suite, test_random) {
    ks_spinlock lock;
    long long counter = 0;
    int threadNum = 10;
    int loopNum = 10;
    std::vector<std::thread> threads(threadNum);
    for (int i = 0; i < threadNum; ++i) {
        threads[i] = std::thread([&]() {
            // 任务时长随机
            for (int j = 0; j < loopNum; ++j) {
                // 随机选择锁定方式
                if (rand() % 2 == 0) {
                    lock.lock();
                } else {
                    while(!lock.try_lock()){}
                }

                // 模拟随机任务
                counter++;
                int taskDuration = rand() % 3; // 0-9毫秒
                std::this_thread::sleep_for(std::chrono::milliseconds(taskDuration));

                lock.unlock();
            }
        });
    }

    for (int i = 0; i < threadNum; ++i) {
        threads[i].join();
    }

    EXPECT_EQ(counter, static_cast<long long>(threadNum) * loopNum)
        << "Counter value does not match the expected result.";

}

// 单个长任务+N个短任务，等待测试
TEST(test_spindlock_mutil_thread_suite, test_wait_feature) {
    ks_spinlock lock;
    // 长短任务
    long long longTaskDuration = 1000;
    long long shortTaskDuration = 10;
    // 长任务持有锁
    std::thread longTaskThread([&]() {
        simTask(lock, longTaskDuration);
    });
    // N个短任务等待长任务
    int shortTaskNum = 50;
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 确保长任务先开始执行
    std::vector<std::thread> shortTaskThreads(shortTaskNum);
    for (int i = 0; i < shortTaskNum; ++i) {
        shortTaskThreads[i] = std::thread([&]() {
            simTask(lock, shortTaskDuration);
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 确保短任务进入等待
    // 检查短任务是否被挂起
    double cpu = 0;
    for (int i = 0; i < 10; ++i) {
        double cpuUsage = CpuUsage::GetCpuUsage();
        std::cout << "CPU usage: " << cpuUsage << "%" << std::endl;
        cpu = (std::max)(cpu, cpuUsage);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EXPECT_LE(cpu, 50.0) << "CPU usage should be low during the long task. (Probably using fallback implement of ks_spinlock)";

    longTaskThread.join();
    for (int i = 0; i < shortTaskNum; ++i) {
        shortTaskThreads[i].join();
    }

}

