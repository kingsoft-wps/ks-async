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

#ifndef __KS_WAITGROUP_WIN_DEF
#define __KS_WAITGROUP_WIN_DEF
#if defined(_WIN32)

#include "ks_concurrency_helper.h"


namespace _KSConcurrencyImpl {

class ks_waitgroup_win_synch {
public:
    explicit ks_waitgroup_win_synch(ptrdiff_t desired)
        : m_counter(desired) {
        ASSERT(desired >= 0);
        ASSERT(_helper::__getWinSynchApis()->pfnWaitOnAddress != nullptr &&
               _helper::__getWinSynchApis()->pfnWakeByAddressSingle != nullptr &&
               _helper::__getWinSynchApis()->pfnWakeByAddressAll != nullptr);
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_waitgroup_win_synch);

    void add(ptrdiff_t update) {
        ASSERT(update > 0);
        m_counter.fetch_add(update, std::memory_order_relaxed);
    }

    void done() {
        constexpr ptrdiff_t update = 1;
        ASSERT(update > 0);
        ptrdiff_t current = m_counter.fetch_sub(update, std::memory_order_release) - update;
        ASSERT(current >= 0);
        if (current == 0) {
            _helper::__atomic_notify_all(&m_counter);
        }
    }

    void wait() const {
        for (;;) {
            ptrdiff_t current = m_counter.load(std::memory_order_acquire);
            ASSERT(current >= 0);
            if (current == 0) 
                return;

            _helper::__atomic_wait_explicit(&m_counter, current, std::memory_order_relaxed);
        }
    }

    _NODISCARD bool try_wait() const {
        return m_counter.load(std::memory_order_acquire) == 0;
    }

    template<class Rep, class Period>
    _NODISCARD bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const {
        return this->wait_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    _NODISCARD bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const {
        for (;;) {
            ptrdiff_t current = m_counter.load(std::memory_order_acquire);
            ASSERT(current >= 0);
            if (current == 0) 
                return true;

            if (Clock::now() >= abs_time)
                return false; //timeout

            auto remain_time = abs_time -  std::chrono::steady_clock::now();
            long long remain_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remain_time).count();
            if (remain_ms < 0)
                remain_ms = 0;
            else if (remain_ms >= (long long)MAXDWORD)
                remain_ms = (long long)MAXDWORD - 1; //可能分为多次wait

            _helper::__atomic_wait_until_explicit(&m_counter, current, abs_time, std::memory_order_relaxed);
        }
    }

private:
    std::atomic<ptrdiff_t> m_counter;
};


class ks_waitgroup_win_ev {
public:
    explicit ks_waitgroup_win_ev(ptrdiff_t desired)
        : m_counter(desired) {
        ASSERT(desired >= 0);
        m_eventHandle = ::CreateEventW(NULL, TRUE, desired > 0, NULL);
        if (m_eventHandle == NULL) {
            ASSERT(false);
            throw std::runtime_error("Failed to create event handle");
        }
    }

    ~ks_waitgroup_win_ev() {
        ::CloseHandle(m_eventHandle);
        m_eventHandle = NULL;
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_waitgroup_win_ev);

    void add(ptrdiff_t update) {
        ASSERT(update >= 0);
        ptrdiff_t old_value = m_counter.fetch_add(update, std::memory_order_relaxed);
        ASSERT(old_value + update >= 0);
        if (update != 0 && old_value <= 0 && old_value + update > 0) {
            ::ResetEvent(m_eventHandle);
        }
    }

    void done() {
        constexpr ptrdiff_t update = 1;
        ASSERT(update >= 0);
        ptrdiff_t old_value = m_counter.fetch_sub(update, std::memory_order_release);
        ASSERT(old_value - update >= 0);
        if (update != 0 && old_value > 0 && old_value - update <= 0) {
            ::SetEvent(m_eventHandle);
        }
    }

    void wait() const {
        for (;;) {
            ptrdiff_t current = m_counter.load(std::memory_order_acquire);
            ASSERT(current >= 0);
            if (current == 0)
                return;

            std::this_thread::yield(); //因为counter++与ResetEvent有时间差，故这里额外yield一下

            DWORD ret = ::WaitForSingleObject(m_eventHandle, INFINITE);
            if (ret != WAIT_OBJECT_0) {
                ASSERT(false);
                throw std::runtime_error("Failed to wait event");
            }
        }
    }

    _NODISCARD bool try_wait() const {
        return m_counter.load(std::memory_order_acquire) == 0;
    }

    template<class Rep, class Period>
    _NODISCARD bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const {
        return this->wait_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    _NODISCARD bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const {
        for (;;) {
            ptrdiff_t current = m_counter.load(std::memory_order_acquire);
            ASSERT(current >= 0);
            if (current == 0)
                return true;

            if (Clock::now() >= abs_time)
                return false; //timeout

            std::this_thread::yield(); //因为counter++与ResetEvent有时间差，故这里额外yield一下

            auto remain_time = abs_time -  std::chrono::steady_clock::now();
            long long remain_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remain_time).count();
            if (remain_ms < 0)
                remain_ms = 0;
            else if (remain_ms >= (long long)MAXDWORD)
                remain_ms = (long long)MAXDWORD - 1; //可能分为多次wait

            DWORD ret = ::WaitForSingleObject(m_eventHandle, (DWORD)remain_ms);
            if (ret != WAIT_OBJECT_0 && ret != WAIT_TIMEOUT) {
                ASSERT(false);
                throw std::runtime_error("Failed to wait event");
            }
        }
    }

private:
    std::atomic<ptrdiff_t> m_counter;
    HANDLE m_eventHandle;
};


class ks_waitgroup_win {
public:
    explicit ks_waitgroup_win(ptrdiff_t desired) {
        if (m_use_synch)
            new (&m_synch_impl) ks_waitgroup_win_synch(desired);
        else
            new (&m_ev_impl) ks_waitgroup_win_ev(desired);
    }

    ~ks_waitgroup_win() {
        if (m_use_synch)
            m_synch_impl.~ks_waitgroup_win_synch();
        else
            m_ev_impl.~ks_waitgroup_win_ev();
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_waitgroup_win);

    void add(ptrdiff_t update) {
        if (m_use_synch)
            m_synch_impl.add(update);
        else
            m_ev_impl.add(update);
    }

    void done() {
        if (m_use_synch)
            m_synch_impl.done();
        else
            m_ev_impl.done();
    }

    void wait() const {
        if (m_use_synch)
            m_synch_impl.wait();
        else
            m_ev_impl.wait();
    }

    _NODISCARD bool try_wait() const {
        if (m_use_synch)
            return m_synch_impl.try_wait();
        else
            return m_ev_impl.try_wait();
    }

    template<class Rep, class Period>
    _NODISCARD bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const {
        if (m_use_synch)
            return m_synch_impl.wait_for(rel_time);
        else
            return m_ev_impl.wait_for(rel_time);
    }

    template<class Clock, class Duration>
    _NODISCARD bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const {
        if (m_use_synch)
            return m_synch_impl.wait_until(abs_time);
        else
            return m_ev_impl.wait_until(abs_time);
    }

private:
    union {
        ks_waitgroup_win_synch m_synch_impl;
        ks_waitgroup_win_ev m_ev_impl;
    };

    const bool m_use_synch = (bool)(_helper::__getWinSynchApis()->pfnWaitOnAddress);
};

} // namespace _KSConcurrencyImpl

#endif // _WIN32
#endif // __KS_WAITGROUP_WIN_DEF
