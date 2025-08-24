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

#include <atomic>

class ks_refcount : private std::atomic<ptrdiff_t> {
    using __underlying_atomic_type = std::atomic<ptrdiff_t>;

public:
    ks_refcount(ptrdiff_t desired) : __underlying_atomic_type(desired) { ASSERT(desired >= 0); }
    ~ks_refcount() { ASSERT(this->peek_value() == 0); }
    _DISABLE_COPY_CONSTRUCTOR(ks_refcount);

    ptrdiff_t add(ptrdiff_t update = 1) {
        ASSERT(update > 0);
        ptrdiff_t new_value = __underlying_atomic_type::fetch_add(update, std::memory_order_relaxed) + update;
        ASSERT(new_value > 0);
        return new_value;
    }

    _NODISCARD ptrdiff_t sub(ptrdiff_t update = 1) {
        ASSERT(update > 0);
        ptrdiff_t new_value = __underlying_atomic_type::fetch_sub(update, std::memory_order_release) - update;
        ASSERT(new_value >= 0);
        if (new_value == 0) {
            std::atomic_thread_fence(std::memory_order_acquire);
        }
        return new_value;
    }

    ptrdiff_t operator++() { return this->add(1); }
    ptrdiff_t operator++(int) { return this->add(1) - 1; }
    ptrdiff_t operator+=(ptrdiff_t update) { return this->add(update); }

    _NODISCARD ptrdiff_t operator--() { return this->sub(1); }
    _NODISCARD ptrdiff_t operator--(int) { return this->sub(1) + 1; }
    _NODISCARD ptrdiff_t operator-=(ptrdiff_t update) { return this->sub(update); }

    _NODISCARD ptrdiff_t peek_value() const {
        return __underlying_atomic_type::load(std::memory_order_relaxed);
    }
};

#endif // __KS_REFCOUNT_DEF
