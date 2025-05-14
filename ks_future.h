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
	ks_future(ks_future&&) noexcept = default;

	ks_future& operator=(const ks_future&) = default;
	ks_future& operator=(ks_future&&) noexcept = default;

	//让ks_future看起来像一个智能指针
	ks_future* operator->() { return this; }
	const ks_future* operator->() const { return this; }

	using value_type = T;
	using this_future_type = ks_future<T>;

public: //resolved, rejected
	static ks_future<T> resolved(const T& value) {
		ks_apartment* apartment_hint = ks_apartment::current_thread_apartment_or_default_mta();
		ks_raw_future_ptr raw_future = ks_raw_future::resolved(ks_raw_value::of<T>(value), apartment_hint);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> resolved(T&& value) {
		ks_apartment* apartment_hint = ks_apartment::current_thread_apartment_or_default_mta();
		ks_raw_future_ptr raw_future = ks_raw_future::resolved(ks_raw_value::of<T>(std::move(value)), apartment_hint);
		return ks_future<T>::__from_raw(raw_future);
	}

	static ks_future<T> rejected(const ks_error& error) {
		ks_apartment* apartment_hint = ks_apartment::current_thread_apartment_or_default_mta();
		ks_raw_future_ptr raw_future = ks_raw_future::rejected(error, apartment_hint);
		return ks_future<T>::__from_raw(raw_future);
	}

	static ks_future<T> __from_result(const ks_result<T>& result) {
		ks_apartment* apartment_hint = ks_apartment::current_thread_apartment_or_default_mta();
		ks_raw_future_ptr raw_future = ks_raw_future::__from_result(result.__get_raw(), apartment_hint);
		return ks_future<T>::__from_raw(raw_future);
	}

public: //post, post_delayed, post_pending
	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<T()>> ||
		std::is_convertible_v<FN, std::function<ks_result<T>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<T>()>> ||
		std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> post(ks_apartment* apartment, FN&& task_fn, const ks_async_context& context = {}) {
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::current_thread_apartment_or_default_mta();
		return ks_future<T>::__choose_post(apartment, context, std::forward<FN>(task_fn));
	}
	template <class FN>
	static ks_future<T> post(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn) { //only for compat
		return ks_future<T>::post(apartment, std::forward<FN>(task_fn), context);
	}

	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<T()>> ||
		std::is_convertible_v<FN, std::function<ks_result<T>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<T>()>> ||
		std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> post_delayed(ks_apartment* apartment, FN&& task_fn, int64_t delay, const ks_async_context& context = {}) {
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::current_thread_apartment_or_default_mta();
		return ks_future<T>::__choose_post_delayed(apartment, context, std::forward<FN>(task_fn), delay);
	}
	template <class FN>
	static ks_future<T> post_delayed(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay) { //only for compat
		return ks_future<T>::post_delayed(apartment, std::forward<FN>(task_fn), delay, context);
	}

	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<T()>> ||
		std::is_convertible_v<FN, std::function<ks_result<T>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<T>()>> ||
		std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> post_pending(ks_apartment* apartment, FN&& task_fn, ks_pending_trigger* trigger, const ks_async_context& context = {}) {
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::current_thread_apartment_or_default_mta();
		return ks_future<T>::__choose_post_pending(apartment, context, std::forward<FN>(task_fn), trigger);
	}
	template <class FN>
	static ks_future<T> post_pending(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger) { //only for compat
		return ks_future<T>::post_pending(apartment, std::forward<FN>(task_fn), trigger, context);
	}

public: //then, transform
	template <class R, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<R(const T&)>> ||
		std::is_convertible_v<FN, std::function<ks_result<R>(const T&)>> ||
		std::is_convertible_v<FN, std::function<ks_future<R>(const T&)>> ||
		std::is_convertible_v<FN, std::function<R(const T&, ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<R>(const T&, ks_cancel_inspector*)>>>>
	ks_future<R> then(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const { 
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::current_thread_apartment_or_default_mta();
		return this->__choose_then<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->then<R>(apartment, std::forward<FN>(fn), context);
	}

	template <class R, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<R(const ks_result<T>&)>> || 
		std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&)>> || 
		std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<T>&)>> ||
		std::is_convertible_v<FN, std::function<R(const ks_result<T>&, ks_cancel_inspector*)>> || 
		std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>> || 
		std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<T>&, ks_cancel_inspector*)>>>>
	ks_future<R> transform(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::current_thread_apartment_or_default_mta();
		return this->__choose_transform<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->transform<R>(apartment, std::forward<FN>(fn), context);
	}

