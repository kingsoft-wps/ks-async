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

#ifndef __KS_SEMAPHORE_MAC_DEF
#define __KS_SEMAPHORE_MAC_DEF
#if defined(__APPLE__)

#include "ks_concurrency_helper.h"


namespace _KSConcurrencyImpl {

class ks_semaphore_mac_gdc {
public:
    explicit ks_semaphore_mac_gdc(ptrdiff_t desired) {
        ASSERT(desired >= 0);

        m_dispatchSemaphore = dispatch_semaphore_create(0);
        if (m_dispatchSemaphore == nullptr) {
            ASSERT(false);
            throw std::runtime_error("Failed to create semaphore");
        }

        for (ptrdiff_t i = 0; i < desired; ++i) {
            dispatch_semaphore_signal(m_dispatchSemaphore);
        }
    }

    ~ks_semaphore_mac_gdc() {
#if !__has_feature(objc_arc)
        dispatch_release(m_dispatchSemaphore);
#endif
        m_dispatchSemaphore = nullptr;
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_semaphore_mac_gdc);

    void release(ptrdiff_t update = 1) {
		ASSERT(update > 0);
        while (--update >= 0) {
            dispatch_semaphore_signal(m_dispatchSemaphore);
        }
    }

    void acquire() {
        if (dispatch_semaphore_wait(m_dispatchSemaphore, DISPATCH_TIME_FOREVER) != 0) {
            ASSERT(false);
            throw std::runtime_error("Failed to acquire semaphore");
        }
    }

    _NODISCARD bool try_acquire() {
        return dispatch_semaphore_wait(m_dispatchSemaphore, DISPATCH_TIME_NOW) == 0;
    }

    // 屏蔽try_acquire_for、try_acquire_until

private:
    dispatch_semaphore_t m_dispatchSemaphore;
};

using ks_semaphore_mac = ks_semaphore_mac_gdc;

} // namespace _KSConcurrencyImpl

#endif // __APPLE__
#endif // __KS_SEMAPHORE_MAC_DEF
