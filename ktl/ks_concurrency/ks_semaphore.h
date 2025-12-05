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

#ifndef __KS_SEMAPHORE_DEF
#define __KS_SEMAPHORE_DEF

#if defined(_WIN32)
#   include "_implement/ks_semaphore_win.h"
#elif defined(__APPLE__)
#   include "_implement/ks_semaphore_mac.h"
#elif defined(__linux__)
#   include "_implement/ks_semaphore_linux.h"
#else
#   include "_implement/ks_semaphore_std.h"
#endif

class ks_semaphore {
public:
    explicit ks_semaphore(ptrdiff_t desired) : m_impl(desired) {}
    _DISABLE_COPY_CONSTRUCTOR(ks_semaphore);

    void release(ptrdiff_t update = 1) { m_impl.release(update); }
    void acquire() { m_impl.acquire(); }

    _NODISCARD bool try_acquire() { return m_impl.try_acquire(); }

private:
#if defined(_WIN32)
    using _SemaphoreImpl = _KSConcurrencyImpl::ks_semaphore_win;
#elif defined(__APPLE__)
    using _SemaphoreImpl = _KSConcurrencyImpl::ks_semaphore_mac;
#elif defined(__linux__)
    using _SemaphoreImpl = _KSConcurrencyImpl::ks_semaphore_linux;
#else
    using _SemaphoreImpl = _KSConcurrencyImpl::ks_semaphore_std;
#endif

    _SemaphoreImpl m_impl;
};

#endif // __KS_SEMAPHORE_DEF
