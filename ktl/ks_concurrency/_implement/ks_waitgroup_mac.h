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

#ifndef __KS_WAITGROUP_MAC_DEF
#define __KS_WAITGROUP_MAC_DEF
#if defined(__APPLE__)

#include "ks_concurrency_helper.h"


namespace _KSConcurrencyImpl {

class ks_waitgroup_mac_gdc {
public:
    explicit ks_waitgroup_mac_gdc(ptrdiff_t desired) {
        ASSERT(desired >= 0);

        m_dispatchGroup = dispatch_group_create();
        if (m_dispatchGroup == nullptr) {
            ASSERT(false);
            throw std::runtime_error("Failed to create dispatch group");
        }

        for (ptrdiff_t i = 0; i < desired; ++i) {
            dispatch_group_enter(m_dispatchGroup);
        }
    }

    ~ks_waitgroup_mac_gdc() {
#if !__has_feature(objc_arc)
        dispatch_release(m_dispatchGroup);
#endif
        //m_dispatchGroup = nullptr;
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_waitgroup_mac_gdc);

    void add(ptrdiff_t update) {
        ASSERT(update >= 0);
        for (ptrdiff_t i = 0; i < update; ++i) {
            dispatch_group_enter(m_dispatchGroup);
        }
    }

    void done() {
 		dispatch_group_leave(m_dispatchGroup);
    }

    void wait() const {
        if (dispatch_group_wait(m_dispatchGroup, DISPATCH_TIME_FOREVER) != 0) {
            ASSERT(false);
            throw std::runtime_error("Failed to wait on dispatch group");
        }
    }

    _NODISCARD bool try_wait() const {
        return dispatch_group_wait(m_dispatchGroup, DISPATCH_TIME_NOW) == 0;
    }

    template<class Rep, class Period>
    _NODISCARD bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const {
        dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, std::chrono::duration_cast<std::chrono::nanoseconds>(rel_time).count());
        return dispatch_group_wait(m_dispatchGroup, timeout) == 0;
    }

    template<class Clock, class Duration>
    _NODISCARD bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const {
        return this->wait_for(abs_time - Clock::now());
    }

private:
    dispatch_group_t m_dispatchGroup;
};


//note: gdc is avail since iOS 4.0, macOS 10.6
using ks_waitgroup_mac = ks_waitgroup_mac_gdc;

} // namespace _KSConcurrencyImpl

#endif // __APPLE__
#endif // __KS_WAITGROUP_MAC_DEF
