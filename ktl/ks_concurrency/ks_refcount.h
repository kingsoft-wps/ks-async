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

template <class T>
class ks_refcount : private std::atomic<T> {
    static_assert(std::is_integral<T>::value, "ks_refcount<T>: T must be an integral type");
    using __underlying_atomic_type = std::atomic<T>;

public:
    ks_refcount(T desired) noexcept : __underlying_atomic_type(desired) { ASSERT(desired >= 0); }
    ~ks_refcount() noexcept { ASSERT(this->peek_value() == 0); }
    _DISABLE_COPY_CONSTRUCTOR(ks_refcount);

    T add(T update = 1) noexcept {
        ASSERT(update >= 0);
        T new_value = __underlying_atomic_type::fetch_add(update, std::memory_order_relaxed) + update;
        ASSERT(new_value > 0 && new_value >= new_value - update);
        return new_value;
    }

    _NODISCARD T sub(T update = 1) noexcept {
        ASSERT(update >= 0);
        T new_value = __underlying_atomic_type::fetch_sub(update, std::memory_order_release) - update;
        ASSERT(new_value >= 0 && new_value <= new_value + update);
        if (new_value == 0) {
            std::atomic_thread_fence(std::memory_order_acquire);
        }
        return new_value;
    }

    T operator++() noexcept { return this->add(1); }
    T operator++(int) noexcept { return this->add(1) - 1; }
    T operator+=(T update) noexcept { return this->add(update); }

    _NODISCARD T operator--() noexcept { return this->sub(1); }
    _NODISCARD T operator--(int) noexcept { return this->sub(1) + 1; }
    _NODISCARD T operator-=(T update) noexcept { return this->sub(update); }

    _NODISCARD T peek_value() const noexcept {
        return __underlying_atomic_type::load(std::memory_order_relaxed);
    }
};

#endif // __KS_REFCOUNT_DEF
