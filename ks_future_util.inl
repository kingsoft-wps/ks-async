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

template <class T> class ks_future;
template <class T> class ks_promise;


_NAMESPACE_LIKE class ks_future_util { //as namespace
private:
	using ks_raw_future = __ks_async_raw::ks_raw_future;
	using ks_raw_future_ptr = __ks_async_raw::ks_raw_future_ptr;
	using ks_raw_promise = __ks_async_raw::ks_raw_promise;
	using ks_raw_promise_ptr = __ks_async_raw::ks_raw_promise_ptr;
	using ks_raw_result = __ks_async_raw::ks_raw_result;
	using ks_raw_value = __ks_async_raw::ks_raw_value;

public: //resolved, rejected
	template <class T, class X = T, class _ = std::enable_if_t<std::is_void_v<T> ? std::is_nothing_v<X> : std::is_convertible_v<X, T>>>
	static ks_future<T> resolved(X&& value) {
		return ks_future<T>::resolved(std::forward<X>(value));
	}
	template <class T, class _ = std::enable_if_t<std::is_void_v<T>>>
	static ks_future<void> resolved(nothing_t = nothing) {
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

public: //tuple aggr
	template <class... Ts, class _ = std::enable_if_t<!std::has_some_void_v<Ts...> && sizeof...(Ts) != 0>>
	static ks_future<std::tuple<Ts...>> all(const ks_future<Ts>&... futures) {
		std::vector<ks_raw_future_ptr> raw_arg_futures{ futures.__get_raw()... };
		ks_raw_future_ptr raw_future = ks_raw_future::all(raw_arg_futures, nullptr)
			->then(
				[](const ks_raw_value& raw_value_aggr) -> ks_raw_result {
					const std::vector<ks_raw_value>& raw_value_vector = raw_value_aggr.get<std::vector<ks_raw_value>>();
					std::tuple<Ts...> typed_value_tuple = __convert_raw_value_vector_to_typed_value_tuple<Ts...>(raw_value_vector, std::index_sequence_for<Ts...>());
					return ks_raw_value::of<std::tuple<Ts...>>(std::move(typed_value_tuple));
				}, make_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](const auto&) {
				for (auto& rawf : raw_arg_futures)
					rawf->try_cancel(true);
			}, make_async_context().set_priority(0x10000), nullptr);
		}
		return ks_future<std::tuple<Ts...>>::__from_raw(raw_future);
	}

	template <class... Ts, class _ = std::enable_if_t<std::is_all_void_v<Ts...>>>
	static ks_future<void> all(const ks_future<Ts>&... futures) {  //all<void...>特化
		static_assert(std::is_all_void_v<Ts...>, "the type of all futures must be ks_future<void>");
		std::vector<ks_raw_future_ptr> raw_arg_futures{ futures.__get_raw()... };
		ks_raw_future_ptr raw_future = ks_raw_future::all(raw_arg_futures, nullptr)
			->then(
				[](const ks_raw_value& raw_value_aggr) -> ks_raw_result {
					return ks_raw_value::of_nothing();
				}, make_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](const auto&) {
				for (auto& rawf : raw_arg_futures)
					rawf->try_cancel(true);
			}, make_async_context().set_priority(0x10000), nullptr);
		}
		return ks_future<void>::__from_raw(raw_future);
	}

	template <class... Ts, class T = std::variadic_element_t<0, Ts...>, class _ = std::enable_if_t<std::is_all_same_v<T, Ts...>>>
	static ks_future<T> any(const ks_future<Ts>&... futures) {
		static_assert(std::is_all_same_v<T, Ts...>, "the type of all futures must be identical");
		std::vector<ks_raw_future_ptr> raw_arg_futures{ futures.template cast<T>().__get_raw()... };
		ks_raw_future_ptr raw_future = ks_raw_future::any(raw_arg_futures, nullptr);
		//for any(), when succ, auto cancel other not-completed futures
		if (true) {
			raw_future->on_success([raw_arg_futures = std::move(raw_arg_futures)](const auto&) {
				for (auto& rawf : raw_arg_futures)
					rawf->try_cancel(true);
			}, make_async_context().set_priority(0x10000), nullptr);
		}
		return ks_future<T>::__from_raw(raw_future);
	}

