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
#include "ks_future.h"
#include "ks_result.h"
#include "ks-async-raw/ks_raw_future.h"
#include "ks-async-raw/ks_raw_promise.h"


template <class T>
class ks_promise final {
public:
	ks_promise(nullptr_t) : m_raw_promise(nullptr) {}

	explicit ks_promise(std::create_inst_t) : m_raw_promise(__do_create_raw_promise()) {}
	static ks_promise<T> create() { return ks_promise<T>(std::create_inst); }

	ks_promise(const ks_promise&) = default;
	ks_promise(ks_promise&&) noexcept = default;

	ks_promise& operator=(const ks_promise&) = default;
	ks_promise& operator=(ks_promise&&) noexcept = default;

	//让ks_promise看起来像一个智能指针
	ks_promise* operator->() { return this; }
	const ks_promise* operator->() const { return this; }

	using arg_type = T;
	using value_type = T;
	using this_promise_type = ks_promise<T>;

public:
	bool is_null() const {
		return m_raw_promise == nullptr;
	}
	bool is_valid() const {
		return m_raw_promise != nullptr;
	}
	bool operator==(nullptr_t) const {
		return m_raw_promise == nullptr;
	}
	bool operator!=(nullptr_t) const {
		return m_raw_promise != nullptr;
	}

	ks_future<T> get_future() const {
		ASSERT(!this->is_null());
		return ks_future<T>::__from_raw(m_raw_promise->get_future());
	}

	void resolve(const T& value) const {
		ASSERT(!this->is_null());
		m_raw_promise->resolve(ks_raw_value::of<T>(value));
	}
	void resolve(T&& value) const {
		ASSERT(!this->is_null());
		m_raw_promise->resolve(ks_raw_value::of<T>(std::move(value)));
	}

	void reject(const ks_error& error) const {
		ASSERT(!this->is_null());
		m_raw_promise->reject(error);
	}

	void try_settle(const ks_result<T>& result) const {
		ASSERT(!this->is_null());
		m_raw_promise->try_settle(result.__get_raw());
	}

private:
	using ks_raw_future = __ks_async_raw::ks_raw_future;
	using ks_raw_future_ptr = __ks_async_raw::ks_raw_future_ptr;
	using ks_raw_promise = __ks_async_raw::ks_raw_promise;
	using ks_raw_promise_ptr = __ks_async_raw::ks_raw_promise_ptr;
	using ks_raw_result = __ks_async_raw::ks_raw_result;
	using ks_raw_value = __ks_async_raw::ks_raw_value;

	explicit ks_promise(const ks_raw_promise_ptr& raw_promise, int) : m_raw_promise(raw_promise) {}
	explicit ks_promise(ks_raw_promise_ptr&& raw_promise, int) : m_raw_promise(std::move(raw_promise)) {}

	static ks_promise<T> __from_raw(const ks_raw_promise_ptr& raw_promise) { return ks_promise<T>(raw_promise, 0); }
	static ks_promise<T> __from_raw(ks_raw_promise_ptr&& raw_promise) { return ks_promise<T>(std::move(raw_promise), 0); }
	const ks_raw_promise_ptr& __get_raw() const { return m_raw_promise; }

	static ks_raw_promise_ptr __do_create_raw_promise() {
		ks_apartment* apartment_hint = ks_apartment::current_thread_apartment_or_default_mta();
		return ks_raw_promise::create(apartment_hint);
	}

	template <class T2> friend class ks_future;
	template <class T2> friend class ks_promise;
	friend class ks_future_util;
	friend class ks_async_flow;

private:
	ks_raw_promise_ptr m_raw_promise;
};


#include "ks_promise_void.inl"
