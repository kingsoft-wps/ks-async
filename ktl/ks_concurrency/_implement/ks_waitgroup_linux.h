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

#ifndef __KS_WAITGROUP_LINUX_DEF
#define __KS_WAITGROUP_LINUX_DEF
#if defined(__linux__)

#include "ks_concurrency_helper.h"


namespace _KSConcurrencyImpl {

class ks_waitgroup_linux_futex {
public:
    explicit ks_waitgroup_linux_futex(ptrdiff_t desired)
        : m_counter(desired) {
        ASSERT(desired >= 0);
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_waitgroup_linux_futex);

    void add(ptrdiff_t update) {
        ASSERT(update > 0);
        m_counter.fetch_add(update, std::memory_order_relaxed);
    }

    void done() {
        constexpr ptrdiff_t update = 1;
		ASSERT(update > 0);
        ptrdiff_t current = m_counter.fetch_sub(update, std::memory_order_release) - update;
        ASSERT(current >= 0);
        if (current == 0) {
            _helper::__atomic_notify_all(&m_counter);
        }
    }

    void wait() const {
        for (;;) {
            ptrdiff_t current = m_counter.load(std::memory_order_acquire);
            ASSERT(current >= 0);
            if (current == 0)
                return;

            _helper::__atomic_wait_explicit(&m_counter, current, std::memory_order_relaxed);
        }
    }

    _NODISCARD bool try_wait() const {
        return m_counter.load(std::memory_order_acquire) == 0;
    }

    template<class Rep, class Period>
    _NODISCARD bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const {
        return this->wait_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    _NODISCARD bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const {
        for (;;) {
            ptrdiff_t current = m_counter.load(std::memory_order_acquire);
            ASSERT(current >= 0);
            if (current == 0)
                return true;

            if (Clock::now() >= abs_time)
                return false; //timeout

            (void)_helper::__atomic_wait_until_explicit(&m_counter, current, abs_time, std::memory_order_relaxed);
        }
    }

private:
    std::atomic<ptrdiff_t> m_counter;
};

//note: futex is avail since v2.6.x
using ks_waitgroup_linux = ks_waitgroup_linux_futex;

} // namespace _KSConcurrencyImpl

#endif // __linux__
#endif // __KS_WAITGROUP_LINUX_DEF
