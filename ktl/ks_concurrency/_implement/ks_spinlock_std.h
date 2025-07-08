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

#ifndef __KS_SPINLOCK_STD_DEF
#define __KS_SPINLOCK_STD_DEF

#include "ks_concurrency_helper.h"
#include <thread>


namespace _KSConcurrencyImpl {

class ks_spinlock_std {
public:
	ks_spinlock_std() : m_spinValue(0) {}

   _DISABLE_COPY_CONSTRUCTOR(ks_spinlock_std);

   static bool __is_always_lock_efficient() noexcept {
       return false;
   }
   
   void lock() {
        while (true) {
            uintptr_t expected = 0;
            if (m_spinValue.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
                return;

            std::this_thread::yield();
        }
    }

    _NODISCARD bool try_lock() {
        uintptr_t expected = 0;
        return m_spinValue.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed);
    }

    void unlock() {
        ASSERT(m_spinValue.load(std::memory_order_relaxed) == 1);
        m_spinValue.store(0, std::memory_order_release);
    }

private:
	std::atomic<uintptr_t> m_spinValue;
};

} // namespace _KSConcurrencyImpl

#endif // __KS_SPINLOCK_STD_DEF
