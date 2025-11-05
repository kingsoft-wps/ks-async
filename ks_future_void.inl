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
	ks_future(nullptr_t) noexcept : m_nothing_future(nullptr) {}

	ks_future(const ks_future&) noexcept = default;
	ks_future(ks_future&&) noexcept = default;

	ks_future& operator=(const ks_future&) noexcept = default;
	ks_future& operator=(ks_future&&) noexcept = default;

	//让ks_future看起来像一个智能指针
	ks_future* operator->() noexcept { return this; }
	const ks_future* operator->() const noexcept { return this; }

	using value_type = void;
	using this_future_type = ks_future<void>;

public: //resolved, rejected
	static ks_future<void> resolved() {
		return ks_future<nothing_t>::resolved(nothing).template cast<void>();
	}
	static ks_future<void> resolved(nothing_t) {
		return ks_future<nothing_t>::resolved(nothing).template cast<void>();
	}

	static ks_future<void> rejected(const ks_error& error) {
		return ks_future<nothing_t>::rejected(error).template cast<void>();
	}

	static ks_future<void> __from_result(const ks_result<void>& result) {
		ks_apartment* apartment_hint = ks_apartment::current_thread_apartment_or_default_mta();
		ks_raw_future_ptr raw_future = ks_raw_future::__from_result(result.__get_raw(), apartment_hint);
		return ks_future<void>::__from_raw(raw_future);
	}

