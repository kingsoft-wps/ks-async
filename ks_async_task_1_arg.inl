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

template <class T, class ARG>
class _DECL_DEPRECATED ks_async_task<T, ARG> final {
public:
	ks_async_task() = default;
	ks_async_task(ks_async_task&&) noexcept = default;
	ks_async_task& operator=(ks_async_task&&) noexcept = default;
	_DISABLE_COPY_CONSTRUCTOR(ks_async_task);

	explicit ks_async_task(ks_apartment* apartment, const ks_async_context& context, std::function<T(const ARG&)>&& fn) {
		this->do_init(apartment, context, std::move(fn));
	}

	template <class FN = std::function<ks_result<T>(const ARG&)>, class _ = std::enable_if_t<std::is_same_v<std::invoke_result_t<FN, const ARG&>, ks_result<T>>>>
	explicit ks_async_task(ks_apartment* apartment, const ks_async_context& context, FN&& fn) {
		static_assert(!std::is_lvalue_reference_v<FN>, "FN must be rvalue-ref type");
		this->do_init_ex(apartment, context, std::function<ks_result<T>(const ARG&)>(std::move(fn)));
	}

	using arg_type = ARG;
	using value_type = T;
	using this_async_task_type = ks_async_task<T, ARG>;

private:
	void do_init(ks_apartment* apartment, const ks_async_context& context, std::function<T(const ARG&)>&& fn) {
		m_arg_promise = ks_promise<ARG>::create();
		m_result_promise = ks_promise<T>::create();
		m_arg_promise.get_future()
			.template then<T>(apartment, context, std::move(fn))
			.deliver_to_promise(m_result_promise);
	}

	void do_init_ex(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>(const ARG&)>&& fn) {
		m_arg_promise = ks_promise<ARG>::create();
		m_result_promise = ks_promise<T>::create();
		m_arg_promise.get_future()
			.template then<T>(apartment, context, std::move(fn))
			.deliver_to_promise(m_result_promise);
	}

public:
	template <class... Xs>
	const this_async_task_type& connect(const ks_async_task<ARG, Xs...>& prev_task) const {
		return this->connect(prev_task.get_future());
	}

	const this_async_task_type& connect(const ks_promise<ARG>& prev_promise) const {
		return this->connect(prev_promise.get_future());
	}

	const this_async_task_type& connect(const ks_future<ARG>& prev_future) const {
		ASSERT(this->is_valid());
		prev_future.on_completion(
			ks_apartment::default_mta(), ks_async_context().set_priority(0x10000),
			[arg_promise = m_arg_promise](auto& result) { arg_promise.try_complete(result); });
		return *this;
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
	ks_promise<ARG> m_arg_promise = nullptr;
	ks_promise<T> m_result_promise = nullptr;
};
