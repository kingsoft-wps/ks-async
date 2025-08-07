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

#ifndef __KS_ATOMIC_POINTER_DEF
#define __KS_ATOMIC_POINTER_DEF

#include "ks_concurrency_helper.h"
#include "../../ks_type_traits.h"
#include <atomic>


namespace _KSConcurrencyImpl {

//在不同平台上，wait-on-address操作要求T的size必须为:
//  windows: 1, 2, 4, 8
//  mac: 4, 8
//  linix: 4
//为此，我们限制T的size只能是4或8。
//特别在linux上，若T的size为8（64位），wait-on-address操作仅在低32位上进行，
//这样做的逻辑仍是正确的，因为wait有old参数，至多内部可能会出现假醒情况，且这对使用者是无感的。
template <class T>
class ks_atomic_pointer : private std::atomic<T> {
    static_assert(
        std::is_pointer<T>::value && (sizeof(T) == 4 || sizeof(T) == 8),
        "ks_atomic_pointer can only be used with pointer types");

public:
    ks_atomic_pointer() noexcept : std::atomic<T>(nullptr) {} //与std不同，我们默认初始化为0
    ks_atomic_pointer(T desired) noexcept : std::atomic<T>(desired) {}
    _DISABLE_COPY_CONSTRUCTOR(ks_atomic_pointer);

    using std::atomic<T>::load;
    using std::atomic<T>::store;
    using std::atomic<T>::exchange;
    using std::atomic<T>::compare_exchange_strong;
    using std::atomic<T>::compare_exchange_weak;

    using std::atomic<T>::operator T;

    using std::atomic<T>::is_lock_free;

    _NODISCARD bool __is_wait_efficient() const volatile noexcept {
        return _KSConcurrencyImpl::_helper::__atomic_is_wait_efficient<T>(this);
    }

    void __wait(T old, std::memory_order order = std::memory_order_seq_cst) const {
        return _KSConcurrencyImpl::_helper::__atomic_wait_explicit<T>(this, old, order);
    }
    void __notify_one() {
        return _KSConcurrencyImpl::_helper::__atomic_notify_one<T>(this);
    }
    void __notify_all() {
        return _KSConcurrencyImpl::_helper::__atomic_notify_all<T>(this);
    }
};


} // namespace _KSConcurrencyImpl

#endif // __KS_ATOMIC_POINTER_DEF
