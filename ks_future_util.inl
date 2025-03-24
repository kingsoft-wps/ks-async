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

template <class T> class ks_future;
template <class T> class ks_promise;


class ks_future_util { //as namespace
private:
	ks_future_util() = delete;
	_DISABLE_COPY_CONSTRUCTOR(ks_future_util);

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
	static ks_future<void> resolved() {
		return ks_future<void>::resolved();
	}

	template <class T>
	static ks_future<T> rejected(const ks_error& error) {
		return ks_future<T>::rejected(error);
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
					return ks_raw_value::of(std::move(typed_value_tuple));
				}, make_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](auto&) {
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
					return ks_raw_value::of(nothing);
				}, make_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](auto&) {
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
			raw_future->on_success([raw_arg_futures = std::move(raw_arg_futures)](auto&) {
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
					return ks_raw_value::of(std::move(typed_value_vector));
				}, make_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](auto&) {
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
					return ks_raw_value::of(nothing);
				}, make_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](auto&) {
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
			raw_future->on_success([raw_arg_futures = std::move(raw_arg_futures)](auto&) {
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

public: //periodic, repetitive
	static ks_future<void> periodic(
		ks_apartment* apartment, std::function<ks_future<void>()>&& fn, 
		int64_t interval, int64_t first_delay = 0,
		const ks_async_context& context = {}) {

		ASSERT(apartment != nullptr);
		if (apartment == nullptr)
			apartment = ks_apartment::default_mta();

		std::shared_ptr<__periodic_data_t> data = std::make_shared<__periodic_data_t>();
		data->apartment = apartment;
		data->fn = std::move(fn);
		data->interval = interval;
		data->first_delay = first_delay;
		data->context = context;
		data->create_time = std::chrono::steady_clock::now();
		data->raw_final_promise_void = ks_raw_promise::create(apartment);

		data->raw_final_promise_void->get_future()->on_failure(
			//注：支持在final_future上调用try_cancel
			[data_weak = std::weak_ptr<__periodic_data_t>(data)](const ks_error& error) {
				if (error.get_code() == ks_error::CANCELLED_ERROR_CODE) {
					auto data = data_weak.lock();
					if (data != nullptr)
						data->cancel_ctrl = true;
				}
			},
			make_async_context().set_priority(0x10000), apartment);

		__schedule_periodic_once(data, first_delay);

		return ks_future<void>::__from_raw(data->raw_final_promise_void->get_future());
	}

	template <class V>
	static ks_future<void> repetitive(
		ks_apartment* produce_apartment, std::function<ks_future<V>()>&& produce_fn,
		ks_apartment* consume_apartment, std::function<ks_future<void>(const V&)>&& consume_fn,
		const ks_async_context& context = {}) {

		ASSERT(produce_apartment != nullptr);
		ASSERT(consume_apartment != nullptr);
		if (produce_apartment == nullptr)
			produce_apartment = ks_apartment::default_mta();
		if (consume_apartment == nullptr)
			consume_apartment = ks_apartment::default_mta();

		std::shared_ptr<__repetitive_data_t<V>> data = std::make_shared<__repetitive_data_t<V>>();
		data->produce_apartment = produce_apartment;
		data->consume_apartment = consume_apartment;
		data->produce_fn = std::move(produce_fn);
		data->consume_fn = std::move(consume_fn);
		data->context = context;
		data->create_time = std::chrono::steady_clock::now();
		data->raw_final_promise_void = ks_raw_promise::create(consume_apartment);

		data->raw_final_promise_void->get_future()->on_failure(
			//注：支持在final_future上调用try_cancel
			[data_weak = std::weak_ptr<__repetitive_data_t<V>>(data)](const ks_error& error) {
				if (error.get_code() == ks_error::CANCELLED_ERROR_CODE) {
					auto data = data_weak.lock();
					if (data != nullptr)
						data->cancel_ctrl = true;
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
		int64_t interval;
		int64_t first_delay;
		ks_async_context context;

		std::chrono::steady_clock::time_point create_time{};
		uint64_t rounds = 0;
		volatile bool cancel_ctrl = false;
		ks_raw_promise_ptr raw_final_promise_void = nullptr;
	};

	static void __schedule_periodic_once(const std::shared_ptr<__periodic_data_t>& data, int64_t delay) {
		ks_future_util
			::post_delayed<void>(
				data->apartment,
				[data]() -> ks_result<void> {
					if (data->cancel_ctrl)
						return ks_error::cancelled_error();
					else
						return nothing;
				},
				delay, data->context)
			.on_completion(
				data->apartment,
				[data](const ks_result<void>& schedule_result) {
					__on_trigger_periodic_once(data, schedule_result);
				},
				make_async_context().set_priority(0x10000));
	}

	static void __on_trigger_periodic_once(const std::shared_ptr<__periodic_data_t>& data, const ks_result<void>& schedule_result) {
		if (!schedule_result.is_value()) {
			data->raw_final_promise_void->try_complete(__last_error_to_raw_result_void(schedule_result.to_error()));
			return;
		}
		ks_future_util
			::post<void>(data->apartment, data->fn);

		ks_future_util
			::post<void>(data->apartment, data->fn)
			.on_success(
				data->apartment, 
				[data]() {
					data->rounds++;
					const std::chrono::steady_clock::time_point now_time = std::chrono::steady_clock::now();
					const std::chrono::steady_clock::time_point next_time = data->create_time + std::chrono::milliseconds(data->first_delay + data->interval * data->rounds);
					const int64_t next_delay = std::chrono::duration_cast<std::chrono::milliseconds>(next_time - now_time).count();
					__schedule_periodic_once(data, next_delay);
				},
				make_async_context().set_priority(0x10000))
			.on_failure(
				data->apartment,
				[data](const ks_error& error) {
					data->raw_final_promise_void->try_complete(__last_error_to_raw_result_void(error));
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
		volatile bool cancel_ctrl = false;
		ks_raw_promise_ptr raw_final_promise_void = nullptr;
	};

	template <class V>
	static void __pump_repetitive_once(const std::shared_ptr< __repetitive_data_t<V>>& data) {
		ks_future_util
			::post<V>(
				data->produce_apartment,
				[data]() -> ks_future<V> {
					if (data->cancel_ctrl)
						return ks_future<V>::rejected(ks_error::cancelled_error());
					ks_future<V> fut = data->produce_fn();
					ASSERT(fut.is_valid());
					if (!fut.is_valid()) 
						fut = ks_future<V>::rejected(ks_error::unexpected_error());
					return fut;
				},
				data->context)
			.template flat_then<void>(
				data->consume_apartment,
				[data](const V& value) -> ks_future<void> {
					if (data->cancel_ctrl)
						return ks_future<void>::rejected(ks_error::cancelled_error());
					ks_future<void> fut = data->consume_fn(value);
					ASSERT(fut.is_valid());
					if (!fut.is_valid())
						fut = ks_future<void>::rejected(ks_error::unexpected_error());
					return fut;
				},
				data->context)
			.on_completion(
				data->consume_apartment,
				[data](const ks_result<void>& result) {
					if (result.is_value()) {
						data->rounds++;
						__pump_repetitive_once<V>(data);
					}
					else {
						ks_error error = result.to_error();
						if (error.get_code() == 0 || error.get_code() == ks_error::EOF_ERROR_CODE)
							data->raw_final_promise_void->resolve(ks_raw_value::of(nothing));
						else
							data->raw_final_promise_void->reject(error);
					}
				},
				make_async_context().set_priority(0x10000));
	}

	static ks_raw_result __last_error_to_raw_result_void(const ks_error& error) {
		if (error.get_code() == 0 || error.get_code() == ks_error::EOF_ERROR_CODE)
			return ks_raw_value::of(nothing);
		else
			return error;
	}
};
