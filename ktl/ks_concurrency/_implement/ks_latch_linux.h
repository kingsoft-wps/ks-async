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

#ifndef __KS_LATCH_LINUX_DEF
#define __KS_LATCH_LINUX_DEF
#if defined(__linux__)

#include "ks_concurrency_helper.h"


namespace _KSConcurrencyImpl {

class ks_latch_linux_futex {
public:
    explicit ks_latch_linux_futex(ptrdiff_t desired)
        : m_counter(desired) {
        ASSERT(desired >= 0);
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_latch_linux_futex);

    void add(ptrdiff_t update = 1) {
        ASSERT(update > 0);
        m_counter.fetch_add(update, std::memory_order_relaxed);
    }

    void count_down(ptrdiff_t update = 1) {
		ASSERT(update > 0);
        ptrdiff_t current = m_counter.fetch_sub(update, std::memory_order_release) - update;
        ASSERT(current >= 0);
        if (current == 0) {
            //即使m_counter是64位，futex也只操作低32位的地址就够了
            _helper::linux_futex32_wake_all((uint32_t*)&m_counter);
        }
    }

    void wait() const {
        for (;;) {
            ptrdiff_t current = m_counter.load(std::memory_order_acquire);
            ASSERT(current >= 0);
            if (current == 0)
                return;

            //即使m_counter是64位，futex也只操作低32位的地址就够了
            if (_helper::linux_futex32_wait((uint32_t*)&m_counter, (uint32_t)current, nullptr) == _helper::_LINUX_FUTEX32_NOT_SUPPORTED) {
                ASSERT(false);
                std::this_thread::yield();
            }
        }
    }

    _NODISCARD bool try_wait() const {
        return m_counter.load(std::memory_order_acquire) == 0;
    }

private:
    std::atomic<ptrdiff_t> m_counter;
};

//note: futex is avail since v2.6.x
using ks_latch_linux = ks_latch_linux_futex;

} // namespace _KSConcurrencyImpl

#endif // __linux__
#endif // __KS_LATCH_LINUX_DEF
