#pragma once

#include "../ks_async_base.h"
#include "ks_raw_future.h"
#include "ks_raw_promise.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <regex>

//for flow_future_wrapped
#include "../ks_future.h"
class ks_async_flow;

__KS_ASYNC_RAW_BEGIN


class ks_raw_async_flow;
using ks_raw_async_flow_ptr = std::shared_ptr<ks_raw_async_flow>;

class ks_raw_async_flow : public std::enable_shared_from_this<ks_raw_async_flow> {
public:
	KS_ASYNC_API static ks_raw_async_flow_ptr create();

public:
	KS_ASYNC_API void set_default_apartment(ks_apartment* apartment);
	KS_ASYNC_API void set_j(size_t j);

public:
	KS_ASYNC_API bool add_task(
		const char* name_and_dependencies,
		ks_apartment* apartment,
		std::function<ks_raw_result(const ks_raw_async_flow_ptr& flow)>&& fn,
		const ks_async_context& context,
		const std::type_info* value_typeinfo = nullptr);
	KS_ASYNC_API bool add_flat_task(
		const char* name_and_dependencies,
		ks_apartment* apartment,
		std::function<ks_raw_future_ptr(const ks_raw_async_flow_ptr& flow)>&& fn,
		const ks_async_context& context,
		const std::type_info* value_typeinfo = nullptr);

public:
	KS_ASYNC_API uint64_t add_flow_running_observer(ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow)>&& fn, const ks_async_context& context);
	KS_ASYNC_API uint64_t add_flow_completed_observer(ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow, const ks_error& error)>&& fn, const ks_async_context& context);

	KS_ASYNC_API uint64_t add_task_running_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow, const char* task_name)>&& fn, const ks_async_context& context);
	KS_ASYNC_API uint64_t add_task_completed_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow, const char* task_name, const ks_error& error)>&& fn, const ks_async_context& context);

	KS_ASYNC_API bool remove_observer(uint64_t id);

public:
	KS_ASYNC_API bool start();
	KS_ASYNC_API void try_cancel();

	//慎用，使用不当可能会造成死锁或卡顿！
	KS_ASYNC_API _DECL_DEPRECATED void wait();

	//强制清理，一般不需要调用，出现循环引用时可用
	KS_ASYNC_API void __force_cleanup();

public:
	KS_ASYNC_API bool is_flow_running();
	KS_ASYNC_API bool is_task_running(const char* task_name);

	KS_ASYNC_API bool is_flow_completed();
	KS_ASYNC_API bool is_task_completed(const char* task_name);

	KS_ASYNC_API ks_raw_result get_task_result(const char* task_name, const std::type_info* value_typeinfo = nullptr);
	KS_ASYNC_API ks_raw_value get_task_value(const char* task_name, const std::type_info* value_typeinfo = nullptr);
	KS_ASYNC_API ks_error get_task_error(const char* task_name);

	KS_ASYNC_API void set_user_data(const char* key, const ks_any& value);
	KS_ASYNC_API ks_any get_user_data(const char* key);

	KS_ASYNC_API ks_error get_last_error();
	KS_ASYNC_API std::string get_failed_task_name();

	KS_ASYNC_API ks_raw_future_ptr get_task_future(const char* task_name, const std::type_info* value_typeinfo = nullptr);
	KS_ASYNC_API ks_raw_future_ptr  get_flow_future_void();
	KS_ASYNC_API ks_future<ks_async_flow> get_flow_future_wrapped(); //由于future对象声明期管理的原因，这里会直接以ks_future<ks_async_flow>类型直接进行管理

private:
	enum __raw_ctor { v };

public: //called by make_shared in create
	explicit ks_raw_async_flow(__raw_ctor);
	~ks_raw_async_flow();
	_DISABLE_COPY_CONSTRUCTOR(ks_raw_async_flow);

