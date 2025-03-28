﻿/* Copyright 2024 The Kingsoft's ks-async Authors. All Rights Reserved.

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
#include "ktl/ks_functional.h"


class ks_apartment {
protected:
	KS_ASYNC_API ks_apartment() = default;
	KS_ASYNC_API virtual ~ks_apartment() = default;  //protected
	_DISABLE_COPY_CONSTRUCTOR(ks_apartment);

public:
	KS_ASYNC_API static ks_apartment* ui_sta();         //UI[单线程]套间，需由APP框架提供（一般是主线程，也可以不是）
	KS_ASYNC_API static ks_apartment* master_sta();     //主逻辑[单线程]套间，亦由APP框架提供（可以与ui-sta相同，但最好区别开）
	KS_ASYNC_API static ks_apartment* background_sta(); //后台[单线程]套间
	KS_ASYNC_API static ks_apartment* default_mta();    //默认[多线程]套间

	KS_ASYNC_API static ks_apartment* current_thread_apartment();
	KS_ASYNC_API static ks_apartment* current_thread_apartment_or_default_mta();
	KS_ASYNC_API static ks_apartment* current_thread_apartment_or(ks_apartment* or_apartment);

	KS_ASYNC_API static ks_apartment* find_public_apartment(const char* name);

public:
	KS_ASYNC_INLINE_API static ks_apartment* __virtual_inplace_apartment() { return (ks_apartment*)(void*)(-1); }

public:
	enum { //feature consts
		sequential_feature = 0x01,
	};

public:
	virtual const char* name() = 0;
	virtual uint features() = 0;

public:
	virtual bool start() = 0;
	virtual void async_stop() = 0;
	virtual void wait() = 0;

	virtual bool is_stopped() = 0;
	virtual bool is_stopping_or_stopped() = 0;

public:
	//注：schedule方法的uint返回值代表异步过程的id，且成功时要求返回一个“非零值”，后续可被用于try_unschedule调用传参。
	virtual uint64_t schedule(std::function<void()>&& fn, int priority) = 0;
	virtual uint64_t schedule_delayed(std::function<void()>&& fn, int priority, int64_t delay) = 0;
	//注：try_unschedule方法会尝试取消指定的异步过程，其前提是指定的异步过程还未开始执行，若已开始（甚至已完成）则不会再被取消了。
	virtual void try_unschedule(uint64_t id) = 0;

public:
	virtual void atfork_prepare() { ASSERT(false); }
	virtual void atfork_parent() { ASSERT(false); }
	virtual void atfork_child() { ASSERT(false); }

protected:
	//注：主动泵下一条异步任务。
	virtual bool __try_pump_once() { ASSERT(false); return false; }

public:
	//注：ui_sta和master_sta由APP框架提供。
	//注意：current_thread_apartment是TLS变量，各色套间线程实现者务必对其进行正确初始化。
	KS_ASYNC_API static void __set_ui_sta(ks_apartment* ui_sta);
	KS_ASYNC_API static void __set_master_sta(ks_apartment* master_sta);
	KS_ASYNC_API static void __tls_set_current_thread_apartment(ks_apartment* current_thread_apartment);

	KS_ASYNC_API static void __unset_ui_sta(ks_apartment* ui_sta);
	KS_ASYNC_API static void __unset_master_sta(ks_apartment* master_sta);
	KS_ASYNC_API static void __tls_unset_current_thread_apartment(ks_apartment* current_thread_apartment);

	KS_ASYNC_API static void __register_public_apartment(const char* name, ks_apartment* apartment);
	KS_ASYNC_API static void __unregister_public_apartment(const char* name, ks_apartment* apartment);

public:
	//注：设定default-mta最大线程数，请在首次调用default_mta()方法前调用
	KS_ASYNC_API static void __set_default_mta_max_thread_count(size_t max_thread_count);
	//注：主动泵当前线程所属套间的下一条异步任务
	KS_ASYNC_API static bool __try_pump_current_thread_apartment_once();
};
