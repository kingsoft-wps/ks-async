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

#ifndef __KS_LATCH_MAC_DEF
#define __KS_LATCH_MAC_DEF
#if defined(__APPLE__)

#include "ks_concurrency_helper.h"


namespace _KSConcurrencyImpl {

class ks_latch_mac_ossync {
public:
    explicit ks_latch_mac_ossync(ptrdiff_t desired)
        : m_counter(desired) {
        ASSERT(desired >= 0);
        ASSERT(_helper::getOSSyncApis()->wait_on_address != nullptr &&
               _helper::getOSSyncApis()->wake_by_address_any != nullptr &&
               _helper::getOSSyncApis()->wake_by_address_all != nullptr);
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_latch_mac_ossync);

    void add(ptrdiff_t update = 1) {
        ASSERT(update > 0);
        m_counter.fetch_add(update, std::memory_order_release);
    }

    void count_down(ptrdiff_t update = 1) {
		ASSERT(update > 0);
        ptrdiff_t current = m_counter.fetch_sub(update, std::memory_order_release) - update;
        ASSERT(current >= 0);
        if (current == 0) {
            _helper::getOSSyncApis()->wake_by_address_all(&m_counter, sizeof(ptrdiff_t), _helper::OSSYNC_WAKE_BY_ADDRESS_NONE);
        }
    }

    void wait() const {
        for (;;) {
            ptrdiff_t current = m_counter.load(std::memory_order_acquire);
            ASSERT(current >= 0);
            if (current == 0) 
                return;

            _helper::getOSSyncApis()->wait_on_address(&m_counter, current, sizeof(ptrdiff_t), _helper::OSSYNC_WAIT_ON_ADDRESS_NONE);
        }
    }

    _NODISCARD bool try_wait() const {
        return m_counter.load(std::memory_order_acquire) == 0;
    }

private:
    std::atomic<ptrdiff_t> m_counter;
};


class ks_latch_mac_gdc {
public:
    explicit ks_latch_mac_gdc(ptrdiff_t desired) {
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

    ~ks_latch_mac_gdc() {
        dispatch_release(m_dispatchGroup);
        m_dispatchGroup = nullptr;
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_latch_mac_gdc);

    void add(ptrdiff_t update = 1) {
        ASSERT(update >= 0);
        for (ptrdiff_t i = 0; i < update; ++i) {
            dispatch_group_enter(m_dispatchGroup);
        }
    }

    void count_down(ptrdiff_t update = 1) {
        ASSERT(update >= 0);
        for (ptrdiff_t i = 0; i < update; ++i) {
 			dispatch_group_leave(m_dispatchGroup);
        }
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

private:
    dispatch_group_t m_dispatchGroup;
};


class ks_latch_mac {
public:
    explicit ks_latch_mac(ptrdiff_t desired) {
        if (m_use_ossync) {
            new (&m_ossync_impl) ks_latch_mac_ossync(desired);
        } else {
            new (&m_gdc_impl) ks_latch_mac_gdc(desired);
        }
    }

    ~ks_latch_mac() {
        if (m_use_ossync) {
            m_ossync_impl.~ks_latch_mac_ossync();
        } else {
            m_gdc_impl.~ks_latch_mac_gdc();
        }
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_latch_mac);

    void add(ptrdiff_t update = 1) {
        if (m_use_ossync) {
            m_ossync_impl.add(update);
        }
        else {
            m_gdc_impl.add(update);
        }
    }

    void count_down(ptrdiff_t update = 1) {
        if (m_use_ossync) {
            m_ossync_impl.count_down(update);
        } else {
            m_gdc_impl.count_down(update);
        }
    }

    void wait() const {
        if (m_use_ossync) {
            m_ossync_impl.wait();
        } else {
            m_gdc_impl.wait();
        }
    }

    _NODISCARD bool try_wait() const {
        if (m_use_ossync) {
            return m_ossync_impl.try_wait();
        } else {
            return m_gdc_impl.try_wait();
        }
    }

private:
    union {
        ks_latch_mac_ossync m_ossync_impl;
        ks_latch_mac_gdc m_gdc_impl;
    };

    const bool m_use_ossync = (bool)(_helper::getOSSyncApis()->wait_on_address);
};

} // namespace _KSConcurrencyImpl

#endif // __APPLE__
#endif // __KS_LATCH_MAC_DEF