private:
	enum class status_t {
		not_start = -1, //未开始
		running = 0,  //正在执行
		succeeded = 1,  //成功
		failed = 2,  //失败
	};

	static const status_t __not_start_but_pending_status = (status_t)(-1001);

	struct _TASK_ITEM {
		std::string task_name; //const-like
		std::vector<std::string> task_dependencies; //const-like
		ks_apartment* task_apartment; //const-like
		const std::type_info* task_value_typeinfo; //const-like

		int task_level = 0;
		std::unordered_set<std::string> task_waiting_for_dependencies;

		status_t task_status = status_t::not_start;
		ks_condition_variable task_completed_cv{};

		ks_raw_result task_pending_arg_void;
		ks_raw_promise_ptr task_trigger_void;
		ks_raw_result task_result{}; //result<T>

		ks_raw_promise_ptr task_promise_opt = nullptr;
	};

	struct _FLOW_OBSERVER_ITEM {
		ks_apartment* apartment;
		std::function<void(const ks_raw_async_flow_ptr& flow)> on_running_fn = nullptr;
		std::function<void(const ks_raw_async_flow_ptr& flow, const ks_error& error)> on_completed_fn = nullptr;
		ks_async_context observer_context;
	};

	struct _TASK_OBSERVER_ITEM {
		std::regex task_name_pattern_re;
		ks_apartment* apartment;
		std::function<void(const ks_raw_async_flow_ptr& flow, const char* task_name)> on_running_fn = nullptr;
		std::function<void(const ks_raw_async_flow_ptr& flow, const char* task_name, const ks_error& error)> on_completed_fn = nullptr;
		ks_async_context observer_context;
	};

private:
	void do_make_flow_running_locked(std::unique_lock<ks_mutex>& lock);
	void do_make_flow_completed_locked(const ks_error& flow_error, std::unique_lock<ks_mutex>& lock);

	void do_make_task_pending_locked(const std::shared_ptr<_TASK_ITEM>& task_item, const ks_raw_result& arg, std::unique_lock<ks_mutex>& lock);
	void do_make_task_running_locked(const std::shared_ptr<_TASK_ITEM>& task_item, std::unique_lock<ks_mutex>& lock);
	void do_make_task_completed_locked(const std::shared_ptr<_TASK_ITEM>& task_item, const ks_raw_result& task_result, std::unique_lock<ks_mutex>& lock);

	void do_drain_pending_task_queue_locked(std::unique_lock<ks_mutex>& lock);

	void do_fire_flow_observers_locked(status_t status, const ks_error& error, std::unique_lock<ks_mutex>& lock);
	void do_fire_task_observers_locked(const std::string& task_name, status_t status, const ks_error& error, std::unique_lock<ks_mutex>& lock);

	void do_force_cleanup_data_locked(std::unique_lock<ks_mutex>& lock);

private:
	ks_mutex m_mutex;

	ks_apartment* m_default_apartment = ks_apartment::default_mta();
	size_t m_j = size_t(-1);

	std::unordered_map<std::string, std::shared_ptr<_TASK_ITEM>> m_task_map{};

	std::unordered_map<uint64_t, std::shared_ptr<_FLOW_OBSERVER_ITEM>> m_flow_observer_map{};
	std::unordered_map<uint64_t, std::shared_ptr<_TASK_OBSERVER_ITEM>> m_task_observer_map{};
	std::atomic<uint64_t> m_last_x_observer_id = { 0 };

	std::unordered_map<std::string, ks_any> m_user_data_map{};

	size_t m_not_start_task_count = 0;
	size_t m_pending_task_count = 0;
	size_t m_running_task_count = 0;
	size_t m_succeeded_task_count = 0;
	size_t m_failed_task_count = 0;
	std::vector<std::shared_ptr<_TASK_ITEM>> m_temp_pending_task_queue{};

	status_t m_flow_status = status_t::not_start;
	ks_condition_variable m_flow_completed_cv{};

	volatile bool m_cancelled_flag_v = false;
	volatile bool m_force_cleanup_flag_v = false;

	std::string m_1st_failed_task_name{};
	ks_error m_last_error{};

	ks_raw_promise_ptr m_flow_promise_void_opt = nullptr;

	//flow_promise_ext本质上是ks_future<ks_async_flow>
	//keeper仅在flow执行完成前才会保持，完成时清空，以避免循环引用
	std::weak_ptr<ks_raw_promise> m_flow_promise_wrapped_weak = {};
	ks_raw_promise_ptr m_flow_promise_wrapped_keepper_until_completed = nullptr;

	std::shared_ptr<ks_raw_async_flow> m_self_running_keeper = nullptr;
};


__KS_ASYNC_RAW_END
