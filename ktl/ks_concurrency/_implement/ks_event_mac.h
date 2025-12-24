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
#include "ks_waitgroup_mac.h"


namespace _KSConcurrencyImpl {

class ks_event_mac_ossync {
public:
    explicit ks_event_mac_ossync(bool initialState, bool manualReset) 
        : m_atomicState32(initialState ? 1 : 0), m_manualReset(manualReset) {
        ASSERT(_helper::__getAppleSyncApis()->wait_on_address != nullptr &&
                _helper::__getAppleSyncApis()->wait_on_address_with_timeout != nullptr &&
               _helper::__getAppleSyncApis()->wake_by_address_any != nullptr &&
               _helper::__getAppleSyncApis()->wake_by_address_all != nullptr);
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_event_mac_ossync);

    void set_event() {
       uint32_t expected32 = 0;
        if (m_atomicState32.compare_exchange_strong(expected32, 1, std::memory_order_release, std::memory_order_relaxed)) {
            if (m_manualReset)
                _helper::__atomic_notify_all(&m_atomicState32);
            else
                _helper::__atomic_notify_one(&m_atomicState32);
        }
    }

    void reset_event() {
         m_atomicState32.store(0, std::memory_order_relaxed);
    }

    void wait() const {
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

            _helper::__atomic_wait_explicit(&m_atomicState32, (uint32_t)0, std::memory_order_relaxed);
        }
    }

    _NODISCARD bool try_wait() const {
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

    template<class Rep, class Period>
    _NODISCARD bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const {
        return this->wait_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    _NODISCARD bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const {
        for (;;) {
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

            if (Clock::now() >= abs_time)
                return false; //timeout

            (void)_helper::__atomic_wait_until_explicit(&m_atomicState32, (uint32_t)0, abs_time, std::memory_order_relaxed);
        }
    }

private:
    mutable std::atomic<uint32_t> m_atomicState32;
    const bool m_manualReset;
};


class ks_event_mac_gdc {
public:
    explicit ks_event_mac_gdc(bool initialState = false, bool manualReset = false)
        : m_spinlock(), m_state_waitgroup(initialState ? 0 : 1)
        , m_state(initialState), m_manualReset(manualReset) {
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_event_mac_gdc);

    void set_event() {
        m_spinlock.lock();

        if (!m_state) {
            m_state = true;

            ASSERT(!m_state_waitgroup.try_wait());
            m_state_waitgroup.done();
            ASSERT(m_state_waitgroup.try_wait());
        }

        m_spinlock.unlock();
    }

    void reset_event() {
        m_spinlock.lock();

        if (m_state) {
            m_state = false;

            ASSERT(m_state_waitgroup.try_wait());
            m_state_waitgroup.add(1);
            ASSERT(!m_state_waitgroup.try_wait());
        }

        m_spinlock.unlock();
    }

    void wait() const {
        m_spinlock.lock();

        while (!m_state) {
            m_spinlock.unlock();
            m_state_waitgroup.wait();
            m_spinlock.lock();
        }

        if (!m_manualReset) {
            m_state = false;

            ASSERT(m_state_waitgroup.try_wait());
            m_state_waitgroup.add(1);
            ASSERT(!m_state_waitgroup.try_wait());
        }

        m_spinlock.unlock();
    }

    _NODISCARD bool try_wait() const {
        bool ret = false;
        m_spinlock.lock();

        if (m_state) {
            ret = true;

            if (!m_manualReset) {
                m_state = false;

                ASSERT(m_state_waitgroup.try_wait());
                m_state_waitgroup.add(1);
                ASSERT(!m_state_waitgroup.try_wait());
            }
        }

        m_spinlock.unlock();
        return ret;
    }

    template<class Rep, class Period>
    _NODISCARD bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const {
        return this->wait_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    _NODISCARD bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const {
        m_spinlock.lock();

        while (!m_state) {
            m_spinlock.unlock();
            if (Clock::now() >= abs_time)
                return false; //timeout
            (void)m_state_waitgroup.wait_until(abs_time);
            m_spinlock.lock();
        }

        if (!m_manualReset) {
            m_state = false;

            ASSERT(m_state_waitgroup.try_wait());
            m_state_waitgroup.add(1);
            ASSERT(!m_state_waitgroup.try_wait());
        }

        m_spinlock.unlock();
        return true;
    }

private:
    mutable ks_spinlock_mac_unfair m_spinlock;
    mutable ks_waitgroup_mac_gdc m_state_waitgroup;
    mutable bool m_state;
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

    template<class Rep, class Period>
    _NODISCARD bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const {
        if (m_use_ossync) {
            return m_ossync_impl.wait_for(rel_time);
        } else {
            return m_gdc_impl.wait_for(rel_time);
        }
    }

    template<class Clock, class Duration>
    _NODISCARD bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const {
        if (m_use_ossync) {
            return m_ossync_impl.wait_until(abs_time);
        } else {
            return m_gdc_impl.wait_until(abs_time);
        }
    }

private:
    union {
        ks_event_mac_ossync m_ossync_impl;
        ks_event_mac_gdc m_gdc_impl;
    };

    const bool m_use_ossync = (bool)(_helper::__getAppleSyncApis()->wait_on_address);
};

} // namespace _KSConcurrencyImpl

#endif // __APPLE__
#endif // __KS_EVENT_MAC_DEF
