#pragma once

#include "ks_async_base.h"
#include "ks-async-raw/ks_raw_async_flow.h"
#include "ks_future.h"
#include "ks_promise.h"


class ks_async_flow final {
public:
	ks_async_flow() : m_raw_flow(ks_raw_async_flow::create()) {}

	_DISABLE_COPY_CONSTRUCTOR(ks_async_flow);
	ks_async_flow(ks_async_flow&&) noexcept = default;

public:
	void set_j(size_t j) const {
		ASSERT(!this->is_null());
		return m_raw_flow->set_j(j);
	}

public:
	template <class T, class FN, class _ = std::enable_if_t<
		std::is_convertible_v<FN, std::function<T(const ks_async_flow& flow)>> ||
		std::is_convertible_v<FN, std::function<ks_result<T>(const ks_async_flow& flow)>> ||
		std::is_convertible_v<FN, std::function<ks_future<T>(const ks_async_flow& flow)>>>>
	bool add_task(
		const char* name_and_dependencies,
		ks_apartment* apartment, FN&& fn, const ks_async_context& context = {}) const {
		ASSERT(!this->is_null());
		return __choose_add_task<T>(name_and_dependencies, apartment, std::forward<FN>(fn), context);
	}

public:
	uint64_t add_flow_running_observer(
		ks_apartment* apartment, std::function<void(const ks_async_flow& flow)> fn, const ks_async_context& context = {}) const {
		ASSERT(!this->is_null());
		return m_raw_flow->add_flow_running_observer(
			apartment,
			[fn = std::move(fn)](const ks_raw_async_flow_ptr& flow) { fn(ks_async_flow::__from_raw(flow)); },
			context);
	}
	uint64_t add_flow_completed_observer(
		ks_apartment* apartment, std::function<void(const ks_async_flow& flow, const ks_error& error)> fn, const ks_async_context& context = {}) const {
		ASSERT(!this->is_null());
		return m_raw_flow->add_flow_completed_observer(
			apartment,
			[fn = std::move(fn)](const ks_raw_async_flow_ptr& flow, const ks_error& error) { fn(ks_async_flow::__from_raw(flow), error); },
			context);
	}

	uint64_t add_task_running_observer(
		const char* task_name_pattern, 
		ks_apartment* apartment, std::function<void(const ks_async_flow& flow, const char* task_name)> fn, const ks_async_context& context = {}) const {
		ASSERT(!this->is_null());
		return m_raw_flow->add_task_running_observer(
			task_name_pattern, apartment,
			[fn = std::move(fn)](const ks_raw_async_flow_ptr& flow, const char* task_name) { fn(ks_async_flow::__from_raw(flow), task_name); },
			context);
	}
	uint64_t add_task_completed_observer(
		const char* task_name_pattern, 
		ks_apartment* apartment, std::function<void(const ks_async_flow& flow, const char* task_name, const ks_error& error)> fn, const ks_async_context& context = {}) const {
		ASSERT(!this->is_null());
		return m_raw_flow->add_task_completed_observer(
			task_name_pattern, apartment,
			[fn = std::move(fn)](const ks_raw_async_flow_ptr& flow, const char* task_name, const ks_error& error) { fn(ks_async_flow::__from_raw(flow), task_name, error); },
			context);
	}

	void remove_observer(uint64_t id) const {
		ASSERT(!this->is_null());
		return m_raw_flow->remove_observer(id);
	}

public:
	template <class T>
	T get_value(const char* key) const {
		ASSERT(!this->is_null());
		return m_raw_flow->get_value(key).get<T>();
	}

	template <class T, class X = T, class _ = std::enable_if_t<std::is_convertible_v<X, T>>>
	void put_custom_value(const char* key, X&& value) const {
		ASSERT(!this->is_null());
		return m_raw_flow->put_custom_value(key, ks_raw_value::of<T>(std::forward<X>(value)));
	}

public:
	bool start() const {
		ASSERT(!this->is_null());
		return m_raw_flow->start();
	}

