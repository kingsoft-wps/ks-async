#pragma once

#include "ks_async_base.h"
#include "ks_future.h"
#include "ks_promise.h"
#include "ks-async-raw/ks_raw_internal_helper.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <regex>

class ks_async_flow;
using ks_async_flow_ptr = std::shared_ptr<ks_async_flow>;


class ks_async_flow : public std::enable_shared_from_this<ks_async_flow> {
public:
	enum class status_t {
		not_start = -1,
		running = 0, 
		succeeded = 1, 
		failed = 2 
	};

private:
	enum class __raw_ctor { v };
	using ks_raw_value = __ks_async_raw::ks_raw_value;
	using ks_raw_living_context_rtstt = __ks_async_raw::ks_raw_living_context_rtstt;
	static const status_t __not_start_but_pending_status = (status_t)(-1001);

public:
	KS_ASYNC_API static ks_async_flow_ptr create();

public:
	KS_ASYNC_API void set_j(size_t j);

public:
	template <class T>
	KS_ASYNC_INLINE_API bool add_task(
		const char* task_name, const char* task_dependencies,
		ks_apartment* apartment, std::function<T(const ks_async_flow_ptr& flow)>&& fn, const ks_async_context& context = {});
	template <class T>
	KS_ASYNC_INLINE_API bool add_task(
		const char* task_name, const char* task_dependencies,
		ks_apartment* apartment, std::function<ks_result<T>(const ks_async_flow_ptr& flow)>&& fn, const ks_async_context& context = {});
	template <class T>
	KS_ASYNC_INLINE_API bool add_task(
		const char* task_name, const char* task_dependencies,
		ks_apartment* apartment, std::function<ks_future<T>(const ks_async_flow_ptr& flow)> fn, const ks_async_context& context = {});

private:
	KS_ASYNC_API bool do_add_task(
		const char* task_name, const char* task_dependencies, const std::type_info* task_result_value_typeinfo,
		ks_apartment* apartment, const std::function<ks_future<ks_raw_value>()>& eval_fn, const ks_async_context& context); //called in add_task<T>, so need export

public:
	KS_ASYNC_API uint64_t add_flow_observer(ks_apartment* apartment, std::function<void(const ks_async_flow_ptr& flow, status_t flow_status)>&& observer_fn, const ks_async_context& context);
	KS_ASYNC_API uint64_t add_task_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_async_flow_ptr& flow, const char* task_name, status_t task_status)>&& observer_fn, const ks_async_context& context);
	KS_ASYNC_API void remove_observer(uint64_t id);

public:
	KS_ASYNC_API bool start();
	KS_ASYNC_API void try_cancel();
	KS_ASYNC_API _DECL_DEPRECATED void wait(); //等待flow结束，因为有死锁隐患，所以暂为deprecated，不允许被调用

	//重置为not_start状态（task_result会被重置，但user_data会被保留）
	KS_ASYNC_API bool reset();

	//不带状态和observer克隆flow，新flow对象状态为init
	KS_ASYNC_API ks_async_flow_ptr spawn();

public:
	KS_ASYNC_API bool is_flow_completed();
	KS_ASYNC_API bool is_task_completed(const char* task_name);

	KS_ASYNC_API status_t get_flow_status();
	KS_ASYNC_API status_t get_task_status(const char* task_name);

	template <class T>
	KS_ASYNC_INLINE_API ks_result<T> get_task_result(const char* task_name);
	template <class T>
	KS_ASYNC_INLINE_API T get_task_result_value(const char* task_name);

	KS_ASYNC_API std::string get_failed_task_name();
	KS_ASYNC_API ks_error get_failed_task_error();

	KS_ASYNC_API ks_future<void> get_flow_future();

public:
	template <class T>
	KS_ASYNC_INLINE_API void set_user_data(const char* name, const T& value);
	template <class T>
	KS_ASYNC_INLINE_API T get_user_data(const char* name);

