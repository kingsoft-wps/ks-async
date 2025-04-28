#pragma once

#include "../ks_async_base.h"
#include "ks_raw_future.h"
#include "ks_raw_promise.h"
#include <vector>
#include <map>
#include <set>
#include <regex>

//for flow_future_wrapped
#include "../ks_future.h"
class ks_async_flow;

__KS_ASYNC_RAW_BEGIN


class ks_raw_async_flow;
using ks_raw_async_flow_ptr = std::shared_ptr<ks_raw_async_flow>;

class ks_raw_async_flow final : public std::enable_shared_from_this<ks_raw_async_flow> {
public:
	KS_ASYNC_API static ks_raw_async_flow_ptr create();

public:
	KS_ASYNC_API void set_j(size_t j);

public:
	KS_ASYNC_API bool add_task(
		const char* name_and_dependencies,
		ks_apartment* apartment,
		std::function<ks_raw_result(const ks_raw_async_flow_ptr& flow)>&& fn,
		const ks_async_context& context,
		bool need_apply_value = true,
		const std::type_info* value_typeinfo = nullptr);

	KS_ASYNC_API bool add_flat_task(
		const char* name_and_dependencies,
		ks_apartment* apartment,
		std::function<ks_raw_future_ptr(const ks_raw_async_flow_ptr& flow)>&& fn,
		const ks_async_context& context,
		bool need_apply_value = true,
		const std::type_info* value_typeinfo = nullptr);

public:
	KS_ASYNC_API uint64_t add_flow_running_observer(ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow)>&& fn, const ks_async_context& context);
	KS_ASYNC_API uint64_t add_flow_completed_observer(ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow, const ks_error& error)>&& fn, const ks_async_context& context);

	KS_ASYNC_API uint64_t add_task_running_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow, const char* task_name)>&& fn, const ks_async_context& context);
	KS_ASYNC_API uint64_t add_task_completed_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow, const char* task_name, const ks_error& error)>&& fn, const ks_async_context& context);

	KS_ASYNC_API bool remove_observer(uint64_t id);

public:
	KS_ASYNC_API ks_raw_value get_value(const char* key);
	KS_ASYNC_API void put_custom_value(const char* key, const ks_raw_value& value);

public:
	KS_ASYNC_API bool start();
	KS_ASYNC_API void try_cancel();

	//强制清理，一般不需要调用，出现循环引用时可用
	KS_ASYNC_API void __force_cleanup();

	//慎用，使用不当可能会造成死锁或卡顿！
	KS_ASYNC_API void __wait();

public:
	KS_ASYNC_API bool is_task_running(const char* task_name);
	KS_ASYNC_API bool is_task_completed(const char* task_name);

	KS_ASYNC_API bool is_flow_running();
	KS_ASYNC_API bool is_flow_completed();

	KS_ASYNC_API ks_error get_last_error();
	KS_ASYNC_API std::string get_last_failed_task_name();

	KS_ASYNC_API ks_raw_result peek_task_result(const char* task_name, const std::type_info* value_typeinfo = nullptr);
	KS_ASYNC_API ks_error peek_task_error(const char* task_name);

	KS_ASYNC_API ks_raw_future_ptr get_task_future(const char* task_name, const std::type_info* value_typeinfo = nullptr);

	KS_ASYNC_API ks_raw_future_ptr get_flow_future_void();
	KS_ASYNC_API ks_future<ks_async_flow> get_flow_future_this_wrapped(); //由于future对象声明期管理的原因，这里会直接以ks_future<ks_async_flow>类型直接进行管理

