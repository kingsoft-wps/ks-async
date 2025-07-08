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

#ifndef __KS_EVENT_STD_DEF
#define __KS_EVENT_STD_DEF

#include "ks_concurrency_helper.h"
#include <mutex>
#include <condition_variable>

namespace _KSConcurrencyImpl {

class ks_event_std {
public:
    explicit ks_event_std(bool initialState, bool manualReset)
        : m_state(initialState), m_manualReset(manualReset) {
    }

    _DISABLE_COPY_CONSTRUCTOR(ks_event_std);

    void set_event() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_state) {
            m_state = true;

            if (m_manualReset)
                m_cv.notify_all();
            else
                m_cv.notify_one();
        }
    }
    void reset_event() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_state = false;
    }

    void wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_state) {
            m_cv.wait(lock);
        }

        if (!m_manualReset) {
            m_state = false;
        }
    }

    _NODISCARD bool try_wait() {
		std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_state)
            return false;

        if (!m_manualReset) {
            m_state = false;
        }
        return true;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_state;
    const bool m_manualReset;
};

} // namespace _KSConcurrencyImpl

#endif // __KS_EVENT_STD_DEF
