#include "ks_raw_async_flow.h"
#include "ks_raw_internal_helper.hpp"
#include "../ks_async_flow.h"  //for flow_promise_wrapped
#include <cstring>
#include <string.h>
#include <set>

void __forcelink_to_ks_raw_flow_cpp() {}


__KS_ASYNC_RAW_BEGIN


static bool __do_check_name_legal(const char* name, bool allow_wild) {
	if (name == nullptr || name[0] == 0)
		return false;

	const char* illegal_chars = allow_wild
		? " \t\r\n,;:&|!()[]"
		: " \t\r\n,;:&|!()[]*?";

	for (char c = *name; c != 0; c = *name++) {
		if (strchr(illegal_chars, c) != nullptr)
			return false;
	}

	return true;
}

static void __do_string_trim(std::string* str) {
	if (str->empty())
		return;

	const char* space_chars = " \t\r\n";

	size_t pos_right = str->find_last_not_of(space_chars);
	if (pos_right != size_t(-1))
		str->resize(pos_right + 1);

	size_t pos_left = str->find_first_not_of(space_chars);
	if (pos_left != 0)
		str->erase(0, pos_left);
}

static void __do_string_split(const char* str, const char* seps, bool skip_empty, std::vector<std::string>* out) {
	size_t len = str != nullptr ? strlen(str) : 0;
	if (len == 0)
		return;

	if (seps == nullptr || seps[0] == 0)
		seps = " \t";

	size_t pos = 0;
	while (pos < len) {
		size_t pos_end = pos;
		while (pos_end < len && strchr(seps, str[pos_end]) == nullptr)
			pos_end++;

		std::string item(str + pos, pos_end - pos);
		__do_string_trim(&item);

		if (!(skip_empty && item.empty())) {
			out->push_back(item);
		}

		pos = pos_end + 1;
	}
}

static void __do_pattern_to_regex(const char* pattern, std::regex* re) {
	//注：pattern是逗号分隔的多项式，且各项支持*和?通配符，转为正则
	size_t pattern_len = pattern != nullptr ? strlen(pattern) : 0;
	if (pattern_len == 0) {
		re->assign("");
		return;
	}

	std::string pattern_re_lit;
	pattern_re_lit.reserve(pattern_len);

	std::vector<std::string> pattern_vec;
	__do_string_split(pattern, ",;| \t", true, &pattern_vec);

	for (const std::string& pattern_item : pattern_vec) {
		ASSERT(__do_check_name_legal(pattern_item.c_str(), true));

		if (!pattern_re_lit.empty()) 
			pattern_re_lit.append("|");

		for (char c : pattern_item) {
			if (c == '*')
				pattern_re_lit.append(".*");
			else if (c == '?')
				pattern_re_lit.append(".");
			else if (strchr(".+^$|\\()[]{}", c) != nullptr)
				pattern_re_lit.append(1, '\\').append(1, c);
			else
				pattern_re_lit.append(1, c);
		}
	}

	re->assign(pattern_re_lit);
}


//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

ks_raw_async_flow::ks_raw_async_flow(__raw_ctor) {
}

ks_raw_async_flow::~ks_raw_async_flow() {
	ASSERT(m_flow_status_v != status_t::running);
}

ks_raw_async_flow_ptr ks_raw_async_flow::create() {
	return std::make_shared<ks_raw_async_flow>(__raw_ctor::v);
}


void ks_raw_async_flow::set_j(size_t j) {
	std::unique_lock<ks_mutex> lock(m_mutex);
	m_j = j;
	if (m_j == 0)
		m_j = size_t(-1);
}


bool ks_raw_async_flow::add_task(
	const char* name_and_dependencies,
	ks_apartment* apartment,
	std::function<ks_raw_result(const ks_raw_async_flow_ptr& flow)>&& fn,
	const ks_async_context& context,
	bool need_apply_value,
	const std::type_info* value_typeinfo) {

	return this->add_flat_task(
		name_and_dependencies,
		apartment,
		[fn = std::move(fn)](const ks_raw_async_flow_ptr& flow) -> ks_raw_future_ptr {
			ks_raw_result result = fn(flow);
			ASSERT(result.is_completed());
			if (result.is_value())
				return ks_raw_future::resolved(result.to_value(), ks_apartment::current_thread_apartment());
			else
				return ks_raw_future::rejected(result.to_error(), ks_apartment::current_thread_apartment());
		},
		context, 
		need_apply_value,
		value_typeinfo
	);
}

