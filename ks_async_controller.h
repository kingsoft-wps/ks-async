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
	ks_async_controller() : m_controller_data_ptr(std::make_shared<_CONTROLLER_DATA>()) {}

	_DISABLE_COPY_CONSTRUCTOR(ks_async_controller);
	ks_async_controller(ks_async_controller&&) noexcept = default;

	~ks_async_controller() { ASSERT(this->is_all_completed()); }

public:
	void try_cancel() {
		m_controller_data_ptr->cancel_ctrl_v = true;
	}

	bool check_cancelled() {
		return m_controller_data_ptr->cancel_ctrl_v;
	}

private:
	bool is_all_completed() const {
		return m_controller_data_ptr->pending_latch.try_wait();
	}

	//慎用，使用不当可能会造成死锁或卡顿！
	void __wait_all() const {
		ks_apartment* cur_apartment = ks_apartment::current_thread_apartment();
		ASSERT(cur_apartment == nullptr || (cur_apartment->features() & ks_apartment::nested_pump_aware_future) != 0);
		if (cur_apartment != nullptr && (cur_apartment->features() & ks_apartment::nested_pump_suppressed_future) == 0) {
			bool was_satisfied = cur_apartment->__run_nested_pump_loop_for_extern_waiting(
				m_controller_data_ptr.get(),
				[controller_data_ptr = m_controller_data_ptr]() -> bool { return controller_data_ptr->pending_latch.try_wait(); });
			ASSERT(was_satisfied ? m_controller_data_ptr->pending_latch.try_wait() : true);
		}
		else {
			m_controller_data_ptr->pending_latch.wait();
		}
	}

private:
	struct _CONTROLLER_DATA {
		ks_latch pending_latch { 0 };
		volatile bool cancel_ctrl_v = false;
	};

	std::shared_ptr<_CONTROLLER_DATA> m_controller_data_ptr;

	friend class ks_async_context;
};
