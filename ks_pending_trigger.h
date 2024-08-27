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
	ks_pending_trigger() : m_pending_promise(ks_raw_promise::create(ks_apartment::current_thread_apartment_or_default_mta())) {}
	_DISABLE_COPY_CONSTRUCTOR(ks_pending_trigger);

	~ks_pending_trigger() {
		if (!m_pending_promise->get_future()->peek_result().is_completed()) {
			ASSERT(false);
			m_pending_promise->resolve(ks_raw_value::of(nothing)); //auto start
		}
	}

public:
	void start() {
		m_pending_promise->resolve(ks_raw_value::of(nothing));
	}
	void cancel() {
		m_pending_promise->reject(ks_error::was_cancelled_error());
	}

private:
	using ks_raw_promise = __ks_async_raw::ks_raw_promise;
	using ks_raw_promise_ptr = __ks_async_raw::ks_raw_promise_ptr;

	using ks_raw_result = __ks_async_raw::ks_raw_result;
	using ks_raw_value = __ks_async_raw::ks_raw_value;

	template <class T> friend class ks_future;

private:
	ks_raw_promise_ptr m_pending_promise;
};
