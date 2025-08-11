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

#ifndef __KS_SEMAPHORE_LINUX_DEF
#define __KS_SEMAPHORE_LINUX_DEF
#if defined(__linux__)

#include "ks_concurrency_helper.h"
#include <semaphore.h>


namespace _KSConcurrencyImpl {

class ks_semaphore_linux_posix {
public:
    explicit ks_semaphore_linux_posix(ptrdiff_t desired) {
        ASSERT(desired >= 0);
        if (sem_init(&m_sem, 0, desired) != 0) {
            ASSERT(false);
            throw std::runtime_error("Failed to initialize semaphore");
        }
    }
    
    ~ks_semaphore_linux_posix() {
        sem_destroy(&m_sem);
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_semaphore_linux_posix);

    void release(ptrdiff_t update = 1) {
        ASSERT(update > 0);
        for (ptrdiff_t i = 0; i < update; ++i) {
            sem_post(&m_sem);
        }
    }

    void acquire() {
        while (true) {
            if (sem_wait(&m_sem) == 0)
                return;

            if (errno == EINTR) {
                continue;   // 被信号中断，继续重试
            }
            else {
                ASSERT(false);
                throw std::runtime_error("Failed to acquire semaphore");
            }
        }
    }

    _NODISCARD bool try_acquire() {
        return sem_trywait(&m_sem) == 0;
    }

    // 屏蔽try_acquire_for、try_acquire_until

private:
    sem_t m_sem;
};

using ks_semaphore_linux = ks_semaphore_linux_posix;

} // namespace _KSConcurrencyImpl

#endif // __linux__
#endif // __KS_SEMAPHORE_LINUX_DEF
