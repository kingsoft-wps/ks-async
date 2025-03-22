#include "ks_async_flow.h"
#include "ks-async-raw/ks_raw_internal_helper.h"


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


ks_async_flow::ks_async_flow(__raw_ctor) {
	m_flow_promise = ks_promise<void>::create();
}

ks_async_flow_ptr ks_async_flow::create() {
	return std::make_shared<ks_async_flow>(__raw_ctor::v);
}


void ks_async_flow::set_default_apartment(ks_apartment* apartment) {
	std::unique_lock<ks_mutex> lock(m_mutex);
	m_default_apartment = apartment != nullptr ? apartment : ks_apartment::default_mta();
}

void ks_async_flow::set_j(size_t j) {
	std::unique_lock<ks_mutex> lock(m_mutex);
	m_j = j != 0 ? j : size_t(-1);
}

bool ks_async_flow::do_add_task_raw(
	const char* name_and_dependencies, const std::type_info* result_value_typeinfo,
	ks_apartment* apartment, std::function<ks_future<ks_raw_value>()>&& eval_fn, const ks_async_context& context) {

	if (name_and_dependencies == nullptr || name_and_dependencies[0] == 0) {
		ASSERT(false);
		return true;
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

		for (auto dep_name : task_dependencies) {
			if (!__do_check_name_legal(dep_name.c_str(), false)) {
				ASSERT(false);
				return false;
			}
			if (dep_name == task_name) {
				ASSERT(false);
				return false;
			}
		}
	}


	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_flow_status != status_t::not_start) {
		ASSERT(false);
		return false;
	}
	if (m_task_map.find(task_name) != m_task_map.cend()) {
		ASSERT(false);
		return false;
	}

	std::shared_ptr<_TASK_ITEM> task_item = std::make_shared<_TASK_ITEM>();
	task_item->task_name.swap(task_name);
	task_item->task_dependencies.swap(task_dependencies);

#ifdef _DEBUG
	task_item->task_result_value_typeinfo = result_value_typeinfo;
#endif

	task_item->task_apartment = apartment;
	task_item->task_eval_fn = std::move(eval_fn);
	task_item->task_context = context;

	m_task_map[task_item->task_name] = task_item;
	m_not_start_task_count++;
	return true;
}


inline ks_apartment* ks_async_flow::do_sel_apartment(ks_apartment* apartment) {
	return apartment != nullptr ? apartment : ks_apartment::default_mta();
}

inline std::function<ks_future<ks_async_flow::ks_raw_value>()> ks_async_flow::do_wrap_task_eval_fn(const std::shared_ptr<_TASK_ITEM>& task_item) {
	return [this, this_ptr = this->shared_from_this(), task_item]() -> ks_future<ks_async_flow::ks_raw_value> {
		if (m_flow_cancelled_flag_v)
			return ks_future<ks_raw_value>::rejected(ks_error::was_cancelled_error());

		ks_raw_living_context_rtstt context_rtstt;
		context_rtstt.apply(task_item->task_context);

		if (task_item->task_context.__check_cancel_all_ctrl() || task_item->task_context.__check_owner_expired())
			return ks_future<ks_raw_value>::rejected(ks_error::was_cancelled_error());

		return task_item->task_eval_fn();
	};
}


uint64_t ks_async_flow::add_flow_observer(ks_apartment* apartment, std::function<void(const ks_async_flow_ptr& flow, status_t flow_status)>&& observer_fn, const ks_async_context& context) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	std::shared_ptr<_FLOW_OBSERVER_ITEM> observer_item = std::make_shared<_FLOW_OBSERVER_ITEM>();
	observer_item->observer_apartment = apartment;
	observer_item->observer_fn = std::move(observer_fn);
	observer_item->observer_context = context;

	uint64_t observer_id = ++m_last_x_observer_id;
	m_flow_observer_map[observer_id] = observer_item;

	return observer_id;
}

