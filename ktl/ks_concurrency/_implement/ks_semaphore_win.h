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

#ifndef __KS_SEMAPHORE_WIN_DEF
#define __KS_SEMAPHORE_WIN_DEF
#if defined(_WIN32)

#include "ks_concurrency_helper.h"


namespace _KSConcurrencyImpl {

class ks_semaphore_win_synch {
public:
    explicit ks_semaphore_win_synch(ptrdiff_t desired) {
        ASSERT(desired >= 0 && (LONG)desired >= 0);
        m_semaphoreHandle = ::CreateSemaphoreW(NULL, (LONG)desired, LONG_MAX, NULL);
        if (m_semaphoreHandle == NULL) {
            ASSERT(false);
            throw std::runtime_error("Failed to create semaphore");
        }
    }

    ~ks_semaphore_win_synch() {
        ::CloseHandle(m_semaphoreHandle);
        //m_semaphoreHandle = NULL;
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_semaphore_win_synch);

    void release(ptrdiff_t update = 1) {
        ASSERT(update > 0 && (ptrdiff_t)(LONG)update == update);
        ::ReleaseSemaphore(m_semaphoreHandle, (LONG)update, NULL);
    }

    void acquire() {
        DWORD ret = ::WaitForSingleObject(m_semaphoreHandle, INFINITE);
        if (ret != WAIT_OBJECT_0) {
            ASSERT(false);
            throw std::runtime_error("Failed to acquire semaphore");
        }
    }

    _NODISCARD bool try_acquire() {
        DWORD ret = ::WaitForSingleObject(m_semaphoreHandle, 0);
        if (ret == WAIT_OBJECT_0) 
            return true;

        if (ret == WAIT_TIMEOUT) {
            return false;
        } else {
            ASSERT(false);
            throw std::runtime_error("Failed to try acquire semaphore");
        }
    }

private:
    HANDLE m_semaphoreHandle;
};

//note: synch is avail always
using ks_semaphore_win = ks_semaphore_win_synch;

} // namespace _KSConcurrencyImpl

#endif // _WIN32
#endif // __KS_SEMAPHORE_WIN_DEF
