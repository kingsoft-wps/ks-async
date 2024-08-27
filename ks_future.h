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
#include "ks_result.h"
#include "ks-async-raw/ks_raw_future.h"
#include "ks_cancel_inspector.h"
#include "ks_pending_trigger.h"

template <class T> class ks_promise;


template <class T>
class ks_future final {
public:
	ks_future(nullptr_t) : m_raw_future(nullptr) {}
	ks_future(const ks_future&) = default;
	ks_future& operator=(const ks_future&) = default;
	ks_future(ks_future&&) noexcept = default;
	ks_future& operator=(ks_future&&) noexcept = default;

	using value_type = T;
	using this_future_type = ks_future<T>;

public:
	static ks_future<T> resolved(const T& value) {
		ks_apartment* apartment_hint = ks_apartment::default_mta();
		ks_raw_future_ptr raw_future = ks_raw_future::resolved(ks_raw_value::of(value), apartment_hint);
		return ks_future<T>::__from_raw(raw_future);
	}

	static ks_future<T> resolved(T&& value) {
		ks_apartment* apartment_hint = ks_apartment::default_mta();
		ks_raw_future_ptr raw_future = ks_raw_future::resolved(ks_raw_value::of(std::move(value)), apartment_hint);
		return ks_future<T>::__from_raw(raw_future);
	}

	static ks_future<T> rejected(const ks_error& error) {
		ks_apartment* apartment_hint = ks_apartment::default_mta();
		ks_raw_future_ptr raw_future = ks_raw_future::rejected(error, apartment_hint);
		return ks_future<T>::__from_raw(raw_future);
	}

public:
	static ks_future<T> post(ks_apartment* apartment, const ks_async_context& context, std::function<T()>&& task_fn) {
		return ks_future<T>::__choose_post(apartment, context, std::move(task_fn));
	}
	static ks_future<T> post(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>()>&& task_fn) {
		return ks_future<T>::__choose_post(apartment, context, std::move(task_fn));
	}
	static ks_future<T> post(ks_apartment* apartment, const ks_async_context& context, std::function<T(ks_cancel_inspector*)>&& task_fn) {
		return ks_future<T>::__choose_post(apartment, context, std::move(task_fn));
	}
	static ks_future<T> post(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>(ks_cancel_inspector*)>&& task_fn) {
		return ks_future<T>::__choose_post(apartment, context, std::move(task_fn));
	}
	template <class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>) ||
		(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>)>>
	static ks_future<T> post(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn) {
		return ks_future<T>::__choose_post(apartment, context, std::forward<FN>(task_fn));
	}

	static ks_future<T> post_delayed(ks_apartment* apartment, const ks_async_context& context, std::function<T()>&& task_fn, int64_t delay) {
		return ks_future<T>::__choose_post_delayed(apartment, context, std::move(task_fn), delay);
	}
	static ks_future<T> post_delayed(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>()>&& task_fn, int64_t delay) {
		return ks_future<T>::__choose_post_delayed(apartment, context, std::move(task_fn), delay);
	}
	static ks_future<T> post_delayed(ks_apartment* apartment, const ks_async_context& context, std::function<T(ks_cancel_inspector*)>&& task_fn, int64_t delay) {
		return ks_future<T>::__choose_post_delayed(apartment, context, std::move(task_fn), delay);
	}
	static ks_future<T> post_delayed(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>(ks_cancel_inspector*)>&& task_fn, int64_t delay) {
		return ks_future<T>::__choose_post_delayed(apartment, context, std::move(task_fn), delay);
	}
	template <class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>) ||
		(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>)>>
	static ks_future<T> post_delayed(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay) {
		return ks_future<T>::__choose_post_delayed(apartment, context, std::forward<FN>(task_fn), delay);
	}

	static ks_future<T> post_pending(ks_apartment* apartment, const ks_async_context& context, std::function<T()>&& task_fn, ks_pending_trigger* trigger) {
		return ks_future<T>::__choose_post_pending(apartment, context, std::move(task_fn), trigger);
	}
	static ks_future<T> post_pending(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>()>&& task_fn, ks_pending_trigger* trigger) {
		return ks_future<T>::__choose_post_pending(apartment, context, std::move(task_fn), trigger);
	}
	static ks_future<T> post_pending(ks_apartment* apartment, const ks_async_context& context, std::function<T(ks_cancel_inspector*)>&& task_fn, ks_pending_trigger* trigger) {
		return ks_future<T>::__choose_post_pending(apartment, context, std::move(task_fn), trigger);
	}
	static ks_future<T> post_pending(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>(ks_cancel_inspector*)>&& task_fn, ks_pending_trigger* trigger) {
		return ks_future<T>::__choose_post_pending(apartment, context, std::move(task_fn), trigger);
	}
	template <class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>) ||
		(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>)>>
	static ks_future<T> post_pending(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger) {
		return ks_future<T>::__choose_post_pending(apartment, context, std::forward<FN>(task_fn), trigger);
	}