public: //vector aggr
	template <class T, class _ = std::enable_if_t<!std::is_void_v<T>>>
	static ks_future<std::vector<T>> all(const std::vector<ks_future<T>>& futures) {
		std::vector<ks_raw_future_ptr> raw_arg_futures;
		raw_arg_futures.reserve(futures.size());
		for (auto& future : futures)
			raw_arg_futures.push_back(future.__get_raw());
		ks_raw_future_ptr raw_future = ks_raw_future::all(raw_arg_futures, nullptr)
			->then(
				[](const ks_raw_value& raw_value_aggr) -> ks_raw_result {
					const std::vector<ks_raw_value>& raw_value_vector = raw_value_aggr.get<std::vector<ks_raw_value>>();
					std::vector<T> typed_value_vector;
					typed_value_vector.reserve(raw_value_vector.size());
					for (auto& raw_value : raw_value_vector)
						typed_value_vector.push_back(raw_value.get<T>());
					return ks_raw_value::of<std::vector<T>>(std::move(typed_value_vector));
				}, make_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](const auto&) {
				for (auto& rawf : raw_arg_futures)
					rawf->try_cancel(true);
			}, make_async_context().set_priority(0x10000), nullptr);
		}
		return ks_future<std::vector<T>>::__from_raw(raw_future);
	}

	static ks_future<void> all(const std::vector<ks_future<void>>& futures) {  //all<void>特化
		std::vector<ks_raw_future_ptr> raw_arg_futures;
		raw_arg_futures.reserve(futures.size());
		for (auto& future : futures)
			raw_arg_futures.push_back(future.__get_raw());
		ks_raw_future_ptr raw_future = ks_raw_future::all(raw_arg_futures, nullptr)
			->then(
				[](const ks_raw_value& raw_value_aggr) -> ks_raw_result {
					return ks_raw_value::of_nothing();
				}, make_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](const auto&) {
				for (auto& rawf : raw_arg_futures)
					rawf->try_cancel(true);
			}, make_async_context().set_priority(0x10000), nullptr);
		}
		return ks_future<void>::__from_raw(raw_future);
	}

	template <class T>
	static ks_future<T> any(const std::vector<ks_future<T>>& futures) {
		std::vector<ks_raw_future_ptr> raw_arg_futures;
		raw_arg_futures.reserve(futures.size());
		for (auto& future : futures)
			raw_arg_futures.push_back(future.__get_raw());
		ks_raw_future_ptr raw_future = ks_raw_future::any(raw_arg_futures, nullptr);
		//for any(), when succ, auto cancel other not-completed futures
		if (true) {
			raw_future->on_success([raw_arg_futures = std::move(raw_arg_futures)](const auto&) {
				for (auto& rawf : raw_arg_futures)
					rawf->try_cancel(true);
			}, make_async_context().set_priority(0x10000), nullptr);
		}
		return ks_future<T>::__from_raw(raw_future);
	}

private:
	template <class... Ts, size_t... IDXs>
	static std::tuple<Ts...> __convert_raw_value_vector_to_typed_value_tuple(const std::vector<ks_raw_value>& value_vec, std::index_sequence<IDXs...>) {
		return std::tuple<Ts...>(value_vec.at(IDXs).get<Ts>()...);
	}