private:
	struct _TASK_ITEM {
		std::string task_name; //const-like
		std::vector<std::string> task_dependencies; //const-like

		ks_apartment* task_apartment = nullptr; //const-like
		std::function<ks_future<ks_raw_value>()> task_eval_fn; //const-like
		ks_async_context task_context; //const-like

#ifdef _DEBUG
		const std::type_info* task_result_value_typeinfo = nullptr;
#endif

		int task_level = 0;
		std::unordered_set<std::string> task_waiting_for_dependencies;

		status_t task_status = status_t::not_start;
		ks_result<void> task_pending_arg;

		ks_result<ks_raw_value> task_result;
		ks_promise<void> task_trigger = nullptr;
	};

	struct _FLOW_OBSERVER_ITEM {
		ks_apartment* apartment;
		std::function<void(const ks_async_flow_ptr& flow, status_t task_status)> observer_fn;
		ks_async_context context;
	};

	struct _TASK_OBSERVER_ITEM {
		std::regex task_name_pattern_re;
		ks_apartment* apartment;
		std::function<void(const ks_async_flow_ptr& flow, const char* task_name, status_t task_status)> observer_fn;
		ks_async_context context;
	};

private:
	void do_make_flow_running_locked(std::unique_lock<ks_mutex>& lock);
	void do_make_flow_completed_locked(std::unique_lock<ks_mutex>& lock);

	void do_make_task_pending_locked(const std::shared_ptr<_TASK_ITEM>& task_item, const ks_result<void>& arg, std::unique_lock<ks_mutex>& lock);
	void do_make_task_running_locked(const std::shared_ptr<_TASK_ITEM>& task_item, std::unique_lock<ks_mutex>& lock);
	void do_make_task_completed_locked(const std::shared_ptr<_TASK_ITEM>& task_item, const ks_result<ks_raw_value>& task_result, std::unique_lock<ks_mutex>& lock);

	void do_drain_pending_task_queue_locked(std::unique_lock<ks_mutex>& lock);

	void do_fire_flow_observers_locked(status_t flow_status, std::unique_lock<ks_mutex>& lock);
	void do_fire_task_observers_locked(const std::string& task_name, status_t task_status, std::unique_lock<ks_mutex>& lock);

public:
	explicit ks_async_flow(__raw_ctor); //inner use, called in create, public it because make_shared
	_DISABLE_COPY_CONSTRUCTOR(ks_async_flow);

private:
	ks_mutex m_mutex;

	size_t m_j = size_t(-1);

	std::unordered_map<std::string, std::shared_ptr<_TASK_ITEM>> m_task_map;
	ks_promise<void> m_flow_promise = nullptr;

	std::unordered_map<uint64_t, std::shared_ptr<_FLOW_OBSERVER_ITEM>> m_flow_observer_map;
	std::unordered_map<uint64_t, std::shared_ptr<_TASK_OBSERVER_ITEM>> m_task_observer_map;
	std::atomic<uint64_t> m_last_x_observer_id = { 0 };

	status_t m_flow_status = status_t::not_start;

	size_t m_not_start_task_count = 0;
	size_t m_pending_task_count = 0;
	size_t m_running_task_count = 0;
	size_t m_succeeded_task_count = 0;
	size_t m_failed_task_count = 0;
	std::vector<std::shared_ptr<_TASK_ITEM>> m_temp_pending_task_queue;

	volatile bool m_flow_cancelled_flag_v = false;
	std::shared_ptr<_TASK_ITEM> m_early_failed_task_item = nullptr;

	std::unordered_map<std::string, ks_any> m_user_data_map;
};


//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//ks_async_flow::模板方法实现...
template <class T>
bool ks_async_flow::add_task(
	const char* task_name, const char* task_dependencies,
	ks_apartment* apartment, std::function<T(const ks_async_flow_ptr& flow)>&& fn, const ks_async_context& context) {

#ifdef _DEBUG
	const std::type_info* task_result_value_typeinfo = &typeid(T);
#else
	const std::type_info* task_result_value_typeinfo = nullptr;
#endif

	return do_add_task(
		task_name, task_dependencies, task_result_value_typeinfo, apartment,
		[this, this_ptr = this->shared_from_this(), apartment, fn = std::move(fn), context]() -> ks_future<ks_raw_value> {

		if (m_flow_cancelled_flag_v) 
			return ks_future<ks_raw_value>::rejected(ks_error::was_cancelled_error());

		ks_raw_living_context_rtstt context_rtstt;
		context_rtstt.apply(context);

		if (context.__check_cancel_all_ctrl() || context.__check_owner_expired()) 
			return ks_future<ks_raw_value>::rejected(ks_error::was_cancelled_error());

		return ks_future<ks_raw_value>::resolved(ks_raw_value::of<T>(fn(this_ptr)));
	}, context);
}