bool ks_raw_async_flow::add_flat_task(
	const char* name_and_dependencies,
	ks_apartment* apartment,
	std::function<ks_raw_future_ptr(const ks_raw_async_flow_ptr& flow)>&& fn,
	const ks_async_context& context,
	bool need_apply_value,
	const std::type_info* value_typeinfo) {

	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_flow_status_v != status_t::not_start || m_force_cleanup_flag_v) {
		ASSERT(false);
		return false;
	}

	if (name_and_dependencies == nullptr || name_and_dependencies[0] == 0) {
		ASSERT(false);
		return false;
	}

	const char* p_colon = strchr(name_and_dependencies, ':');

	std::string task_name(name_and_dependencies, p_colon != nullptr ? p_colon - name_and_dependencies : strlen(name_and_dependencies));
	__do_string_trim(&task_name);
	if (!__do_check_name_legal(task_name.c_str(), false)) {
		ASSERT(false);
		return false;
	}

	std::vector<std::string> task_dependencies;
	if (p_colon != nullptr) {
		__do_string_split(p_colon + 1, ",;& \t", true, &task_dependencies);
		for (auto& dep_name : task_dependencies) {
			if (dep_name == task_name || !__do_check_name_legal(dep_name.c_str(), false)) {
				ASSERT(false);
				return false;
			}
		}
	}

	if (m_task_map.find(task_name) != m_task_map.cend()) {
		ASSERT(false);
		return false;
	}

	std::shared_ptr<_TASK_ITEM> task_item = std::make_shared<_TASK_ITEM>();
	task_item->task_name.swap(task_name);
	task_item->task_dependencies.swap(task_dependencies);
	task_item->task_apartment = apartment != nullptr ? apartment : ks_apartment::default_mta();
	task_item->need_apply_value = need_apply_value;
	task_item->task_value_typeinfo = value_typeinfo;

	task_item->task_trigger_void = ks_raw_promise::create(task_item->task_apartment);
	task_item->task_trigger_void->get_future()->flat_then(
		[this_weak = std::weak_ptr<ks_raw_async_flow>(this->shared_from_this()), fn = std::move(fn)](const ks_raw_value& _) -> ks_raw_future_ptr {
			std::shared_ptr< ks_raw_async_flow> this_ptr = this_weak.lock();
			if (this_ptr == nullptr) {
				ASSERT(false);
				return ks_raw_future::rejected(ks_error::terminated_error(), ks_apartment::current_thread_apartment());
			}

			if (this_ptr->m_force_cleanup_flag_v) {
				return ks_raw_future::rejected(ks_error::terminated_error(), ks_apartment::current_thread_apartment());
			}

			if (this_ptr->m_flow_controller.check_cancel_all()) {
				return ks_raw_future::rejected(ks_error::cancelled_error(), ks_apartment::current_thread_apartment());
			}

			return fn(this_ptr);
		}, 
		make_async_context().bind_controller(&m_flow_controller).set_parent(context, true),
		task_item->task_apartment
	)->transform(
		[this_weak = std::weak_ptr<ks_raw_async_flow>(this->shared_from_this()), task_item](const ks_raw_result& task_result)->ks_raw_result {
			std::shared_ptr< ks_raw_async_flow> this_ptr = this_weak.lock();
			if (this_ptr == nullptr) {
				ASSERT(false);
				return ks_error::terminated_error();
			}

			std::unique_lock<ks_mutex> lock(this_ptr->m_mutex);
			this_ptr->do_make_task_completed_locked(task_item, task_result, lock);
			return task_result;
		},
		make_async_context().set_priority(0x10000), 
		task_item->task_apartment
	);

	m_task_map[task_item->task_name] = task_item;
	m_not_start_task_count++;
	return true;
}


uint64_t ks_raw_async_flow::add_flow_running_observer(ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow)>&& fn, const ks_async_context& context) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	if (m_flow_status_v != status_t::not_start || m_force_cleanup_flag_v) {
		ASSERT(false);
		return 0;
	}

	std::shared_ptr<_FLOW_OBSERVER_ITEM> observer_item = std::make_shared<_FLOW_OBSERVER_ITEM>();
	observer_item->kind = _x_observer_kind_t::for_running;
	observer_item->apartment = apartment != nullptr ? apartment : ks_apartment::default_mta();
	observer_item->on_flow_running_fn = std::move(fn);
	observer_item->observer_context = context;

	uint64_t observer_id = ++m_last_x_observer_id;
	m_flow_observer_map[observer_id] = observer_item;

	return observer_id;
}