private:
	enum class __raw_ctor { v };

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
		bool need_apply_value; //const-like
		const std::type_info* task_value_typeinfo; //const-like

		int task_level = 0;
		std::set<std::string> task_waiting_for_dependencies;

		status_t task_status = status_t::not_start;

		ks_raw_result task_pending_arg_void;
		ks_raw_promise_ptr task_trigger_void;
		ks_raw_result task_result{}; //ks_result<T>

		ks_raw_promise_ptr task_promise_opt = nullptr;
	};

	enum class _x_observer_kind_t { for_running, for_completed };

	struct _FLOW_OBSERVER_ITEM {
		_x_observer_kind_t kind;
		ks_apartment* apartment;
		std::function<void(const ks_raw_async_flow_ptr& flow)> on_flow_running_fn = nullptr;
		std::function<void(const ks_raw_async_flow_ptr& flow, const ks_error& error)> on_flow_completed_fn = nullptr;
		ks_async_context observer_context;
	};

	struct _TASK_OBSERVER_ITEM {
		_x_observer_kind_t kind;
		std::regex task_name_pattern_re;
		ks_apartment* apartment;
		std::function<void(const ks_raw_async_flow_ptr& flow, const char* task_name)> on_task_running_fn = nullptr;
		std::function<void(const ks_raw_async_flow_ptr& flow, const char* task_name, const ks_error& error)> on_task_completed_fn = nullptr;
		ks_async_context observer_context;
	};

private:
	bool do_start_locked(std::unique_lock<ks_mutex>& lock);

	void do_make_flow_running_locked(std::unique_lock<ks_mutex>& lock);
	void do_make_flow_completed_locked(const ks_error& flow_error, std::unique_lock<ks_mutex>& lock);

	void do_make_task_pending_locked(const std::shared_ptr<_TASK_ITEM>& task_item, const ks_raw_result& arg, std::unique_lock<ks_mutex>& lock);
	void do_make_task_running_locked(const std::shared_ptr<_TASK_ITEM>& task_item, std::unique_lock<ks_mutex>& lock);
	void do_make_task_completed_locked(const std::shared_ptr<_TASK_ITEM>& task_item, const ks_raw_result& task_result, std::unique_lock<ks_mutex>& lock);

	void do_drain_pending_task_queue_locked(std::unique_lock<ks_mutex>& lock);

	void do_fire_flow_observers_locked(_x_observer_kind_t kind, const ks_error& error, std::unique_lock<ks_mutex>& lock);
	void do_fire_task_observers_locked(_x_observer_kind_t kind, const std::string& task_name, const ks_error& error, std::unique_lock<ks_mutex>& lock);

	void do_final_force_cleanup_value_refs_locked(std::unique_lock<ks_mutex>& lock);

private:
	ks_mutex m_mutex;

	size_t m_j = size_t(-1);

	std::map<std::string, std::shared_ptr<_TASK_ITEM>> m_task_map{};

	std::map<uint64_t, std::shared_ptr<_FLOW_OBSERVER_ITEM>> m_flow_observer_map{};
	std::map<uint64_t, std::shared_ptr<_TASK_OBSERVER_ITEM>> m_task_observer_map{};
	uint64_t m_last_x_observer_id = 0;

	std::map<std::string, ks_raw_value> m_raw_value_map{};

	size_t m_not_start_task_count = 0;
	size_t m_pending_task_count = 0;
	size_t m_running_task_count = 0;
	size_t m_succeeded_task_count = 0;
	size_t m_failed_task_count = 0;
	std::vector<std::shared_ptr<_TASK_ITEM>> m_temp_pending_task_queue{};

	volatile status_t m_flow_status_v = status_t::not_start;
	volatile bool m_force_cleanup_flag_v = false;

	ks_error m_last_error{};
	std::string m_1st_failed_task_name{};

	ks_async_controller m_flow_controller{};

	ks_raw_promise_ptr m_flow_promise_void_opt = nullptr;

	//flow_promise_wrapped本质上是ks_future<ks_async_flow>
	//keeper仅在flow执行完成前才会保持，完成时清空，以避免循环引用
	std::weak_ptr<ks_raw_promise> m_flow_promise_this_wrapped_weak = {};
	ks_raw_promise_ptr m_flow_promise_this_wrapped_keepper_until_completed = nullptr;

	//self keeper, during running
	std::shared_ptr<ks_raw_async_flow> m_self_running_keeper = nullptr;
};


__KS_ASYNC_RAW_END
