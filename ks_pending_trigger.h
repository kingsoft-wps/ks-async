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
#include "ks-async-raw/ks_raw_promise.h"

class ks_pending_trigger final {
public:
	KS_ASYNC_INLINE_API ks_pending_trigger() : m_trigger_promise(__do_create_raw_trigger_promise()) {}

	_DISABLE_COPY_CONSTRUCTOR(ks_pending_trigger);
	KS_ASYNC_INLINE_API ks_pending_trigger(ks_pending_trigger&&) noexcept = default;

public:
	KS_ASYNC_INLINE_API void start() {
		return m_trigger_promise->resolve(ks_raw_value::of<nothing_t>(nothing));
	}

	KS_ASYNC_INLINE_API void cancel() {
		return m_trigger_promise->reject(ks_error::cancelled_error());
	}

private:
	using ks_raw_promise = __ks_async_raw::ks_raw_promise;
	using ks_raw_promise_ptr = __ks_async_raw::ks_raw_promise_ptr;
	using ks_raw_future = __ks_async_raw::ks_raw_future;
	using ks_raw_future_ptr = __ks_async_raw::ks_raw_future_ptr;
	using ks_raw_result = __ks_async_raw::ks_raw_result;
	using ks_raw_value = __ks_async_raw::ks_raw_value;

private:
	KS_ASYNC_INLINE_API ks_raw_future_ptr __get_raw_trigger_future() const noexcept {
		return m_trigger_promise->get_future();
	}

	KS_ASYNC_INLINE_API bool __was_triggered() const noexcept {
		return m_trigger_promise->get_future()->peek_result().is_completed();
	}

private:
	static ks_raw_promise_ptr __do_create_raw_trigger_promise() {
		ks_apartment* apartment_hint = ks_apartment::current_thread_apartment_or_default_mta();
		return ks_raw_promise::create(apartment_hint);
	}

	template <class T> friend class ks_future;

private:
	ks_raw_promise_ptr m_trigger_promise;
};
