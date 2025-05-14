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
#include "ks_promise.h"


_NAMESPACE_LIKE class ks_future_util final { //as namespace
private:
	using ks_raw_future = __ks_async_raw::ks_raw_future;
	using ks_raw_future_ptr = __ks_async_raw::ks_raw_future_ptr;
	using ks_raw_promise = __ks_async_raw::ks_raw_promise;
	using ks_raw_promise_ptr = __ks_async_raw::ks_raw_promise_ptr;
	using ks_raw_result = __ks_async_raw::ks_raw_result;
	using ks_raw_value = __ks_async_raw::ks_raw_value;

public: //resolved, rejected
	template <class T, class X = T, class _ = std::enable_if_t<!std::is_void_v<T> && std::is_convertible_v<X, T>>>
	static ks_future<T> resolved(X&& value) {
		return ks_future<T>::resolved(std::forward<X>(value));
	}
	template <class T, class _ = std::enable_if_t<std::is_void_v<T>>>
	static ks_future<void> resolved(nothing_t) {
		return ks_future<void>::resolved(nothing);
	}
	template <class T, class _ = std::enable_if_t<std::is_void_v<T>>>
	static ks_future<void> resolved() {
		return ks_future<void>::resolved();
	}

	template <class T>
	static ks_future<T> rejected(const ks_error& error) {
		return ks_future<T>::rejected(error);
	}

	template <class T>
	static ks_future<T> __from_result(const ks_result<T>& result) {
		return ks_future<T>::__from_result(result);
	}

public: //post, post_delayed, post_pending
	template <class T, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<T()>> || 
		std::is_convertible_v<FN, std::function<ks_result<T>()>> || 
		std::is_convertible_v<FN, std::function<ks_future<T>()>> ||
		std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || 
		std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>> || 
		std::is_convertible_v<FN, std::function<ks_future<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> post(ks_apartment* apartment, FN&& task_fn, const ks_async_context& context = {}) {
		return ks_future<T>::post(apartment, context, std::forward<FN>(task_fn));
	}

	template <class T, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<T()>> ||
		std::is_convertible_v<FN, std::function<ks_result<T>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<T>()>> ||
		std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> post_delayed(ks_apartment* apartment, FN&& task_fn, int64_t delay, const ks_async_context& context = {}) {
		return ks_future<T>::post_delayed(apartment, std::forward<FN>(task_fn), delay, context);
	}

	template <class T, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<T()>> ||
		std::is_convertible_v<FN, std::function<ks_result<T>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<T>()>> ||
		std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<T>(ks_cancel_inspector*)>>>>
	static ks_future<T> post_pending(ks_apartment* apartment, FN&& task_fn, ks_pending_trigger* trigger, const ks_async_context& context = {}) {
		return ks_future<T>::post_pending(apartment, std::forward<FN>(task_fn), trigger, context);
	}

public: //all, any (for va-list)
	template <class... Ts, class _ = std::enable_if_t<!std::has_some_void_v<Ts...> && sizeof...(Ts) != 0>>
	static ks_future<std::tuple<Ts...>> all(const ks_future<Ts>&... futures) {
		return __all_for_tuple(std::index_sequence_for<Ts...>(), std::make_tuple(futures...));
	}
	template <class... Ts, class _ = std::enable_if_t<std::is_all_void_v<Ts...>>>
	static ks_future<void> all(const ks_future<Ts>&... futures) {  //all<void>特化
		return __all_for_tuple_x(std::index_sequence_for<Ts...>(), std::make_tuple(futures...));
	}

	template <class... Ts, class _ = std::enable_if_t<std::is_all_same_v<Ts...>>>
	static ks_future<std::variadic_element_t<0, Ts...>> any(const ks_future<Ts>&... futures) {
		return __any_for_tuple(std::index_sequence_for<Ts...>(), std::make_tuple(futures...));
	}

public: //all, any (for tuple)
	template <class... Ts, class _ = std::enable_if_t<!std::has_some_void_v<Ts...> && sizeof...(Ts) != 0>>
	static ks_future<std::tuple<Ts...>> all(const std::tuple<ks_future<Ts>...>& futures) {
		return __all_for_tuple(std::index_sequence_for<Ts...>(), futures);
	}
	template <class... Ts, class _ = std::enable_if_t<std::is_all_void_v<Ts...>>>
	static ks_future<void> all(const std::tuple<ks_future<Ts>...>& futures) {  //all<void>特化
		return __all_for_tuple_x(std::index_sequence_for<Ts...>(), futures);
	}

	template <class... Ts, class _ = std::enable_if_t<std::is_all_same_v<Ts...>>>
	static ks_future<std::variadic_element_t<0, Ts...>> any(const std::tuple<ks_future<Ts>...>& futures) {
		return __any_for_tuple(std::index_sequence_for<Ts...>(), futures);
	}

public: //all, any (for vector)
	template <class T, class _ = std::enable_if_t<!std::is_void_v<T>>>
	static ks_future<std::vector<T>> all(const std::vector<ks_future<T>>& futures) {
		return __all_for_vector(futures);
	}
	static ks_future<void> all(const std::vector<ks_future<void>>& futures) {  //all<void>特化
		return __all_for_vector_x(futures);
	}

	template <class T>
	static ks_future<T> any(const std::vector<ks_future<T>>& futures) {
		return __any_for_vector(futures);
	}

private:
	template <class... Ts, size_t... IDXs>
	static ks_future<std::tuple<Ts...>> __all_for_tuple(std::index_sequence<IDXs...>, const std::tuple<ks_future<Ts>...>& futures);
	template <class T>
	static ks_future<std::vector<T>> __all_for_vector(const std::vector<ks_future<T>>& futures);

	template <class... Ts, size_t... IDXs>
	static ks_future<void> __all_for_tuple_x(std::index_sequence<IDXs...>, const std::tuple<ks_future<Ts>...>& futures);
	template <class _ = void>
	static ks_future<void> __all_for_vector_x(const std::vector<ks_future<void>>& futures);

	template <class... Ts, size_t... IDXs>
	static ks_future<std::variadic_element_t<0, Ts...>> __any_for_tuple(std::index_sequence<IDXs...>, const std::tuple<ks_future<Ts>...>& futures);
	template <class T>
	static ks_future<T> __any_for_vector(const std::vector<ks_future<T>>& futures);

public: //parallel, parallel_n
	template <class FNS, class _ = std::enable_if_t <
		std::is_convertible_v<typename FNS::value_type, std::function<void()>> ||
		std::is_convertible_v<typename FNS::value_type, std::function<ks_result<void>()>> ||
		std::is_convertible_v<typename FNS::value_type, std::function<ks_future<void>()>>>>
	static ks_future<void> parallel(
		ks_apartment* apartment, const FNS& fns,
		const ks_async_context& context = {});

	template <class FN, class _ = std::enable_if_t <
		std::is_convertible_v<FN, std::function<void()>> ||
		std::is_convertible_v<FN, std::function<ks_result<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>()>>>>
	static ks_future<void> parallel_n(
		ks_apartment* apartment, FN&& fn, size_t n,
		const ks_async_context& context = {});

public: //sequential, sequential_n
	template <class FNS, class _ = std::enable_if_t <
		std::is_convertible_v<typename FNS::value_type, std::function<void()>> ||
		std::is_convertible_v<typename FNS::value_type, std::function<ks_result<void>()>> ||
		std::is_convertible_v<typename FNS::value_type, std::function<ks_future<void>()>> ||
		std::is_convertible_v<typename FNS::value_type, std::function<void(ks_cancel_inspector*)>> ||
		std::is_convertible_v<typename FNS::value_type, std::function<ks_result<void>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<typename FNS::value_type, std::function<ks_future<void>(ks_cancel_inspector*)>>>>
	static ks_future<void> sequential(
		ks_apartment* apartment, const FNS& fns,
		const ks_async_context& context = {});

	template <class FN, class _ = std::enable_if_t <
		std::is_convertible_v<FN, std::function<void()>> ||
		std::is_convertible_v<FN, std::function<ks_result<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>()>> ||
		std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>(ks_cancel_inspector*)>>>>
	static ks_future<void> sequential_n(
		ks_apartment* apartment, FN&& fn, size_t n,
		const ks_async_context& context = {});

public: //repeat, repeat_periodic, repeat_productive
	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<ks_result<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>(ks_cancel_inspector*)>>>>
	static ks_future<void> repeat(
		ks_apartment* apartment, FN&& fn,
		const ks_async_context& context = {});

	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<ks_result<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>()>>>>
	static ks_future<void> repeat_periodic(
		ks_apartment* apartment, FN&& fn, 
		int64_t delay, int64_t interval,
		const ks_async_context& context = {});

	template <class V, class PRODUCE_FN, class CONSUME_FN, class _ = std::enable_if_t<
		(std::is_convertible_v<PRODUCE_FN, std::function<ks_result<V>()>> ||
		std::is_convertible_v<PRODUCE_FN, std::function<ks_future<V>()>> ||
		std::is_convertible_v<PRODUCE_FN, std::function<ks_result<V>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<PRODUCE_FN, std::function<ks_future<V>(ks_cancel_inspector*)>>) 
		&& 
		(std::is_convertible_v<CONSUME_FN, std::function<void(const V&)>> ||
		std::is_convertible_v<CONSUME_FN, std::function<ks_result<void>(const V&)>> ||
		std::is_convertible_v<CONSUME_FN, std::function<ks_future<void>(const V&)>> ||
		std::is_convertible_v<CONSUME_FN, std::function<void(const V&, ks_cancel_inspector*)>> ||
		std::is_convertible_v<CONSUME_FN, std::function<ks_result<void>(const V&, ks_cancel_inspector*)>> ||
		std::is_convertible_v<CONSUME_FN, std::function<ks_future<void>(const V&, ks_cancel_inspector*)>>)>>
	static ks_future<void> repeat_productive(
		ks_apartment* produce_apartment, PRODUCE_FN&& produce_fn,
		ks_apartment* consume_apartment, CONSUME_FN&& consume_fn,
		const ks_async_context& context = {});

private:
	struct __periodic_data_t {
		ks_apartment* apartment;
		std::function<ks_future<void>()> fn;
		int64_t delay;
		int64_t interval;
		ks_async_context context;

		std::chrono::steady_clock::time_point create_time{};
		uint64_t rounds = 0;

		ks_async_controller controller{};
		ks_raw_promise_ptr raw_final_promise_void = nullptr;
	};

	template <class _ = void>
	static void __schedule_periodic_once(const std::shared_ptr<__periodic_data_t>& data, int64_t next_delay);

private:
	template <class V>
	struct __repetitive_data_t {
		ks_apartment* produce_apartment;
		ks_apartment* consume_apartment;
		std::function<ks_future<V>()> produce_fn;
		std::function<ks_future<void>(const V&)> consume_fn;
		ks_async_context context;

		std::chrono::steady_clock::time_point create_time{};
		uint64_t rounds = 0;

		ks_async_controller controller{};
		ks_raw_promise_ptr raw_final_promise_void = nullptr;
	};

	template <class V>
	static void __pump_repetitive_once(const std::shared_ptr< __repetitive_data_t<V>>& data);

private:
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0(FN&& fn);

	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist(FN&& fn, std::integral_constant<int, 1>);
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist(FN&& fn, std::integral_constant<int, 2>);

	template <class T, class FN>
	static std::function<ks_future<void>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>);
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>);
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>);
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 3>);

	template <class T, class FN>
	static std::function<ks_future<void>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>);
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>);
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>);
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 3>);

private:
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1(FN&& fn);

	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist(FN&& fn, std::integral_constant<int, 1>);
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist(FN&& fn, std::integral_constant<int, 2>);

	template <class T, class ARG1, class FN>
	static std::function<ks_future<void>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>);
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>);
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>);
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 3>);

	template <class T, class ARG1, class FN>
	static std::function<ks_future<void>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>);
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>);
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>);
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 3>);

private:
	ks_future_util() = delete;
	_DISABLE_COPY_CONSTRUCTOR(ks_future_util);
};


#include "ks_future_util.inl"
