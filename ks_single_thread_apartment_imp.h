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


class ks_single_thread_apartment_imp final : public ks_apartment {
public:
	enum { //flag consts
		inplace_thread_flag  = 0x01,
		auto_register_flag   = 0x10,
		be_ui_sta_flag       = 0x20,
		be_master_sta_flag   = 0x40,
		__all_flags          = 0x71,
	};

	KS_ASYNC_API explicit ks_single_thread_apartment_imp(const char* name, uint flags = 0);
	KS_ASYNC_API ~ks_single_thread_apartment_imp();
	_DISABLE_COPY_CONSTRUCTOR(ks_single_thread_apartment_imp);

public:
	virtual const char* name() override;
	virtual uint features() override;

	virtual bool start() override;
	virtual void async_stop() override;
	virtual void wait_for_stopped() override;

	virtual bool is_stopped() override;
	virtual bool is_stopping_or_stopped() override;

	virtual uint64_t schedule(std::function<void()>&& fn, int priority) override;
	virtual uint64_t schedule_delayed(std::function<void()>&& fn, int priority, int64_t delay) override;
	virtual void try_unschedule(uint64_t id) override;

#if __KS_APARTMENT_ATFORK_ENABLED
	virtual void atfork_prepare() override;
	virtual void atfork_parent() override;
	virtual void atfork_child() override;
#endif

protected:
	virtual bool __try_pump_once() override;

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
#if __KS_APARTMENT_ATFORK_ENABLED
		ks_mutex busy_unique_mutex; //因为是sta，所以这里不必使用ks_shared_mutex
#endif
		ks_mutex mutex;

		//prior简化为三级：>0为高优先，=0为普通，<0为低且简单地加入到idle队列
		std::deque<_FN_ITEM> now_fn_queue_prior;
		std::deque<_FN_ITEM> now_fn_queue_normal;
		std::deque<_FN_ITEM> now_fn_queue_idle; //idle任务地位低下，与prior和normal不是同等对待
		std::deque<_FN_ITEM> delaying_fn_queue;
		ks_condition_variable any_fn_queue_cv{};

		std::shared_ptr<std::thread> isolated_thread_opt; //only when isolated_thread_flag
		//bool isolated_thread_presented_flag = false;  //被移动位置，使内存更紧凑

		//volatile _STATE state_v = _STATE::NOT_START;  //被移动位置，使内存更紧凑
		ks_condition_variable stopped_state_cv{};

		std::atomic<uint64_t> atomic_last_fn_id = { 0 };

		//为了使内存布局更紧凑，将部分成员变量集中安置
		std::string name; //const-like
		uint flags; //const-like
		volatile _STATE state_v = _STATE::NOT_START;
#if __KS_APARTMENT_ATFORK_ENABLED
		volatile bool atfork_prepared_flag_v = false;
		bool atfork_calling_in_my_thread_flag = false;
#endif
	};

	_SINGLE_THREAD_APARTMENT_DATA* m_d;
};