template <class T>
bool ks_async_flow::add_task(
	const char* task_name, const char* task_dependencies,
	ks_apartment* apartment, std::function<ks_result<T>(const ks_async_flow_ptr& flow)>&& fn, const ks_async_context& context) {

#ifdef _DEBUG
	const std::type_info* task_result_value_typeinfo = &typeid(T);
#else
	const std::type_info* task_result_value_typeinfo = nullptr;
#endif

	return do_add_task(
		task_name, task_dependencies, task_result_value_typeinfo, apartment,
		[this, this_ptr = this->shared_from_this(), apartment, fn = std::move(fn), context]() -> ks_future<ks_raw_value> {

		if (m_flow_cancelled_flag_v) 
			return ks_future<ks_raw_value>::rejected(ks_error::was_cancelled_error());

		ks_raw_living_context_rtstt context_rtstt;
		context_rtstt.apply(context);

		if (context.__check_cancel_all_ctrl() || context.__check_owner_expired()) 
			return ks_future<ks_raw_value>::rejected(ks_error::was_cancelled_error());

		ks_result<T> typed_task_result = fn(this_ptr);
		if (typed_task_result.is_value())
			return ks_future<ks_raw_value>::resolved(ks_raw_value::of<T>(typed_task_result.to_value()));
		else
			return ks_future<ks_raw_value>::rejected(typed_task_result.to_error());
	}, context);
}

template <class T>
bool ks_async_flow::add_task(
	const char* task_name, const char* task_dependencies,
	ks_apartment* apartment, std::function<ks_future<T>(const ks_async_flow_ptr& flow)> fn, const ks_async_context& context) {

#ifdef _DEBUG
	const std::type_info* task_result_value_typeinfo = &typeid(T);
#else
	const std::type_info* task_result_value_typeinfo = nullptr;
#endif

	return do_add_task(
		task_name, task_dependencies, task_result_value_typeinfo, apartment,
		[this, this_ptr = this->shared_from_this(), apartment, fn = std::move(fn), context]() ->ks_future<ks_raw_value> {

		if (m_flow_cancelled_flag_v) 
			return ks_future<ks_raw_value>::rejected(ks_error::was_cancelled_error());

		ks_raw_living_context_rtstt context_rtstt;
		context_rtstt.apply(context);

		if (context.__check_cancel_all_ctrl() || context.__check_owner_expired()) 
			return ks_future<ks_raw_value>::rejected(ks_error::was_cancelled_error());

		return fn(this_ptr).then<ks_raw_value>(
			apartment,
			[](const T& value) -> ks_result<ks_raw_value> { return ks_raw_value::of<T>(value); },
			ks_async_context().set_priority(0x10000));
	}, context);
}


template <class T>
ks_result<T> ks_async_flow::get_task_result(const char* task_name) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	auto it = m_task_map.find(task_name);
	if (it == m_task_map.cend()) {
		ASSERT(false);
		throw std::runtime_error("task result not found");
	}

	ASSERT(it->second->task_result_value_typeinfo != nullptr && (*it->second->task_result_value_typeinfo == typeid(XT) || strcmp(it->second->task_result_value_typeinfo->name(), typeid(XT).name()) == 0));

	if (it->second->task_status != status_t::succeeded && it->second->task_status != status_t::failed) {
		ASSERT(false);
		throw std::runtime_error("task result not available");
	}

	const ks_result<ks_raw_value>& task_result = it->second->task_result;
	if (!task_result.is_completed()) {
		ASSERT(false);
		throw std::runtime_error("task result not available");
	}

	if (task_result.is_value())
		return task_result.to_value().get<T>();
	else
		return task_result.to_error();
}

template <class T>
T ks_async_flow::get_task_result_value(const char* task_name) {
	ks_result<T> task_result = this->get_task_result(task_name);
	if (!task_result.is_value()) {
		ASSERT(false);
		throw std::runtime_error("task result value not available");
	}

	return task_result.to_value().get<T>();
}


template <class T>
void ks_async_flow::set_user_data(const char* name, const T& value) {
	std::unique_lock<ks_mutex> lock(m_mutex);
	m_user_data_map[name] = ks_any::of<T>(value);
}

template <class T>
T ks_async_flow::get_user_data(const char* name) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	auto it = m_user_data_map.find(name);
	if (it == m_user_data_map.cend()) {
		ASSERT(false);
		throw std::runtime_error("user data not found");
	}

	return it->second.cast<T>();
}
