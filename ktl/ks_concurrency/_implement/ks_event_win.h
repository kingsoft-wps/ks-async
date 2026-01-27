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

#ifndef __KS_EVENT_WIN_DEF
#define __KS_EVENT_WIN_DEF
#if defined(_WIN32)

#include "ks_concurrency_helper.h"


namespace _KSConcurrencyImpl {

class ks_event_win_synch {
public:
    explicit ks_event_win_synch(bool initialState, bool manualReset) {
        m_eventHandle = ::CreateEventW(nullptr, manualReset, initialState, nullptr);
        if (m_eventHandle == NULL) {
            ASSERT(false);
            throw std::runtime_error("Failed to create event");
        }
    }

    ~ks_event_win_synch() {
        ::CloseHandle(m_eventHandle);
        //m_eventHandle = NULL;
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_event_win_synch);

    void set_event() {
        ::SetEvent(m_eventHandle);
    }

    void reset_event() {
        ::ResetEvent(m_eventHandle);
    }

    void wait() const {
        DWORD ret = ::WaitForSingleObject(m_eventHandle, INFINITE);
        if (ret != WAIT_OBJECT_0) {
            ASSERT(false);
            throw std::runtime_error("Failed to wait event");
        }
    }

    _NODISCARD bool try_wait() const {
        DWORD ret = ::WaitForSingleObject(m_eventHandle, 0);
        if (ret == WAIT_OBJECT_0) 
            return true;

        if (ret == WAIT_TIMEOUT) {
            return false;
        } else {
            ASSERT(false);
            throw std::runtime_error("Failed to try wait event");
        }
    }

    template<class Rep, class Period>
    _NODISCARD bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const {
        return this->wait_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    _NODISCARD bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const {
        for (;;) {
            auto remain_time = abs_time - Clock::now();
            long long remain_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remain_time).count();
            if (remain_ms < 0)
                remain_ms = 0;
            else if (remain_ms >= (long long)MAXDWORD)
                remain_ms = (long long)MAXDWORD - 1; //可能分为多次wait

            DWORD ret = ::WaitForSingleObject(m_eventHandle, (DWORD)remain_ms);
            if (ret == WAIT_OBJECT_0) {
                return true;
            }
            else if (ret == WAIT_TIMEOUT) {
                if (remain_ms == 0 || Clock::now() >= abs_time)
                    return false; //timeout
                else
                    continue;
            }
            else {
                ASSERT(false);
                throw std::runtime_error("Failed to try wait event");
            }
        }
    }

private:
    HANDLE m_eventHandle;
};

//note: synch is avail always
using ks_event_win = ks_event_win_synch;

} // namespace _KSConcurrencyImpl

#endif // _WIN32
#endif // __KS_EVENT_WIN_DEF