uint64_t ks_raw_async_flow::add_flow_completed_observer(ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow, const ks_error& error)>&& fn, const ks_async_context& context) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	if (m_flow_status_v != status_t::not_start || m_force_cleanup_flag_v) {
		ASSERT(false);
		return 0;
	}

	std::shared_ptr<_FLOW_OBSERVER_ITEM> observer_item = std::make_shared<_FLOW_OBSERVER_ITEM>();
	observer_item->kind = _x_observer_kind_t::for_completed;
	observer_item->apartment = apartment != nullptr ? apartment : ks_apartment::default_mta();
	observer_item->on_flow_completed_fn = std::move(fn);
	observer_item->observer_context = context;

	uint64_t observer_id = ++m_last_x_observer_id;
	m_flow_observer_map[observer_id] = observer_item;

	return observer_id;
}

uint64_t ks_raw_async_flow::add_task_running_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow, const char* task_name)>&& fn, const ks_async_context& context) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	if (m_flow_status_v != status_t::not_start || m_force_cleanup_flag_v) {
		ASSERT(false);
		return 0;
	}

	std::shared_ptr<_TASK_OBSERVER_ITEM> observer_item = std::make_shared<_TASK_OBSERVER_ITEM>();
	observer_item->kind = _x_observer_kind_t::for_running;
	__do_pattern_to_regex(task_name_pattern, &observer_item->task_name_pattern_re);
	observer_item->apartment = apartment != nullptr ? apartment : ks_apartment::default_mta();
	observer_item->on_task_running_fn = std::move(fn);
	observer_item->observer_context = context;

	uint64_t observer_id = ++m_last_x_observer_id;
	m_task_observer_map[observer_id] = observer_item;

	return observer_id;
}

uint64_t ks_raw_async_flow::add_task_completed_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_raw_async_flow_ptr& flow, const char* task_name, const ks_error& error)>&& fn, const ks_async_context& context) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	if (m_flow_status_v != status_t::not_start || m_force_cleanup_flag_v) {
		ASSERT(false);
		return 0;
	}

	std::shared_ptr<_TASK_OBSERVER_ITEM> observer_item = std::make_shared<_TASK_OBSERVER_ITEM>();
	observer_item->kind = _x_observer_kind_t::for_completed;
	__do_pattern_to_regex(task_name_pattern, &observer_item->task_name_pattern_re);
	observer_item->apartment = apartment != nullptr ? apartment : ks_apartment::default_mta();
	observer_item->on_task_completed_fn = std::move(fn);
	observer_item->observer_context = context;

	uint64_t observer_id = ++m_last_x_observer_id;
	m_task_observer_map[observer_id] = observer_item;

	return observer_id;
}

void ks_raw_async_flow::remove_observer(uint64_t observer_id) {
	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_flow_observer_map.erase(observer_id) != 0)
		return;
	if (m_task_observer_map.erase(observer_id) != 0)
		return;
}


ks_raw_value ks_raw_async_flow::get_value(const char* key) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	auto it = m_raw_value_map.find(key);
	if (it == m_raw_value_map.cend()) {
		ASSERT(false);
		throw std::runtime_error("no such value");
	}

	return it->second;
}

void ks_raw_async_flow::put_custom_value(const char* key, const ks_raw_value& value) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	if (m_force_cleanup_flag_v && m_flow_status_v == status_t::not_start) {
		ASSERT(false);
		return;
	}

	m_raw_value_map[key] = value;
}


bool ks_raw_async_flow::start() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	return do_start_locked(lock);
}

void ks_raw_async_flow::try_cancel() {
	std::unique_lock<ks_mutex> lock(m_mutex);

	m_flow_controller.try_cancel_all();

	if (m_flow_status_v == status_t::not_start) {
		do_start_locked(lock); //自动start，使cancel生效
	}
}

void ks_raw_async_flow::__force_cleanup() {
	std::unique_lock<ks_mutex> lock(m_mutex);

	m_force_cleanup_flag_v = true;

	if (m_flow_status_v == status_t::not_start) {
		do_start_locked(lock); //自动start，使force_cleanup生效
	}
}

void ks_raw_async_flow::__wait() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_flow_status_v == status_t::succeeded || m_flow_status_v == status_t::failed)
		return;

	lock.unlock();
	return this->get_flow_future_void()->__wait();
}

