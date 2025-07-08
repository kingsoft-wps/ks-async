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

#ifndef __KS_SPINLOCK_WIN_DEF
#define __KS_SPINLOCK_WIN_DEF
#if defined(_WIN32)

#include "ks_concurrency_helper.h"


namespace _KSConcurrencyImpl {


class ks_spinlock_win_srw {
public:
    ks_spinlock_win_srw() : m_srwLock{ 0 } {
        ASSERT(_helper::getWinSynchApis()->pfnAcquireSRWLockExclusive != nullptr &&
               _helper::getWinSynchApis()->pfnTryAcquireSRWLockExclusive != nullptr &&
               _helper::getWinSynchApis()->pfnReleaseSRWLockExclusive != nullptr);
    }

   _DISABLE_COPY_CONSTRUCTOR(ks_spinlock_win_srw);

   static bool __is_always_lock_efficient() noexcept {
       return true;
   }
   
   void lock() {
        _helper::getWinSynchApis()->pfnAcquireSRWLockExclusive(&m_srwLock);
    }

    _NODISCARD bool try_lock() {
        return (bool)_helper::getWinSynchApis()->pfnTryAcquireSRWLockExclusive(&m_srwLock);
    }

    void unlock() {
        _helper::getWinSynchApis()->pfnReleaseSRWLockExclusive(&m_srwLock);
    }

private:
    union {
        _helper::SRWLOCK m_srwLock;
        void* m_alignment; // 用于对齐
    };
};


//note: srw is avail since Vista
using ks_spinlock_win = ks_spinlock_win_srw;


} // namespace _KSConcurrencyImpl

#endif // _WIN32
#endif // __KS_SPINLOCK_WIN_DEF