	//不希望直接使用flow.try_cancel，更应使用controller.try_cancel
	void __try_cancel() const {
		ASSERT(!this->is_null());
		return m_raw_flow->__try_cancel();
	}

	//慎用，使用不当可能会造成死锁或卡顿！
	void __wait() const {
		ASSERT(!this->is_null());
		return m_raw_flow->__wait();
	}

	//强制清理，一般不需要调用，出现循环引用时可用
	void __force_cleanup() const {
		ASSERT(!this->is_null());
		return m_raw_flow->__force_cleanup();
	}

public:
	bool is_null() const {
		return m_raw_flow == nullptr;
	}

	bool is_task_running(const char* task_name) const {
		ASSERT(!this->is_null());
		return m_raw_flow->is_task_running(task_name);
	}
	bool is_task_completed(const char* task_name) const {
		ASSERT(!this->is_null());
		return m_raw_flow->is_task_completed(task_name);
	}

	bool is_flow_running() const {
		ASSERT(!this->is_null());
		return m_raw_flow->is_flow_running();
	}
	bool is_flow_completed() const {
		ASSERT(!this->is_null());
		return m_raw_flow->is_flow_completed();
	}

	ks_error get_last_error() const {
		ASSERT(!this->is_null());
		return m_raw_flow->get_last_error();
	}
	std::string get_last_failed_task_name() const {
		ASSERT(!this->is_null());
		return m_raw_flow->get_last_failed_task_name();
	}

	template <class T>
	ks_result<T> peek_task_result(const char* task_name) const {
		ASSERT(!this->is_null());
		ks_raw_result raw_result = m_raw_flow->peek_task_result(task_name, __typeinfo_of<T>());
		return ks_result<T>::__from_raw(std::move(raw_result));
	}
	ks_error peek_task_error(const char* task_name) const {
		ASSERT(!this->is_null());
		return m_raw_flow->peek_task_error(task_name);
	}

	template <class T>
	ks_future<T> get_task_future(const char* task_name) const {
		ASSERT(!this->is_null());
		ks_raw_future_ptr raw_future = m_raw_flow->get_task_future(task_name, __typeinfo_of<T>());
		return ks_future<T>::__from_raw(raw_future);
	}

	ks_future<ks_async_flow> get_flow_future() const {
		ASSERT(!this->is_null());
		return m_raw_flow->get_flow_future_this_wrapped();
	}

private:
	template <class T, class FN>
	bool __choose_add_task(
		const char* name_and_dependencies,
		ks_apartment* apartment, FN&& fn, const ks_async_context& context) const {

		constexpr int ret_mode =
			std::is_void_v<std::invoke_result_t<FN, const ks_async_flow&>> ? -1 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_async_flow&>, ks_future<T>> ? 3 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_async_flow&>, ks_result<T>> ? 2 :
			std::is_convertible_v<std::invoke_result_t<FN, const ks_async_flow&>, T> ? 1 : 0;
		static_assert(ret_mode != 0, "illegal task_fn's ret");

		return __choose_add_task_by_ret<T>(
			std::integral_constant<int, ret_mode>(),
			name_and_dependencies,
			apartment, std::forward<FN>(fn), context,
			!std::is_void_v<T>, __typeinfo_of<T>());
	}

	template <class T>
	bool __choose_add_task_by_ret(
		std::integral_constant<int, -1>,
		const char* name_and_dependencies,
		ks_apartment* apartment, std::function<void(const ks_async_flow& flow)> fn, const ks_async_context& context,
		bool need_apply_value, const std::type_info* value_typeinfo) const {
		static_assert(std::is_void_v<T>, "T must be void");
		return m_raw_flow->add_task(
			name_and_dependencies, apartment,
			[fn = std::move(fn)](const ks_raw_async_flow_ptr& flow)->ks_raw_result { return ks_raw_value::of<T>((fn(ks_async_flow::__from_raw(flow)), nothing)); },
			context, 
			need_apply_value, value_typeinfo);
	}