bool ks_raw_async_flow::is_task_running(const char* task_name) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	auto it = m_task_map.find(task_name);
	if (it == m_task_map.cend()) {
		ASSERT(false);
		return false;
	}

	status_t task_status = it->second->task_status;
	return task_status == status_t::running;
}

bool ks_raw_async_flow::is_task_completed(const char* task_name) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	auto it = m_task_map.find(task_name);
	if (it == m_task_map.cend()) {
		ASSERT(false);
		return false;
	}

	status_t task_status = it->second->task_status;
	return task_status == status_t::succeeded || task_status == status_t::failed;
}

bool ks_raw_async_flow::is_flow_running() {
	std::unique_lock<ks_mutex> lock(m_mutex);

	status_t flow_status = m_flow_status_v;
	return flow_status == status_t::running;
}

bool ks_raw_async_flow::is_flow_completed() {
	std::unique_lock<ks_mutex> lock(m_mutex);

	status_t flow_status = m_flow_status_v;
	return flow_status == status_t::succeeded || flow_status == status_t::failed;
}

ks_error ks_raw_async_flow::get_last_error() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	return m_last_error;
}

std::string ks_raw_async_flow::get_last_failed_task_name() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	return m_1st_failed_task_name;
}

__ks_async_raw::ks_raw_result ks_raw_async_flow::peek_task_result(const char* task_name, const std::type_info* value_typeinfo) {
	std::unique_lock<ks_mutex> lock(m_mutex);
	auto it = m_task_map.find(task_name);
	if (it == m_task_map.cend()) {
		ASSERT(false);
		throw std::runtime_error("no such task");
	}

	const std::shared_ptr<_TASK_ITEM>& task_item = it->second;
	ASSERT(*task_item->task_value_typeinfo == *value_typeinfo || strcmp(task_item->task_value_typeinfo->name(), value_typeinfo->name()) == 0);
	return task_item->task_result;
}

ks_error ks_raw_async_flow::peek_task_error(const char* task_name) {
	std::unique_lock<ks_mutex> lock(m_mutex);
	auto it = m_task_map.find(task_name);
	if (it == m_task_map.cend()) {
		ASSERT(false);
		throw std::runtime_error("no such task");
	}

	const std::shared_ptr<_TASK_ITEM>& task_item = it->second;
	return task_item->task_result.is_error() ? task_item->task_result.to_error() : ks_error();
}

__ks_async_raw::ks_raw_future_ptr ks_raw_async_flow::get_task_future(const char* task_name, const std::type_info* value_typeinfo) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	if (m_force_cleanup_flag_v && m_flow_status_v == status_t::not_start) {
		ASSERT(false);
		return ks_raw_future::rejected(ks_error::unexpected_error(), ks_apartment::default_mta());
	}

	auto it = m_task_map.find(task_name);
	if (it == m_task_map.cend()) {
		ASSERT(false);
		return ks_raw_future::rejected(ks_error::unexpected_error(), ks_apartment::default_mta());
	}

	const std::shared_ptr<_TASK_ITEM>& task_item = it->second;
	ASSERT(*task_item->task_value_typeinfo == *value_typeinfo || strcmp(task_item->task_value_typeinfo->name(), value_typeinfo->name()) == 0);

	if (task_item->task_promise_opt == nullptr) {
		task_item->task_promise_opt = ks_raw_promise::create(task_item->task_apartment);
		if (task_item->task_status == status_t::succeeded || task_item->task_status == status_t::failed) {
			ASSERT(task_item->task_result.is_completed());
			task_item->task_promise_opt->try_settle(task_item->task_result);
		}
	}

	return task_item->task_promise_opt->get_future();
}

ks_raw_future_ptr ks_raw_async_flow::get_flow_future_void() {
	std::unique_lock<ks_mutex> lock(m_mutex);

	if (m_force_cleanup_flag_v && m_flow_status_v == status_t::not_start) {
		ASSERT(false);
		return ks_raw_future::rejected(ks_error::unexpected_error(), ks_apartment::default_mta());
	}

	if (m_flow_promise_void_opt == nullptr) {
		m_flow_promise_void_opt = ks_raw_promise::create(ks_apartment::default_mta());
		if (m_flow_status_v == status_t::succeeded || m_flow_status_v == status_t::failed) {
			if (m_last_error.get_code() == 0)
				m_flow_promise_void_opt->resolve(ks_raw_value::of_nothing());
			else
				m_flow_promise_void_opt->reject(m_last_error);
		}
	}

	return m_flow_promise_void_opt->get_future();
}