public: //flat_then, flat_transform
	template <class R, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<ks_future<R>(const T&)>> ||
		std::is_convertible_v<FN, std::function<ks_future<R>(const T&, ks_cancel_inspector*)>>>>
	ks_future<R> flat_then(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		return this->then<R>(apartment, std::forward<FN>(fn), context);
	}
	template <class R, class FN>
	ks_future<R> flat_then(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->flat_then<R>(apartment, std::forward<FN>(fn), context);
	}

	template <class R, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<R(const ks_result<T>&)>> ||
		std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&)>> ||
		std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<T>&)>> ||
		std::is_convertible_v<FN, std::function<R(const ks_result<T>&, ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<T>&, ks_cancel_inspector*)>>>>
	ks_future<R> flat_transform(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		return this->transform<R>(apartment, std::forward<FN>(fn), context);
	}
	template <class R, class FN>
	ks_future<R> flat_transform(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->flat_transform<R>(apartment, std::forward<FN>(fn), context);
	}

public: //on_success, on_failure, on_completion
	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<void(const T&)>> && std::is_void_v<std::invoke_result_t<FN, const T&>>>>
	ks_future<T> on_success(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::current_thread_apartment_or_default_mta();
		auto raw_fn = [fn = std::forward<FN>(fn)](const ks_raw_value& raw_value) -> void {
			fn(raw_value.get<T>());
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->on_success(std::move(raw_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future2);
	}
	template <class FN>
	ks_future<T> on_success(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->on_success(apartment, std::forward<FN>(fn), context);
	}

	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<void(const ks_error&)>> && std::is_void_v<std::invoke_result_t<FN, const ks_error&>>>>
	ks_future<T> on_failure(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::current_thread_apartment_or_default_mta();
		auto raw_fn = [fn = std::forward<FN>(fn)](const ks_error& error) -> void {
			fn(error);
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->on_failure(std::move(raw_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future2);
	}
	template <class FN>
	ks_future<T> on_failure(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->on_failure(apartment, std::forward<FN>(fn), context);
	}

	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<void(const ks_result<T>&)>> && std::is_void_v<std::invoke_result_t<FN, const ks_result<T>&>>>>
	ks_future<T> on_completion(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		ASSERT(this->is_valid());
		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::current_thread_apartment_or_default_mta();
		auto raw_fn = [fn = std::forward<FN>(fn)](const ks_raw_result& raw_result) -> void {
			fn(ks_result<T>::__from_raw(raw_result));
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->on_completion(std::move(raw_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future2);
	}
	template <class FN>
	ks_future<T> on_completion(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->on_completion(apartment, std::forward<FN>(fn), context);
	}

public: //map, map_value, cast
	template <class R, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<R(const T&)>>>>
	ks_future<R> map(FN&& fn) const {
		ASSERT(this->is_valid());
		ks_raw_future_ptr raw_future2 = m_raw_future->then(
			[fn = std::forward<FN>(fn)](const ks_raw_value& value)->ks_raw_result { 
				return ks_raw_value::of<R>(fn(value.get<T>())); 
			},
			make_async_context().set_priority(0x10000), nullptr);
		return ks_future<R>::__from_raw(raw_future2);
	}

	template <class R, class X = R, class _ = std::enable_if_t<std::is_convertible_v<X, R>>>
	ks_future<R> map_value(X&& other_value) const {
		ASSERT(this->is_valid());
		ks_raw_future_ptr raw_future2 = m_raw_future->then(
			[other_value = std::forward<X>(other_value)](const ks_raw_value& value)->ks_raw_result {
				return ks_raw_value::of<R>(other_value); 
			},
			make_async_context().set_priority(0x10000), nullptr);
		return ks_future<R>::__from_raw(raw_future2);
	}

	template <class R, class _ = std::enable_if_t<std::is_convertible_v<T, R> || std::is_void_v<R> || std::is_nothing_v<R>>>
	ks_future<R> cast() const {
		ASSERT(this->is_valid());
		constexpr __raw_cast_mode_t cast_mode = __determine_raw_cast_mode<R>();
		return __do_cast<R>(std::integral_constant<__raw_cast_mode_t, cast_mode>());
	}

public: //try_cancel, set_timeout
	const this_future_type& try_cancel() const {
		ASSERT(this->is_valid());
		m_raw_future->try_cancel(true);
		return *this;
	}

	const this_future_type& set_timeout(int64_t timeout) const {
		ASSERT(this->is_valid());
		m_raw_future->set_timeout(timeout, true);
		return *this;
	}

public: //is_valid, is_completed, peek_result, wait
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
	void __wait() const {
		ASSERT(this->is_valid());
		return m_raw_future->__wait();
	}

private: //__choose_post
	template <class FN>
	static ks_future<T> __choose_post(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_future<T>(ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>> || std::is_convertible_v<FN, std::function<ks_future<T>()>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal post's arglist");
		return ks_future<T>::__choose_post_by_arglist(apartment, context, std::forward<FN>(task_fn), std::integral_constant<int, arglist_mode>());
	}
	
	template <class FN>
	static ks_future<T> __choose_post_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_convertible_v<std::invoke_result_t<FN>, ks_future<T>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN>, ks_result<T>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN>, T> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post's ret");
		return ks_future<T>::__choose_post_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class FN>
	static ks_future<T> __choose_post_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_future<T>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_result<T>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, T> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post's ret");
		return ks_future<T>::__choose_post_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class FN>
	static ks_future<T> __choose_post_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_of_arglist_1_ret_1(apartment, context, std::forward<FN>(task_fn));
	}
	template <class FN>
	static ks_future<T> __choose_post_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_of_arglist_1_ret_2(apartment, context, std::forward<FN>(task_fn));
	}
	template <class FN>
	static ks_future<T> __choose_post_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 1>, std::integral_constant<int, 3>) {
		return ks_future<T>::__post_of_arglist_1_ret_3(apartment, context, std::forward<FN>(task_fn));
	}

	template <class FN>
	static ks_future<T> __choose_post_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_of_arglist_2_ret_1(apartment, context, std::forward<FN>(task_fn));
	}
	template <class FN>
	static ks_future<T> __choose_post_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_of_arglist_2_ret_2(apartment, context, std::forward<FN>(task_fn));
	}
	template <class FN>
	static ks_future<T> __choose_post_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, std::integral_constant<int, 2>, std::integral_constant<int, 3>) {
		return ks_future<T>::__post_of_arglist_2_ret_3(apartment, context, std::forward<FN>(task_fn));
	}

private: //__choose_post_delayed
	template <class FN>
	static ks_future<T> __choose_post_delayed(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_future<T>(ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>> || std::is_convertible_v<FN, std::function<ks_future<T>()>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal post_delayed's arglist");
		return ks_future<T>::__choose_post_delayed_by_arglist(apartment, context, std::forward<FN>(task_fn), delay, std::integral_constant<int, arglist_mode>());
	}

	template <class FN>
	static ks_future<T> __choose_post_delayed_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_convertible_v<std::invoke_result_t<FN>, ks_future<T>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN>, ks_result<T>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN>, T> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post_delayed's ret");
		return ks_future<T>::__choose_post_delayed_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), delay, std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class FN>
	static ks_future<T> __choose_post_delayed_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_future<T>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_result<T>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, T> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post_delayed's ret");
		return ks_future<T>::__choose_post_delayed_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), delay, std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class FN>
	static ks_future<T> __choose_post_delayed_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_delayed_of_arglist_1_ret_1(apartment, context, std::forward<FN>(task_fn), delay);
	}
	template <class FN>
	static ks_future<T> __choose_post_delayed_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_delayed_of_arglist_1_ret_2(apartment, context, std::forward<FN>(task_fn), delay);
	}
	template <class FN>
	static ks_future<T> __choose_post_delayed_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 1>, std::integral_constant<int, 3>) {
		return ks_future<T>::__post_delayed_of_arglist_1_ret_3(apartment, context, std::forward<FN>(task_fn), delay);
	}

	template <class FN>
	static ks_future<T> __choose_post_delayed_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_delayed_of_arglist_2_ret_1(apartment, context, std::forward<FN>(task_fn), delay);
	}
	template <class FN>
	static ks_future<T> __choose_post_delayed_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_delayed_of_arglist_2_ret_2(apartment, context, std::forward<FN>(task_fn), delay);
	}
	template <class FN>
	static ks_future<T> __choose_post_delayed_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay, std::integral_constant<int, 2>, std::integral_constant<int, 3>) {
		return ks_future<T>::__post_delayed_of_arglist_2_ret_3(apartment, context, std::forward<FN>(task_fn), delay);
	}