public: //parallel, parallel_n
	template <class FNS, class _ = std::enable_if_t <
		std::is_convertible_v<typename FNS::value_type, std::function<void()>> ||
		std::is_convertible_v<typename FNS::value_type, std::function<ks_result<void>()>> ||
		std::is_convertible_v<typename FNS::value_type, std::function<ks_future<void>()>>>>
	static ks_future<void> parallel(
			ks_apartment* apartment, const FNS& fns, 
			const ks_async_context& context = {}) {

		if (fns.empty()) {
			return ks_future<void>::resolved(nothing);
		}
		else if (fns.size() == 1) {
			return ks_future_util::post<void>(apartment, fns.front(), context);
		}
		else {
			std::vector<ks_future<void>> future_vec;
			future_vec.reserve(fns.size());
			for (const auto& fn : fns) {
				future_vec.push_back(
					ks_future_util::post<void>(apartment, fn, context));
			}

			return ks_future_util::all(future_vec);
		}
	}

	template <class FN, class _ = std::enable_if_t <
		std::is_convertible_v<FN, std::function<void()>> ||
		std::is_convertible_v<FN, std::function<ks_result<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>()>>>>
	static ks_future<void> parallel_n(
		ks_apartment* apartment, FN&& fn, size_t n,
		const ks_async_context& context = {}) {

		if (n == 0) {
			std::__try_prune_if_mutable_rvalue_reference<FN>(fn);
			return ks_future<void>::resolved(nothing);
		}
		else if (n == 1) {
			return ks_future_util::post<void>(
				apartment, std::forward<FN>(fn), context);
		}
		else {
			std::vector<ks_future<void>> future_vec;
			future_vec.reserve(n);

			for (size_t i = 0; i < n; ++i) {
				future_vec.push_back(
					ks_future_util::post<void>(apartment, fn, context)
				);
			}

			std::__try_prune_if_mutable_rvalue_reference<FN>(fn);
			return ks_future_util::all(future_vec);
		}
	}

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
		const ks_async_context& context = {}) {

		if (fns.empty()) {
			return ks_future<void>::resolved(nothing);
		}
		else if (fns.size() == 1) {
			return ks_future_util::post<void>(apartment, fns.front(), context);
		}
		else {
			struct __sequential_data_t {
				std::vector<std::function<ks_future<void>()>> fns;
				size_t index = 0;
				ks_error last_error = ks_error::unexpected_error();
			};

			std::shared_ptr<__sequential_data_t> data = std::make_shared< __sequential_data_t>();
			data->fns.reserve(fns.size());
			for (const auto& fn : fns) {
				data->fns.push_back(__wrap_async_fn_0<void>(fn));
			}

			const auto fn_wrapper = [apartment, data]() -> ks_future<void> {
				if (data->index < data->fns.size()) {
					const auto& fn = data->fns[data->index++];
					return fn().on_failure(
						apartment,
						[data](const ks_error& error) { data->last_error = error; },
						make_async_context().set_priority(0x10000));
				}
				else {
					return ks_future<void>::rejected(ks_error::eof_error());
				}
			};

			return ks_future_util
				::repeat(apartment, fn_wrapper, context)
				.template then<void>(
					apartment, 
					[data]() -> ks_result<void> {
						if (data->index < data->fns.size())
							return data->last_error; //若repeat返回成功，但index未达n，则意味着中间遇到了eof，那么我们还原为返回eof错误
						else
							return nothing;
					},
					make_async_context().set_priority(0x10000));
		}
	}

	template <class FN, class _ = std::enable_if_t <
		std::is_convertible_v<FN, std::function<void()>> ||
		std::is_convertible_v<FN, std::function<ks_result<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>()>> ||
		std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>(ks_cancel_inspector*)>>>>
	static ks_future<void> sequential_n(
		ks_apartment* apartment, FN&& fn, size_t n,
		const ks_async_context& context = {}) {

		if (n == 0) {
			std::__try_prune_if_mutable_rvalue_reference<FN>(fn);
			return ks_future<void>::resolved(nothing);
		}
		else if (n == 1) {
			return ks_future_util::post<void>(
				apartment, std::forward<FN>(fn), context);
		}
		else {
			struct __sequential_data_t {
				std::function<ks_future<void>()> fn;
				size_t n;
				size_t index = 0;
				ks_error last_error = ks_error::unexpected_error();
			};

			std::shared_ptr<__sequential_data_t> data = std::make_shared< __sequential_data_t>();
			data->fn = __wrap_async_fn_0<void>(std::forward<FN>(fn));
			data->n = n;

			auto fn_wrapper = [apartment, data]() -> ks_future<void> {
				if (data->index < data->n) {
					data->index++;
					return data->fn().on_failure(
							apartment,
							[data](const ks_error& error) { data->last_error = error; },
							make_async_context().set_priority(0x10000));
				}
				else {
					return ks_future<void>::rejected(ks_error::eof_error());
				}
			};

			return ks_future_util
				::repeat(apartment, fn_wrapper, context)
				.template then<void>(
					apartment, 
					[data]() -> ks_result<void> {
						if (data->index < data->n)
							return data->last_error; //若repeat返回成功，但index未达n，则意味着中间遇到了eof，那么我们还原为返回eof错误
						else
							return nothing;
					},
					make_async_context().set_priority(0x10000));
		}
	}

