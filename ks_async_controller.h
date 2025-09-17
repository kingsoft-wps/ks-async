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
	KS_ASYNC_INLINE_API ks_async_controller() 
		: m_controller_data_ptr(std::make_shared<_CONTROLLER_DATA>()) {
	}

	KS_ASYNC_INLINE_API ~ks_async_controller() {
		ASSERT(!m_controller_data_ptr->is_bound_with_aproc ? this->is_all_completed() : true);
		this->try_cancel();  //析构时自动try_cancel
	}

	_DISABLE_COPY_CONSTRUCTOR(ks_async_controller);

public:
	void __mark_bound_with_aproc(bool bound_with_aproc) {
		ASSERT(bound_with_aproc);
		ASSERT(!m_controller_data_ptr->is_bound_with_aproc);
		ASSERT(m_controller_data_ptr->pending_count == 0);
		const_cast<bool&>(m_controller_data_ptr->is_bound_with_aproc) = bound_with_aproc;
	}

public:
	KS_ASYNC_INLINE_API void try_cancel() {
		m_controller_data_ptr->cancel_ctrl_v = true;
	}

	KS_ASYNC_INLINE_API bool check_cancelled() {
		return m_controller_data_ptr->cancel_ctrl_v;
	}

public:
	KS_ASYNC_INLINE_API bool is_all_completed() const {
		return m_controller_data_ptr->pending_count == 0;
	}

	//慎用，使用不当可能会造成死锁或卡顿！
	_DECL_DEPRECATED KS_ASYNC_API void __wait_all() const;

private:
	struct _CONTROLLER_DATA {
		volatile bool cancel_ctrl_v = false;
		const bool is_bound_with_aproc = false; //const-like
		std::atomic<int> pending_count{ 0 };
	};

	const std::shared_ptr<_CONTROLLER_DATA> m_controller_data_ptr;

	friend class ks_async_context;
};
