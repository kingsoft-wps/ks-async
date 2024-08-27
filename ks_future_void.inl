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

template <>
class ks_future<void> final {
public:
	ks_future(nullptr_t) : m_nothing_future(nullptr) {}
	ks_future(const ks_future&) = default;
	ks_future& operator=(const ks_future&) = default;
	ks_future(ks_future&&) noexcept = default;
	ks_future& operator=(ks_future&&) noexcept = default;

	using value_type = void;
	using this_future_type = ks_future<void>;

public:
	static ks_future<void> resolved(nothing_t _ = nothing) {
		return ks_future<nothing_t>::resolved(nothing).template cast<void>();
	}

	static ks_future<void> rejected(const ks_error& error) {
		return ks_future<nothing_t>::rejected(error).template cast<void>();
	}

public:
	static ks_future<void> post(ks_apartment* apartment, const ks_async_context& context, std::function<void()>&& task_fn) {
		return ks_future<nothing_t>::post(apartment, context, __wrap_task_fn(std::move(task_fn))).template cast<void>();
	}
	static ks_future<void> post(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<void>()>&& task_fn) {
		return ks_future<nothing_t>::post(apartment, context, __wrap_task_fn(std::move(task_fn))).template cast<void>();
	}
	static ks_future<void> post(ks_apartment* apartment, const ks_async_context& context, std::function<void(ks_cancel_inspector*)>&& task_fn) {
		return ks_future<nothing_t>::post(apartment, context, __wrap_task_fn(std::move(task_fn))).template cast<void>();
	}
	static ks_future<void> post(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<void>(ks_cancel_inspector*)>&& task_fn) {
		return ks_future<nothing_t>::post(apartment, context, __wrap_task_fn(std::move(task_fn))).template cast<void>();
	}
	template <class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<void()>> || std::is_convertible_v<FN, std::function<ks_result<void>()>>) ||
		(std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>>)>>
	static ks_future<void> post(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn) {
		return ks_future<nothing_t>::post(apartment, context, __wrap_task_fn(std::forward<FN>(task_fn))).template cast<void>();
	}

	static ks_future<void> post_delayed(ks_apartment* apartment, const ks_async_context& context, std::function<void()>&& task_fn, int64_t delay) {
		return ks_future<nothing_t>::post_delayed(apartment, context, __wrap_task_fn(std::move(task_fn)), delay).template cast<void>();
	}
	static ks_future<void> post_delayed(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<void>()>&& task_fn, int64_t delay) {
		return ks_future<nothing_t>::post_delayed(apartment, context, __wrap_task_fn(std::move(task_fn)), delay).template cast<void>();
	}
	static ks_future<void> post_delayed(ks_apartment* apartment, const ks_async_context& context, std::function<void(ks_cancel_inspector*)>&& task_fn, int64_t delay) {
		return ks_future<nothing_t>::post_delayed(apartment, context, __wrap_task_fn(std::move(task_fn)), delay).template cast<void>();
	}
	static ks_future<void> post_delayed(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<void>(ks_cancel_inspector*)>&& task_fn, int64_t delay) {
		return ks_future<nothing_t>::post_delayed(apartment, context, __wrap_task_fn(std::move(task_fn)), delay).template cast<void>();
	}
	template <class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<void()>> || std::is_convertible_v<FN, std::function<ks_result<void>()>>) ||
		(std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>>)>>
	static ks_future<void> post_delayed(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay) {
		return ks_future<nothing_t>::post_delayed(apartment, context, __wrap_task_fn(std::forward<FN>(task_fn)), delay).template cast<void>();
	}

	static ks_future<void> post_pending(ks_apartment* apartment, const ks_async_context& context, std::function<void()>&& task_fn, ks_pending_trigger* trigger) {
		return ks_future<nothing_t>::post_pending(apartment, context, __wrap_task_fn(std::move(task_fn)), trigger).template cast<void>();
	}
	static ks_future<void> post_pending(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<void>()>&& task_fn, ks_pending_trigger* trigger) {
		return ks_future<nothing_t>::post_pending(apartment, context, __wrap_task_fn(std::move(task_fn)), trigger).template cast<void>();
	}
	static ks_future<void> post_pending(ks_apartment* apartment, const ks_async_context& context, std::function<void(ks_cancel_inspector*)>&& task_fn, ks_pending_trigger* trigger) {
		return ks_future<nothing_t>::post_pending(apartment, context, __wrap_task_fn(std::move(task_fn)), trigger).template cast<void>();
	}
	static ks_future<void> post_pending(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<void>(ks_cancel_inspector*)>&& task_fn, ks_pending_trigger* trigger) {
		return ks_future<nothing_t>::post_pending(apartment, context, __wrap_task_fn(std::move(task_fn)), trigger).template cast<void>();
	}
	template <class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<void()>> || std::is_convertible_v<FN, std::function<ks_result<void>()>>) ||
		(std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>>)>>
	static ks_future<void> post(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger) {
		return ks_future<nothing_t>::post_pending(apartment, context, __wrap_task_fn(std::forward<FN>(task_fn)), trigger).template cast<void>();
	}