public: //repeat, repeat_periodic, repeat_productive
	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<ks_result<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>(ks_cancel_inspector*)>>>>
	static ks_future<void> repeat(
		ks_apartment* apartment, FN&& fn,
		const ks_async_context& context = {}) {

		//repeat可以简单地变换为repeat_productive，一个空的produce，后接fn
		return ks_future_util::repeat_productive<nothing_t>(
			apartment, []() -> nothing_t { return nothing; },
			apartment, [fn = std::forward<FN>(fn)](const nothing_t&) -> auto { return fn(); },
			context);
	}

	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<ks_result<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>()>>>>
	static ks_future<void> repeat_periodic(
		ks_apartment* apartment, FN&& fn, 
		int64_t delay, int64_t interval,
		const ks_async_context& context = {}) {

		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::current_thread_apartment_or_default_mta();

		if (delay == 0 && interval == 0) {
			//如果delay和interval都为0，则等价于repeat
			return ks_future_util::repeat(apartment, fn, context);
		}

		std::shared_ptr<__periodic_data_t> data = std::make_shared<__periodic_data_t>();
		data->apartment = apartment;
		data->fn = __wrap_async_fn_0<void>(std::forward<FN>(fn));
		data->delay = delay;
		data->interval = interval;
		data->context = make_async_context().bind_controller(&data->controller).set_parent(context, true);
		data->create_time = std::chrono::steady_clock::now();
		data->raw_final_promise_void = ks_raw_promise::create(apartment);

		data->raw_final_promise_void->get_future()->on_failure(
			//注：支持在final_future上调用try_cancel
			[data_weak = std::weak_ptr<__periodic_data_t>(data)](const ks_error& error) {
				if (error.get_code() == ks_error::CANCELLED_ERROR_CODE) {
					auto data = data_weak.lock();
					if (data != nullptr)
						data->controller.try_cancel_all();
				}
			},
			make_async_context().set_priority(0x10000), apartment);

		__schedule_periodic_once(data, delay);

		return ks_future<void>::__from_raw(data->raw_final_promise_void->get_future());
	}

	template <class V, class PRODUCE_FN, class CONSUME_FN, 
		class __PRODUCE_FN_VERIFY__ = std::enable_if_t<
			std::is_convertible_v<PRODUCE_FN, std::function<ks_result<V>()>> ||
			std::is_convertible_v<PRODUCE_FN, std::function<ks_future<V>()>> ||
			std::is_convertible_v<PRODUCE_FN, std::function<ks_result<V>(ks_cancel_inspector*)>> ||
			std::is_convertible_v<PRODUCE_FN, std::function<ks_future<V>(ks_cancel_inspector*)>>>,
		class __CONSUME_FN_VERIFY__ = std::enable_if_t <
			std::is_convertible_v<CONSUME_FN, std::function<void(const V&)>> ||
			std::is_convertible_v<CONSUME_FN, std::function<ks_result<void>(const V&)>> ||
			std::is_convertible_v<CONSUME_FN, std::function<ks_future<void>(const V&)>> ||
			std::is_convertible_v<CONSUME_FN, std::function<void(const V&, ks_cancel_inspector*)>> ||
			std::is_convertible_v<CONSUME_FN, std::function<ks_result<void>(const V&, ks_cancel_inspector*)>> ||
			std::is_convertible_v<CONSUME_FN, std::function<ks_future<void>(const V&, ks_cancel_inspector*)>>>>
	static ks_future<void> repeat_productive(
		ks_apartment* produce_apartment, PRODUCE_FN&& produce_fn,
		ks_apartment* consume_apartment, CONSUME_FN&& consume_fn,
		const ks_async_context& context = {}) {

		ASSERT(produce_apartment != nullptr);
		ASSERT(consume_apartment != nullptr);
		if (produce_apartment == nullptr)
			produce_apartment = ks_apartment::current_thread_apartment_or_default_mta();
		if (consume_apartment == nullptr)
			consume_apartment = ks_apartment::current_thread_apartment_or_default_mta();

		std::shared_ptr<__repetitive_data_t<V>> data = std::make_shared<__repetitive_data_t<V>>();
		data->produce_apartment = produce_apartment;
		data->consume_apartment = consume_apartment;
		data->produce_fn = __wrap_async_fn_0<V>(std::forward<PRODUCE_FN>(produce_fn));
		data->consume_fn = __wrap_async_fn_1<void, V>(std::forward<CONSUME_FN>(consume_fn));
		data->context = make_async_context().bind_controller(&data->controller).set_parent(context, true);
		data->create_time = std::chrono::steady_clock::now();
		data->raw_final_promise_void = ks_raw_promise::create(consume_apartment);

		data->raw_final_promise_void->get_future()->on_failure(
			//注：支持在final_future上调用try_cancel
			[data_weak = std::weak_ptr<__repetitive_data_t<V>>(data)](const ks_error& error) {
				if (error.get_code() == ks_error::CANCELLED_ERROR_CODE) {
					auto data = data_weak.lock();
					if (data != nullptr)
						data->controller.try_cancel_all();
				}
			}, 
			make_async_context().set_priority(0x10000), produce_apartment);

		__pump_repetitive_once<V>(data);

		return ks_future<void>::__from_raw(data->raw_final_promise_void->get_future());
	}

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

	static void __schedule_periodic_once(const std::shared_ptr<__periodic_data_t>& data, int64_t next_delay) {
		ks_future_util
			::post_delayed<void>(
				data->apartment,
				data->fn,
				next_delay, 
				data->context)
			.on_completion(
				data->apartment,
				[data](const ks_result<void>& result) {
					if (result.is_value()) {
						data->rounds++;
						const std::chrono::steady_clock::time_point now_time = std::chrono::steady_clock::now();
						const std::chrono::steady_clock::time_point next_time = data->create_time + std::chrono::milliseconds((long long)(data->delay + data->interval * data->rounds));
						const int64_t next_delay = std::chrono::duration_cast<std::chrono::milliseconds>(next_time - now_time).count();
						__schedule_periodic_once(data, next_delay);
					}
					else {
						data->raw_final_promise_void->try_settle(__last_error_to_raw_result_void(result.to_error()));
					}
				},
				make_async_context().set_priority(0x10000));
	}

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
	static void __pump_repetitive_once(const std::shared_ptr< __repetitive_data_t<V>>& data) {
		ks_future_util
			::post<V>(
				data->produce_apartment,
				data->produce_fn,
				data->context)
			.template flat_then<void>(
				data->consume_apartment,
				data->consume_fn,
				data->context)
			.on_completion(
				data->consume_apartment,
				[data](const ks_result<void>& result) {
					if (result.is_value()) {
						data->rounds++;
						__pump_repetitive_once<V>(data);
					}
					else {
						data->raw_final_promise_void->try_settle(__last_error_to_raw_result_void(result.to_error()));
					}
				},
				make_async_context().set_priority(0x10000));
	}

