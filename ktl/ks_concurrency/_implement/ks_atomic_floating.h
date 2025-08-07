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

#ifndef __KS_ATOMIC_FLOATING_DEF
#define __KS_ATOMIC_FLOATING_DEF

#include "ks_concurrency_helper.h"
#include "../../ks_type_traits.h"
#include <atomic>


namespace _KSConcurrencyImpl {

template <class T>
class ks_atomic_floating : private std::atomic<T> {
    static_assert(
        std::is_floating_point<T>::value,
        "ks_atomic_floating can only be used with floating-point types");

public:
    ks_atomic_floating() noexcept : std::atomic<T>(T(0)) {} //与std不同，我们默认初始化为0
    ks_atomic_floating(T desired) noexcept : std::atomic<T>(desired) {}
    _DISABLE_COPY_CONSTRUCTOR(ks_atomic_floating);

    using std::atomic<T>::load;
    using std::atomic<T>::store;
    using std::atomic<T>::exchange;
    using std::atomic<T>::compare_exchange_strong;
    using std::atomic<T>::compare_exchange_weak;

    using std::atomic<T>::operator T;

    using std::atomic<T>::is_lock_free;
};

} // namespace _KSConcurrencyImpl

#endif // __KS_ATOMIC_FLOATING_DEF