ks_future<ks_async_flow> ks_raw_async_flow::get_flow_future_this_wrapped() {
	std::unique_lock<ks_mutex> lock(m_mutex);

	if (m_force_cleanup_flag_v && m_flow_status_v == status_t::not_start) {
		ASSERT(false);
		return ks_async_flow::__wrap_raw_flow_future(ks_raw_future::rejected(ks_error::unexpected_error(), ks_apartment::default_mta()));
	}

	if (m_flow_promise_this_wrapped_keepper_until_completed != nullptr) {
		ASSERT(m_flow_promise_this_wrapped_keepper_until_completed == m_flow_promise_this_wrapped_weak.lock());
		return ks_async_flow::__wrap_raw_flow_future(m_flow_promise_this_wrapped_keepper_until_completed->get_future());
	}

	ks_raw_promise_ptr flow_promise_this_wrapped = m_flow_promise_this_wrapped_weak.lock();
	if (flow_promise_this_wrapped == nullptr) {
		flow_promise_this_wrapped = ks_raw_promise::create(ks_apartment::default_mta());
		m_flow_promise_this_wrapped_weak = flow_promise_this_wrapped;

		if (m_flow_status_v == status_t::succeeded || m_flow_status_v == status_t::failed) {
			if (m_last_error.get_code() == 0) {
				ks_async_flow flow_wrapped = ks_async_flow::__from_raw(this->shared_from_this());
				flow_promise_this_wrapped->resolve(ks_raw_value::of<ks_async_flow>(std::move(flow_wrapped)));
			}
			else {
				flow_promise_this_wrapped->reject(m_last_error);
			}

			ASSERT(m_flow_promise_this_wrapped_keepper_until_completed == nullptr);
		}
		else {
			m_flow_promise_this_wrapped_keepper_until_completed = flow_promise_this_wrapped;
		}
	}

	return ks_async_flow::__wrap_raw_flow_future(flow_promise_this_wrapped->get_future());
}


bool ks_raw_async_flow::do_start_locked(std::unique_lock<ks_mutex>& lock) {
	if (m_flow_status_v != status_t::not_start) {
		ASSERT(false);
		return false;
	}

	if (!m_task_map.empty()) {
		//check dep valid
		for (auto& entry : m_task_map) {
			const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
			for (auto& dep_name : task_item->task_dependencies) {
				if (m_task_map.find(dep_name) == m_task_map.cend()) {
					ASSERT(false);
					return false;
				}
			}
		}

		//decide task-levels
		if (true) {
			for (auto& entry : m_task_map) {
				const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
				task_item->task_level = 0;
			}

			size_t decided_task_count = 0;
			while (decided_task_count < m_task_map.size()) {
				const size_t decided_task_count_orig = decided_task_count;
				for (auto& entry : m_task_map) {
					const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
					if (task_item->task_level != 0)
						continue; //task level has been decided

					if (task_item->task_dependencies.empty()) {
						task_item->task_level = 1;
						decided_task_count++;
					}
					else {
						int task_level_temp = 0;
						bool any_undecided_dep_level_appeared = false;
						for (auto& dep_name : task_item->task_dependencies) {
							const std::shared_ptr<_TASK_ITEM>& dep_item = m_task_map.at(dep_name);
							if (dep_item->task_level <= 0) {
								any_undecided_dep_level_appeared = true;
								break;
							}
							else if (task_level_temp < dep_item->task_level + 1) {
								task_level_temp = dep_item->task_level + 1;
							}
						}

						if (!any_undecided_dep_level_appeared) {
							ASSERT(task_level_temp != 0);
							task_item->task_level = task_level_temp;

							decided_task_count++;
							if (decided_task_count == m_task_map.size())
								break;
						}
					}
				}

				if (decided_task_count == decided_task_count_orig) {
					ASSERT(false);
					return false;
				}
			}

			ASSERT(decided_task_count == m_task_map.size());
			ASSERT(std::find_if(
				m_task_map.cbegin(), m_task_map.cend(),
				[](const auto& entry) -> bool {
					return entry.second->task_level == 0;
				}) == m_task_map.cend());
		}

		//init task_waiting_for_dependencies
		for (auto& entry : m_task_map) {
			entry.second->task_waiting_for_dependencies.insert(
				entry.second->task_dependencies.cbegin(), entry.second->task_dependencies.cend());
		}
	}

	//do start
	do_make_flow_running_locked(lock);
	return true;
}

