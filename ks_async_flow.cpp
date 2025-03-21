#include "ks_async_flow.h"
#include "ks-async-raw/ks_raw_internal_helper.h"


static void __do_string_split(const char* str, std::vector<std::string>* out) {
	size_t len = str != nullptr ? strlen(str) : 0;
	if (len == 0)
		return;

	size_t pos = 0;
	while (pos < len) {
		size_t pos_end = pos;
		while (pos_end < len && strchr(",; \t", str[pos_end]) == nullptr)
			pos_end++;

		if (pos < pos_end) {
			out->push_back(std::string(str + pos, pos_end - pos));
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
	__do_string_split(pattern, &pattern_vec);
	for (const std::string& pattern_item : pattern_vec) {
		ASSERT(!pattern_item.empty());

		if (pattern_re_lit.empty()) {
			pattern_re_lit.append("|");
		}

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


ks_async_flow::ks_async_flow(__raw_ctor) {
	m_flow_promise = ks_promise<void>::create();
}

ks_async_flow_ptr ks_async_flow::create() {
	return std::make_shared<ks_async_flow>(__raw_ctor::v);
}


void ks_async_flow::set_j(size_t j) {
	m_j = j != 0 ? j : size_t(-1);
}

bool ks_async_flow::do_add_task(
	const char* task_name, const char* task_dependencies,
	ks_apartment* apartment, const std::function<ks_future<ks_raw_value>()>& eval_fn, const ks_async_context& context) {

	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_flow_status != status_t::not_start) {
		ASSERT(false);
		return false;
	}

	std::shared_ptr<_TASK_ITEM> task_item = std::make_shared<_TASK_ITEM>();
	task_item->task_name = task_name;
	__do_string_split(task_dependencies, &task_item->task_dependencies);

	task_item->task_apartment = apartment;
	task_item->task_eval_fn = eval_fn;
	task_item->task_context = context;

	if (task_item->task_name.find_first_of(",; \t") != size_t(-1)) {
		ASSERT(false);
		return false;
	}
	if (m_task_map.find(task_item->task_name) != m_task_map.cend()) {
		ASSERT(false);
		return false;
	}

	task_item->task_trigger = ks_promise<void>::create();
	task_item->task_trigger.get_future().then<ks_raw_value>(
		task_item->task_apartment,
		task_item->task_eval_fn,
		task_item->task_context
		).on_completion(
			task_item->task_apartment,
			[this, this_ptr = this->shared_from_this(), task_item](const ks_result<ks_raw_value>& task_result) {
		std::unique_lock<ks_mutex> lock(m_mutex);
		do_make_task_completed_locked(task_item, task_result, lock);
	},
			ks_async_context().set_priority(0x10000)
		);

	m_task_map[task_item->task_name] = task_item;
	m_not_start_task_count++;
	return true;
}


uint64_t ks_async_flow::add_flow_observer(ks_apartment* apartment, std::function<void(const ks_async_flow_ptr& flow, status_t flow_status)>&& observer_fn, const ks_async_context& context) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	std::shared_ptr<_FLOW_OBSERVER_ITEM> observer_item = std::make_shared<_FLOW_OBSERVER_ITEM>();
	observer_item->apartment = apartment;
	observer_item->observer_fn = std::move(observer_fn);
	observer_item->context = context;

	uint64_t observer_id = ++m_last_x_observer_id;
	m_flow_observer_map[observer_id] = observer_item;

	return observer_id;
}

uint64_t ks_async_flow::add_task_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_async_flow_ptr& flow, const char* task_name, status_t task_status)>&& observer_fn, const ks_async_context& context) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	std::shared_ptr<_TASK_OBSERVER_ITEM> observer_item = std::make_shared<_TASK_OBSERVER_ITEM>();
	__do_pattern_to_regex(task_name_pattern, &observer_item->task_name_pattern_re);
	observer_item->apartment = apartment;
	observer_item->observer_fn = std::move(observer_fn);
	observer_item->context = context;

	uint64_t observer_id = ++m_last_x_observer_id;
	m_task_observer_map[observer_id] = observer_item;

	return observer_id;
}

