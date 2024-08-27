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
#include "ktl/ks_concurrency.h"


class ks_async_controller final {
public:
	ks_async_controller() : m_controller_data_ptr(std::make_shared<_CONTROLLER_DATA>()) {}
	_DISABLE_COPY_CONSTRUCTOR(ks_async_controller);

	~ks_async_controller() {
		ASSERT(!this->has_pending_futures());
		this->try_cancel_all(); //自动cancel-all
	}

public:
	void try_cancel_all() {
		m_controller_data_ptr->cancel_all_ctrl_v = true;
	}

	bool has_pending_futures() const {
		return m_controller_data_ptr->pending_latch.try_wait() == false;
	}

	//慎用，使用不当可能会造成死锁或卡顿！
	template <class _ = void>
	_DECL_DEPRECATED void wait_pending_futures_done() const {
		//特别说明：
		//与ks_raw_future::wait情况类似，甚至更危险，因为这里连基本的跨apartment检查都没有，极易死锁。
		m_controller_data_ptr->pending_latch.wait();
	}

private:
	struct _CONTROLLER_DATA {
		ks_latch pending_latch { 0 };
		volatile bool cancel_all_ctrl_v = false;
	};

	std::shared_ptr<_CONTROLLER_DATA> m_controller_data_ptr;

	friend class ks_async_context;
};