private: //__choose_post_pending
	template <class FN>
	static ks_future<T> __choose_post_pending(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_future<T>(ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>> || std::is_convertible_v<FN, std::function<ks_future<T>()>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal post_pending's arglist");
		return ks_future<T>::__choose_post_pending_by_arglist(apartment, context, std::forward<FN>(task_fn), trigger, std::integral_constant<int, arglist_mode>());
	}

	template <class FN>
	static ks_future<T> __choose_post_pending_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_convertible_v<std::invoke_result_t<FN>, ks_future<T>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN>, ks_result<T>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN>, T> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post_pending's ret");
		return ks_future<T>::__choose_post_pending_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), trigger, std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class FN>
	static ks_future<T> __choose_post_pending_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_future<T>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_result<T>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, T> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal post_pending's ret");
		return ks_future<T>::__choose_post_pending_by_arglist_ret(apartment, context, std::forward<FN>(task_fn), trigger, std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class FN>
	static ks_future<T> __choose_post_pending_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_pending_of_arglist_1_ret_1(apartment, context, std::forward<FN>(task_fn), trigger);
	}
	template <class FN>
	static ks_future<T> __choose_post_pending_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_pending_of_arglist_1_ret_2(apartment, context, std::forward<FN>(task_fn), trigger);
	}
	template <class FN>
	static ks_future<T> __choose_post_pending_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 1>, std::integral_constant<int, 3>) {
		return ks_future<T>::__post_pending_of_arglist_1_ret_3(apartment, context, std::forward<FN>(task_fn), trigger);
	}

	template <class FN>
	static ks_future<T> __choose_post_pending_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return ks_future<T>::__post_pending_of_arglist_2_ret_1(apartment, context, std::forward<FN>(task_fn), trigger);
	}
	template <class FN>
	static ks_future<T> __choose_post_pending_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return ks_future<T>::__post_pending_of_arglist_2_ret_2(apartment, context, std::forward<FN>(task_fn), trigger);
	}
	template <class FN>
	static ks_future<T> __choose_post_pending_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger, std::integral_constant<int, 2>, std::integral_constant<int, 3>) {
		return ks_future<T>::__post_pending_of_arglist_2_ret_3(apartment, context, std::forward<FN>(task_fn), trigger);
	}

