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


class ks_future_util {
private:
	ks_future_util() = delete;
	_DISABLE_COPY_CONSTRUCTOR(ks_future_util);

	using ks_raw_future = __ks_async_raw::ks_raw_future;
	using ks_raw_future_ptr = __ks_async_raw::ks_raw_future_ptr;

	using ks_raw_result = __ks_async_raw::ks_raw_result;
	using ks_raw_value = __ks_async_raw::ks_raw_value;

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
				}, ks_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](auto&) {
				for (auto& rawf : raw_arg_futures)
					rawf->try_cancel(true);
			}, ks_async_context().set_priority(0x10000), nullptr);
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
				}, ks_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](auto&) {
				for (auto& rawf : raw_arg_futures)
					rawf->try_cancel(true);
			}, ks_async_context().set_priority(0x10000), nullptr);
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
			}, ks_async_context().set_priority(0x10000), nullptr);
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
				}, ks_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](auto&) {
				for (auto& rawf : raw_arg_futures)
					rawf->try_cancel(true);
			}, ks_async_context().set_priority(0x10000), nullptr);
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
				}, ks_async_context().set_priority(0x10000), nullptr);
		//for all(), when error, auto cancel other not-completed futures
		if (true) {
			raw_future->on_failure([raw_arg_futures = std::move(raw_arg_futures)](auto&) {
				for (auto& rawf : raw_arg_futures)
					rawf->try_cancel(true);
			}, ks_async_context().set_priority(0x10000), nullptr);
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
			}, ks_async_context().set_priority(0x10000), nullptr);
		}
		return ks_future<T>::__from_raw(raw_future);
	}

private:
	template <class... Ts, size_t... IDXs>
	static std::tuple<Ts...> __convert_raw_value_vector_to_typed_value_tuple(const std::vector<ks_raw_value>& value_vec, std::index_sequence<IDXs...>) {
		return std::tuple<Ts...>(value_vec.at(IDXs).get<Ts>()...);
	}

};
