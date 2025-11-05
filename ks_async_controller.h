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

#include "ks_async_base.h"
#include "ks_apartment.h"
#include "ktl/ks_concurrency.h"


class ks_async_controller final {
public:
	KS_ASYNC_API ks_async_controller();
	_DISABLE_COPY_CONSTRUCTOR(ks_async_controller);

	KS_ASYNC_API ~ks_async_controller() noexcept;

public:
	KS_ASYNC_INLINE_API void __mark_bound_with_aproc(bool bound_with_aproc) noexcept {
		//ASSERT(bound_with_aproc);
		//ASSERT(!m_controller_data_ptr->is_bound_with_aproc);
		//ASSERT(m_controller_data_ptr->pending_count == 0);
		//const_cast<bool&>(m_controller_data_ptr->is_bound_with_aproc) = bound_with_aproc;
		_NOOP();
	}

public:
	KS_ASYNC_INLINE_API void try_cancel() noexcept {
		m_controller_data_ptr->do_try_cancel();
	}

	KS_ASYNC_INLINE_API bool check_cancelled() noexcept {
		return m_controller_data_ptr->do_check_cancelled();
	}

private:
	struct _CONTROLLER_DATA {
		std::atomic<bool> cancel_ctrl{false};

		void do_try_cancel() noexcept { this->cancel_ctrl.store(true, std::memory_order_relaxed); }
		bool do_check_cancelled() noexcept { return this->cancel_ctrl.load(std::memory_order_relaxed); }
	};

private:
	const std::shared_ptr<_CONTROLLER_DATA> m_controller_data_ptr;

	friend class ks_async_context;
};