	template <class T>
	bool __choose_add_task_by_ret(
		std::integral_constant<int, 1>,
		const char* name_and_dependencies,
		ks_apartment* apartment, std::function<T(const ks_async_flow& flow)> fn, const ks_async_context& context,
		bool need_apply_value, const std::type_info* value_typeinfo) const {
		return m_raw_flow->add_task(
			name_and_dependencies, apartment,
			[fn = std::move(fn)](const ks_raw_async_flow_ptr& flow)->ks_raw_result { return ks_raw_value::of<T>(fn(ks_async_flow::__from_raw(flow))); },
			context, 
			need_apply_value, value_typeinfo);
	}

	template <class T>
	bool __choose_add_task_by_ret(
		std::integral_constant<int, 2>,
		const char* name_and_dependencies,
		ks_apartment* apartment, std::function<ks_result<T>(const ks_async_flow& flow)> fn, const ks_async_context& context,
		bool need_apply_value, const std::type_info* value_typeinfo) const {
		return m_raw_flow->add_task(
			name_and_dependencies, apartment,
			[fn = std::move(fn)](const ks_raw_async_flow_ptr& flow)->ks_raw_result { return fn(ks_async_flow::__from_raw(flow)).__get_raw(); },
			context, 
			need_apply_value, value_typeinfo);
	}

	template <class T>
	bool __choose_add_task_by_ret(
		std::integral_constant<int, 3>,
		const char* name_and_dependencies,
		ks_apartment* apartment, std::function<ks_future<T>(const ks_async_flow& flow)> fn, const ks_async_context& context,
		bool need_apply_value, const std::type_info* value_typeinfo) const {
		return m_raw_flow->add_flat_task(
			name_and_dependencies, apartment,
			[fn = std::move(fn)](const ks_raw_async_flow_ptr& flow)->ks_raw_future_ptr { return fn(ks_async_flow::__from_raw(flow)).__get_raw(); },
			context, 
			need_apply_value, value_typeinfo);
	}

private:
	template <class T>
	static inline const std::type_info* __typeinfo_of() {
#	ifdef _DEBUG
		return &typeid(T);
#	else
		return nullptr;
#	endif
	}

private:
	using ks_raw_async_flow = __ks_async_raw::ks_raw_async_flow;
	using ks_raw_async_flow_ptr = __ks_async_raw::ks_raw_async_flow_ptr;
	using ks_raw_future = __ks_async_raw::ks_raw_future;
	using ks_raw_future_ptr = __ks_async_raw::ks_raw_future_ptr;
	using ks_raw_result = __ks_async_raw::ks_raw_result;
	using ks_raw_value = __ks_async_raw::ks_raw_value;

	explicit ks_async_flow(const ks_raw_async_flow_ptr& raw_flow, int) : m_raw_flow(raw_flow) {}
	explicit ks_async_flow(ks_raw_async_flow_ptr&& raw_flow, int) : m_raw_flow(std::move(raw_flow)) {}

	static ks_async_flow __from_raw(const ks_raw_async_flow_ptr& raw_flow) { return ks_async_flow(raw_flow, 0); }
	static ks_async_flow __from_raw(ks_raw_async_flow_ptr&& raw_flow) { return ks_async_flow(std::move(raw_flow), 0); }
	const ks_raw_async_flow_ptr& __get_raw() const { return m_raw_flow; }

	static ks_future<ks_async_flow> __wrap_raw_flow_future(const ks_raw_future_ptr& raw_flow_future_this_wrapped) {
		return ks_future<ks_async_flow>::__from_raw(raw_flow_future_this_wrapped);
	}

	friend class __ks_async_raw::ks_raw_async_flow;

private:
	ks_raw_async_flow_ptr m_raw_flow;
};
