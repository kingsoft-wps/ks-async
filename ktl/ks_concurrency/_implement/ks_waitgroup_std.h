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

#ifndef __KS_WAITGROUP_STD_DEF
#define __KS_WAITGROUP_STD_DEF

#include "ks_concurrency_helper.h"
#include <mutex>
#include <condition_variable>


namespace _KSConcurrencyImpl {

class ks_waitgroup_std {
public:
    explicit ks_waitgroup_std(ptrdiff_t desired)
        : m_counter(desired) {
        ASSERT(desired >= 0);
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_waitgroup_std);

    void add(ptrdiff_t update) {
        std::unique_lock<std::mutex> lock(m_mutex);
        ASSERT(update >= 0);
        m_counter += update;
    }

    void done() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_counter -= 1;
        if (m_counter == 0) {
            m_cv.notify_all();
        }
        else if (m_counter < 0) {
            ASSERT(false);
            throw std::runtime_error("waitgroup::done() underflow");
        }
    }

    void wait() const {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_counter != 0) {
            m_cv.wait(lock);
        }
    }

    _NODISCARD bool try_wait() const {
		std::unique_lock<std::mutex> lock(m_mutex);
		return (m_counter == 0);
    }

    template<class Rep, class Period>
    _NODISCARD bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const {
        return this->wait_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    _NODISCARD bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_counter != 0) {
            if (m_cv.wait_until(lock, abs_time) == std::cv_status::timeout) {
                return m_counter == 0;
            }
        }
        
        return true;
    }

private:
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;
    ptrdiff_t m_counter;
};

} // namespace _KSConcurrencyImpl

#endif // __KS_WAITGROUP_STD_DEF
