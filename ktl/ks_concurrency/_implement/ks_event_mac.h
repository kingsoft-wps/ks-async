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

#ifndef __KS_EVENT_MAC_DEF
#define __KS_EVENT_MAC_DEF
#if defined(__APPLE__)

#include "ks_concurrency_helper.h"
#include "ks_spinlock_mac.h"
#include "ks_latch_mac.h"


namespace _KSConcurrencyImpl {

class ks_event_mac_ossync {
public:
    explicit ks_event_mac_ossync(bool initialState, bool manualReset) 
        : m_atomicState32(initialState ? 1 : 0), m_manualReset(manualReset) {
        ASSERT(_helper::getOSSyncApis()->wait_on_address != nullptr &&
               _helper::getOSSyncApis()->wake_by_address_any != nullptr &&
               _helper::getOSSyncApis()->wake_by_address_all != nullptr);
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_event_mac_ossync);

    void set_event() {
       uint32_t expected32 = 0;
        if (m_atomicState32.compare_exchange_strong(expected32, 1, std::memory_order_release, std::memory_order_relaxed)) {
            if (m_manualReset)
                _helper::getOSSyncApis()->wake_by_address_all(&m_atomicState32, sizeof(uint32_t), _helper::OSSYNC_WAKE_BY_ADDRESS_NONE);
            else
                _helper::getOSSyncApis()->wake_by_address_any(&m_atomicState32, sizeof(uint32_t), _helper::OSSYNC_WAKE_BY_ADDRESS_NONE);
        }
    }

    void reset_event() {
         m_atomicState32.store(0, std::memory_order_relaxed);
    }

    void wait() {
        for (;;) {
            if (m_manualReset) {
                const uint32_t state32 = m_atomicState32.load(std::memory_order_acquire);
                if (state32 != 0)
                    return; //ok
            }
            else {
                uint32_t expected32 = 1;
                if (m_atomicState32.compare_exchange_strong(expected32, 0, std::memory_order_acquire, std::memory_order_relaxed)) 
                    return; //ok
            }

            _helper::getOSSyncApis()->wait_on_address(&m_atomicState32, 0, sizeof(uint32_t), _helper::OSSYNC_WAIT_ON_ADDRESS_NONE);
        }
    }

    _NODISCARD bool try_wait() {
        if (m_manualReset) {
            const uint32_t state32 = m_atomicState32.load(std::memory_order_acquire);
            if (state32 != 0)
                return true; //ok
        }
        else {
            uint32_t expected32 = 1;
            if (m_atomicState32.compare_exchange_strong(expected32, 0, std::memory_order_acquire, std::memory_order_relaxed)) 
                return true; //ok
        }

        return false;
    }

private:
    std::atomic<uint32_t> m_atomicState32;
    const bool m_manualReset;
};


class ks_event_mac_gdc {
public:
    explicit ks_event_mac_gdc(bool initialState = false, bool manualReset = false)
        : m_spinlock(), m_state_latch(initialState ? 0 : 1)
        , m_state(initialState), m_manualReset(manualReset) {
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_event_mac_gdc);

    void set_event() {
        m_spinlock.lock();

        if (!m_state) {
            m_state = true;

            ASSERT(!m_state_latch.try_wait());
            m_state_latch.count_down();
            ASSERT(m_state_latch.try_wait());
        }

        m_spinlock.unlock();
    }

    void reset_event() {
        m_spinlock.lock();

        if (m_state) {
            m_state = false;

            ASSERT(m_state_latch.try_wait());
            m_state_latch.add();
            ASSERT(!m_state_latch.try_wait());
        }

        m_spinlock.unlock();
    }

    void wait() {
        m_spinlock.lock();

        while (!m_state) {
            m_spinlock.unlock();
            m_state_latch.wait();
            m_spinlock.lock();
        }

        if (!m_manualReset) {
            m_state = false;

            ASSERT(m_state_latch.try_wait());
            m_state_latch.add();
            ASSERT(!m_state_latch.try_wait());
        }

        m_spinlock.unlock();
    }

    _NODISCARD bool try_wait() {
        bool ret = false;
        m_spinlock.lock();

        if (m_state) {
            ret = true;

            if (!m_manualReset) {
                m_state = false;

                ASSERT(m_state_latch.try_wait());
                m_state_latch.add();
                ASSERT(!m_state_latch.try_wait());
            }
        }

        m_spinlock.unlock();
        return ret;
    }

private:
    ks_spinlock_mac_unfair m_spinlock;
    ks_latch_mac_gdc m_state_latch;
    bool m_state;
    const bool m_manualReset;
};


class ks_event_mac {
public:
    explicit ks_event_mac(bool initialState, bool manualReset) {
        if (m_use_ossync) {
            new (&m_ossync_impl) ks_event_mac_ossync(initialState, manualReset);
        } else {
            new (&m_gdc_impl) ks_event_mac_gdc(initialState, manualReset);
        }
    }

    ~ks_event_mac() {
        if (m_use_ossync) {
            m_ossync_impl.~ks_event_mac_ossync();
        } else {
            m_gdc_impl.~ks_event_mac_gdc();
        }
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_event_mac);

    void set_event() {
        if (m_use_ossync) {
            m_ossync_impl.set_event();
        } else {
            m_gdc_impl.set_event();
        }
    }

    void reset_event() {
        if (m_use_ossync) {
            m_ossync_impl.reset_event();
        } else {
            m_gdc_impl.reset_event();
        }
    }

    void wait() {
        if (m_use_ossync) {
            m_ossync_impl.wait();
        } else {
            m_gdc_impl.wait();
        }
    }

    _NODISCARD bool try_wait() {
        if (m_use_ossync) {
            return m_ossync_impl.try_wait();
        } else {
            return m_gdc_impl.try_wait();
        }
    }

private:
    union {
        ks_event_mac_ossync m_ossync_impl;
        ks_event_mac_gdc m_gdc_impl;
    };

    const bool m_use_ossync = (bool)(_helper::getOSSyncApis()->wait_on_address);
};

} // namespace _KSConcurrencyImpl

#endif // __APPLE__
#endif // __KS_EVENT_MAC_DEF
