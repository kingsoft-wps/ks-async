﻿/* Copyright 2025 The Kingsoft's ks-async/ktl Authors. All Rights Reserved.

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

#ifndef __KS_EVENT_LINUX_DEF
#define __KS_EVENT_LINUX_DEF
#if defined(__linux__)

#include "ks_concurrency_helper.h"


namespace _KSConcurrencyImpl {

class ks_event_linux_futex {
public:
    explicit ks_event_linux_futex(bool initialState, bool manualReset) 
        : m_atomicState32(initialState ? 1 : 0), m_manualReset(manualReset) {
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_event_linux_futex);

    void set_event() {
        uint32_t expected32 = 0;
        if (m_atomicState32.compare_exchange_strong(expected32, 1, std::memory_order_release, std::memory_order_relaxed)) {
            if (m_manualReset)
                _helper::linux_futex32_wake_all((uint32_t*)&m_atomicState32);
            else
                _helper::linux_futex32_wake_one((uint32_t*)&m_atomicState32);
        }
    }

    void reset_event() {
        m_atomicState32.store(0, std::memory_order_relaxed);
    }

    void wait() {
        for (;;) {
            if (m_manualReset) {
                const uint32_t state32 = m_atomicState32.load(std::memory_order_acquire);
                if (state32 != 0)
                    return; //ok
            }
            else {
                uint32_t expected32 = 1;
                if (m_atomicState32.compare_exchange_strong(expected32, 0, std::memory_order_acquire, std::memory_order_relaxed)) 
                    return; //ok
            }

            if (_helper::linux_futex32_wait((uint32_t*)&m_atomicState32, 0, nullptr) == _helper::_LINUX_FUTEX32_NOT_SUPPORTED) {
                ASSERT(false);
                std::this_thread::yield();
            }
        }
    }

    _NODISCARD bool try_wait() {
        if (m_manualReset) {
            const uint32_t state32 = m_atomicState32.load(std::memory_order_acquire);
            if (state32 != 0)
                return true; //ok
        }
        else {
            uint32_t expected32 = 1;
            if (m_atomicState32.compare_exchange_strong(expected32, 0, std::memory_order_acquire, std::memory_order_relaxed)) 
                return true; //ok
        }

        return false;
    }

private:
    std::atomic<uint32_t> m_atomicState32;
    const bool m_manualReset;
};

//note: futex is avail since v2.6.x
using ks_event_linux = ks_event_linux_futex;

} // namespace _KSConcurrencyImpl

#endif // __linux__
#endif // __KS_EVENT_LINUX_DEF