void ks_async_flow::remove_observer(uint64_t observer_id) {
	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_task_observer_map.erase(observer_id) != 0)
		return;
	if (m_flow_observer_map.erase(observer_id) != 0)
		return;
}


bool ks_async_flow::start() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_flow_status != status_t::not_start)
		return false;

	if (!m_task_map.empty()) {
		//check dep valid
		for (auto& entry : m_task_map) {
			const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
			for (auto& dep : task_item->task_dependencies) {
				auto dep_it = m_task_map.find(dep);
				if (dep_it == m_task_map.cend())
					return false;
			}
		}

		//check depend loop ...
		//init level
		size_t initial_task_count = 0;
		for (auto& entry : m_task_map) {
			const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
			task_item->task_level = 0;
			if (task_item->task_dependencies.empty()) {
				task_item->task_level = 1;
				initial_task_count++;
			}
		}

		if (initial_task_count == 0) {
			ASSERT(false);
			return false;
		}

		//grow level
		if (initial_task_count < m_task_map.size()) {
			while (true) {
				bool some_level_changed = false;
				for (auto& entry : m_task_map) {
					const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
					for (auto& dep_name : task_item->task_dependencies) {
						const std::shared_ptr<_TASK_ITEM>& dep_item = m_task_map[dep_name];
						if (dep_item->task_level == 0)
							continue;
						if (task_item->task_level < dep_item->task_level + 1) {
							task_item->task_level = dep_item->task_level + 1;
							some_level_changed = true;
							if (task_item->task_level > (int)m_task_map.size() * 2)
								return false; //level overflow
						}
					}
				}

				if (!some_level_changed)
					break;
			}

			//check isolated task
			if (std::find_if(m_task_map.cbegin(), m_task_map.cend(),
				[](auto& entry) -> bool { return entry.second->task_level == 0; }) != m_task_map.cend())
				return false;
		}
	}

	//do start
	do_make_flow_running_locked(lock);
	return true;
}

void ks_async_flow::try_cancel() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	m_flow_cancelled_flag_v = true;
}

void ks_async_flow::wait() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	m_flow_promise.get_future().wait();
}

bool ks_async_flow::reset() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_flow_status == status_t::not_start) {
		return true;
	}

	if (m_flow_status != status_t::succeeded && m_flow_status != status_t::failed) {
		ASSERT(false);
		return false;
	}

	m_flow_status = status_t::not_start;

	m_not_start_task_count = m_task_map.size();
	m_pending_task_count = 0;
	m_running_task_count = 0;
	m_succeeded_task_count = 0;
	m_failed_task_count = 0;
	m_temp_pending_task_queue.clear();

	m_flow_cancelled_flag_v = false;
	m_early_failed_task_item = nullptr;

	for (auto& entry : m_task_map) {
		const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
		if (task_item->task_status == status_t::not_start) {
			continue;
		}

		ASSERT(task_item->task_status != status_t::running);

		task_item->task_level = 0;
		task_item->task_waiting_for_dependencies.insert(task_item->task_dependencies.cbegin(), task_item->task_dependencies.cend());

		task_item->task_status = status_t::not_start;
		task_item->task_result = ks_result<ks_raw_value>();

		task_item->task_trigger = ks_promise<void>::create();
		task_item->task_trigger.get_future().then<ks_raw_value>(
			task_item->task_apartment,
			task_item->task_eval_fn,
			task_item->task_context
			).on_completion(
				task_item->task_apartment,
				[this, this_ptr = this->shared_from_this(), task_item](const ks_result<ks_raw_value>& task_result) {
					std::unique_lock<ks_mutex> lock(m_mutex);
					do_make_task_completed_locked(task_item, task_result, lock);
				},
				ks_async_context().set_priority(0x10000)
			);
	}

	m_flow_promise = ks_promise<void>::create();

	if (!m_user_data_map.empty()) {
		auto it = m_user_data_map.begin();
		while (it != m_user_data_map.end()) {
			if (it->first.empty() || it->first[0] != '.') {
				it = m_user_data_map.erase(it);
			}
			else {
				++it;
			}
		}
	}

	return true;
}