public:
	template <class R>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, std::function<R(const T&)>&& fn) const {
		return this->__choose_then<R>(apartment, context, std::move(fn));
	}
	template <class R>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const T&)>&& fn) const {
		return this->__choose_then<R>(apartment, context, std::move(fn));
	}
	template <class R>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, std::function<R(const T&, ks_cancel_inspector*)>&& fn) const {
		return this->__choose_then<R>(apartment, context, std::move(fn));
	}
	template <class R>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>&& fn) const {
		return this->__choose_then<R>(apartment, context, std::move(fn));
	}
	template <class R, class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<R(const T&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const T&)>>) ||
		(std::is_convertible_v<FN, std::function<R(const T&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>>)>>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const {
		return this->__choose_then<R>(apartment, context, std::forward<FN>(fn));
	}

	template <class R>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, std::function<R(const ks_result<T>&)>&& fn) const {
		return this->__choose_transform<R>(apartment, context, std::move(fn));
	}
	template <class R>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const ks_result<T>&)>&& fn) const {
		return this->__choose_transform<R>(apartment, context, std::move(fn));
	}
	template <class R>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, std::function<R(const ks_result<T>&, ks_cancel_inspector*)>&& fn) const {
		return this->__choose_transform<R>(apartment, context, std::move(fn));
	}
	template <class R>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>&& fn) const {
		return this->__choose_transform<R>(apartment, context, std::move(fn));
	}
	template <class R, class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<R(const ks_result<T>&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&)>>) ||
		(std::is_convertible_v<FN, std::function<R(const ks_result<T>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>>)>>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const {
		return this->__choose_transform<R>(apartment, context, std::forward<FN>(fn));
	}

public:
	template <class R>
	ks_future<R> flat_then(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>(const T&)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_future_ptr {
			ks_future<R> typed_future2 = fn(raw_value.get<T>());
			return typed_future2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->flat_then(raw_fn, context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> flat_then(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>(const T&, ks_cancel_inspector*)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_future_ptr {
			ks_future<R> typed_future2 = fn(raw_value.get<T>(), ks_cancel_inspector::__for_future());
			return typed_future2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->flat_then(raw_fn, context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}

	template <class R>
	ks_future<R> flat_transform(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>(const ks_result<T>&)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_future_ptr {
			ks_future<R> typed_future2 = fn(ks_result<T>::__from_raw(raw_result));
			return typed_future2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->flat_transform(raw_fn, context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> flat_transform(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>(const ks_result<T>&, ks_cancel_inspector*)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_future_ptr {
			ks_future<R> typed_future2 = fn(ks_result<T>::__from_raw(raw_result), ks_cancel_inspector::__for_future());
			return typed_future2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->flat_transform(raw_fn, context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}

public:
	ks_future<T> on_success(ks_apartment* apartment, const ks_async_context& context, std::function<void(const T&)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value) -> void {
			fn(raw_value.get<T>());
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->on_success(std::move(raw_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future2);
	}

	ks_future<T> on_failure(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_error&)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_error& error) -> void {
			fn(error);
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->on_failure(std::move(raw_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future2);
	}

	ks_future<T> on_completion(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_result<T>&)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result) -> void {
			fn(ks_result<T>::__from_raw(raw_result));
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->on_completion(std::move(raw_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future2);
	}

public:
	template <class R>
	ks_future<R> cast() const {
		ASSERT(this->is_valid());
		constexpr __cast_mode_t cast_mode = __determine_cast_mode<R>();
		return __do_cast<R>(std::integral_constant<__cast_mode_t, cast_mode>());
	}

	const this_future_type& deliver_to_promise(const ks_promise<T>& promise) const {
		ASSERT(this->is_valid());
		m_raw_future->on_completion(
			[raw_promise = promise.__get_raw()](const ks_raw_result& raw_result) { raw_promise->try_complete(raw_result); },
			ks_async_context().set_priority(0x10000), nullptr);
		return *this;
	}

	const this_future_type& set_timeout(int64_t timeout) const {
		ASSERT(this->is_valid());
		m_raw_future->set_timeout(timeout, true);
		return *this;
	}

public:
	bool is_valid() const {
		return m_raw_future != nullptr;
	}

	bool is_completed() const {
		ASSERT(this->is_valid());
		return m_raw_future->is_completed();
	}

	ks_result<T> peek_result() const {
		ASSERT(this->is_valid());
		return ks_result<T>::__from_raw(m_raw_future->peek_result());
	}

	//慎用，使用不当可能会造成死锁或卡顿！
	template <class _ = void>
	_DECL_DEPRECATED bool wait() const {
		ASSERT(this->is_valid());
		return m_raw_future->wait();
	}

	void try_cancel() const {
		ASSERT(this->is_valid());
		m_raw_future->try_cancel(true);
	}

private:
	template <class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>) ||
		(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>)>>
	static ks_future<T> __choose_post(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal post's arglist");
		return ks_future<T>::__choose_post_by_arglist(apartment, context, std::forward<FN>(task_fn), std::integral_constant<int, arglist_mode>());
	}

	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>>>
	static ks_future<T> __choose_post_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_same_v<ks_result<T>, std::invoke_result_t<FN>> ? 2 :
			std::is_convertible_v<T, std::invoke_result_t<FN>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post's ret");
		return ks_future<T>::__choose_post_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> __choose_post_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_same_v<ks_result<T>, std::invoke_result_t<FN, ks_cancel_inspector*>> ? 2 :
			std::is_convertible_v<T, std::invoke_result_t<FN, ks_cancel_inspector*>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post's ret");
		return ks_future<T>::__choose_post_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T()>>>>
	static ks_future<T> __choose_post_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_of_arglist_1_ret_1(apartment, context, std::function<T()>(std::forward<FN>(task_fn)));
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<T>()>>>>
	static ks_future<T> __choose_post_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_of_arglist_1_ret_2(apartment, context, std::function<ks_result<T>()>(std::forward<FN>(task_fn)));
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>>>>
	static ks_future<T> __choose_post_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_of_arglist_2_ret_1(apartment, context, std::function<T(ks_cancel_inspector*)>(std::forward<FN>(task_fn)));
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> __choose_post_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_of_arglist_2_ret_2(apartment, context, std::function<ks_result<T>(ks_cancel_inspector*)>(std::forward<FN>(task_fn)));
	}

	template <class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>) ||
		(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>)>>
	static ks_future<T> __choose_post_delayed(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal post_delayed's arglist");
		return ks_future<T>::__choose_post_delayed_by_arglist(apartment, context, std::forward<FN>(task_fn), delay, std::integral_constant<int, arglist_mode>());
	}

	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>>>
	static ks_future<T> __choose_post_delayed_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_same_v<ks_result<T>, std::invoke_result_t<FN>> ? 2 :
			std::is_convertible_v<T, std::invoke_result_t<FN>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post_delayed's ret");
		return ks_future<T>::__choose_post_delayed_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), delay, std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> __choose_post_delayed_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_same_v<ks_result<T>, std::invoke_result_t<FN, ks_cancel_inspector*>> ? 2 :
			std::is_convertible_v<T, std::invoke_result_t<FN, ks_cancel_inspector*>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post_delayed's ret");
		return ks_future<T>::__choose_post_delayed_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), delay, std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T()>>>>
	static ks_future<T> __choose_post_delayed_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_delayed_of_arglist_1_ret_1(apartment, context, std::function<T()>(std::forward<FN>(task_fn)), delay);
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<T>()>>>>
	static ks_future<T> __choose_post_delayed_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_delayed_of_arglist_1_ret_2(apartment, context, std::function<ks_result<T>()>(std::forward<FN>(task_fn)), delay);
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>>>>
	static ks_future<T> __choose_post_delayed_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_delayed_of_arglist_2_ret_1(apartment, context, std::function<T(ks_cancel_inspector*)>(std::forward<FN>(task_fn)), delay);
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> __choose_post_delayed_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_delayed_of_arglist_2_ret_2(apartment, context, std::function<ks_result<T>(ks_cancel_inspector*)>(std::forward<FN>(task_fn)), delay);
	}

	template <class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>) ||
		(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>)>>
	static ks_future<T> __choose_post_pending(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal post_pending's arglist");
		return ks_future<T>::__choose_post_pending_by_arglist(apartment, context, std::forward<FN>(task_fn), trigger, std::integral_constant<int, arglist_mode>());
	}

	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>>>>
	static ks_future<T> __choose_post_pending_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_same_v<ks_result<T>, std::invoke_result_t<FN>> ? 2 :
			std::is_convertible_v<T, std::invoke_result_t<FN>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post_pending's ret");
		return ks_future<T>::__choose_post_pending_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), trigger, std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> __choose_post_pending_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_same_v<ks_result<T>, std::invoke_result_t<FN, ks_cancel_inspector*>> ? 2 :
			std::is_convertible_v<T, std::invoke_result_t<FN, ks_cancel_inspector*>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post_pending's ret");
		return ks_future<T>::__choose_post_pending_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), trigger, std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T()>>>>
	static ks_future<T> __choose_post_pending_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_pending_of_arglist_1_ret_1(apartment, context, std::function<T()>(std::forward<FN>(task_fn)), trigger);
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<T>()>>>>
	static ks_future<T> __choose_post_pending_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_pending_of_arglist_1_ret_2(apartment, context, std::function<ks_result<T>()>(std::forward<FN>(task_fn)), trigger);
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>>>>
	static ks_future<T> __choose_post_pending_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_pending_of_arglist_2_ret_1(apartment, context, std::function<T(ks_cancel_inspector*)>(std::forward<FN>(task_fn)), trigger);
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> __choose_post_pending_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_pending_of_arglist_2_ret_2(apartment, context, std::function<ks_result<T>(ks_cancel_inspector*)>(std::forward<FN>(task_fn)), trigger);
	}

private:
	static ks_future<T> __post_of_arglist_1_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T()>&& task_fn) {
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			T typed_value = task_fn();
			return ks_raw_value::of(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post(std::move(raw_task_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_of_arglist_1_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>()>&& task_fn) {
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			ks_result<T> result = task_fn();
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post(std::move(raw_task_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_of_arglist_2_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T(ks_cancel_inspector*)>&& task_fn) {
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			T typed_value = task_fn(ks_cancel_inspector::__for_future());
			return ks_raw_value::of(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post(std::move(raw_task_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_of_arglist_2_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>(ks_cancel_inspector*)>&& task_fn) {
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			ks_result<T> result = task_fn(ks_cancel_inspector::__for_future());
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post(std::move(raw_task_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}

	static ks_future<T> __post_delayed_of_arglist_1_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T()>&& task_fn, int64_t delay) {
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			T typed_value = task_fn();
			return ks_raw_value::of(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post_delayed(std::move(raw_task_fn), context, apartment, delay);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_delayed_of_arglist_1_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>()>&& task_fn, int64_t delay) {
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			ks_result<T> result = task_fn();
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post_delayed(std::move(raw_task_fn), context, apartment, delay);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_delayed_of_arglist_2_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T(ks_cancel_inspector*)>&& task_fn, int64_t delay) {
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			T typed_value = task_fn(ks_cancel_inspector::__for_future());
			return ks_raw_value::of(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post_delayed(std::move(raw_task_fn), context, apartment, delay);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_delayed_of_arglist_2_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>(ks_cancel_inspector*)>&& task_fn, int64_t delay) {
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			ks_result<T> result = task_fn(ks_cancel_inspector::__for_future());
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post_delayed(std::move(raw_task_fn), context, apartment, delay);
		return ks_future<T>::__from_raw(raw_future);
	}

	static ks_future<T> __post_pending_of_arglist_1_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T()>&& task_fn, ks_pending_trigger* trigger) {
		ASSERT(apartment != nullptr);
		ASSERT(trigger != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)](const ks_raw_result&)->ks_raw_result {
			T typed_value = task_fn();
			return ks_raw_value::of(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = (trigger != nullptr)
			? trigger->m_pending_promise->get_future()->then(raw_task_fn, context, apartment)
			: ks_raw_future::resolved(ks_raw_value::of(nothing), ks_apartment::current_thread_apartment_or(apartment))->then(raw_task_fn, context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_pending_of_arglist_1_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>()>&& task_fn, ks_pending_trigger* trigger) {
		ASSERT(apartment != nullptr);
		ASSERT(trigger != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)](const ks_raw_result&)->ks_raw_result {
			ks_result<T> result = task_fn();
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = (trigger != nullptr)
			? trigger->m_pending_promise->get_future()->then(raw_task_fn, context, apartment)
			: ks_raw_future::resolved(ks_raw_value::of(nothing), ks_apartment::current_thread_apartment_or(apartment))->then(raw_task_fn, context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_pending_of_arglist_2_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T(ks_cancel_inspector*)>&& task_fn, ks_pending_trigger* trigger) {
		ASSERT(apartment != nullptr);
		ASSERT(trigger != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)](const ks_raw_result&)->ks_raw_result {
			T typed_value = task_fn(ks_cancel_inspector::__for_future());
			return ks_raw_value::of(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = (trigger != nullptr)
			? trigger->m_pending_promise->get_future()->then(raw_task_fn, context, apartment)
			: ks_raw_future::resolved(ks_raw_value::of(nothing), ks_apartment::current_thread_apartment_or(apartment))->then(raw_task_fn, context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_pending_of_arglist_2_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>(ks_cancel_inspector*)>&& task_fn, ks_pending_trigger* trigger) {
		ASSERT(apartment != nullptr);
		ASSERT(trigger != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_task_fn = [task_fn = std::move(task_fn)](const ks_raw_result&)->ks_raw_result {
			ks_result<T> result = task_fn(ks_cancel_inspector::__for_future());
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = (trigger != nullptr)
			? trigger->m_pending_promise->get_future()->then(raw_task_fn, context, apartment)
			: ks_raw_future::resolved(ks_raw_value::of(nothing), ks_apartment::current_thread_apartment_or(apartment))->then(raw_task_fn, context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}

private:
	template <class R, class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<R(const T&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const T&)>>) ||
		(std::is_convertible_v<FN, std::function<R(const T&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>>)>>
	ks_future<R> __choose_then(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<R(const T&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<R(const T&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const T&)>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal then's arglist");
		return this->__choose_then_by_arglist<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, arglist_mode>());
	}

	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<R(const T&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const T&)>>>>
	ks_future<R> __choose_then_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>) const {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const T&>> ? -1 :
			std::is_same_v<ks_result<R>, std::invoke_result_t<FN, const T&>> ? 2 :
			std::is_convertible_v<R, std::invoke_result_t<FN, const T&>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal then's ret");
		return this->__choose_then_by_arglist_ret<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<R(const T&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>>>>
	ks_future<R> __choose_then_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>) const {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const T&, ks_cancel_inspector*>> ? -1 :
			std::is_same_v<ks_result<R>, std::invoke_result_t<FN, const T&, ks_cancel_inspector*>> ? 2 :
			std::is_convertible_v<R, std::invoke_result_t<FN, const T&, ks_cancel_inspector*>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal then's ret");
		return this->__choose_then_by_arglist_ret<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class R, class FN, class _ = std::enable_if_t<std::is_void_v<R> && std::is_convertible_v<FN, std::function<void(const T&)>>>>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) const {
		return this->__then_of_arglist_1_ret_x<R>(apartment, context, std::function<void(const T&)>(std::forward<FN>(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<!std::is_void_v<R> && std::is_convertible_v<FN, std::function<R(const T&)>>>>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) const {
		return this->__then_of_arglist_1_ret_1<R>(apartment, context, std::function<R(const T&)>(std::forward<FN>(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<R>(const T&)>>>>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) const {
		return this->__then_of_arglist_1_ret_2<R>(apartment, context, std::function<ks_result<R>(const T&)>(std::forward<FN>(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_void_v<R>&& std::is_convertible_v<FN, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>>>>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) const {
		return this->__then_of_arglist_2_ret_x<R>(apartment, context, std::function<void(const T&, ks_cancel_inspector*)>(std::forward<FN>(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<!std::is_void_v<R> && std::is_convertible_v<FN, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>>>>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) const {
		return this->__then_of_arglist_2_ret_1<R>(apartment, context, std::function<R(const T&, ks_cancel_inspector*)>(std::forward<FN>(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>>>>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) const {
		return this->__then_of_arglist_2_ret_2<R>(apartment, context, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>(std::forward<FN>(fn)));
	}

	template <class R, class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<R(const ks_result<T>&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&)>>) ||
		(std::is_convertible_v<FN, std::function<R(const ks_result<T>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>>)>>
	ks_future<R> __choose_transform(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<R(const ks_result<T>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<R(const ks_result<T>&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&)>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal transform's arglist");
		return this->__choose_transform_by_arglist<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, arglist_mode>());
	}

	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<R(const ks_result<T>&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&)>>>>
	ks_future<R> __choose_transform_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>) const {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const ks_result<T>&>> ? -1 :
			std::is_same_v<ks_result<R>, std::invoke_result_t<FN, const ks_result<T>&>> ? 2 :
			std::is_convertible_v<R, std::invoke_result_t<FN, const ks_result<T>&>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal transform's ret");
		return this->__choose_transform_by_arglist_ret<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<R(const ks_result<T>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>>>>
	ks_future<R> __choose_transform_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>) const {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const ks_result<T>&, ks_cancel_inspector*>> ? -1 :
			std::is_same_v<ks_result<R>, std::invoke_result_t<FN, const ks_result<T>&, ks_cancel_inspector*>> ? 2 :
			std::is_convertible_v<R, std::invoke_result_t<FN, const ks_result<T>&, ks_cancel_inspector*>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal transform's ret");
		return this->__choose_transform_by_arglist_ret<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class R, class FN, class _ = std::enable_if_t<std::is_void_v<R> && std::is_convertible_v<FN, std::function<void(const ks_result<T>&)>>>>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) const {
		return this->__transform_of_arglist_1_ret_x<R>(apartment, context, std::function<void(const ks_result<T>&)>(std::forward<FN>(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<!std::is_void_v<R> && std::is_convertible_v<FN, std::function<R(const ks_result<T>&)>>>>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) const {
		return this->__transform_of_arglist_1_ret_1<R>(apartment, context, std::function<R(const ks_result<T>&)>(std::forward<FN>(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&)>>>>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) const {
		return this->__transform_of_arglist_1_ret_2<R>(apartment, context, std::function<ks_result<R>(const ks_result<T>&)>(std::forward<FN>(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_void_v<R> && std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>>>>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) const {
		return this->__transform_of_arglist_2_ret_x<R>(apartment, context, std::function<void(const ks_result<T>&, ks_cancel_inspector*)>(std::forward<FN>(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<!std::is_void_v<R> && std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>>>>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) const {
		return this->__transform_of_arglist_2_ret_1<R>(apartment, context, std::function<R(const ks_result<T>&, ks_cancel_inspector*)>(std::forward<FN>(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>>>>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) const {
		return this->__transform_of_arglist_2_ret_2<R>(apartment, context, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>(std::forward<FN>(fn)));
	}

private:
	template <class R>
	ks_future<R> __then_of_arglist_1_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<R(const T&)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
			R typed_value2 = fn(raw_value.get<T>());
			return ks_raw_value::of(std::move(typed_value2));
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __then_of_arglist_1_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const T&)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
			ks_result<R> typed_result2 = fn(raw_value.get<T>());
			return typed_result2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __then_of_arglist_2_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<R(const T&, ks_cancel_inspector*)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
			R typed_value2 = fn(raw_value.get<T>(), ks_cancel_inspector::__for_future());
			return ks_raw_value::of(std::move(typed_value2));
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __then_of_arglist_2_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
			ks_result<R> typed_result2 = fn(raw_value.get<T>(), ks_cancel_inspector::__for_future());
			return typed_result2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}

	template <class R, class _ = std::enable_if_t<std::is_void_v<R>>> //相当于then<void>特化
	ks_future<void> __then_of_arglist_1_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const T&)>&& fn) const;
	template <class R, class _ = std::enable_if_t<std::is_void_v<R>>> //相当于then<void>特化
	ks_future<void> __then_of_arglist_2_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const T&, ks_cancel_inspector*)>&& fn) const;


	template <class R>
	ks_future<R> __transform_of_arglist_1_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<R(const ks_result<T>&)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
			R typed_value2 = fn(ks_result<T>::__from_raw(raw_result));
			return ks_raw_value::of(std::move(typed_value2));
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->transform(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __transform_of_arglist_1_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const ks_result<T>&)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
			ks_result<R> typed_result2 = fn(ks_result<T>::__from_raw(raw_result));
			return typed_result2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->transform(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __transform_of_arglist_2_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<R(const ks_result<T>&, ks_cancel_inspector*)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
			R typed_value2 = fn(ks_result<T>::__from_raw(raw_result), ks_cancel_inspector::__for_future());
			return ks_raw_value::of(std::move(typed_value2));
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->transform(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __transform_of_arglist_2_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>&& fn) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
			ks_result<R> typed_result2 = fn(ks_result<T>::__from_raw(raw_result), ks_cancel_inspector::__for_future());
			return typed_result2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->transform(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}

	template <class R, class _ = std::enable_if_t<std::is_void_v<R>>> //相当于transform<void>特化
	ks_future<void> __transform_of_arglist_1_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_result<T>&)>&& fn) const;
	template <class R, class _ = std::enable_if_t<std::is_void_v<R>>> //相当于transform<void>特化
	ks_future<void> __transform_of_arglist_2_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_result<T>&, ks_cancel_inspector*)>&& fn) const;

private:
	using __cast_mode_t = typename ks_result<T>::__cast_mode_t;

	template <class R>
	static constexpr __cast_mode_t __determine_cast_mode() {
		return ks_result<T>::template __determine_cast_mode<R>();
	}

	template <class R>
	ks_future<R> __do_cast(std::integral_constant<__cast_mode_t, __cast_mode_t::to_same> __cast_mode) const {
		using XT = std::remove_cvref_t<T>;
		using XR = std::remove_cvref_t<R>;
		using PROXT = std::conditional_t<std::is_void_v<XT>, nothing_t, XT>;
		using PROXR = std::conditional_t<std::is_void_v<XR>, nothing_t, XR>;
		static_assert(std::is_same_v<PROXR, PROXT>, "ks_future::cast_to mode 1 error");

		return ks_future<R>::__from_raw(m_raw_future);
	}

	template <class R>
	ks_future<R> __do_cast(std::integral_constant<__cast_mode_t, __cast_mode_t::to_nothing> __cast_mode) const {
		//using XT = std::remove_cvref_t<T>;
		using XR = std::remove_cvref_t<R>;
		//using PROXT = std::conditional_t<std::is_void_v<XT>, nothing_t, XT>;
		using PROXR = std::conditional_t<std::is_void_v<XR>, nothing_t, XR>;
		static_assert(std::is_nothing_v<PROXR>, "ks_future::cast_to mode 2 error");

		ks_raw_future_ptr raw_future2 = m_raw_future->then(
			[](const ks_raw_value& value) -> ks_raw_result { return ks_raw_value::of(nothing); },
			ks_async_context().set_priority(0x10000), nullptr);
		return ks_future<R>::__from_raw(raw_future2);
	}

	template <class R>
	ks_future<R> __do_cast(std::integral_constant<__cast_mode_t, __cast_mode_t::to_other> __cast_mode) const {
		using XT = std::remove_cvref_t<T>;
		using XR = std::remove_cvref_t<R>;
		using PROXT = std::conditional_t<std::is_void_v<XT>, nothing_t, XT>;
		using PROXR = std::conditional_t<std::is_void_v<XR>, nothing_t, XR>;
		static_assert(std::is_convertible_v<PROXT, PROXR>, "ks_future::cast_to mode 3 error");

		ks_raw_future_ptr raw_future2 = m_raw_future->then(
			[](const ks_raw_value& value) -> ks_raw_result {
				return ks_raw_value::of(static_cast<PROXR>(value.get<PROXT>()));
			},
			ks_async_context().set_priority(0x10000), nullptr);
		return ks_future<R>::__from_raw(raw_future2);
	}

private:
	using ks_raw_future = __ks_async_raw::ks_raw_future;
	using ks_raw_future_ptr = __ks_async_raw::ks_raw_future_ptr;

	using ks_raw_result = __ks_async_raw::ks_raw_result;
	using ks_raw_value = __ks_async_raw::ks_raw_value;

	explicit ks_future(const ks_raw_future_ptr& raw_future, int) : m_raw_future(raw_future) {}
	explicit ks_future(ks_raw_future_ptr&& raw_future, int) : m_raw_future(std::move(raw_future)) {}

	static ks_future<T> __from_raw(const ks_raw_future_ptr& raw_future) { return ks_future<T>(raw_future, 0); }
	static ks_future<T> __from_raw(ks_raw_future_ptr&& raw_future) { return ks_future<T>(std::move(raw_future), 0); }
	const ks_raw_future_ptr& __get_raw() const { return m_raw_future; }

	template <class T2> friend class ks_future;
	template <class T2> friend class ks_promise;
	friend class ks_future_util;

private:
	ks_raw_future_ptr m_raw_future;
};


#include "ks_future_void.inl"
#include "ks_future_util.inl"


//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//ks_future::then<void>和transform<void>特化实现...
template <class T>
template <class R, class _ /*= std::enable_if_t<std::is_void_v<R>>*/>
inline ks_future<void> ks_future<T>::__then_of_arglist_1_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const T&)>&& fn) const {
	ASSERT(this->is_valid());
	ASSERT(apartment != nullptr);
	if (apartment == nullptr)
		apartment = ks_apartment::default_mta();
	auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
		fn(raw_value.get<T>());
		return ks_raw_value::of(nothing);
	};
	ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
	return ks_future<void>::__from_raw(raw_future2);
}
template <class T>
template <class R, class _ /*= std::enable_if_t<std::is_void_v<R>>*/>
inline ks_future<void> ks_future<T>::__then_of_arglist_2_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const T&, ks_cancel_inspector*)>&& fn) const {
	ASSERT(this->is_valid());
	ASSERT(apartment != nullptr);
	if (apartment == nullptr)
		apartment = ks_apartment::default_mta();
	auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
		fn(raw_value.get<T>(), ks_cancel_inspector::__for_future());
		return ks_raw_value::of(nothing);
	};
	ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
	return ks_future<void>::__from_raw(raw_future2);
}

template <class T>
template <class R, class _ /*= std::enable_if_t<std::is_void_v<R>>*/>
ks_future<void> ks_future<T>::__transform_of_arglist_1_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_result<T>&)>&& fn) const {
	ASSERT(this->is_valid());
	ASSERT(apartment != nullptr);
	if (apartment == nullptr)
		apartment = ks_apartment::default_mta();
	auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
		fn(ks_result<T>::__from_raw(raw_result));
		return ks_raw_value::of(nothing);
	};
	ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
	return ks_future<void>::__from_raw(raw_future2);
}
template <class T>
template <class R, class _ /*= std::enable_if_t<std::is_void_v<R>>*/>
ks_future<void> ks_future<T>::__transform_of_arglist_2_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_result<T>&, ks_cancel_inspector*)>&& fn) const {
	ASSERT(this->is_valid());
	ASSERT(apartment != nullptr);
	if (apartment == nullptr)
		apartment = ks_apartment::default_mta();
	auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
		fn(ks_result<T>::__from_raw(raw_result), ks_cancel_inspector::__for_future());
		return ks_raw_value::of(nothing);
	};
	ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
	return ks_future<void>::__from_raw(raw_future2);
}
