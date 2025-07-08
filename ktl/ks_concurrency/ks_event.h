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

#ifndef __KS_EVENT_DEF
#define __KS_EVENT_DEF

#if defined(_WIN32)
#   include "_implement/ks_event_win.h"
#elif defined(__APPLE__)
#   include "_implement/ks_event_mac.h"
#elif defined(__linux__)
#   include "_implement/ks_event_linux.h"
#else
#   include "_implement/ks_event_std.h"
#endif


class ks_event {
public:
    explicit ks_event(bool initialState, bool manualReset) 
        : m_impl(initialState, manualReset) {}
    _DISABLE_COPY_CONSTRUCTOR(ks_event);

    void set_event() { m_impl.set_event(); }
    void reset_event() { m_impl.reset_event(); }
    void wait() { m_impl.wait(); }
    _NODISCARD bool try_wait() { return m_impl.try_wait(); }

private:
#if defined(_WIN32)
    using _EventImpl = _KSConcurrencyImpl::ks_event_win;
#elif defined(__APPLE__)
    using _EventImpl = _KSConcurrencyImpl::ks_event_mac;
#elif defined(__linux__)
    using _EventImpl = _KSConcurrencyImpl::ks_event_linux;
#else
    using _EventImpl = _KSConcurrencyImpl::ks_event_std;
#endif

    _EventImpl m_impl;
};

#endif // __KS_EVENT_DEF