private:
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0(FN&& fn) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<T(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_future<T>(ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<T()>> || std::is_convertible_v<FN, std::function<ks_result<T>()>> || std::is_convertible_v<FN, std::function<ks_future<T>()>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal async-fn's arglist");
		return __wrap_async_fn_0_by_arglist<T>(std::forward<FN>(fn), std::integral_constant<int, arglist_mode>());
	}

	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist(FN&& fn, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN>, ks_future<T>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN>, ks_result<T>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN>, T> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal async-fn's ret");
		return __wrap_async_fn_0_by_arglist_ret<T>(std::forward<FN>(fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist(FN&& fn, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, ks_cancel_inspector*>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_future<T>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_result<T>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, T> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal async-fn's ret");
		return __wrap_async_fn_0_by_arglist_ret<T>(std::forward<FN>(fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class T, class FN>
	static std::function<ks_future<void>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) {
		static_assert(std::is_void_v<T>, "T must be void");
		return [fn = std::forward<FN>(fn)]() -> ks_future<void> {
			fn();
			return ks_future<void>::resolved(nothing);
		};
	}
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return [fn = std::forward<FN>(fn)]()->ks_future<T> {
			T value = fn();
			return ks_future<T>::resolved(std::move(value));
		};
	}
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return [fn = std::forward<FN>(fn)]()->ks_future<T> {
			ks_result<T> result = fn();
			return ks_future<T>::__from_result(result);
		};
	}
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 3>) {
		return[fn = std::forward<FN>(fn)]()->ks_future<T> {
			ks_future<T> future = fn();
			return future;
		};
	}

	template <class T, class FN>
	static std::function<ks_future<void>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) {
		static_assert(std::is_void_v<T>, "T must be void");
		return[fn = std::forward<FN>(fn)]()->ks_future<void> {
			fn(ks_cancel_inspector::__for_future());
			return ks_future<void>::resolved(nothing);
		};
	}
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return[fn = std::forward<FN>(fn)]()->ks_future<T> {
			T value = fn(ks_cancel_inspector::__for_future());
			return ks_future<T>::resolved(std::move(value));
		};
	}
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return[fn = std::forward<FN>(fn)]()->ks_future<T> {
			ks_result<T> result = fn(ks_cancel_inspector::__for_future());
			return ks_future<T>::__from_result(result);
		};
	}
	template <class T, class FN>
	static std::function<ks_future<T>()> __wrap_async_fn_0_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 3>) {
		return[fn = std::forward<FN>(fn)]()->ks_future<T> {
			ks_future<T> future = fn(ks_cancel_inspector::__for_future());
			return future;
		};
	}