void ks_raw_async_flow::do_make_flow_running_locked(std::unique_lock<ks_mutex>& lock) {
	ASSERT(m_flow_status_v == status_t::not_start);

	ASSERT(m_self_running_keeper == nullptr);
	m_self_running_keeper = this->shared_from_this();

	m_flow_status_v = status_t::running;
	do_fire_flow_observers_locked(_x_observer_kind_t::for_running, ks_error(), lock);

	if (m_task_map.empty()) {
		do_make_flow_completed_locked(ks_error(), lock);
	}
	else {
		m_temp_pending_task_queue.reserve(m_task_map.size());

		for (auto& entry : m_task_map) {
			const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
			ASSERT(task_item->task_status == status_t::not_start);
			if (task_item->task_waiting_for_dependencies.empty()) {
				do_make_task_pending_locked(task_item, ks_raw_value::of_nothing(), lock);
			}
		}

		ASSERT(!m_temp_pending_task_queue.empty());
		do_drain_pending_task_queue_locked(lock);
	}
}

void ks_raw_async_flow::do_make_flow_completed_locked(const ks_error& flow_error, std::unique_lock<ks_mutex>& lock) {
	ASSERT(m_flow_status_v == status_t::running);

	m_flow_status_v = flow_error.get_code() == 0 ? status_t::succeeded : status_t::failed;

	if (flow_error.get_code() != 0 && m_last_error.get_code() == 0) {
		m_last_error = flow_error;
	}

	//settle flow-promise (void)
	if (m_flow_promise_void_opt != nullptr) {
		if (flow_error.get_code() == 0)
			m_flow_promise_this_wrapped_keepper_until_completed->resolve(ks_raw_value::of_nothing());
		else
			m_flow_promise_this_wrapped_keepper_until_completed->reject(flow_error);
	}

	//settle flow-promise (wrapped)
	if (m_flow_promise_this_wrapped_keepper_until_completed != nullptr) {
		ASSERT(m_flow_promise_this_wrapped_keepper_until_completed == m_flow_promise_this_wrapped_weak.lock());

		if (flow_error.get_code() == 0) {
			ks_async_flow flow_wrapped = ks_async_flow::__from_raw(this->shared_from_this());
			m_flow_promise_this_wrapped_keepper_until_completed->resolve(ks_raw_value::of<ks_async_flow>(std::move(flow_wrapped)));
		}
		else {
			m_flow_promise_this_wrapped_keepper_until_completed->reject(flow_error);
		}

		//release flow_promise_ext keeper, after completed
		m_flow_promise_this_wrapped_keepper_until_completed.reset();
	}

	//fire flow-observers
	if (flow_error.get_code() == 0)
		do_fire_flow_observers_locked(_x_observer_kind_t::for_completed, ks_error(), lock);
	else
		do_fire_flow_observers_locked(_x_observer_kind_t::for_completed, flow_error, lock);

	//cleanup if need
	if (m_force_cleanup_flag_v) {
		do_final_force_cleanup_value_refs_locked(lock);
	}

	//unkeep
	ASSERT(m_self_running_keeper != nullptr);
	m_self_running_keeper.reset();
}

void ks_raw_async_flow::do_make_task_pending_locked(const std::shared_ptr<_TASK_ITEM>& task_item, const ks_raw_result& arg_void, std::unique_lock<ks_mutex>& lock) {
	ASSERT(m_not_start_task_count != 0);
	ASSERT(task_item->task_waiting_for_dependencies.empty());
	ASSERT(task_item->task_status == status_t::not_start);
	ASSERT(arg_void.is_completed());

	m_not_start_task_count--;
	m_pending_task_count++;

	task_item->task_status = __not_start_but_pending_status;
	task_item->task_pending_arg_void = arg_void;
	m_temp_pending_task_queue.push_back(task_item);

	//任务的pending状态相当于not_start，不必fire
	//do_fire_task_observers_locked(task_item->task_name, task_item->task_status, ks_error(), lock);
}

