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

template <class T>
class _DECL_DEPRECATED ks_async_task<T> final {
public:
	ks_async_task() = default;
	ks_async_task(ks_async_task&&) noexcept = default;
	ks_async_task& operator=(ks_async_task&&) noexcept = default;
	_DISABLE_COPY_CONSTRUCTOR(ks_async_task);

	explicit ks_async_task(const T& value) {
		this->do_init(value);
	}

	explicit ks_async_task(const ks_future<T>& future) {
		this->do_init(future);
	}

	explicit ks_async_task(ks_apartment* apartment, const ks_async_context& context, std::function<T()>&& fn) {
		this->do_init(apartment, context, std::move(fn));
	}
	explicit ks_async_task(ks_apartment* apartment, const ks_async_context& context, std::function<T()>&& fn, ks_pending_trigger* trigger) {
		if (trigger == nullptr)
			this->do_init(apartment, context, std::move(fn));
		else
			this->do_init(apartment, context, std::move(fn), trigger);
	}

	template <class FN = std::function<ks_result<T>()>, class _ = std::enable_if_t<std::is_same_v<std::invoke_result_t<FN>, ks_result<T>>>>
	explicit ks_async_task(ks_apartment* apartment, const ks_async_context& context, FN&& fn) {
		static_assert(!std::is_lvalue_reference_v<FN>, "FN must be rvalue-ref type");
		this->do_init_ex(apartment, context, std::function<ks_result<T>()>(std::move(fn)));
	}
	template <class FN = std::function<ks_result<T>()>, class _ = std::enable_if_t<std::is_same_v<std::invoke_result_t<FN>, ks_result<T>>>>
	explicit ks_async_task(ks_apartment* apartment, const ks_async_context& context, FN&& fn, ks_pending_trigger* trigger) {
		static_assert(!std::is_lvalue_reference_v<FN>, "FN must be rvalue-ref type");
		if (trigger == nullptr)
			this->do_init_ex(apartment, context, std::function<ks_result<T>()>(std::move(fn)));
		else
			this->do_init_ex(apartment, context, std::function<ks_result<T>()>(std::move(fn)), trigger);
	}

	//no arg_type provide!
	using value_type = T;
	using this_async_task_type = ks_async_task<T>;

private:
	void do_init(const T& value) {
		m_result_promise = ks_promise<T>::create();
		m_result_promise.resolve(value);
	}

	void do_init(const ks_future<T>& future) {
		m_result_promise = ks_promise<T>::create();
		future.deliver_to_promise(m_result_promise);
	}

	void do_init(ks_apartment* apartment, const ks_async_context& context, std::function<T()>&& fn) {
		m_result_promise = ks_promise<T>::create();
		ks_future<T>::post(apartment, context, std::move(fn)).deliver_to_promise(m_result_promise);
	}
	void do_init(ks_apartment* apartment, const ks_async_context& context, std::function<T()>&& fn, ks_pending_trigger* trigger) {
		m_result_promise = ks_promise<T>::create();
		ks_future<T>::post_pending(apartment, context, std::move(fn), trigger).deliver_to_promise(m_result_promise);
	}

	void do_init_ex(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>()>&& fn) {
		m_result_promise = ks_promise<T>::create();
		ks_future<T>::post(apartment, context, std::move(fn)).deliver_to_promise(m_result_promise);
	}
	void do_init_ex(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<T>()>&& fn, ks_pending_trigger* trigger) {
		m_result_promise = ks_promise<T>::create();
		ks_future<T>::post_pending(apartment, context, std::move(fn), trigger).deliver_to_promise(m_result_promise);
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
	ks_promise<T> m_result_promise = nullptr;
};
