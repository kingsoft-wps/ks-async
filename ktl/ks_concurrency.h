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
#include <shared_mutex>
#include <condition_variable>
#include <atomic>


#ifndef __KS_MUTEX_DEF
#define __KS_MUTEX_DEF
using ks_mutex = std::mutex;
#endif


#ifndef __KS_CONDITION_VARIABLE_DEF
#define __KS_CONDITION_VARIABLE_DEF
using ks_condition_variable = std::condition_variable;
#endif


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
		if (n == 1)
			m_counter_nonzero_cv.notify_one();
		else
			m_counter_nonzero_cv.notify_all();
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
			if (m_counter == 0) 
				m_counter_zero_cv.notify_all();
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