private: //__wrap_task_fn
	template <class FN>
	inline static auto __wrap_task_fn(FN&& task_fn) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_future<void>(ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<void()>> || std::is_convertible_v<FN, std::function<ks_result<void>()>> || std::is_convertible_v<FN, std::function<ks_future<void>()>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal task-fn's arglist");
		return ks_future<void>::__wrap_task_fn_by_arglist(std::forward<FN>(task_fn), std::integral_constant<int, arglist_mode>());
	}

	template <class FN>
	inline static auto __wrap_task_fn_by_arglist(FN&& task_fn, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN>, ks_future<void>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN>, ks_result<void>> ? 2 : 0;
		static_assert(ret_mode != 0, "illegal task-fn's ret");
		return ks_future<void>::__wrap_task_fn_by_arglist_ret(std::forward<FN>(task_fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class FN>
	inline static auto __wrap_task_fn_by_arglist(FN&& task_fn, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, ks_cancel_inspector*>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_future<void>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_result<void>> ? 2 : 0;
		static_assert(ret_mode != 0, "illegal task-fn's ret");
		return ks_future<void>::__wrap_task_fn_by_arglist_ret(std::forward<FN>(task_fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	_NOINLINE static std::function<ks_result<nothing_t>()> __wrap_task_fn_by_arglist_ret(std::function<void()> task_fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) {
		return[task_fn = std::move(task_fn)]()->ks_result<nothing_t> {
			task_fn();
			return ks_result<nothing_t>(nothing);
		};
	}
	_NOINLINE static std::function<ks_result<nothing_t>()> __wrap_task_fn_by_arglist_ret(std::function<ks_result<void>()> task_fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return[task_fn = std::move(task_fn)]()->ks_result<nothing_t> {
			return task_fn().template cast<nothing_t>();
		};
	}
	_NOINLINE static std::function<ks_future<nothing_t>()> __wrap_task_fn_by_arglist_ret(std::function<ks_future<void>()> task_fn, std::integral_constant<int, 1>, std::integral_constant<int, 3>) {
		return[task_fn = std::move(task_fn)]()->ks_future<nothing_t> {
			return task_fn().template cast<nothing_t>();
		};
	}
	_NOINLINE static std::function<ks_result<nothing_t>()> __wrap_task_fn_by_arglist_ret(std::function<void(ks_cancel_inspector*)> task_fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) {
		return[task_fn = std::move(task_fn)]()->ks_result<nothing_t> {
			task_fn(ks_cancel_inspector::__for_future());
			return ks_result<nothing_t>(nothing);
		};
	}
	_NOINLINE static std::function<ks_result<nothing_t>()> __wrap_task_fn_by_arglist_ret(std::function<ks_result<void>(ks_cancel_inspector*)> task_fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return[task_fn = std::move(task_fn)]()->ks_result<nothing_t> {
			return task_fn(ks_cancel_inspector::__for_future()).template cast<nothing_t>();
		};
	}
	_NOINLINE static std::function<ks_future<nothing_t>()> __wrap_task_fn_by_arglist_ret(std::function<ks_future<void>(ks_cancel_inspector*)> task_fn, std::integral_constant<int, 2>, std::integral_constant<int, 3>) {
		return[task_fn = std::move(task_fn)]()->ks_future<nothing_t> {
			return task_fn(ks_cancel_inspector::__for_future()).template cast<nothing_t>();
		};
	}

private: //__wrap_then_fn
	template <class R, class FN>
	inline static auto __wrap_then_fn(FN&& fn) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<R(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_future<R>(ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<R()>> || std::is_convertible_v<FN, std::function<ks_result<R>()>> || std::is_convertible_v<FN, std::function<ks_future<R>()>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal then-fn's arglist");
		return ks_future<void>::__wrap_then_fn_by_arglist<R>(std::forward<FN>(fn), std::integral_constant<int, arglist_mode>());
	}

	template <class R, class FN>
	inline static auto __wrap_then_fn_by_arglist(FN&& fn, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN>, ks_future<R>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN>, ks_result<R>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN>, R> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal then-fn's ret");
		return ks_future<void>::__wrap_then_fn_by_arglist_ret<R>(std::forward<FN>(fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class R, class FN>
	inline static auto __wrap_then_fn_by_arglist(FN&& fn, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, ks_cancel_inspector*>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_future<R>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, ks_result<R>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, ks_cancel_inspector*>, R> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal then-fn's ret");
		return ks_future<void>::__wrap_then_fn_by_arglist_ret<R>(std::forward<FN>(fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class R>
	_NOINLINE static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(std::function<void()> fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) {
		static_assert(std::is_void_v<R>, "R must be void");
		return[fn = std::move(fn)](const nothing_t&)->ks_result<void> {
			fn();
			return ks_result<void>(nothing);
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(std::function<R()> fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return[fn = std::move(fn)](const nothing_t&)->ks_result<R> {
			return ks_result<R>(fn());
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(std::function<ks_result<R>()> fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return[fn = std::move(fn)](const nothing_t&)->ks_result<R> {
			return fn();
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_future<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(std::function<ks_future<R>()> fn, std::integral_constant<int, 1>, std::integral_constant<int, 3>) {
		return[fn = std::move(fn)](const nothing_t&)->ks_future<R> {
			return fn();
		};
	}

	template <class R>
	_NOINLINE static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(std::function<void(ks_cancel_inspector*)> fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) {
		static_assert(std::is_void_v<R>, "R must be void");
		return[fn = std::move(fn)](const nothing_t&)->ks_result<void> {
			fn(ks_cancel_inspector::__for_future());
			return ks_result<void>(nothing);
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(std::function<R(ks_cancel_inspector*)> fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return[fn = std::move(fn)](const nothing_t&)->ks_result<R> {
			return ks_result<R>(fn(ks_cancel_inspector::__for_future()));
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_result<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(std::function<ks_result<R>(ks_cancel_inspector*)> fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return[fn = std::move(fn)](const nothing_t&)->ks_result<R> {
			return fn(ks_cancel_inspector::__for_future());
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_future<R>(const nothing_t&)> __wrap_then_fn_by_arglist_ret(std::function<ks_future<R>(ks_cancel_inspector*)> fn, std::integral_constant<int, 2>, std::integral_constant<int, 3>) {
		return[fn = std::move(fn)](const nothing_t&)->ks_future<R> {
			return fn(ks_cancel_inspector::__for_future());
		};
	}

private: //__wrap_transform_fn
	template <class R, class FN>
	inline static auto __wrap_transform_fn(FN&& fn) {
		constexpr int arglist_mode =
			(std::is_convertible_v<FN, std::function<R(const ks_result<void>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&, ks_cancel_inspector*)>> || std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<void>&, ks_cancel_inspector*)>>) ? 2 :
			(std::is_convertible_v<FN, std::function<R(const ks_result<void>&)>> || std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&)>> || std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<void>&)>>) ? 1 : 0;
		static_assert(arglist_mode != 0, "illegal transform-fn's arglist");
		return ks_future<void>::__wrap_transform_fn_by_arglist<R>(std::forward<FN>(fn), std::integral_constant<int, arglist_mode>());
	}

	template <class R, class FN>
	inline static auto __wrap_transform_fn_by_arglist(FN&& fn, std::integral_constant<int, 1>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const ks_result<void>&>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<void>&>, ks_future<R>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<void>&>, ks_result<R>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<void>&>, R> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal transform-fn's ret");
		return ks_future<void>::__wrap_transform_fn_by_arglist_ret<R>(std::forward<FN>(fn), std::integral_constant<int, 1>(), std::integral_constant<int, ret_mode>());
	}
	template <class R, class FN>
	inline static auto __wrap_transform_fn_by_arglist(FN&& fn, std::integral_constant<int, 2>) {
		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const ks_result<void>&, ks_cancel_inspector*>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<void>&, ks_cancel_inspector*>, ks_future<R>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<void>&, ks_cancel_inspector*>, ks_result<R>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_result<void>&, ks_cancel_inspector*>, R> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal transform-fn's ret");
		return ks_future<void>::__wrap_transform_fn_by_arglist_ret<R>(std::forward<FN>(fn), std::integral_constant<int, 2>(), std::integral_constant<int, ret_mode>());
	}

	template <class R>
	_NOINLINE static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(std::function<void(const ks_result<void>&)> fn, std::integral_constant<int, 1>, std::integral_constant<int, -1>) {
		static_assert(std::is_void_v<R>, "R must be void");
		return[fn = std::move(fn)](const ks_result<nothing_t>& arg)->ks_result<void> {
			fn(arg.cast<void>());
			return ks_result<void>(nothing);
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(std::function<R(const ks_result<void>&)> fn, std::integral_constant<int, 1>, std::integral_constant<int, 1>) {
		return[fn = std::move(fn)](const ks_result<nothing_t>& arg)->ks_result<R> {
			return ks_result<R>(fn(arg.cast<void>()));
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(std::function<ks_result<R>(const ks_result<void>&)> fn, std::integral_constant<int, 1>, std::integral_constant<int, 2>) {
		return[fn = std::move(fn)](const ks_result<nothing_t>& arg)->ks_result<R> {
			return fn(arg.cast<void>());
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_future<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(std::function<ks_future<R>(const ks_result<void>&)> fn, std::integral_constant<int, 1>, std::integral_constant<int, 3>) {
		return[fn = std::move(fn)](const ks_result<nothing_t>& arg)->ks_future<R> {
			return fn(arg.cast<void>());
		};
	}

	template <class R>
	_NOINLINE static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(std::function<void(const ks_result<void>&, ks_cancel_inspector*)> fn, std::integral_constant<int, 2>, std::integral_constant<int, -1>) {
		static_assert(std::is_void_v<R>, "R must be void");
		return[fn = std::move(fn)](const ks_result<nothing_t>& arg)->ks_result<void> {
			fn(arg.cast<void>(), ks_cancel_inspector::__for_future());
			return ks_result<void>(nothing);
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(std::function<R(const ks_result<void>&, ks_cancel_inspector*)> fn, std::integral_constant<int, 2>, std::integral_constant<int, 1>) {
		return[fn = std::move(fn)](const ks_result<nothing_t>& arg)->ks_result<R> {
			return ks_result<R>(fn(arg.cast<void>(), ks_cancel_inspector::__for_future()));
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_result<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(std::function<ks_result<R>(const ks_result<void>&, ks_cancel_inspector*)> fn, std::integral_constant<int, 2>, std::integral_constant<int, 2>) {
		return[fn = std::move(fn)](const ks_result<nothing_t>& arg)->ks_result<R> {
			return fn(arg.cast<void>(), ks_cancel_inspector::__for_future());
		};
	}
	template <class R>
	_NOINLINE static std::function<ks_future<R>(const ks_result<nothing_t>&)> __wrap_transform_fn_by_arglist_ret(std::function<ks_future<R>(const ks_result<void>&, ks_cancel_inspector*)> fn, std::integral_constant<int, 2>, std::integral_constant<int, 3>) {
		return[fn = std::move(fn)](const ks_result<nothing_t>& arg)->ks_future<R> {
			return fn(arg.cast<void>(), ks_cancel_inspector::__for_future());
		};
	}

public: //post, post_delayed, post_pending
	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<void()>> || 
		std::is_convertible_v<FN, std::function<ks_result<void>()>> || 
		std::is_convertible_v<FN, std::function<ks_future<void>()>> ||
		std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> || 
		std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>> || 
		std::is_convertible_v<FN, std::function<ks_future<void>(ks_cancel_inspector*)>>>>
	static ks_future<void> post(ks_apartment* apartment, FN&& task_fn, const ks_async_context& context = {}) {
		return ks_future<nothing_t>::post(apartment, __wrap_task_fn(std::forward<FN>(task_fn)), context).template cast<void>();
	}
	template <class FN>
	static ks_future<void> post(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn) { //only for compat
		return ks_future<void>::post(apartment, std::forward<FN>(task_fn), context);
	}

	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<void()>> || 
		std::is_convertible_v<FN, std::function<ks_result<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>()>> ||
		std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>(ks_cancel_inspector*)>>>>
	static ks_future<void> post_delayed(ks_apartment* apartment, FN&& task_fn, int64_t delay, const ks_async_context& context = {}) {
		return ks_future<nothing_t>::post_delayed(apartment, __wrap_task_fn(std::forward<FN>(task_fn)), delay, context).template cast<void>();
	}
	template <class FN>
	static ks_future<void> post_delayed(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, int64_t delay) { //only for compat
		return ks_future<void>::post_delayed(apartment, std::forward<FN>(task_fn), delay, context);
	}

	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<void()>> || 
		std::is_convertible_v<FN, std::function<ks_result<void>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>()>> ||
		std::is_convertible_v<FN, std::function<void(ks_cancel_inspector*)>> || 
		std::is_convertible_v<FN, std::function<ks_result<void>(ks_cancel_inspector*)>> ||
		std::is_convertible_v<FN, std::function<ks_future<void>(ks_cancel_inspector*)>>>>
	static ks_future<void> post_pending(ks_apartment* apartment, FN&& task_fn, ks_pending_trigger* trigger, const ks_async_context& context = {}) {
		return ks_future<nothing_t>::post_pending(apartment, __wrap_task_fn(std::forward<FN>(task_fn)), trigger, context).template cast<void>();
	}
	template <class FN>
	static ks_future<void> post_pending(ks_apartment* apartment, const ks_async_context& context, FN&& task_fn, ks_pending_trigger* trigger) { //only for compat
		return ks_future<void>::post_pending(apartment, std::forward<FN>(task_fn), trigger, context);
	}

public: //then, transform
	template <class R, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<R()>> || 
		std::is_convertible_v<FN, std::function<ks_result<R>()>> || 
		std::is_convertible_v<FN, std::function<ks_future<R>()>> ||
		std::is_convertible_v<FN, std::function<R(ks_cancel_inspector*)>> || 
		std::is_convertible_v<FN, std::function<ks_result<R>(ks_cancel_inspector*)>> || 
		std::is_convertible_v<FN, std::function<ks_future<R>(ks_cancel_inspector*)>>>>
	ks_future<R> then(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		return m_nothing_future.then<R>(apartment, __wrap_then_fn<R>(std::forward<FN>(fn)), context);
	}
	template <class R, class FN>
	ks_future<R> then(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->then<R>(apartment, std::forward<FN>(fn), context);
	}

	template <class R, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<R(const ks_result<void>&)>> || 
		std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&)>> || 
		std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<void>&)>> ||
		std::is_convertible_v<FN, std::function<R(const ks_result<void>&, ks_cancel_inspector*)>> || 
		std::is_convertible_v<FN, std::function<ks_result<R>(const ks_result<void>&, ks_cancel_inspector*)>> || 
		std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<void>&, ks_cancel_inspector*)>>>>
	ks_future<R> transform(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		return m_nothing_future.transform<R>(apartment, __wrap_transform_fn<R>(std::forward<FN>(fn)), context);
	}
	template <class R, class FN>
	ks_future<R> transform(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->transform<R>(apartment, std::forward<FN>(fn), context);
	}

public: //flat_then, flat_transform
	template <class R, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<ks_future<R>()>> ||
		std::is_convertible_v<FN, std::function<ks_future<R>(ks_cancel_inspector*)>>>>
	ks_future<R> flat_then(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		return this->then<R>(apartment, std::forward<FN>(fn), context);
	}
	template <class R, class FN>
	ks_future<R> flat_then(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->flat_then<R>(apartment, std::forward<FN>(fn), context);
	}

	template <class R, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<void>&)>> ||
		std::is_convertible_v<FN, std::function<ks_future<R>(const ks_result<void>&, ks_cancel_inspector*)>>>>
	ks_future<R> flat_transform(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		return this->transform<R>(apartment, std::forward<FN>(fn), context);
	}
	template <class R, class FN>
	ks_future<R> flat_transform(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->flat_transform<R>(apartment, std::forward<FN>(fn), context);
	}

public: //on_success, on_failure, on_completion
	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<void()>> && std::is_void_v<std::invoke_result_t<FN>>>>
	ks_future<void> on_success(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		return this->__on_success_impl(apartment, context, std::forward<FN>(fn));
	}
	template <class FN>
	ks_future<void> on_success(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->on_success(apartment, std::forward<FN>(fn), context);
	}

	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<void(const ks_error&)>> && std::is_void_v<std::invoke_result_t<FN, const ks_error&>>>>
	ks_future<void> on_failure(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		return this->__on_failure_impl(apartment, context, std::forward<FN>(fn));
	}
	template <class FN>
	ks_future<void> on_failure(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->on_failure(apartment, std::forward<FN>(fn), context);
	}

	template <class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<void(const ks_result<void>&)>> && std::is_void_v<std::invoke_result_t<FN, const ks_result<void>&>>>>
	ks_future<void> on_completion(ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		return this->__on_completion_impl(apartment, context, std::forward<FN>(fn));
	}
	template <class FN>
	ks_future<void> on_completion(ks_apartment* apartment, const ks_async_context& context, FN&& fn) const { //only for compat
		return this->on_completion(apartment, std::forward<FN>(fn), context);
	}

private: //__on_success, __on_failure, __on_completion
	_NOINLINE ks_future<void> __on_success_impl(ks_apartment* apartment, const ks_async_context& context, std::function<void()> fn) const {
		return m_nothing_future.on_success(
			apartment,
			[fn = std::move(fn)](nothing_t) -> void { fn(); },
			context
		);
	}
	_NOINLINE ks_future<void> __on_failure_impl(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_error&)> fn) const {
		return m_nothing_future.on_failure(
			apartment,
			[fn = std::move(fn)](const ks_error& error) -> void { fn(error); },
			context
		);
	}
	_NOINLINE ks_future<void> __on_completion_impl(ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_result<void>&)> fn) const {
		return m_nothing_future.on_completion(
			apartment,
			[fn = std::move(fn)](const ks_result<nothing_t>& result) -> void { fn(ks_result<void>::__from_other(result)); },
			context
		);
	}

public: //map, map_value, cast
	template <class R>
	_NOINLINE ks_future<R> map(std::function<R()> fn) const {
		return m_nothing_future.template map<R>(
			[fn = std::move(fn)](const nothing_t&) -> R { return fn(); }
		);
	}

	template <class R, class X = R, class _ = std::enable_if_t<std::is_convertible_v<X, R>>>
	ks_future<R> map_value(X&& other_value) const {
		return m_nothing_future.template map_value<R>(std::forward<X>(other_value));
	}

	template <class R, class _ = std::enable_if_t<std::is_void_v<R> || std::is_nothing_v<R>>>
	ks_future<R> cast() const {
		return m_nothing_future.template cast<R>();
	}

public: //set_timeout, try_cancel
	const this_future_type& set_timeout(int64_t timeout) const {
		m_nothing_future.set_timeout(timeout);
		return *this;
	}

	//不希望直接使用future.try_cancel，更应使用controller.try_cancel
	const this_future_type& __try_cancel() const {
		m_nothing_future.__try_cancel();
		return *this;
	}

public: //is_null, is_completed, peek_result, wait
	bool is_null() const {
		return m_nothing_future.is_null();
	}
	bool is_valid() const {
		return m_nothing_future.is_valid();
	}
	bool operator==(nullptr_t) const {
		return m_nothing_future == nullptr;
	}
	bool operator!=(nullptr_t) const {
		return m_nothing_future != nullptr;
	}

	bool is_completed() const {
		return m_nothing_future.is_completed();
	}

	ks_result<void> peek_result() const {
		return ks_result<void>::__from_other(m_nothing_future.peek_result());
	}

	//慎用，使用不当可能会造成死锁或卡顿！
	void __wait() const {
		return m_nothing_future.__wait();
	}

private:
	using __raw_cast_mode_t = typename ks_result<void>::__raw_cast_mode_t;

	template <class R>
	inline static constexpr __raw_cast_mode_t __determine_raw_cast_mode() {
		return ks_result<void>::template __determine_raw_cast_mode<R>();
	}

private:
	using ks_raw_future = __ks_async_raw::ks_raw_future;
	using ks_raw_future_ptr = __ks_async_raw::ks_raw_future_ptr;
	using ks_raw_result = __ks_async_raw::ks_raw_result;
	using ks_raw_value = __ks_async_raw::ks_raw_value;

	ks_future(ks_future<nothing_t>&& nothing_future) noexcept : m_nothing_future(std::move(nothing_future)) {}

	static ks_future<void> __from_raw(const ks_raw_future_ptr& raw_future) noexcept { return ks_future<nothing_t>::__from_raw(raw_future); }
	static ks_future<void> __from_raw(ks_raw_future_ptr&& raw_future) noexcept { return ks_future<nothing_t>::__from_raw(std::move(raw_future)); }
	const ks_raw_future_ptr& __get_raw() const noexcept { return m_nothing_future.__get_raw(); }

	template <class T2> friend class ks_future;
	template <class T2> friend class ks_promise;
	friend class ks_future_util;
	friend class ks_async_flow;

private:
	ks_future<nothing_t> m_nothing_future;
};
