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

template <class T, class... ARGs>
class _DECL_DEPRECATED ks_async_task final {
public:
	ks_async_task() = default;
	ks_async_task(ks_async_task&&) noexcept = default;
	ks_async_task& operator=(ks_async_task&&) noexcept = default;
	_DISABLE_COPY_CONSTRUCTOR(ks_async_task);

	explicit ks_async_task(ks_apartment* apartment, const ks_async_context& context, std::function<T(const ARGs&...)>&& fn) {
		this->do_init(apartment, context, std::move(fn));
	}

	template <class FN = std::function<ks_result<T>(const ARGs&...)>, class _ = std::enable_if_t<std::is_same_v<std::invoke_result_t<FN, const ARGs&...>, ks_result<T>>>>
	explicit ks_async_task(ks_apartment* apartment, const ks_async_context& context, FN&& fn) {
		static_assert(!std::is_lvalue_reference_v<FN>, "FN must be rvalue-ref type");
		this->do_init_ex(apartment, context, std::function<ks_result<T>(const ARGs&...)>(std::move(fn)));
	}

	using arg_type = std::tuple<ARGs...>;
	using value_type = T;
	using this_async_task_type = ks_async_task<T, ARGs...>;

private:
	void do_init(ks_apartment* apartment, const ks_async_context& context, std::function<T(const ARGs&...)>&& fn) {
		m_arg_promise = ks_promise<std::tuple<ARGs...>>::create();
		m_result_promise = ks_promise<T>::create();
		m_arg_promise.get_future()
			.template then<T>(apartment, context, do_wrap_fn(std::move(fn), std::index_sequence_for<ARGs...>()))
			.deliver_to_promise(m_result_promise);
	}

	void do_init_ex(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>(const ARGs&...)>&& fn) {
		m_arg_promise = ks_promise<std::tuple<ARGs...>>::create();
		m_result_promise = ks_promise<T>::create();
		m_arg_promise.get_future()
			.template then<T>(apartment, context, do_wrap_fn_ex(std::move(fn), std::index_sequence_for<ARGs...>()))
			.deliver_to_promise(m_result_promise);
	}

public:
	template <class... TASKs, class _ = std::enable_if_t<std::is_same_v<std::tuple<typename TASKs::value_type...>, std::tuple<ARGs...>>>>
	const this_async_task_type& connect(const TASKs&... prev_tasks) const {
		static_assert(std::is_same_v<std::tuple<typename TASKs::value_type...>, std::tuple<ARGs...>>, "prev_tasks::value_type... must be same to ARGs...");
		this->do_connect(std::index_sequence_for<ARGs...>(), prev_tasks...);
		return *this;
	}

private:
	template <class... TASKs, size_t... IDXs>
	void do_connect(std::index_sequence<IDXs...>, const TASKs&... prev_tasks) const {
		ASSERT(this->is_valid());
		ks_future_util::all(to_prev_future<IDXs>(prev_tasks)...).on_completion(
			ks_apartment::default_mta(), ks_async_context().set_priority(0x10000),
			[arg_promise = m_arg_promise](auto& result) { arg_promise.try_complete(result); });
	}

	template <size_t IDX>
	using arg_elem_t = std::variadic_element_t<IDX, ARGs...>;

	//template <size_t IDX, class... Xs>
	//static ks_future<arg_elem_t<IDX>> to_prev_future(const ks_async_task<arg_elem_t<IDX>, Xs...>& arg_task) {
	template <size_t IDX, class TASK>
	static ks_future<arg_elem_t<IDX>> to_prev_future(const TASK& arg_task) {
	return arg_task.get_future();
	}

	template <size_t IDX>
	static ks_future<arg_elem_t<IDX>> to_prev_future(const ks_promise<arg_elem_t<IDX>>& arg_promise) {
		return arg_promise.get_future();
	}

	template <size_t IDX>
	static ks_future<arg_elem_t<IDX>> to_prev_future(const ks_future<arg_elem_t<IDX>>& arg_future) {
		return arg_future;
	}

	template <size_t IDX>
	static ks_future<arg_elem_t<IDX>> to_prev_future(const arg_elem_t<IDX>& arg) {
		return ks_future<arg_elem_t<IDX>>::resolved(arg);
	}

public:
	bool is_valid() const {
		return m_result_promise.is_valid();
	}

	ks_future<T> get_future() const {
		ASSERT(this->is_valid());
		return m_result_promise.get_future();
	}

private:
	template <size_t... IDXs>
	static std::function<T(const std::tuple<ARGs... > &)> do_wrap_fn(std::function<T(const ARGs &...)>&& fn, std::index_sequence<IDXs...>) {
		return[fn = std::move(fn)](const std::tuple<ARGs...>& argTuple) -> T {
			return fn(std::get<IDXs>(argTuple)...);
		};
	}

	template <size_t... IDXs>
	static std::function< ks_result<T>(const std::tuple<ARGs...> &)> do_wrap_fn_ex(std::function<ks_result<T>(const ARGs &...)>&& fn, std::index_sequence<IDXs...>) {
		return[fn = std::move(fn)](const std::tuple<ARGs...>& argTuple) ->ks_result<T> {
			return fn(std::get<IDXs>(argTuple)...);
		};
	}

private:
	ks_promise<std::tuple<ARGs...>> m_arg_promise = nullptr;
	ks_promise<T> m_result_promise = nullptr;
};