void ks_raw_async_flow::do_make_task_running_locked(const std::shared_ptr<_TASK_ITEM>& task_item, std::unique_lock<ks_mutex>& lock) {
	ASSERT(m_pending_task_count != 0 && m_running_task_count < m_j);
	ASSERT(task_item->task_waiting_for_dependencies.empty());
	ASSERT(task_item->task_status == status_t::not_start || task_item->task_status == __not_start_but_pending_status);
	ASSERT(task_item->task_pending_arg_void.is_completed());

	if (task_item->task_status == status_t::not_start)
		m_not_start_task_count--;
	else if (task_item->task_status == __not_start_but_pending_status)
		m_pending_task_count--;
	else
		ASSERT(false);
	m_running_task_count++;

	task_item->task_status = status_t::running;
	do_fire_task_observers_locked(_x_observer_kind_t::for_running, task_item->task_name, ks_error(), lock);

	ASSERT(task_item->task_pending_arg_void.is_completed());
	task_item->task_trigger_void->try_settle(task_item->task_pending_arg_void);
}

void ks_raw_async_flow::do_make_task_completed_locked(const std::shared_ptr<_TASK_ITEM>& task_item, const ks_raw_result& task_result, std::unique_lock<ks_mutex>& lock) {
	ASSERT(task_result.is_completed());
	ASSERT(task_item->task_status == status_t::running);
	ASSERT(m_running_task_count != 0);

	//处置任务结果
	m_running_task_count--;
	if (task_result.is_value())
		m_succeeded_task_count++;
	else
		m_failed_task_count++;

	task_item->task_status = task_result.is_value() ? status_t::succeeded : status_t::failed;
	task_item->task_result = task_result;

	if (task_item->need_apply_value && task_result.is_value()) {
		ASSERT(m_raw_value_map.find(task_item->task_name) == m_raw_value_map.end());
		m_raw_value_map[task_item->task_name] = task_result.to_value();
	}

	if (task_result.is_error()) {
		if (m_1st_failed_task_name.empty())
			m_1st_failed_task_name = task_item->task_name;
		if (m_last_error.get_code() == 0) {
			m_last_error = task_result.to_error();
			if (m_last_error.get_code() == 0)
				m_last_error = ks_error::unexpected_error();
		}
	}

	//settle task-promise
	if (task_item->task_promise_opt != nullptr) {
		task_item->task_promise_opt->try_settle(task_result);
	}

	//fire task-observers
	if (task_result.is_value())
		do_fire_task_observers_locked(_x_observer_kind_t::for_completed, task_item->task_name, ks_error(), lock);
	else
		do_fire_task_observers_locked(_x_observer_kind_t::for_completed, task_item->task_name, task_result.to_error(), lock);

	//驱动下游任务
	if (m_not_start_task_count != 0) {
		for (auto& entry : m_task_map) {
			const std::shared_ptr<_TASK_ITEM>& next_task_item = entry.second;
			if (next_task_item->task_status != status_t::not_start)
				continue;

			ASSERT(!next_task_item->task_waiting_for_dependencies.empty());
			next_task_item->task_waiting_for_dependencies.erase(task_item->task_name);
			if (!next_task_item->task_waiting_for_dependencies.empty())
				continue;

			ks_raw_result arg_void;
			if (task_result.is_value())
				arg_void = ks_raw_value::of_nothing();
			else
				arg_void = task_result.to_error();

			do_make_task_pending_locked(next_task_item, arg_void, lock);
		}
	}

	do_drain_pending_task_queue_locked(lock);

	//若已无任何任务pending||running，但又有not-start的任务存在，则意味着这些not-start任务声明了根本不存在的上游，那么此时就让这些任务直接failed吧
	//注：理论上不会出现这样的情况，因为在start时已作过dep合法性检查，故这里只是额外的容错
	if (m_pending_task_count == 0 && m_running_task_count == 0 && m_not_start_task_count != 0) {
		ASSERT(false);

		for (auto& entry : m_task_map) {
			const std::shared_ptr<_TASK_ITEM>& next_task_item = entry.second;
			if (next_task_item->task_status != status_t::not_start)
				continue;

			do_make_task_pending_locked(next_task_item, ks_error::unexpected_error(), lock);
			if (m_not_start_task_count == 0)
				break;
		}
	}

	//若全部任务都已完成，则代表整个flow完成
	if (m_succeeded_task_count + m_failed_task_count == m_task_map.size()) {
		ks_error flow_error;
		if (m_failed_task_count != 0) {
			ASSERT(m_last_error.get_code() != 0);
			flow_error = m_last_error.get_code() != 0 ? m_last_error : ks_error::unexpected_error();
		}

		do_make_flow_completed_locked(flow_error, lock);
	}
}

