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

#ifndef __KS_SPINLOCK_MAC_DEF
#define __KS_SPINLOCK_MAC_DEF
#if defined(__APPLE__)

#include "ks_concurrency_helper.h"

#include <os/lock.h>

namespace _KSConcurrencyImpl {

// macOS下直接使用os_unfair_lock
class ks_spinlock_mac_unfair {
public:
    ks_spinlock_mac_unfair() : m_unfairLock(OS_UNFAIR_LOCK_INIT) {}

    // 禁止拷贝构造函数
    // 由于os_unfair_lock是不可拷贝的，因此我们也禁止拷贝构造函数
    // 这可以确保ks_spinlock_mac_unfair对象在多线程环境中是安全的

   _DISABLE_COPY_CONSTRUCTOR(ks_spinlock_mac_unfair);

   static bool __is_always_lock_efficient() noexcept {
       return true;
   }
   
   void lock() {
       const auto unfair_lock_lock_with_flags_fn = _helper::getOSSyncApis()->unfair_lock_lock_with_flags;
        if (unfair_lock_lock_with_flags_fn)
            unfair_lock_lock_with_flags_fn(&m_unfairLock, _helper::OS_UNFAIR_LOCK_FLAG_ADAPTIVE_SPIN);
        else
            os_unfair_lock_lock(&m_unfairLock);
    }

    _NODISCARD bool try_lock() {
        return os_unfair_lock_trylock(&m_unfairLock);
    }

    void unlock() {
        os_unfair_lock_unlock(&m_unfairLock);
    }

private:
    union {
        os_unfair_lock m_unfairLock;
        void* m_alignment; // 用于对齐
    };
};

//note: unfairlock is avail v10.2
using ks_spinlock_mac = ks_spinlock_mac_unfair;

} // namespace _KSConcurrencyImpl

#endif // __APPLE__
#endif // __KS_SPINLOCK_MAC_DEF