public:
	template <class R>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, std::function<R()>&& fn) const {
		return m_nothing_future.then<R>(apartment, context, __wrap_then_fn<R>(std::move(fn)));
	}
	template <class R>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>()>&& fn) const {
		return m_nothing_future.then<R>(apartment, context, __wrap_then_fn<R>(std::move(fn)));
	}
	template <class R>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, std::function<R(ks_cancel_inspector*)>&& fn) const {
		return m_nothing_future.then<R>(apartment, context, __wrap_then_fn<R>(std::move(fn)));
	}
	template <class R>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(ks_cancel_inspector*)>&& fn) const {
		return m_nothing_future.then<R>(apartment, context, __wrap_then_fn<R>(std::move(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<R()>> || std::is_convertible_v<FN, std::function<ks_result<R>()>>) ||
		(std::is_convertible_v<FN, std::function<R(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(ks_cancel_inspector*)>>)>>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const {
		return m_nothing_future.then<R>(apartment, context, __wrap_then_fn<R>(std::forward<FN>(fn)));
	}

	template <class R>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, std::function<R(const ks_result<void>&)>&& fn) const {
		return m_nothing_future.transform<R>(apartment, context, __wrap_transform_fn<R>(std::move(fn)));
	}
	template <class R>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const ks_result<void>&)>&& fn) const {
		return m_nothing_future.transform<R>(apartment, context, __wrap_transform_fn<R>(std::move(fn)));
	}
	template <class R>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, std::function<R(const ks_result<void>&, ks_cancel_inspector*)>&& fn) const {
		return m_nothing_future.transform<R>(apartment, context, __wrap_transform_fn<R>(std::move(fn)));
	}
	template <class R>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, std::function<ks_result<R>(const ks_result<void>&, ks_cancel_inspector*)>&& fn) const {
		return m_nothing_future.transform<R>(apartment, context, __wrap_transform_fn<R>(std::move(fn)));
	}
	template <class R, class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<R(const ks_result<void>&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&)>>) ||
		(std::is_convertible_v<FN, std::function<R(const ks_result<void>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&, ks_cancel_inspector*)>>)>>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const {
		return m_nothing_future.transform<R>(apartment, context, __wrap_transform_fn<R>(std::forward<FN>(fn)));
	}

public:
	template <class R>
	ks_future<R> flat_then(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>()>&& fn) const {
		return m_nothing_future.flat_then<R>(
			apartment, context,
			[fn = std::move(fn)](const nothing_t& result)->ks_future<R> { return fn(ks_result<void>::__from_other(result)); });
	}
	template <class R>
	ks_future<R> flat_then(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>(ks_cancel_inspector*)>&& fn) const {
		return m_nothing_future.flat_then<R>(
			apartment, context,
			[fn = std::move(fn)](const nothing_t& result)->ks_future<R> { return fn(ks_result<void>::__from_other(result), ks_cancel_inspector::__for_future()); });
	}

	template <class R>
	ks_future<R> flat_transform(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>(const ks_result<void>&)>&& fn) const {
		return m_nothing_future.flat_transform<R>(
			apartment, context,
			[fn = std::move(fn)](const ks_result<nothing_t>& result)->ks_future<R> { return fn(ks_result<void>::__from_other(result)); });
	}
	template <class R>
	ks_future<R> flat_transform(ks_apartment* apartment, const ks_async_context& context, std::function<ks_future<R>(const ks_result<void>&, ks_cancel_inspector*)>&& fn) const {
		return m_nothing_future.flat_transform<R>(
			apartment, context,
			[fn = std::move(fn)](const ks_result<nothing_t>& result)->ks_future<R> { return fn(ks_result<void>::__from_other(result), ks_cancel_inspector::__for_future()); });
	}

