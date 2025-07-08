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

#ifndef __KS_SPINLOCK_DEF
#define __KS_SPINLOCK_DEF

#if defined(_WIN32)
#   include "_implement/ks_spinlock_win.h"
#elif defined(__APPLE__)
#   include "_implement/ks_spinlock_mac.h"
#elif defined(__linux__)
#   include "_implement/ks_spinlock_linux.h"
#else
#   include "_implement/ks_spinlock_std.h"
#endif

class ks_spinlock {
public:
    ks_spinlock() : m_impl() {}
    _DISABLE_COPY_CONSTRUCTOR(ks_spinlock);

    void lock() { m_impl.lock(); }
    void unlock() { m_impl.unlock(); }

    _NODISCARD bool try_lock() { return m_impl.try_lock(); }

private:
#if defined(_WIN32)
    using _SpinlockImpl = _KSConcurrencyImpl::ks_spinlock_win;
#elif defined(__APPLE__)
    using _SpinlockImpl = _KSConcurrencyImpl::ks_spinlock_mac;
#elif defined(__linux__)
    using _SpinlockImpl = _KSConcurrencyImpl::ks_spinlock_linux;
#else
    using _SpinlockImpl = _KSConcurrencyImpl::ks_spinlock_std;
#endif

    _SpinlockImpl m_impl;
};

// 确保 ks_spinlock 的对齐和大小
static_assert(alignof(ks_spinlock) == alignof(void*) && sizeof(ks_spinlock) == sizeof(void*), "ks_spinlock align and size must be same to pointer");

#endif // __KS_SPINLOCK_DEF