ks_async_flow_ptr ks_async_flow::spawn() {
	std::unique_lock<ks_mutex> lock(m_mutex);

	ks_async_flow_ptr spawn_flow = ks_async_flow::create();

	spawn_flow->m_j = m_j;

	for (auto& entry : m_task_map) {
		const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
		const std::shared_ptr<_TASK_ITEM>& spawn_task_item = std::make_shared<_TASK_ITEM>();
		spawn_task_item->task_name = task_item->task_name;
		spawn_task_item->task_dependencies = task_item->task_dependencies;
		spawn_task_item->task_apartment = task_item->task_apartment;
		spawn_task_item->task_eval_fn = task_item->task_eval_fn;
		spawn_task_item->task_context = task_item->task_context;

		spawn_task_item->task_level = task_item->task_level;
		spawn_task_item->task_waiting_for_dependencies.insert(spawn_task_item->task_dependencies.cbegin(), spawn_task_item->task_dependencies.cend());

		spawn_task_item->task_trigger = ks_promise<void>::create();
		spawn_task_item->task_trigger.get_future().then<ks_raw_value>(
			spawn_task_item->task_apartment,
			spawn_task_item->task_eval_fn,
			spawn_task_item->task_context
			).on_completion(
				spawn_task_item->task_apartment,
				[spawn_flow, spawn_task_item](const ks_result<ks_raw_value>& task_result) {
					std::unique_lock<ks_mutex> lock(spawn_flow.get()->m_mutex);
					spawn_flow.get()->do_make_task_completed_locked(spawn_task_item, task_result, lock);
				},
				ks_async_context().set_priority(0x10000)
			);

		spawn_flow.get()->m_task_map[spawn_task_item->task_name] = spawn_task_item;
	}

	spawn_flow.get()->m_not_start_task_count = m_task_map.size();

	spawn_flow.get()->m_user_data_map = m_user_data_map;

	return spawn_flow;
}


bool ks_async_flow::is_flow_completed() {
	std::unique_lock<ks_mutex> lock(m_mutex);

	status_t flow_status = m_flow_status;
	return flow_status == status_t::succeeded || flow_status == status_t::failed;
}

bool ks_async_flow::is_task_completed(const char* task_name) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	auto it = m_task_map.find(task_name);
	if (it == m_task_map.cend()) {
		ASSERT(false);
		return false;
	}

	status_t task_status = it->second->task_status;
	return task_status == status_t::succeeded || task_status == status_t::failed;
}

ks_async_flow::status_t ks_async_flow::get_flow_status() {
	std::unique_lock<ks_mutex> lock(m_mutex);

	status_t flow_status = m_flow_status;
	if (flow_status == __not_start_but_pending_status)
		flow_status = status_t::running;

	return flow_status;
}

ks_async_flow::status_t ks_async_flow::get_task_status(const char* task_name) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	auto it = m_task_map.find(task_name);
	if (it == m_task_map.cend()) {
		ASSERT(false);
		return status_t::not_start;
	}

	status_t task_status = it->second->task_status;
	if (task_status == __not_start_but_pending_status)
		task_status = status_t::not_start;

	return task_status;
}

std::string ks_async_flow::get_failed_task_name() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_early_failed_task_item == nullptr) {
		ASSERT(false);
		return "";
	}
	return m_early_failed_task_item->task_name;
}