private: //__choose_then
	template <class R, class FN>
	ks_future<R> __choose_then(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<R(const T&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const T&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_future<R>(const T&, ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<R(const T&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const T&)>> || std::is_convertible_v<FN, std::function<ks_future<R>(const T&)>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal then's arglist");
		return this->__choose_then_by_arglist<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, arglist_mode>());
	}

	template <class R, class FN>
	ks_future<R> __choose_then_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>) const {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const T&>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, const T&>, ks_future<R>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, const T&>, ks_result<R>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, const T&>, R> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal then's ret");
		return this->__choose_then_by_arglist_ret<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class R, class FN>
	ks_future<R> __choose_then_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>) const {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const T&, ks_cancel_inspector*>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, const T&, ks_cancel_inspector*>, ks_future<R>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, const T&, ks_cancel_inspector*>, ks_result<R>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, const T&, ks_cancel_inspector*>, R> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal then's ret");
		return this->__choose_then_by_arglist_ret<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class R, class FN>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) const {
		static_assert(std::is_void_v<R>, "R must be void");
		return this->__then_of_arglist_1_ret_x<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) const {
		return this->__then_of_arglist_1_ret_1<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) const {
		return this->__then_of_arglist_1_ret_2<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 3>) const {
		return this->__then_of_arglist_1_ret_3<R>(apartment, context, std::forward<FN>(fn));
	}

	template <class R, class FN>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) const {
		static_assert(std::is_void_v<R>, "R must be void");
		return this->__then_of_arglist_2_ret_x<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) const {
		return this->__then_of_arglist_2_ret_1<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) const {
		return this->__then_of_arglist_2_ret_2<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_then_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 3>) const {
		return this->__then_of_arglist_2_ret_3<R>(apartment, context, std::forward<FN>(fn));
	}

