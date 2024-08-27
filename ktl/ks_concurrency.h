/* Copyright 2024 The Kingsoft's ks-async/ktl Authors. All Rights Reserved.

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

#pragma once

#include "ks_cxxbase.h"
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>


using ks_mutex = std::mutex;
using ks_condition_variable = std::condition_variable;


#ifndef __KS_SEMAPHORE_DEF
#define __KS_SEMAPHORE_DEF

class ks_semaphore {
public:
	explicit ks_semaphore(std::ptrdiff_t counter) : m_counter(counter) {}
	_DISABLE_COPY_CONSTRUCTOR(ks_semaphore);

	void acquire() {
		std::unique_lock<ks_mutex> lock(m_mutex);
		while (m_counter == 0) 
			m_counter_nonzero_cv.wait(lock);
		--m_counter;
	}

	bool try_acquire() {
		std::unique_lock<ks_mutex> lock(m_mutex);
		if (m_counter == 0)
			return false;
		--m_counter;
		return true;
	}

	void release(std::ptrdiff_t n = 1) {
		std::unique_lock<ks_mutex> lock(m_mutex);
		m_counter += n;
		lock.unlock();
		for (std::ptrdiff_t i = 0; i < n; ++i) 
			m_counter_nonzero_cv.notify_one();
	}

private:
	ks_mutex m_mutex;
	std::ptrdiff_t m_counter;
	ks_condition_variable m_counter_nonzero_cv{};
};

#endif //__KS_SEMAPHORE_DEF


#ifndef __KS_LATCH_DEF
#define __KS_LATCH_DEF

class ks_latch {
public:
	explicit ks_latch(std::ptrdiff_t counter) : m_counter(counter) {}
	_DISABLE_COPY_CONSTRUCTOR(ks_latch);

	void add(std::ptrdiff_t n) {
		std::unique_lock<ks_mutex> lock(m_mutex);
		m_counter += n;
	}

	void count_down(std::ptrdiff_t n = 1) {
		std::unique_lock<ks_mutex> lock(m_mutex);
		if (m_counter != 0) {
			m_counter = (m_counter > n ? m_counter - n : 0);
			if (m_counter == 0) {
				lock.unlock();
				m_counter_zero_cv.notify_all();
			}
		}
	}

	void wait() {
		std::unique_lock<ks_mutex> lock(m_mutex);
		while (m_counter != 0)
			m_counter_zero_cv.wait(lock);
	}

	bool try_wait() {
		std::unique_lock<ks_mutex> lock(m_mutex);
		return (m_counter == 0);
	}

private:
	ks_mutex m_mutex;
	std::ptrdiff_t m_counter;
	ks_condition_variable m_counter_zero_cv{};
};

#endif //__KS_LATCH_DEF


//自旋锁，请谨慎使用
//注：在Linux下应该使用futex技术实现。此外还有一个好玩的rseq技术可以去了解一下。
#ifndef __KS_SPINLOCK_DEF
#define __KS_SPINLOCK_DEF

class ks_spinlock {
public:
	ks_spinlock() : m_spin_value(0) {}
	_DISABLE_COPY_CONSTRUCTOR(ks_spinlock);

	void lock() {
		//立即尝试
		if (true) {
			int expected = 0;
			if (m_spin_value.compare_exchange_weak(expected, 1))
				return;
		}

		static const bool _HAS_MULTI_CPU = std::thread::hardware_concurrency() >= 2;
		if (_HAS_MULTI_CPU) {
			//立即循环尝试
			for (int i = 0; i < 200; ++i) { //具体值可再斟酌
				int expected = 0;
				if (m_spin_value.compare_exchange_weak(expected, 1))
					return;
			}
		}

		//间断循环尝试
		for (;;) {
			int expected = 0;
			if (m_spin_value.compare_exchange_weak(expected, 1))
				return;

			//注：对于spin来说，理论上用yield就够了，虽然用sleep更能确保线程调度。
			//此外，还可以用非原子化方式直接地快速判定值为非0，当非0时再次立即yield；
			//不过，因atomic并未提供标准的非原子化取值方法，这里我们先按MSVC实现。
			//std::this_thread::sleep_for(std::chrono::seconds(0));
			static_assert(sizeof(m_spin_value) == sizeof(int), "error");
			do {
				std::this_thread::yield();
			} while (*(volatile int*)(&m_spin_value) != 0);
		}
	}

	_NODISCARD bool try_lock() {
		int expected = 0;
		return m_spin_value.compare_exchange_strong(expected, 1);
	}

	void unlock() {
		ASSERT(m_spin_value.load() == 1);
		m_spin_value.store(0);
	}

private:
	//注：使用atomic_flag实现与使用atomic_int实现性能相近。
	//使用CS实现比std::mutex要高效些，但比atomic要差一些。
	//使用真正的内核对象Mutex自然是最低效的。
	//atomic/CS/std::mutex/Mutex耗时比（1000000次）：
	//	Release:94/140/187/2829
	//	Debug:1719/140/1172/2969
	//	要注意，Debug与Release性能表现是不同的，我们以Release为准。
	std::atomic<int> m_spin_value; //0或1
};

#endif //__KS_SPINLOCK_DEF