ks_error ks_async_flow::get_failed_task_error() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_early_failed_task_item == nullptr) {
		ASSERT(false);
		return ks_error();
	}
	if (!m_early_failed_task_item->task_result.is_error()) {
		ASSERT(false);
		return ks_error::unexpected_error();
	}
	return m_early_failed_task_item->task_result.to_error();
}

ks_future<void> ks_async_flow::get_flow_future() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	return m_flow_promise.get_future();
}


void ks_async_flow::do_make_flow_running_locked(std::unique_lock<ks_mutex>& lock) {
	ASSERT(m_flow_status == status_t::not_start);

	m_flow_status = status_t::running;
	do_fire_flow_observers_locked(m_flow_status, lock);

	if (!m_task_map.empty()) {
		m_temp_pending_task_queue.reserve(m_task_map.size());

		for (auto& entry : m_task_map) {
			const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
			ASSERT(task_item->task_status == status_t::not_start);
			if (task_item->task_waiting_for_dependencies.empty()) {
				do_make_task_pending_locked(task_item, ks_result<void>(nothing), lock);
			}
		}

		ASSERT(!m_temp_pending_task_queue.empty());
		do_drain_pending_task_queue_locked(lock);
	}
	else {
		do_make_flow_completed_locked(lock);
	}
}

void ks_async_flow::do_make_flow_completed_locked(std::unique_lock<ks_mutex>& lock) {
	ASSERT(m_flow_status == status_t::running);

	m_flow_status = m_failed_task_count == 0 ? status_t::succeeded : status_t::failed;
	do_fire_flow_observers_locked(m_flow_status, lock);

	if (m_flow_status == status_t::succeeded)
		m_flow_promise.resolve();
	else
		m_flow_promise.reject(m_early_failed_task_item != nullptr ? m_early_failed_task_item->task_result.to_error() : ks_error::unexpected_error());
}

void ks_async_flow::do_make_task_pending_locked(const std::shared_ptr<_TASK_ITEM>& task_item, const ks_result<void>& arg, std::unique_lock<ks_mutex>& lock) {
	ASSERT(m_not_start_task_count != 0);
	ASSERT(task_item->task_waiting_for_dependencies.empty());
	ASSERT(task_item->task_status == status_t::not_start);
	ASSERT(arg.is_completed());

	m_not_start_task_count--;
	m_pending_task_count++;

	task_item->task_status = __not_start_but_pending_status;
	task_item->task_pending_arg = arg;
	m_temp_pending_task_queue.push_back(task_item);

	//任务的pending状态相当于not_start，不必fire
	//do_fire_task_observers_locked(task_item->task_name, task_item->task_status, lock);
}

void ks_async_flow::do_make_task_running_locked(const std::shared_ptr<_TASK_ITEM>& task_item, std::unique_lock<ks_mutex>& lock) {
	ASSERT(m_pending_task_count != 0 && m_running_task_count < m_j);
	ASSERT(task_item->task_waiting_for_dependencies.empty());
	ASSERT(task_item->task_status == status_t::not_start || task_item->task_status == __not_start_but_pending_status);
	ASSERT(task_item->task_pending_arg.is_completed());

	if (task_item->task_status == status_t::not_start)
		m_not_start_task_count--;
	else if (task_item->task_status == __not_start_but_pending_status)
		m_pending_task_count--;
	else
		ASSERT(false);
	m_running_task_count++;

	task_item->task_status = status_t::running;

	do_fire_task_observers_locked(task_item->task_name, task_item->task_status, lock);

	if (task_item->task_pending_arg.is_value())
		task_item->task_trigger.resolve();
	else
		task_item->task_trigger.reject(task_item->task_pending_arg.to_error());
}