public:
	ks_future<void> on_success(ks_apartment* apartment, const ks_async_context& context, std::function<void()>&& fn) const {
		return m_nothing_future.on_success(
			apartment, context,
			[fn = std::move(fn)](nothing_t) -> void { fn(); }
		); 
	}

	ks_future<void> on_failure(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_error&)>&& fn) const {
		return m_nothing_future.on_failure(
			apartment, context,
			[fn = std::move(fn)](const ks_error& error) -> void { fn(error); }
		);
	}

	ks_future<void> on_completion(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_result<void>&)>&& fn) const {
		return m_nothing_future.on_completion(
			apartment, context,
			[fn = std::move(fn)](const ks_result<nothing_t>& result) -> void { fn(ks_result<void>::__from_other(result)); }
		); 
	}

public:
	template <class R>
	ks_future<R> cast() const {
		return m_nothing_future.cast<R>();
	}

	template <class X /*= void*/, class _ = std::enable_if_t<std::is_void_v<X>>>
	const this_future_type& deliver_to_promise(const ks_promise<X>& promise) const {
		m_nothing_future.deliver_to_promise(promise.m_nothing_promise);
		return *this;
	}

	const this_future_type& set_timeout(int64_t timeout) const {
		m_nothing_future.set_timeout(timeout);
		return *this;
	}

public:
	bool is_valid() const {
		return m_nothing_future.is_valid();
	}

	bool is_completed() const {
		return m_nothing_future.is_completed();
	}

	ks_result<void> peek_result() const {
		return ks_result<void>::__from_other(m_nothing_future.peek_result());
	}

	//慎用，使用不当可能会造成死锁或卡顿！
	template <class _ = void>
	_DECL_DEPRECATED bool wait() const {
		return m_nothing_future.wait();
	}

	void try_cancel() const {
		m_nothing_future.try_cancel();
	}

