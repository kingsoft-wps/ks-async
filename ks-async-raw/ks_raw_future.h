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
#include "../ks_apartment.h"
#include "../ks_async_context.h"
#include "ks_raw_result.h"

__KS_ASYNC_RAW_BEGIN


class ks_raw_future;
using ks_raw_future_ptr = std::shared_ptr<ks_raw_future>;

_ABSTRACT class ks_raw_future {
protected:
	ks_raw_future() = default;
	~ks_raw_future() = default;  //protected
	_DISABLE_COPY_CONSTRUCTOR(ks_raw_future);

public:
	KS_ASYNC_API static ks_raw_future_ptr resolved(const ks_raw_value& value, ks_apartment* apartment);
	KS_ASYNC_API static ks_raw_future_ptr rejected(const ks_error& error, ks_apartment* apartment);
	KS_ASYNC_API static ks_raw_future_ptr __from_result(const ks_raw_result& result, ks_apartment* apartment);

	KS_ASYNC_API static ks_raw_future_ptr post(std::function<ks_raw_result()>&& task_fn, const ks_async_context& context, ks_apartment* apartment);
	KS_ASYNC_API static ks_raw_future_ptr post_delayed(std::function<ks_raw_result()>&& task_fn, const ks_async_context& context, ks_apartment* apartment, int64_t delay);

	KS_ASYNC_API static ks_raw_future_ptr all(const std::vector<ks_raw_future_ptr>& futures, ks_apartment* apartment);
	KS_ASYNC_API static ks_raw_future_ptr all_completed(const std::vector<ks_raw_future_ptr>& futures, ks_apartment* apartment);
	KS_ASYNC_API static ks_raw_future_ptr any(const std::vector<ks_raw_future_ptr>& futures, ks_apartment* apartment);

public:
	virtual ks_raw_future_ptr then(std::function<ks_raw_result(const ks_raw_value &)>&& fn, const ks_async_context& context, ks_apartment* apartment) = 0;
	virtual ks_raw_future_ptr trap(std::function<ks_raw_result(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) = 0;
	virtual ks_raw_future_ptr transform(std::function<ks_raw_result(const ks_raw_result &)>&& fn, const ks_async_context& context, ks_apartment* apartment) = 0;

	virtual ks_raw_future_ptr flat_then(std::function<ks_raw_future_ptr(const ks_raw_value&)>&& fn, const ks_async_context& context, ks_apartment* apartment) = 0;
	virtual ks_raw_future_ptr flat_trap(std::function<ks_raw_future_ptr(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) = 0;
	virtual ks_raw_future_ptr flat_transform(std::function<ks_raw_future_ptr(const ks_raw_result&)>&& fn, const ks_async_context& context, ks_apartment* apartment) = 0;

	virtual ks_raw_future_ptr on_success(std::function<void(const ks_raw_value&)>&& fn, const ks_async_context& context, ks_apartment* apartment) = 0;
	virtual ks_raw_future_ptr on_failure(std::function<void(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) = 0;
	virtual ks_raw_future_ptr on_completion(std::function<void(const ks_raw_result&)>&& fn, const ks_async_context& context, ks_apartment* apartment) = 0;

	virtual ks_raw_future_ptr noop(ks_apartment* apartment) = 0;

public:
	virtual bool is_completed() = 0;
	virtual ks_raw_result peek_result() = 0;

	virtual void set_timeout(int64_t timeout, bool backtrack);

	//不希望直接使用future.try_cancel，更应使用controller.try_cancel
	virtual void __try_cancel(bool backtrack);
	KS_ASYNC_API static bool __check_current_future_cancelled(bool with_extra);
	KS_ASYNC_API static ks_error __acquire_current_future_cancelled_error(const ks_error& def_error, bool with_extra);

	//慎用，使用不当可能会造成死锁或卡顿！
	virtual void __wait();

protected:
	virtual void do_add_next(const ks_raw_future_ptr& next_future) = 0;
	virtual void do_add_next_multi(const std::vector<ks_raw_future_ptr>& next_futures) = 0;

	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) = 0;
	virtual void do_complete(const ks_raw_result& result, ks_apartment* prefer_apartment, bool from_internal) = 0;

	virtual void do_set_timeout(int64_t timeout, const ks_error& error, bool backtrack) = 0;

	virtual void do_try_cancel(const ks_error& error, bool backtrack) = 0;
	virtual bool is_cancelable_self() = 0;
	virtual bool do_check_cancelled() = 0;
	virtual ks_error do_acquire_cancelled_error(const ks_error& def_error) = 0;

	virtual bool do_wait() = 0;

protected:
	friend class ks_raw_future_baseimp;
	friend class ks_raw_dx_future;
	friend class ks_raw_promise_future;
	friend class ks_raw_task_future;
	friend class ks_raw_pipe_future;
	friend class ks_raw_flatten_future;
	friend class ks_raw_aggr_future;
};


__KS_ASYNC_RAW_END
