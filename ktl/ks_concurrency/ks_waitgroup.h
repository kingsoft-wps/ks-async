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

#ifndef __KS_WAITGROUP_DEF
#define __KS_WAITGROUP_DEF

#if defined(_WIN32)
#   include "_implement/ks_waitgroup_win.h"
#elif defined(__APPLE__)
#   include "_implement/ks_waitgroup_mac.h"
#elif defined(__linux__)
#   include "_implement/ks_waitgroup_linux.h"
#else
#   include "_implement/ks_waitgroup_std.h"
#endif


class ks_waitgroup {
public:
    explicit ks_waitgroup(ptrdiff_t expected) : m_impl(expected) {}
    _DISABLE_COPY_CONSTRUCTOR(ks_waitgroup);

    void add(ptrdiff_t update) { m_impl.add(update); }
    void done() { m_impl.done(); }
    void wait() { m_impl.wait(); }

    _NODISCARD bool try_wait() const { return m_impl.try_wait(); }

    template<class Rep, class Period>
    _NODISCARD bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const { return m_impl.wait_for(rel_time); }
    template<class Clock, class Duration>
    _NODISCARD bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const { return m_impl.wait_until(abs_time); }

private:
#if defined(_WIN32)
    using _WaitGroupImpl = _KSConcurrencyImpl::ks_waitgroup_win;
#elif defined(__APPLE__)
    using _WaitGroupImpl = _KSConcurrencyImpl::ks_waitgroup_mac;
#elif defined(__linux__)
    using _WaitGroupImpl = _KSConcurrencyImpl::ks_waitgroup_linux;
#else
    using _WaitGroupImpl = _KSConcurrencyImpl::ks_waitgroup_std;
#endif

    _WaitGroupImpl m_impl;
};

#endif // __KS_WAITGROUP_DEF