void ks_raw_async_flow::do_drain_pending_task_queue_locked(std::unique_lock<ks_mutex>& lock) {
	if (!m_temp_pending_task_queue.empty() && m_running_task_count < m_j) {
		size_t c = m_temp_pending_task_queue.size();
		if (c > m_j - m_running_task_count)
			c = m_j - m_running_task_count;

		for (size_t i = 0; i < c; ++i) {
			do_make_task_running_locked(m_temp_pending_task_queue[i], lock);
		}

		m_temp_pending_task_queue.erase(m_temp_pending_task_queue.begin(), m_temp_pending_task_queue.begin() + c);
	}
}

void ks_raw_async_flow::do_fire_flow_observers_locked(_x_observer_kind_t kind, const ks_error& error, std::unique_lock<ks_mutex>& lock) {
	if (!m_flow_observer_map.empty()) {
		auto it = m_flow_observer_map.begin();
		while (it != m_flow_observer_map.end()) {
			const std::shared_ptr<_FLOW_OBSERVER_ITEM>& observer_item = it->second;
			if (observer_item->kind != kind) {
				++it;
				continue;
			}

			ks_raw_living_context_rtstt observer_context_rtstt;
			observer_context_rtstt.apply(observer_item->observer_context);

			if (observer_item->observer_context.__check_cancel_all_ctrl() || observer_item->observer_context.__check_owner_expired()) {
				it = m_flow_observer_map.erase(it);
				continue;
			}

			observer_item->apartment->schedule(
				[this_ptr = this->shared_from_this(), error, observer_item, observer_owner_locker = observer_context_rtstt.get_owner_locker()]() {
					switch (observer_item->kind) {
					case _x_observer_kind_t::for_running:
						observer_item->on_flow_running_fn(this_ptr);
						break;
					case _x_observer_kind_t::for_completed:
						observer_item->on_flow_completed_fn(this_ptr, error);
						break;
					}
				}, observer_item->observer_context.__get_priority());

			++it;
		}
	}
}

void ks_raw_async_flow::do_fire_task_observers_locked(_x_observer_kind_t kind, const std::string& task_name, const ks_error& error, std::unique_lock<ks_mutex>& lock) {
	if (!m_task_observer_map.empty()) {
		auto it = m_task_observer_map.begin();
		while (it != m_task_observer_map.end()) {
			const std::shared_ptr<_TASK_OBSERVER_ITEM>& observer_item = it->second;
			if (observer_item->kind != kind) {
				++it;
				continue;
			}
			if (!std::regex_match(task_name, observer_item->task_name_pattern_re)) {
				++it;
				continue;
			}

			ks_raw_living_context_rtstt observer_context_rtstt;
			observer_context_rtstt.apply(observer_item->observer_context);

			if (observer_item->observer_context.__check_cancel_all_ctrl() || observer_item->observer_context.__check_owner_expired()) {
				it = m_task_observer_map.erase(it);
				continue;
			}

			observer_item->apartment->schedule(
				[this_ptr = this->shared_from_this(), task_name, error, observer_item, observer_owner_locker = observer_context_rtstt.get_owner_locker()]() {
					switch (observer_item->kind) {
					case _x_observer_kind_t::for_running:
						observer_item->on_task_running_fn(this_ptr, task_name.c_str());
						break;
					case _x_observer_kind_t::for_completed:
						observer_item->on_task_completed_fn(this_ptr, task_name.c_str(), error);
						break;
					}
				}, observer_item->observer_context.__get_priority());

			++it;
		}
	}
}

void ks_raw_async_flow::do_final_force_cleanup_value_refs_locked(std::unique_lock<ks_mutex>& lock) {
	ASSERT(m_force_cleanup_flag_v);
	ASSERT(m_flow_status_v == status_t::succeeded || m_flow_status_v == status_t::failed);

	//注：我们要做的就是将所有直接和间接持有的 “值对象” 全部清除掉
	for (auto& entry : m_task_map) {
		if (entry.second->task_result.is_value()) {
			entry.second->task_result = ks_error::terminated_error();
			entry.second->task_promise_opt.reset();
		}
	}

	m_raw_value_map.clear();

	m_task_observer_map.clear();
	m_flow_observer_map.clear();
}


__KS_ASYNC_RAW_END