private:
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1(FN&& fn) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<T(const ARG1&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<T>(const ARG1& , ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_future<T>(const ARG1&, ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<T(const ARG1&)>> || std::is_convertible_v<FN, std::function<ks_result<T>(const ARG1&)>> || std::is_convertible_v<FN, std::function<ks_future<T>(const ARG1&)>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal async-fn's arglist");
		return __wrap_async_fn_1_by_arglist<T, ARG1>(std::forward<FN>(fn), std::integral_constant<int, arglist_mode>());
	}

	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist(FN&& fn, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const ARG1&>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, const ARG1&>, ks_future<T>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, const ARG1&>, ks_result<T>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, const ARG1&>, T> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal async-fn's ret");
		return __wrap_async_fn_1_by_arglist_ret<T, ARG1>(std::forward<FN>(fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist(FN&& fn, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const ARG1&, ks_cancel_inspector*>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, const ARG1&, ks_cancel_inspector*>, ks_future<T>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, const ARG1&, ks_cancel_inspector*>, ks_result<T>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, const ARG1&, ks_cancel_inspector*>, T> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal async-fn's ret");
		return __wrap_async_fn_1_by_arglist_ret<T, ARG1>(std::forward<FN>(fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class T, class ARG1, class FN>
	static std::function<ks_future<void>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) {
		static_assert(std::is_void_v<T>, "T must be void");
		return [fn = std::forward<FN>(fn)](const ARG1& arg1)->ks_future<void> {
			fn(arg1);
			return ks_future<void>::resolved(nothing);
		};
	}
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return [fn = std::forward<FN>(fn)](const ARG1& arg1)->ks_future<T> {
			T value = fn(arg1);
			return ks_future<T>::resolved(std::move(value));
		};
	}
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return [fn = std::forward<FN>(fn)](const ARG1& arg1)->ks_future<T> {
			ks_result<T> result = fn(arg1);
			return ks_future<T>::__from_result(result);
		};
	}
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 3>) {
		return[fn = std::forward<FN>(fn)](const ARG1& arg1)->ks_future<T> {
			ks_future<T> future = fn(arg1);
			return future;
		};
	}

	template <class T, class ARG1, class FN>
	static std::function<ks_future<void>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) {
		static_assert(std::is_void_v<T>, "T must be void");
		return[fn = std::forward<FN>(fn)](const ARG1& arg1)->ks_future<void> {
			fn(arg1, ks_cancel_inspector::__for_future());
			return ks_future<void>::resolved(nothing);
		};
	}
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return[fn = std::forward<FN>(fn)](const ARG1& arg1)->ks_future<T> {
			T value = fn(arg1, ks_cancel_inspector::__for_future());
			return ks_future<T>::resolved(std::move(value));
		};
	}
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return[fn = std::forward<FN>(fn)](const ARG1& arg1)->ks_future<T> {
			ks_result<T> result = fn(arg1, ks_cancel_inspector::__for_future());
			return ks_future<T>::__from_result(result);
		};
	}
	template <class T, class ARG1, class FN>
	static std::function<ks_future<T>(const ARG1&)> __wrap_async_fn_1_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 3>) {
		return[fn = std::forward<FN>(fn)](const ARG1& arg1)->ks_future<T> {
			ks_future<T> future = fn(arg1, ks_cancel_inspector::__for_future());
			return future;
		};
	}

private:
	static ks_raw_result __last_error_to_raw_result_void(const ks_error& error) {
		if (error.get_code() == 0 || error.get_code() == ks_error::EOF_ERROR_CODE)
			return ks_raw_value::of_nothing();
		else
			return error;
	}

private:
	ks_future_util() = delete;
	_DISABLE_COPY_CONSTRUCTOR(ks_future_util);
};
