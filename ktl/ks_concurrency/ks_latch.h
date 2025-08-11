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

#ifndef __KS_LATCH_DEF
#define __KS_LATCH_DEF

#if defined(_WIN32)
#   include "_implement/ks_latch_win.h"
#elif defined(__APPLE__)
#   include "_implement/ks_latch_mac.h"
#elif defined(__linux__)
#   include "_implement/ks_latch_linux.h"
#else
#   include "_implement/ks_latch_std.h"
#endif


class ks_latch {
public:
    explicit ks_latch(ptrdiff_t expected) : m_impl(expected) {}
    _DISABLE_COPY_CONSTRUCTOR(ks_latch);

    void add(ptrdiff_t update = 1) { m_impl.add(update); }
    void count_down(ptrdiff_t update = 1) { m_impl.count_down(update); }
    void wait() { m_impl.wait(); }

    _NODISCARD bool try_wait() { return m_impl.try_wait(); }

private:
#if defined(_WIN32)
    using _LatchImpl = _KSConcurrencyImpl::ks_latch_win;
#elif defined(__APPLE__)
    using _LatchImpl = _KSConcurrencyImpl::ks_latch_mac;
#elif defined(__linux__)
    using _LatchImpl = _KSConcurrencyImpl::ks_latch_linux;
#else
    using _LatchImpl = _KSConcurrencyImpl::ks_latch_std;
#endif

    _LatchImpl m_impl;
};

#endif // __KS_LATCH_DEF