private: //__choose_transform
	template <class R, class FN>
	ks_future<R> __choose_transform(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<R(const ks_result<T>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<T>&, ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<R(const ks_result<T>&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<T>&)>> || std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<T>&)>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal transform's arglist");
		return this->__choose_transform_by_arglist<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, arglist_mode>());
	}

	template <class R, class FN>
	ks_future<R> __choose_transform_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>) const {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const ks_result<T>&>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<T>&>, ks_future<R>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<T>&>, ks_result<R>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<T>&>, R> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal transform's ret");
		return this->__choose_transform_by_arglist_ret<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class R, class FN>
	ks_future<R> __choose_transform_by_arglist(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>) const {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const ks_result<T>&, ks_cancel_inspector*>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<T>&, ks_cancel_inspector*>, ks_future<R>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<T>&, ks_cancel_inspector*>, ks_result<R>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<T>&, ks_cancel_inspector*>, R> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal transform's ret");
		return this->__choose_transform_by_arglist_ret<R>(apartment, context, std::forward<FN>(fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class R, class FN>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) const {
		static_assert(std::is_void_v<R>, "R must be void");
		return this->__transform_of_arglist_1_ret_x<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) const {
		return this->__transform_of_arglist_1_ret_1<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) const {
		return this->__transform_of_arglist_1_ret_2<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 3>) const {
		return this->__transform_of_arglist_1_ret_3<R>(apartment, context, std::forward<FN>(fn));
	}

	template <class R, class FN>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) const {
		static_assert(std::is_void_v<R>, "R must be void");
		return this->__transform_of_arglist_2_ret_x<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) const {
		return this->__transform_of_arglist_2_ret_1<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) const {
		return this->__transform_of_arglist_2_ret_2<R>(apartment, context, std::forward<FN>(fn));
	}
	template <class R, class FN>
	ks_future<R> __choose_transform_by_arglist_ret(ks_apartment* apartment, const ks_async_context& context, FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 3>) const {
		return this->__transform_of_arglist_2_ret_3<R>(apartment, context, std::forward<FN>(fn));
	}

private: //__post
	static ks_future<T> __post_of_arglist_1_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T()> task_fn) {
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			T typed_value = task_fn();
			return ks_raw_value::of<T>(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post(std::move(raw_task_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_of_arglist_1_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>()> task_fn) {
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			ks_result<T> result = task_fn();
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post(std::move(raw_task_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_of_arglist_1_ret_3(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<T>()> task_fn) {
		return ks_future<ks_future<T>>::post(apartment, context, std::move(task_fn))
			.template flat_then<T>(apartment, context, [](const ks_future<T>& value_future) ->ks_future<T> { return value_future; });
	}
	static ks_future<T> __post_of_arglist_2_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T(ks_cancel_inspector*)> task_fn) {
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			T typed_value = task_fn(ks_cancel_inspector::__for_future());
			return ks_raw_value::of<T>(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post(std::move(raw_task_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_of_arglist_2_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>(ks_cancel_inspector*)> task_fn) {
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			ks_result<T> result = task_fn(ks_cancel_inspector::__for_future());
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post(std::move(raw_task_fn), context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_of_arglist_2_ret_3(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<T>(ks_cancel_inspector*)> task_fn) {
		return ks_future<ks_future<T>>::post(apartment, context, std::move(task_fn))
			.template flat_then<T>(apartment, context, [](const ks_future<T>& value_future) ->ks_future<T> { return value_future; });
	}

private: //__post_delayed
	static ks_future<T> __post_delayed_of_arglist_1_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T()> task_fn, int64_t delay) {
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			T typed_value = task_fn();
			return ks_raw_value::of<T>(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post_delayed(std::move(raw_task_fn), context, apartment, delay);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_delayed_of_arglist_1_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>()> task_fn, int64_t delay) {
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			ks_result<T> result = task_fn();
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post_delayed(std::move(raw_task_fn), context, apartment, delay);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_delayed_of_arglist_1_ret_3(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<T>()> task_fn, int64_t delay) {
		return ks_future<ks_future<T>>::post_delayed(apartment, context, std::move(task_fn), delay)
			.template flat_then<T>(apartment, context, [](const ks_future<T>& value_future) -> ks_future<T> { return value_future; });
	}
	static ks_future<T> __post_delayed_of_arglist_2_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T(ks_cancel_inspector*)> task_fn, int64_t delay) {
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			T typed_value = task_fn(ks_cancel_inspector::__for_future());
			return ks_raw_value::of<T>(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post_delayed(std::move(raw_task_fn), context, apartment, delay);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_delayed_of_arglist_2_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>(ks_cancel_inspector*)> task_fn, int64_t delay) {
		auto raw_task_fn = [task_fn = std::move(task_fn)]()->ks_raw_result {
			ks_result<T> result = task_fn(ks_cancel_inspector::__for_future());
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = ks_raw_future::post_delayed(std::move(raw_task_fn), context, apartment, delay);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_delayed_of_arglist_2_ret_3(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<T>(ks_cancel_inspector*)> task_fn, int64_t delay) {
		return ks_future<ks_future<T>>::post_delayed(apartment, context, std::move(task_fn), delay)
			.template flat_then<T>(apartment, context, [](const ks_future<T>& value_future) -> ks_future<T> { return value_future; });
	}

private: //__post_pending
	static ks_future<T> __post_pending_of_arglist_1_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T()> task_fn, ks_pending_trigger* trigger) {
		auto raw_task_fn = [task_fn = std::move(task_fn)](const ks_raw_result&)->ks_raw_result {
			T typed_value = task_fn();
			return ks_raw_value::of<T>(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = (trigger != nullptr)
			? trigger->__get_raw_trigger_future()->then(raw_task_fn, context, apartment)
			: ks_raw_future::resolved(ks_raw_value::of_nothing(), ks_apartment::current_thread_apartment_or(apartment))->then(raw_task_fn, context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_pending_of_arglist_1_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>()> task_fn, ks_pending_trigger* trigger) {
		auto raw_task_fn = [task_fn = std::move(task_fn)](const ks_raw_result&)->ks_raw_result {
			ks_result<T> result = task_fn();
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = (trigger != nullptr)
			? trigger->__get_raw_trigger_future()->then(raw_task_fn, context, apartment)
			: ks_raw_future::resolved(ks_raw_value::of_nothing(), ks_apartment::current_thread_apartment_or(apartment))->then(raw_task_fn, context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_pending_of_arglist_1_ret_3(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<T>()> task_fn, ks_pending_trigger* trigger) {
		return ks_future<ks_future<T>>::post_pending(apartment, context, std::move(task_fn), trigger)
			.template flat_then<T>(apartment, context, [](const ks_future<T>& value_future) -> ks_future<T> { return value_future; });
	}
	static ks_future<T> __post_pending_of_arglist_2_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<T(ks_cancel_inspector*)> task_fn, ks_pending_trigger* trigger) {
		auto raw_task_fn = [task_fn = std::move(task_fn)](const ks_raw_result&)->ks_raw_result {
			T typed_value = task_fn(ks_cancel_inspector::__for_future());
			return ks_raw_value::of<T>(std::move(typed_value));
		};
		ks_raw_future_ptr raw_future = (trigger != nullptr)
			? trigger->__get_raw_trigger_future()->then(raw_task_fn, context, apartment)
			: ks_raw_future::resolved(ks_raw_value::of_nothing(), ks_apartment::current_thread_apartment_or(apartment))->then(raw_task_fn, context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_pending_of_arglist_2_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>(ks_cancel_inspector*)> task_fn, ks_pending_trigger* trigger) {
		auto raw_task_fn = [task_fn = std::move(task_fn)](const ks_raw_result&)->ks_raw_result {
			ks_result<T> result = task_fn(ks_cancel_inspector::__for_future());
			return result.__get_raw();
		};
		ks_raw_future_ptr raw_future = (trigger != nullptr)
			? trigger->__get_raw_trigger_future()->then(raw_task_fn, context, apartment)
			: ks_raw_future::resolved(ks_raw_value::of_nothing(), ks_apartment::current_thread_apartment_or(apartment))->then(raw_task_fn, context, apartment);
		return ks_future<T>::__from_raw(raw_future);
	}
	static ks_future<T> __post_pending_of_arglist_2_ret_3(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<T>(ks_cancel_inspector*)> task_fn, ks_pending_trigger* trigger) {
		return ks_future<ks_future<T>>::post_pending(apartment, context, std::move(task_fn), trigger)
			.template flat_then<T>(apartment, context, [](const ks_future<T>& value_future) -> ks_future<T> { return value_future; });
	}

private: //__then
	template <class R>
	ks_future<R> __then_of_arglist_1_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<R(const T&)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
			R typed_value2 = fn(raw_value.get<T>());
			return ks_raw_value::of<R>(std::move(typed_value2));
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __then_of_arglist_1_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const T&)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
			ks_result<R> typed_result2 = fn(raw_value.get<T>());
			return typed_result2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __then_of_arglist_1_ret_3(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>(const T&)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_future_ptr {
			ks_future<R> typed_future2 = fn(raw_value.get<T>());
			return typed_future2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->flat_then(raw_fn, context, apartment);
		return ks_future<R>::__from_raw(raw_future2);

	}
	template <class R>
	ks_future<R> __then_of_arglist_2_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<R(const T&, ks_cancel_inspector*)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
			R typed_value2 = fn(raw_value.get<T>(), ks_cancel_inspector::__for_future());
			return ks_raw_value::of<R>(std::move(typed_value2));
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __then_of_arglist_2_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const T&, ks_cancel_inspector*)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
			ks_result<R> typed_result2 = fn(raw_value.get<T>(), ks_cancel_inspector::__for_future());
			return typed_result2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __then_of_arglist_2_ret_3(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>(const T&, ks_cancel_inspector*)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_future_ptr {
			ks_future<R> typed_future2 = fn(raw_value.get<T>(), ks_cancel_inspector::__for_future());
			return typed_future2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->flat_then(raw_fn, context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}

	template <class R>
	ks_future<R> __then_of_arglist_1_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const T&)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
			fn(raw_value.get<T>());
			return ks_raw_value::of_nothing();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __then_of_arglist_2_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const T&, ks_cancel_inspector*)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_value& raw_value)->ks_raw_result {
			fn(raw_value.get<T>(), ks_cancel_inspector::__for_future());
			return ks_raw_value::of_nothing();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}

private: //__transform
	template <class R>
	ks_future<R> __transform_of_arglist_1_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<R(const ks_result<T>&)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
			R typed_value2 = fn(ks_result<T>::__from_raw(raw_result));
			return ks_raw_value::of<R>(std::move(typed_value2));
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->transform(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __transform_of_arglist_1_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const ks_result<T>&)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
			ks_result<R> typed_result2 = fn(ks_result<T>::__from_raw(raw_result));
			return typed_result2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->transform(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __transform_of_arglist_1_ret_3(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>(const ks_result<T>&)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_future_ptr {
			ks_future<R> typed_future2 = fn(ks_result<T>::__from_raw(raw_result));
			return typed_future2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->flat_transform(raw_fn, context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __transform_of_arglist_2_ret_1(ks_apartment* apartment, const ks_async_context& context, std::function<R(const ks_result<T>&, ks_cancel_inspector*)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
			R typed_value2 = fn(ks_result<T>::__from_raw(raw_result), ks_cancel_inspector::__for_future());
			return ks_raw_value::of<R>(std::move(typed_value2));
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->transform(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __transform_of_arglist_2_ret_2(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
			ks_result<R> typed_result2 = fn(ks_result<T>::__from_raw(raw_result), ks_cancel_inspector::__for_future());
			return typed_result2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->transform(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __transform_of_arglist_2_ret_3(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>(const ks_result<T>&, ks_cancel_inspector*)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_future_ptr {
			ks_future<R> typed_future2 = fn(ks_result<T>::__from_raw(raw_result), ks_cancel_inspector::__for_future());
			return typed_future2.__get_raw();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->flat_transform(raw_fn, context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}

	template <class R>
	ks_future<R> __transform_of_arglist_1_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_result<T>&)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
			fn(ks_result<T>::__from_raw(raw_result));
			return ks_raw_value::of_nothing();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}
	template <class R>
	ks_future<R> __transform_of_arglist_2_ret_x(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_result<T>&, ks_cancel_inspector*)> fn) const {
		auto raw_fn = [fn = std::move(fn)](const ks_raw_result& raw_result)->ks_raw_result {
			fn(ks_result<T>::__from_raw(raw_result), ks_cancel_inspector::__for_future());
			return ks_raw_value::of_nothing();
		};
		ks_raw_future_ptr raw_future2 = m_raw_future->then(std::move(raw_fn), context, apartment);
		return ks_future<R>::__from_raw(raw_future2);
	}

private:
	using __raw_cast_mode_t = typename ks_result<T>::__raw_cast_mode_t;

	template <class R>
	static constexpr __raw_cast_mode_t __determine_raw_cast_mode() {
		return ks_result<T>::template __determine_raw_cast_mode<R>();
	}

	template <class R>
	ks_future<R> __do_cast(std::integral_constant<__raw_cast_mode_t, __raw_cast_mode_t::to_same> __cast_mode) const {
		return ks_future<R>::__from_raw(m_raw_future);
	}

	template <class R>
	ks_future<R> __do_cast(std::integral_constant<__raw_cast_mode_t, __raw_cast_mode_t::to_nothing> __cast_mode) const {
		ks_raw_future_ptr raw_future2 = m_raw_future->then(
			[](const ks_raw_value& value) -> ks_raw_result { return ks_raw_value::of_nothing(); },
			make_async_context().set_priority(0x10000), nullptr);
		return ks_future<R>::__from_raw(raw_future2);
	}

	template <class R>
	ks_future<R> __do_cast(std::integral_constant<__raw_cast_mode_t, __raw_cast_mode_t::to_other> __cast_mode) const {
		ks_raw_future_ptr raw_future2 = m_raw_future->then(
			[](const ks_raw_value& value) -> ks_raw_result {
				return ks_raw_value::of<R>(static_cast<R>(value.get<T>()));
			},
			make_async_context().set_priority(0x10000), nullptr);
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
	friend class ks_async_flow;

private:
	ks_raw_future_ptr m_raw_future;
};


#include "ks_future_void.inl"
