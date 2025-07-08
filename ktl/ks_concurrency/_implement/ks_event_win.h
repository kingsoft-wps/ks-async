﻿/* Copyright 2025 The Kingsoft's ks-async/ktl Authors. All Rights Reserved.

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
        m_eventHandle = nullptr;
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_event_win_synch);

    void set_event() {
        ::SetEvent(m_eventHandle);
    }

    void reset_event() {
        ::ResetEvent(m_eventHandle);
    }

    void wait() {
        DWORD ret = ::WaitForSingleObject(m_eventHandle, INFINITE);
        if (ret != WAIT_OBJECT_0) {
            ASSERT(false);
            throw std::runtime_error("Failed to wait event");
        }
    }

    _NODISCARD bool try_wait() {
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

private:
    HANDLE m_eventHandle;
};

//note: synch is avail always
using ks_event_win = ks_event_win_synch;

} // namespace _KSConcurrencyImpl

#endif // _WIN32
#endif // __KS_EVENT_WIN_DEF
