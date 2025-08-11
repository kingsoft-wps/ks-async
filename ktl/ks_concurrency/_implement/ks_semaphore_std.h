/* Copyright 2025 The Kingsoft's ks-async/ktl Authors. All Rights Reserved.

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

#ifndef __KS_SEMAPHORE_STD_DEF
#define __KS_SEMAPHORE_STD_DEF

#include "ks_concurrency_helper.h"
#include <mutex>
#include <condition_variable>


namespace _KSConcurrencyImpl {

class ks_semaphore_std {
public:
    explicit ks_semaphore_std(ptrdiff_t desired)
        : m_counter(desired) {
		ASSERT(desired >= 0);
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_semaphore_std);

    void release(ptrdiff_t update = 1) {
        std::unique_lock<std::mutex> lock(m_mutex);
        ASSERT(update > 0);
        m_counter += update;

        if (update == 1) {
            m_cv.notify_one();
        }
        else {
            m_cv.notify_all();
        }
    }

    void acquire() {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_counter == 0) {
            m_cv.wait(lock);
        }

        ASSERT(m_counter > 0);
        --m_counter;
    }

    _NODISCARD bool try_acquire() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_counter == 0)
            return false;

        ASSERT(m_counter > 0);
        --m_counter;
        return true;
    }

private:
	std::mutex m_mutex;
	std::condition_variable m_cv{};
    ptrdiff_t m_counter;
};

} // namespace _KSConcurrencyImpl

#endif // __KS_SEMAPHORE_STD_DEF
