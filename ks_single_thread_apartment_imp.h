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
#include <deque>
#include <deque>


class ks_single_thread_apartment_imp final : public ks_apartment {
public:
	KS_ASYNC_API explicit ks_single_thread_apartment_imp(bool isolated_thread_mode);
	KS_ASYNC_API ~ks_single_thread_apartment_imp();
	_DISABLE_COPY_CONSTRUCTOR(ks_single_thread_apartment_imp);

	KS_ASYNC_INLINE_API void __use_this_as_ui_sta() { ks_apartment::__set_ui_sta(this); }
	KS_ASYNC_INLINE_API void __use_this_as_master_sta() { ks_apartment::__set_master_sta(this); }

public:
	virtual bool start() override;
	virtual void async_stop() override;
	virtual void wait() override;

	virtual bool is_stopping_or_stopped() override;
	virtual bool is_stopped() override;

	virtual uint64_t schedule(std::function<void()>&& fn, int priority) override;
	virtual uint64_t schedule_delayed(std::function<void()>&& fn, int priority, int64_t delay) override;
	virtual void try_unschedule(uint64_t id) override;

private:
	void _try_start_locked(std::unique_lock<ks_mutex>& lock);
	void _try_stop_locked(std::unique_lock<ks_mutex>& lock);

	void _prepare_single_thread_locked(std::unique_lock<ks_mutex>& lock);
	void _single_thread_proc();

	bool _debug_check_fn_id_exists_locked(uint64_t id, std::unique_lock<ks_mutex>& lock) const;

private:
	struct _FN_ITEM {
		std::function<void()> fn;
		int priority;
		bool is_delaying_fn;
		int64_t delay;
		std::chrono::steady_clock::time_point until_time;
		uint64_t fn_id;
	};

	void _do_put_fn_item_into_now_list_locked(_FN_ITEM&& fn_item, std::unique_lock<ks_mutex>& lock);
	void _do_put_fn_item_into_delaying_list_locked(_FN_ITEM&& fn_item, std::unique_lock<ks_mutex>& lock);

private:
	enum class _STATE { NOT_START, RUNNING, STOPPING, STOPPED };

	struct _SINGLE_THREAD_APARTMENT_DATA {
		ks_mutex mutex;

		//prior简化为三级：>0为高优先，=0为普通，<0为低且简单地加入到idle队列
		std::deque<_FN_ITEM> now_fn_queue_prior;
		std::deque<_FN_ITEM> now_fn_queue_normal;
		std::deque<_FN_ITEM> now_fn_queue_idle; //idle任务地位低下，与prior和normal不是同等对待
		std::deque<_FN_ITEM> delaying_fn_queue;
		ks_condition_variable any_fn_queue_cv{};

		//bool isolated_thread_mode;  //const-like  //被移动位置，使内存更紧凑

		std::shared_ptr<std::thread> isolated_thread_opt; //only when isolated_thread_mode
		//bool isolated_thread_presented_flag = false;  //被移动位置，使内存更紧凑

		//volatile _STATE state_v = _STATE::NOT_START;  //被移动位置，使内存更紧凑

		std::thread::id prime_waiting_thread_id{};
		ks_condition_variable prime_waiting_thread_cv{};

		std::atomic<uint64_t> atomic_last_fn_id = { 0 };

		//为了使内存布局更紧凑，将部分成员变量集中安置
		volatile _STATE state_v = _STATE::NOT_START;
		bool isolated_thread_presented_flag = false;
		bool isolated_thread_mode;  //const-like
	};

	_SINGLE_THREAD_APARTMENT_DATA* m_d;
};
