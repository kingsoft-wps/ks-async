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

#ifndef __KS_ATOMIC_DEF
#define __KS_ATOMIC_DEF


#include "_implement/ks_atomic_storage.h"
#include "_implement/ks_atomic_integral_common.h"
#include "_implement/ks_atomic_integral_waitable.h"
#include "_implement/ks_atomic_enum_common.h"
#include "_implement/ks_atomic_enum_waitable.h"
#include "_implement/ks_atomic_bool.h"
#include "_implement/ks_atomic_floating.h"
#include "_implement/ks_atomic_pointer.h"


template <class T>
using ks_atomic =
    std::conditional_t<std::is_same<T, bool>::value, _KSConcurrencyImpl::ks_atomic_bool<T>,
    std::conditional_t<std::is_enum<T>::value && (sizeof(T) == 4 || sizeof(T) == 8), _KSConcurrencyImpl::ks_atomic_enum_waitable<T>,
    std::conditional_t<std::is_enum<T>::value, _KSConcurrencyImpl::ks_atomic_enum_common<T>,
    std::conditional_t<std::is_integral<T>::value && (sizeof(T) == 4 || sizeof(T) == 8), _KSConcurrencyImpl::ks_atomic_integral_waitable<T>,
    std::conditional_t<std::is_integral<T>::value, _KSConcurrencyImpl::ks_atomic_integral_common<T>,
    std::conditional_t<std::is_floating_point<T>::value, _KSConcurrencyImpl::ks_atomic_floating<T>,
    std::conditional_t<std::is_pointer<T>::value, _KSConcurrencyImpl::ks_atomic_pointer<T>,
    _KSConcurrencyImpl::ks_atomic_storage<T>>>>>>>>;


class ks_atomic_flag : private ks_atomic<int> {
    using __underlying_atomic_type = ks_atomic<int>;

public:
    ks_atomic_flag() noexcept : __underlying_atomic_type(0) {} //与std不同，我们默认初始化为false（即0）
    ks_atomic_flag(bool desired) noexcept : __underlying_atomic_type(desired ? 1 : 0) {} //与std不同，我们还提供指定初值构造
    _DISABLE_COPY_CONSTRUCTOR(ks_atomic_flag);

    void clear(std::memory_order order = std::memory_order_seq_cst) {
        __underlying_atomic_type::store(0, order);
    }

    bool test(std::memory_order order = std::memory_order_seq_cst) const {
        return __underlying_atomic_type::load(order) != 0;
    }

    bool test_and_set(std::memory_order order = std::memory_order_seq_cst) {
        return __underlying_atomic_type::exchange(1, order) != 0;
    }

    using __underlying_atomic_type::is_lock_free;
    using __underlying_atomic_type::__is_wait_efficient;

    void __wait(bool old, std::memory_order order = std::memory_order_seq_cst) const {
        return __underlying_atomic_type::__wait(old ? 1 : 0, order);
    }

    void __notify_one() {
        return __underlying_atomic_type::__notify_one();
    }

    void __notify_all() {
        return __underlying_atomic_type::__notify_all();
    }
};


#endif // __KS_ATOMIC_DEF
