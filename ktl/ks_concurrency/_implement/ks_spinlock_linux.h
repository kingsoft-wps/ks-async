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

#ifndef __KS_SPINLOCK_LINUX_DEF
#define __KS_SPINLOCK_LINUX_DEF
#if defined(__linux__)

#include "ks_concurrency_helper.h"
#include <thread>


namespace _KSConcurrencyImpl {

class ks_spinlock_linux_futex {
public:
	ks_spinlock_linux_futex() : m_spinValue(0) {}

    _DISABLE_COPY_CONSTRUCTOR(ks_spinlock_linux_futex);

    static bool __is_always_lock_efficient() noexcept {
        return true;
    }

    void lock() {
        std::atomic<uint8_t>* low_atomic_uint8_p = (std::atomic<uint8_t>*)(&m_spinValue);

        // 立即尝试
        if (true) {
            uint8_t expected8 = 0;
            if (low_atomic_uint8_p->compare_exchange_strong(expected8, 1, std::memory_order_acquire, std::memory_order_relaxed))
                return;
        }

        // 自旋一小下
        static const bool _HAS_MULTI_CPU = std::thread::hardware_concurrency() >= 2;
        if (_HAS_MULTI_CPU) {
            constexpr int spin_count = 5;
            for (int i = 0; i < spin_count; ++i) {
                std::this_thread::yield();
                uint8_t expected8 = 0;
                if (low_atomic_uint8_p->compare_exchange_strong(expected8, 1, std::memory_order_acquire, std::memory_order_relaxed))
                    return;
            }
        }

        // 自旋后仍未获得锁，进入内核等待
        uint32_t current32 = m_spinValue.fetch_add(uint32_t(0x00000100), std::memory_order_relaxed) + uint32_t(0x00000100); //等待者计数加1
        while (true) {
            ASSERT((current32 & 0xFFFFFF00) != 0);
            if ((uint8_t)(current32) != 0) {
                if (_helper::linux_futex32_wait((uint32_t*)&m_spinValue, current32, nullptr) == _helper::_LINUX_FUTEX32_NOT_SUPPORTED)
                    std::this_thread::yield();
            }

            uint8_t expected8 = 0;
            if (low_atomic_uint8_p->compare_exchange_strong(expected8, 1, std::memory_order_acquire, std::memory_order_relaxed)) {
                ASSERT((m_spinValue.load(std::memory_order_relaxed) & 0xFFFFFF00) != 0);
                m_spinValue.fetch_sub(uint32_t(0x00000100), std::memory_order_relaxed); //等待者计数减1
                return;
            }

            current32 = m_spinValue.load(std::memory_order_relaxed);
        }
    }

    _NODISCARD bool try_lock() {
        std::atomic<uint8_t>* low_atomic_uint8_p = (std::atomic<uint8_t>*)(&m_spinValue);

        uint8_t expected8 = 0;
        return low_atomic_uint8_p->compare_exchange_strong(expected8, 1, std::memory_order_acquire, std::memory_order_relaxed);
    }

    void unlock() {
        std::atomic<uint8_t>* low_atomic_uint8_p = (std::atomic<uint8_t>*)(&m_spinValue);

        ASSERT(low_atomic_uint8_p->load(std::memory_order_relaxed) == 1);
        low_atomic_uint8_p->store(0, std::memory_order_release);

        uint32_t current = m_spinValue.load(std::memory_order_relaxed);
        if ((uint8_t)(current) == 0 && (current & 0xFFFFFF00) != 0) {
            _helper::linux_futex32_wake_one((uint32_t*)&m_spinValue); //唤醒1个等待者
        }
    }

private:
    union {
        // m_waitingCount 被拆分为两部分 
        // [ 高3字节:用于等待线程计数 | 低1字节:记录自旋锁状态0/1 ]
        std::atomic<uint32_t> m_spinValue;
        void* m_alignment; // 用于对齐
    };

};

//note: futex is avail since v2.6.x
using ks_spinlock_linux = ks_spinlock_linux_futex;

} // namespace _KSConcurrencyImpl

#endif // __linux__
#endif // __KS_SPINLOCK_LINUX_DEF