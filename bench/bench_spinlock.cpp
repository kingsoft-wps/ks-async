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

#ifndef GBENCH_SPINLOCK_H
#define GBENCH_SPINLOCK_H

#include "bench_base.h"


static void SpinlockBasicBench_LockAndUnlock(benchmark::State& state) {
    ks_spinlock spinlock;
    for (auto _ : state) {
        spinlock.lock();
        spinlock.unlock();
    }
}
BENCHMARK(SpinlockBasicBench_LockAndUnlock)->Iterations(1000000000);

static void SpinlockBasicBench_TryLock(benchmark::State& state) {
    ks_spinlock spinlock;
    for (auto _ : state) {
        if (spinlock.try_lock()) {
            spinlock.unlock();
        }
    }

}
BENCHMARK(SpinlockBasicBench_TryLock)->Iterations(100000000);


void simTask(ks_spinlock& lock, long long duration)
{
    lock.lock();
    // std::cout << "Thread " << std::this_thread::get_id() << " is doing a long task..." << std::endl;

    // 使用原子操作防止优化
    std::atomic<bool> keep_running{true};
    auto start_time = std::chrono::steady_clock::now();

    // 循环直到超时
    while (keep_running) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time).count();

        if (elapsed >= duration) {
            keep_running = false;
        }

        // 添加不会优化的计算（计算平方根）
        volatile double x = 12345.6789;
        for (int i = 0; i < 1000; ++i) {
            x = std::sqrt(x); // 实际计算操作
        }
    }
    // std::cout << "Thread " << std::this_thread::get_id() << " finished long task." << std::endl;
    lock.unlock();
}


static void SpinlockBasicBench_SimTaskCpu(benchmark::State& state) {
    ks_spinlock spinlock;

    for (auto _ : state) { // 基准测试循环
        int num_threads = state.range(0); // 线程数
        int task_duration = state.range(1); // 任务时长（毫秒）
        int num_iterations = state.range(2); // 迭代次数

        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        // 启动多个线程
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&]() {
                for (int j = 0; j < num_iterations; ++j) {
                    simTask(spinlock, task_duration);
                }
            });
        }

        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
    }
}
// 配置线程数和任务时长的范围
BENCHMARK(SpinlockBasicBench_SimTaskCpu)
    ->Args({1, 0, 100000})         // 单线程短任务反复获取/释放锁
    ->Args({1, 100 * 1000, 1})  // 单线程长任务反复获取/释放锁
    ->Args({cpu_thread()/2, 0, 1000})        // 多线程(小于核心数)短任务反复获取/释放锁
    ->Args({cpu_thread()/2, 100 * 1000, 1}) // 多线程(小于核心数)长任务反复获取/释放锁
    ->Args({cpu_thread(), 0, 1000})        // 多线程(等于核心数)短任务反复获取/释放锁
    ->Args({cpu_thread(), 100 * 1000, 1}) // 多线程(等于核心数)长任务反复获取/释放锁
    ->Args({cpu_thread()*2, 0, 1000})        // 多线程(大于核心数)短任务反复获取/释放锁
    ->Args({cpu_thread()*2, 100 * 1000, 1}) // 多线程(大于核心数)长任务反复获取/释放锁
    ->Unit(benchmark::kMillisecond)
    ->Iterations(2);


static void SpinlockBasicBench_SimTaskTime(benchmark::State& state) {
    ks_spinlock spinlock;

    for (auto _ : state) { // 基准测试循环
        int num_threads = state.range(0); // 线程数
        int thread_loop_num = state.range(1); // 每个线程循环次数
        int task_loop_num = state.range(2); // 每个任务循环次数

        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        ks_spinlock lock;

        long long int counter = 0;

        // 启动多个线程
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&]() {
                for (int j = 0; j < thread_loop_num; ++j) {
                    // 每个线程执行多次任务
                    lock.lock();
                    for (int k = 0; k < task_loop_num; ++k) {
                        counter++;
                    }
                    lock.unlock();
                }
            });
        }

        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }

        // 检查结果
        long long expected_count = static_cast<long long>(num_threads) * thread_loop_num * task_loop_num;
        if (counter != expected_count) {
            state.SkipWithError("Counter value does not match the expected result.");
        }
    }
}
BENCHMARK(SpinlockBasicBench_SimTaskTime)
    ->Args({1, 1, 100000000})       // 单线程，长任务，无竞争
    ->Args({1, 100000, 1000})         // 单线程, 短任务，无竞争（反复上锁）
    ->Args({cpu_thread()/2, 1, 100000000})      // 多线程(小于核心数)，长任务，低竞争
    ->Args({cpu_thread()/2, 10000, 1000})        // 多线程(小于核心数)，短任务，高竞争
    ->Args({cpu_thread(), 1, 100000000})      // 多线程(等于核心数)，长任务，低竞争
    ->Args({cpu_thread(), 10000, 1000})        // 多线程(等于核心数)，短任务，高竞争
    ->Args({cpu_thread()*2, 1, 100000000})      // 多线程(大于核心数)，长任务，低竞争
    ->Args({cpu_thread()*2, 10000, 1000})        // 多线程(大于核心数)，短任务，高竞争

    ->Unit(benchmark::kMillisecond)
    ->Iterations(5);



long long int counters[2000] = {0};
static void SpinlockBasicBench_SimTaskCo(benchmark::State& state) {
    for (auto _ : state) {
        int num_threads = state.range(0); // 线程数
        long long int task_num = state.range(1); // 任务总数
        long long int task_load = state.range(2); // 每个任务的负载

        long long int job_num = task_num / num_threads; // 每个线程的任务数

        std::vector<std::thread> threads;
        threads.reserve(num_threads);
        ks_spinlock spinlock;

        // 启动多个线程
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&spinlock, i, job_num, task_load]() {
                // 将job拆分为每份task_load
                for (long long int j = 0; j < job_num / task_load; ++j) {
                    spinlock.lock();
                    for (long long int k = 0; k < task_load; ++k) {
                        counters[i*16]++;
                    }
                    spinlock.unlock();
                }
            });
        }

        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }

    }
}
BENCHMARK(SpinlockBasicBench_SimTaskCo)
    ->Args({1,  100000000, 100000000})
    ->Args({1,  100000000, 10})
    ->Args({2,  100000000, 10})
    ->Args({4,  100000000, 10})
    ->Args({8,  100000000, 10})
    ->Args({16, 100000000, 10})
    ->Args({32, 100000000, 10})
    ->Args({64, 100000000, 10})
    ->Args({1,  100000000, 100})
    ->Args({2,  100000000, 100})
    ->Args({4,  100000000, 100})
    ->Args({8,  100000000, 100})
    ->Args({16, 100000000, 100})
    ->Args({32, 100000000, 100})
    ->Args({64, 100000000, 100})
    ->Args({1,  100000000, 1000})
    ->Args({2,  100000000, 1000})
    ->Args({4,  100000000, 1000})
    ->Args({8,  100000000, 1000})
    ->Args({16, 100000000, 1000})
    ->Args({32, 100000000, 1000})
    ->Args({64, 100000000, 1000})
    ->Args({1,  100000000, 10000})
    ->Args({2,  100000000, 10000})
    ->Args({4,  100000000, 10000})
    ->Args({8,  100000000, 10000})
    ->Args({16, 100000000, 10000})
    ->Args({32, 100000000, 10000})
    ->Args({64, 100000000, 10000})

    ->Unit(benchmark::kMillisecond)
    ->Iterations(5);


#endif // GBENCH_SPINLOCK_H