private:
	template <class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<void()>> || std::is_convertible_v<FN, std::function<ks_result<void>()>>) ||
		(std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>>)>>
	static std::function<ks_result<nothing_t>()> __wrap_task_fn(FN&& task_fn) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<void()>> || std::is_convertible_v<FN, std::function<ks_result<void>()>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal task-fn's arglist");
		return ks_future<void>::__wrap_task_fn_by_arglist(std::forward<FN>(task_fn), std::integral_constant<int, arglist_mode>());
	}

	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<void()>> || std::is_convertible_v<FN, std::function<ks_result<void>()>>>>
	static std::function<ks_result<nothing_t>()> __wrap_task_fn_by_arglist(FN&& task_fn, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN>> ? -1 :
			std::is_same_v<ks_result<void>, std::invoke_result_t<FN>> ? 2 : 0;
		static_assert(ret_mode != 0, "illegal task-fn's ret");
		return ks_future<void>::__wrap_task_fn_by_arglist_ret(std::forward<FN>(task_fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>>>>
	static std::function<ks_result<nothing_t>()> __wrap_task_fn_by_arglist(FN&& task_fn, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, ks_cancel_inspector*>> ? -1 :
			std::is_same_v<ks_result<void>, std::invoke_result_t<FN, ks_cancel_inspector*>> ? 2 : 0;
		static_assert(ret_mode != 0, "illegal task-fn's ret");
		return ks_future<void>::__wrap_task_fn_by_arglist_ret(std::forward<FN>(task_fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<void()>>>>
	static std::function<ks_result<nothing_t>()> __wrap_task_fn_by_arglist_ret(FN&& task_fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) {
		return[task_fn = std::move(task_fn)]()->ks_result<nothing_t> {
			task_fn();
			return ks_result<nothing_t>(nothing);
		};
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<void>()>>>>
	static std::function<ks_result<nothing_t>()> __wrap_task_fn_by_arglist_ret(FN&& task_fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return[task_fn = std::move(task_fn)]()->ks_result<nothing_t> {
			return task_fn().template cast<nothing_t>();
		};
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>>>>
	static std::function<ks_result<nothing_t>()> __wrap_task_fn_by_arglist_ret(FN&& task_fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) {
		return[task_fn = std::move(task_fn)]()->ks_result<nothing_t> {
			task_fn(ks_cancel_inspector::__for_future());
			return ks_result<nothing_t>(nothing);
		};
	}
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>>>>
	static std::function<ks_result<nothing_t>()> __wrap_task_fn_by_arglist_ret(FN&& task_fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return[task_fn = std::move(task_fn)]()->ks_result<nothing_t> {
			return task_fn(ks_cancel_inspector::__for_future()).template cast<nothing_t>();
		};
	}

private:
	template <class R, class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<R()>> || std::is_convertible_v<FN, std::function<ks_result<R>()>>) ||
		(std::is_convertible_v<FN, std::function<R(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(ks_cancel_inspector*)>>)>>
	static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn(FN&& fn) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<R(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<R()>> || std::is_convertible_v<FN, std::function<ks_result<R>()>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal then-fn's arglist");
		return ks_future<void>::__wrap_then_fn_by_arglist<R>(std::forward<FN>(fn), std::integral_constant<int, arglist_mode>());
	}

	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<R()>> || std::is_convertible_v<FN, std::function<ks_result<R>()>>>>
	static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist(FN&& fn, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN>> ? -1 :
			std::is_same_v<ks_result<R>, std::invoke_result_t<FN>> ? 2 :
			std::is_convertible_v<R, std::invoke_result_t<FN>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal then-fn's ret");
		return ks_future<void>::__wrap_then_fn_by_arglist_ret<R>(std::forward<FN>(fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<R(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(ks_cancel_inspector*)>>>>
	static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist(FN&& fn, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, ks_cancel_inspector*>> ? -1 :
			std::is_same_v<ks_result<R>, std::invoke_result_t<FN, ks_cancel_inspector*>> ? 2 :
			std::is_convertible_v<R, std::invoke_result_t<FN, ks_cancel_inspector*>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal then-fn's ret");
		return ks_future<void>::__wrap_then_fn_by_arglist_ret<R>(std::forward<FN>(fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class R, class FN, class _ = std::enable_if_t<std::is_void_v<R> && std::is_convertible_v<FN, std::function<void()>>>>
	static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) {
		return[fn = std::forward<FN>(fn)](const nothing_t&)->ks_result<void> {
			fn();
			return ks_result<void>(nothing);
		};
	}
	template <class R, class FN, class _ = std::enable_if_t<!std::is_void_v<R> && std::is_convertible_v<FN, std::function<R()>>>>
	static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return[fn = std::forward<FN>(fn)](const nothing_t&)->ks_result<R> {
			return ks_result<R>(fn());
		};
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<R>()>>>>
	static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return[fn = std::forward<FN>(fn)](const nothing_t&)->ks_result<R> {
			return fn();
		};
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_void_v<R>&& std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>>>>
	static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) {
		return[fn = std::forward<FN>(fn)](const nothing_t&)->ks_result<void> {
			fn(ks_cancel_inspector::__for_future());
			return ks_result<void>(nothing);
		};
	}
	template <class R, class FN, class _ = std::enable_if_t<!std::is_void_v<R>&& std::is_convertible_v<FN, std::function<R(ks_cancel_inspector*)>>>>
	static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return[fn = std::forward<FN>(fn)](const nothing_t&)->ks_result<R> {
			return ks_result<R>(fn(ks_cancel_inspector::__for_future()));
		};
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<R>(ks_cancel_inspector*)>>>>
	static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return[fn = std::forward<FN>(fn)](const nothing_t&)->ks_result<R> {
			return fn(ks_cancel_inspector::__for_future());
		};
	}

	template <class R, class FN, class _ = std::enable_if_t<
		(std::is_convertible_v<FN, std::function<R(const ks_result<void>&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&)>>) ||
		(std::is_convertible_v<FN, std::function<R(const ks_result<void>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&, ks_cancel_inspector*)>>)>>
	static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn(FN&& fn) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<R(const ks_result<void>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&, ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<R(const ks_result<void>&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&)>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal transform-fn's arglist");
		return ks_future<void>::__wrap_transform_fn_by_arglist<R>(std::forward<FN>(fn), std::integral_constant<int, arglist_mode>());
	}

	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<R(const ks_result<void>&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&)>>>>
	static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist(FN&& fn, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const ks_result<void>&>> ? -1 :
			std::is_same_v<ks_result<R>, std::invoke_result_t<FN, const ks_result<void>&>> ? 2 :
			std::is_convertible_v<R, std::invoke_result_t<FN, const ks_result<void>&>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal transform-fn's ret");
		return ks_future<void>::__wrap_transform_fn_by_arglist_ret<R>(std::forward<FN>(fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<R(const ks_result<void>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&, ks_cancel_inspector*)>>>>
	static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist(FN&& fn, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const ks_result<void>&, ks_cancel_inspector*>> ? -1 :
			std::is_same_v<ks_result<R>, std::invoke_result_t<FN, const ks_result<void>&, ks_cancel_inspector*>> ? 2 :
			std::is_convertible_v<R, std::invoke_result_t<FN, const ks_result<void>&, ks_cancel_inspector*>> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal transform-fn's ret");
		return ks_future<void>::__wrap_transform_fn_by_arglist_ret<R>(std::forward<FN>(fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class R, class FN, class _ = std::enable_if_t<std::is_void_v<R>&& std::is_convertible_v<FN, std::function<void(const ks_result<void>&)>>>>
	static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) {
		return[fn = std::forward<FN>(fn)](const ks_result<nothing_t>& arg)->ks_result<void> {
			fn(arg.cast<void>());
			return ks_result<void>(nothing);
		};
	}
	template <class R, class FN, class _ = std::enable_if_t<!std::is_void_v<R>&& std::is_convertible_v<FN, std::function<R(const ks_result<void>&)>>>>
	static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return[fn = std::forward<FN>(fn)](const ks_result<nothing_t>& arg)->ks_result<R> {
			return ks_result<R>(fn(arg.cast<void>()));
		};
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&)>>>>
	static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return[fn = std::forward<FN>(fn)](const ks_result<nothing_t>& arg)->ks_result<R> {
			return fn(arg.cast<void>());
		};
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_void_v<R>&& std::is_convertible_v<FN, std::function<void(const ks_result<void>&, ks_cancel_inspector*)>>>>
	static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) {
		return[fn = std::forward<FN>(fn)](const ks_result<nothing_t>& arg)->ks_result<void> {
			fn(arg.cast<void>(), ks_cancel_inspector::__for_future());
			return ks_result<void>(nothing);
		};
	}
	template <class R, class FN, class _ = std::enable_if_t<!std::is_void_v<R>&& std::is_convertible_v<FN, std::function<R(const ks_result<void>&, ks_cancel_inspector*)>>>>
	static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return[fn = std::forward<FN>(fn)](const ks_result<nothing_t>& arg)->ks_result<R> {
			return ks_result<R>(fn(arg.cast<void>(), ks_cancel_inspector::__for_future()));
		};
	}
	template <class R, class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&, ks_cancel_inspector*)>>>>
	static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(FN&& fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return[fn = std::forward<FN>(fn)](const ks_result<nothing_t>& arg)->ks_result<R> {
			return fn(arg.cast<void>(), ks_cancel_inspector::__for_future());
		};
	}

private:
	using ks_raw_future = __ks_async_raw::ks_raw_future;
	using ks_raw_future_ptr = __ks_async_raw::ks_raw_future_ptr;

	using ks_raw_result = __ks_async_raw::ks_raw_result;
	using ks_raw_value = __ks_async_raw::ks_raw_value;

	ks_future(ks_future<nothing_t>&& nothing_future) noexcept : m_nothing_future(std::move(nothing_future)) {}

	static ks_future<void> __from_raw(const ks_raw_future_ptr& raw_future) { return ks_future<nothing_t>::__from_raw(raw_future); }
	static ks_future<void> __from_raw(ks_raw_future_ptr&& raw_future) { return ks_future<nothing_t>::__from_raw(std::move(raw_future)); }
	const ks_raw_future_ptr& __get_raw() const { return m_nothing_future.__get_raw(); }

	template <class T2> friend class ks_future;
	template <class T2> friend class ks_promise;
	friend class ks_future_util;

private:
	ks_future<nothing_t> m_nothing_future;
};