void ks_async_flow::do_make_task_completed_locked(const std::shared_ptr<_TASK_ITEM>& task_item, const ks_result<ks_raw_value>& task_result, std::unique_lock<ks_mutex>& lock) {
	ASSERT(m_running_task_count != 0);
	ASSERT(task_item->task_status == status_t::running);

	//处置任务结果
	m_running_task_count--;
	m_succeeded_task_count++;

	task_item->task_status = task_result.is_value() ? status_t::succeeded : status_t::failed;
	task_item->task_result = task_result;

	if (task_result.is_error() && m_early_failed_task_item == nullptr) {
		m_early_failed_task_item = task_item;
	}

	do_fire_task_observers_locked(task_item->task_name, task_item->task_status, lock);

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

			ks_result<void> arg = m_early_failed_task_item != nullptr ? ks_result<void>(m_early_failed_task_item->task_result.to_error()) : ks_result<void>(nothing);
			do_make_task_pending_locked(next_task_item, arg, lock);
		}
	}

	do_drain_pending_task_queue_locked(lock);

	//若已无任何任务pending||running，但又有not-start的任务存在，则意味着这些not-start任务声明了根本不存在的上游，那么此时就让这些任务直接failed吧
	if (m_pending_task_count == 0 && m_running_task_count == 0 && m_not_start_task_count != 0) {
		ASSERT(false);

		for (auto& entry : m_task_map) {
			const std::shared_ptr<_TASK_ITEM>& next_task_item = entry.second;
			if (next_task_item->task_status != status_t::not_start)
				continue;

			do_make_task_pending_locked(next_task_item, ks_error::was_cancelled_error(), lock);
			if (m_not_start_task_count == 0)
				break;
		}
	}

	//若全部任务都已完成，则代表整个flow完成
	if (m_succeeded_task_count + m_failed_task_count == m_task_map.size()) {
		do_make_flow_completed_locked(lock);
	}
}

void ks_async_flow::do_drain_pending_task_queue_locked(std::unique_lock<ks_mutex>& lock) {
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

void ks_async_flow::do_fire_flow_observers_locked(status_t flow_status, std::unique_lock<ks_mutex>& lock) {
	if (!m_flow_observer_map.empty()) {
		auto it = m_flow_observer_map.begin();
		while (it != m_flow_observer_map.end()) {
			const std::shared_ptr<_FLOW_OBSERVER_ITEM>& observer_item = it->second;

			ks_raw_living_context_rtstt observer_context_rtstt;
			observer_context_rtstt.apply(observer_item->context);

			if (observer_item->context.__check_cancel_all_ctrl() || observer_item->context.__check_owner_expired()) {
				it = m_flow_observer_map.erase(it);
				continue;
			}

			observer_item->apartment->schedule(
				[this, this_ptr = this->shared_from_this(), flow_status, observer_item, observer_owner_locker = observer_context_rtstt.get_owner_locker()]() {
					observer_item->observer_fn(this_ptr, flow_status);
				}, observer_item->context.__get_priority());

			++it;
		}
	}
}

void ks_async_flow::do_fire_task_observers_locked(const std::string& task_name, status_t task_status, std::unique_lock<ks_mutex>& lock) {
	if (!m_task_observer_map.empty()) {
		auto it = m_task_observer_map.begin();
		while (it != m_task_observer_map.end()) {
			const std::shared_ptr<_TASK_OBSERVER_ITEM>& observer_item = it->second;
			if (!std::regex_match(task_name, observer_item->task_name_pattern_re))
				continue;

			ks_raw_living_context_rtstt observer_context_rtstt;
			observer_context_rtstt.apply(observer_item->context);

			if (observer_item->context.__check_cancel_all_ctrl() || observer_item->context.__check_owner_expired()) {
				it = m_task_observer_map.erase(it);
				continue;
			}

			observer_item->apartment->schedule(
				[this, this_ptr = this->shared_from_this(), task_name, task_status, observer_item, observer_owner_locker = observer_context_rtstt.get_owner_locker()]() {
				observer_item->observer_fn(this_ptr, task_name.c_str(), task_status);
			}, observer_item->context.__get_priority());

			++it;
		}
	}
}
