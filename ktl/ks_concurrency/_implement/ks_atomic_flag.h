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

#ifndef __KS_ATOMIC_FLAG_DEF
#define __KS_ATOMIC_FLAG_DEF

#include "ks_concurrency_helper.h"
#include "ks_atomic_integral_waitable.h"


namespace _KSConcurrencyImpl {

class ks_atomic_flag : private ks_atomic_integral_waitable<int> {
    using __underly_atomic_type = ks_atomic_integral_waitable<int>;

public:
    ks_atomic_flag() noexcept : __underly_atomic_type(0) {} //与std不同，我们默认初始化为false（即0）
    ks_atomic_flag(bool desired) noexcept : __underly_atomic_type(desired ? 1 : 0) {} //与std不同，我们还提供指定初值构造
    _DISABLE_COPY_CONSTRUCTOR(ks_atomic_flag);

    void clear(std::memory_order order = std::memory_order_seq_cst) {
        __underly_atomic_type::store(0, order);
    }

    bool test(std::memory_order order = std::memory_order_seq_cst) const {
        return __underly_atomic_type::load(order) != 0;
    }

    bool test_and_set(std::memory_order order = std::memory_order_seq_cst) {
        return __underly_atomic_type::exchange(1, order) != 0;
    }

    using __underly_atomic_type::is_lock_free;
    using __underly_atomic_type::__is_wait_efficient;

    void __wait(bool old, std::memory_order order = std::memory_order_seq_cst) const {
        return __underly_atomic_type::__wait(old ? 1 : 0, order);
    }

    void __notify_one() {
        return __underly_atomic_type::__notify_one();
    }

    void __notify_all() {
        return __underly_atomic_type::__notify_all();
    }
};

} // namespace _KSConcurrencyImpl

#endif // __KS_ATOMIC_FLAG_DEF