uint64_t ks_async_flow::add_task_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_async_flow_ptr& flow, const char* task_name, status_t task_status)>&& observer_fn, const ks_async_context& context) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	std::shared_ptr<_TASK_OBSERVER_ITEM> observer_item = std::make_shared<_TASK_OBSERVER_ITEM>();
	__do_pattern_to_regex(task_name_pattern, &observer_item->task_name_pattern_re);
	observer_item->observer_apartment = apartment;
	observer_item->observer_fn = std::move(observer_fn);
	observer_item->observer_context = context;

	uint64_t observer_id = ++m_last_x_observer_id;
	m_task_observer_map[observer_id] = observer_item;

	return observer_id;
}

bool ks_async_flow::remove_observer(uint64_t observer_id) {
	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_flow_observer_map.erase(observer_id) != 0)
		return true;
	if (m_task_observer_map.erase(observer_id) != 0)
		return true;
	return false;
}


bool ks_async_flow::start() {
	std::unique_lock<ks_mutex> lock(m_mutex);
	if (m_flow_status != status_t::not_start) {
		ASSERT(false);
		return false;
	}

	if (!m_task_map.empty()) {
		//check task deps valid
		for (auto& entry : m_task_map) {
			const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
			for (auto& dep : task_item->task_dependencies) {
				auto dep_it = m_task_map.find(dep);
				if (dep_it == m_task_map.cend()) {
					ASSERT(false);
					return false;
				}
			}
		}

		//check task deps loop ...
		//init task levels
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

		//grow task levels
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
							if (task_item->task_level > (int)m_task_map.size() * 2) {
								ASSERT(false);
								return false; //level overflow
							}
						}
					}
				}

				if (!some_level_changed)
					break;
			}

			//check isolated task
			if (std::find_if(m_task_map.cbegin(), m_task_map.cend(),
				[](auto& entry) -> bool { return entry.second->task_level == 0; }) != m_task_map.cend()) {
				ASSERT(false);
				return false;
			}
		}

		//build task triggers
		for (auto& entry : m_task_map) {
			const std::shared_ptr<_TASK_ITEM>& task_item = entry.second;
			task_item->task_trigger = ks_promise<void>::create();
			task_item->task_trigger.get_future().then<ks_raw_value>(
				do_sel_apartment(task_item->task_apartment),
				do_wrap_task_eval_fn(task_item),
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

		task_item->task_trigger = nullptr;
	}

	m_flow_promise = ks_promise<void>::create();

	if (!m_user_data_map.empty()) {
		std::unordered_map<std::string, ks_any> user_data_map_bk;
		user_data_map_bk.swap(m_user_data_map);

		m_user_data_map.clear();

		for (auto& entry : user_data_map_bk) {
			if (!entry.first.empty() && entry.first[0] == '.')
				m_user_data_map.insert(entry);
		}
	}

	return true;
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
			observer_context_rtstt.apply(observer_item->observer_context);

			if (observer_item->observer_context.__check_cancel_all_ctrl() || observer_item->observer_context.__check_owner_expired()) {
				it = m_flow_observer_map.erase(it);
				continue;
			}

			do_sel_apartment(observer_item->observer_apartment)->schedule(
				[this, this_ptr = this->shared_from_this(), flow_status, observer_item, observer_owner_locker = observer_context_rtstt.get_owner_locker()]() {
					observer_item->observer_fn(this_ptr, flow_status);
				}, observer_item->observer_context.__get_priority());

			++it;
		}
	}
}

void ks_async_flow::do_fire_task_observers_locked(const std::string& task_name, status_t task_status, std::unique_lock<ks_mutex>& lock) {
	if (!m_task_observer_map.empty()) {
		auto it = m_task_observer_map.begin();
		while (it != m_task_observer_map.end()) {
			const std::shared_ptr<_TASK_OBSERVER_ITEM>& observer_item = it->second;
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

			do_sel_apartment(observer_item->observer_apartment)->schedule(
				[this, this_ptr = this->shared_from_this(), task_name, task_status, observer_item, observer_owner_locker = observer_context_rtstt.get_owner_locker()]() {
				observer_item->observer_fn(this_ptr, task_name.c_str(), task_status);
			}, observer_item->observer_context.__get_priority());

			++it;
			continue;
		}
	}
}
