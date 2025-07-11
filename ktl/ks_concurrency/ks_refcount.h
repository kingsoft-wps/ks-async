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

#ifndef __KS_REFCOUNT_DEF
#define __KS_REFCOUNT_DEF

#include "ks_atomic.h"

class ks_refcount {
public:
    ks_refcount(int init_value = 1) : m_atomic_value(init_value) {}
    ~ks_refcount() { ASSERT(m_atomic_value.load(std::memory_order_relaxed) == 0); }
    _DISABLE_COPY_CONSTRUCTOR(ks_refcount);

    void add(int delta = 1) {
        ASSERT(m_atomic_value.load(std::memory_order_relaxed) + delta >= 0);
        (void)m_atomic_value.fetch_add(delta, std::memory_order_relaxed);
    }

    _NODISCARD int sub_for_release(int delta = 1) {
        ASSERT(m_atomic_value.load(std::memory_order_relaxed) - delta >= 0);
        int new_value = m_atomic_value.fetch_sub(delta, std::memory_order_release) - delta;
        if (new_value <= 0) {
            //(void)m_atomic_value.load(std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_acquire);
        }
        return new_value;
    }

    _NODISCARD int peek_value(bool with_acquire_order = false) const {
        return m_atomic_value.load(with_acquire_order ? std::memory_order_acquire : std::memory_order_relaxed);
    }

private:
    ks_atomic<int> m_atomic_value;
};

#endif // __KS_REFCOUNT_DEF
