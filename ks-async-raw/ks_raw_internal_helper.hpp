/* Copyright 2024 The Kingsoft's ks-async Authors. All Rights Reserved.

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

#include "../ks_async_base.h"
#include "ks_raw_future.h"

__KS_ASYNC_RAW_BEGIN


class ks_raw_running_future_rtstt final {
public:
	ks_raw_running_future_rtstt() {}
	~ks_raw_running_future_rtstt() { this->try_unapply(); }

	_DISABLE_COPY_CONSTRUCTOR(ks_raw_running_future_rtstt);

public:
	void apply(ks_raw_future* cur_future, ks_raw_future** tls_current_thread_running_future_addr) {
		if (m_applied_flag) {
			ASSERT(false);
			this->try_unapply();
		}

		ASSERT(cur_future != nullptr);
		ASSERT(tls_current_thread_running_future_addr != nullptr);

		m_applied_flag = true;
		m_tls_current_thread_running_future_addr = tls_current_thread_running_future_addr;
		m_cur_future = cur_future;
		m_current_thread_future_backup = *m_tls_current_thread_running_future_addr;
		*m_tls_current_thread_running_future_addr = cur_future;
	}

	void try_unapply() {
		if (!m_applied_flag)
			return;

		ASSERT(*m_tls_current_thread_running_future_addr == m_cur_future);

		m_applied_flag = false;
		*m_tls_current_thread_running_future_addr = m_current_thread_future_backup;
		m_tls_current_thread_running_future_addr = nullptr;
		m_cur_future = nullptr;
		m_current_thread_future_backup = nullptr;
	}

private:
	bool m_applied_flag = false;
	ks_raw_future** m_tls_current_thread_running_future_addr = nullptr;
	ks_raw_future* m_cur_future = nullptr;
	ks_raw_future* m_current_thread_future_backup = nullptr;
};


class ks_raw_living_context_rtstt final {
public:
	ks_raw_living_context_rtstt() {}
	~ks_raw_living_context_rtstt() { this->try_unapply(); }

	_DISABLE_COPY_CONSTRUCTOR(ks_raw_living_context_rtstt);

public:
	void apply(const ks_async_context& cur_context) {
		if (m_applied_flag) {
			ASSERT(false);
			this->try_unapply();
		}

		m_applied_flag = true;

		m_cur_context = cur_context;
		m_cur_context_owner_locker = m_cur_context.__lock_owner_ptr();
		m_cur_context.__increment_pending_count();
	}

	void try_unapply() {
		if (!m_applied_flag)
			return;

		m_cur_context.__unlock_owner_ptr(m_cur_context_owner_locker);
		m_cur_context.__decrement_pending_count();

		m_applied_flag = false;
		m_cur_context = {};
		m_cur_context_owner_locker = {};
	}

	ks_any get_owner_locker() const {
		return m_cur_context_owner_locker;
	}

private:
	bool m_applied_flag = false;
	ks_async_context m_cur_context;
	ks_any m_cur_context_owner_locker;
};


__KS_ASYNC_RAW_END
