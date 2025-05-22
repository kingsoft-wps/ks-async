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

#include "ks_notification_center.h"
#include "ks_thread_pool_apartment_imp.h"
#include "ks-async-raw/ks_raw_internal_helper.hpp"
#include "ktl/ks_concurrency.h"
#include <string>
#include <deque>
#include <list>
#include <map>
#include <string.h>
#include <algorithm>

void __forcelink_to_ks_notification_center_cpp() {}


static std::atomic<uint64_t> g_last_reg_id{ 0 };


class ks_notification_center::__ks_notification_center_data final {
public:
	__ks_notification_center_data(const char* center_name) { ASSERT(center_name != nullptr); m_center_name = center_name; }
	_DISABLE_COPY_CONSTRUCTOR(__ks_notification_center_data);

public:
	const char* name();

public:
	uint64_t add_observer(
		const void* observer, const char* notification_name_pattern,
		std::function<void(const ks_notification&)>&& fn, const ks_async_context& context, ks_apartment* apartment);

	void remove_observer(const void* observer, uint64_t id);
	void remove_observer(const void* observer);

public:
	void do_post_notification(const ks_notification& notification);

private:
	using _EXPANDED_NOTIFICATION_NAME_SEQ = std::deque<std::string>;

	struct _PARSED_NOTIFICATION_NAME_PATTERN {
		std::string base_name;
		bool is_wildcard_mode = false;
	};

private:
	struct _ENTRY_DATA : private _PARSED_NOTIFICATION_NAME_PATTERN {
		using _PARSED_NOTIFICATION_NAME_PATTERN::base_name;
		using _PARSED_NOTIFICATION_NAME_PATTERN::is_wildcard_mode;
		uint64_t reg_id = 0;
		const void* observer = nullptr;
		std::function<void(const ks_notification&)> fn;
		ks_async_context context;
		ks_apartment* apartment = nullptr;
		volatile bool removed_flag_v = false;

		void init_parsed_notification_name_pattern(const _PARSED_NOTIFICATION_NAME_PATTERN& v) { *static_cast<_PARSED_NOTIFICATION_NAME_PATTERN*>(this) = v; }
	};

	using _ENTRY_PTR_LIST = std::list<std::shared_ptr<_ENTRY_DATA>>;

	struct _GROUP_DATA {
		_ENTRY_PTR_LIST entry_ptr_list;

		bool is_group_empty() { return this->entry_ptr_list.empty(); }
	};

	using _GROUP_MAP = std::map<std::string, _GROUP_DATA, std::less<std::string>>; //key is notification's base-name

	struct _OBSERVER_DATA {
		_GROUP_MAP his_group_map;

		bool is_observer_empty() { return this->his_group_map.empty(); }
	};

	using _OBSERVER_MAP = std::map<const void*, _OBSERVER_DATA>;

	struct _INDEXING_BUNDLE {
		_GROUP_MAP exact_high_group_map;
		_GROUP_MAP exact_normal_group_map;
		_GROUP_MAP exact_low_group_map;
		_GROUP_MAP wildcard_high_group_map;
		_GROUP_MAP wildcard_normal_group_map;
		_GROUP_MAP wildcard_low_group_map;

		_GROUP_MAP& select_indexing_group_map(int entry_priority, bool is_wildcard_mode) {
			if (entry_priority == 0)
				return is_wildcard_mode ? this->wildcard_normal_group_map : this->exact_normal_group_map;
			else if (entry_priority > 0)
				return is_wildcard_mode ? this->wildcard_high_group_map : this->exact_high_group_map;
			else
				return is_wildcard_mode ? this->wildcard_low_group_map : this->exact_low_group_map;
		}
	};

private:
	void do_erase_entry_from_observer_map_locked(const std::shared_ptr<_ENTRY_DATA>& entry_ptr, std::unique_lock<ks_mutex>& lock);
	void do_erase_entry_from_indexing_bundle_locked(const std::shared_ptr<_ENTRY_DATA>& entry_ptr, std::unique_lock<ks_mutex>& lock);

	static _PARSED_NOTIFICATION_NAME_PATTERN do_parse_notification_name_pattern(const char* notification_name_pattern);
	static _EXPANDED_NOTIFICATION_NAME_SEQ do_expand_notification_name(const char* notification_name);

private:
	ks_mutex m_mutex;

	std::string m_center_name; //const-like

	_OBSERVER_MAP m_observer_map;
	_INDEXING_BUNDLE m_indexing_bundle;
};


const char* ks_notification_center::__ks_notification_center_data::name() {
	return m_center_name.c_str();
}

uint64_t ks_notification_center::__ks_notification_center_data::add_observer(const void* observer, const char* notification_name_pattern, std::function<void(const ks_notification&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	ASSERT(observer != nullptr);
	ASSERT(apartment != nullptr);
	if (observer == nullptr)
		return 0;

	_PARSED_NOTIFICATION_NAME_PATTERN parsed_notification_name_pattern = do_parse_notification_name_pattern(notification_name_pattern);
	ASSERT(!parsed_notification_name_pattern.base_name.empty());
	if (parsed_notification_name_pattern.base_name.empty())
		return 0;

	if (apartment == nullptr)
		apartment = ks_apartment::current_thread_apartment_or_default_mta();

	std::unique_lock<ks_mutex> lock(m_mutex);

	std::shared_ptr<_ENTRY_DATA> entry_ptr = std::make_shared<_ENTRY_DATA>();
	entry_ptr->init_parsed_notification_name_pattern(parsed_notification_name_pattern);
	entry_ptr->reg_id = ++g_last_reg_id;
	entry_ptr->observer = observer;
	entry_ptr->fn = std::move(fn);
	entry_ptr->context = context;
	entry_ptr->apartment = apartment;

	m_observer_map[observer]
		.his_group_map[entry_ptr->base_name]
		.entry_ptr_list.push_back(entry_ptr);

	m_indexing_bundle
		.select_indexing_group_map(entry_ptr->context.__get_priority(), entry_ptr->is_wildcard_mode)[entry_ptr->base_name]
		.entry_ptr_list.push_back(entry_ptr);

	return entry_ptr->reg_id;
}

void ks_notification_center::__ks_notification_center_data::remove_observer(const void* observer, uint64_t id) {
	ASSERT(observer != nullptr && id != 0);
	if (observer == nullptr || id == 0)
		return;

	std::unique_lock<ks_mutex> lock(m_mutex);

	auto his_observer_it = m_observer_map.find(observer);
	if (his_observer_it == m_observer_map.end())
		return;

	_OBSERVER_DATA& his_observer_data = his_observer_it->second;

	auto his_group_it = his_observer_data.his_group_map.begin();
	while (his_group_it != his_observer_data.his_group_map.end()) {
		_GROUP_DATA& his_group = his_group_it->second;

		auto his_entry_ptr_it = std::find_if(
			his_group.entry_ptr_list.begin(), his_group.entry_ptr_list.end(), 
			[id](const auto& item) { return item->reg_id == id; });
		if (his_entry_ptr_it != his_group.entry_ptr_list.end()) {
			std::shared_ptr<_ENTRY_DATA>& his_entry_ptr = *his_entry_ptr_it;
			his_entry_ptr->removed_flag_v = true;
			do_erase_entry_from_indexing_bundle_locked(his_entry_ptr, lock);
			his_entry_ptr_it = his_group.entry_ptr_list.erase(his_entry_ptr_it);

			if (his_group.entry_ptr_list.empty()) {
				ASSERT(his_group.is_group_empty());
				his_group_it = his_observer_data.his_group_map.erase(his_group_it);
				if (his_observer_data.is_observer_empty()) {
					his_observer_it = m_observer_map.erase(his_observer_it);
				}
			}

			return;  //at most one his-entry exists, so after removing it, we can (and should) return (or break) immediately.
		}

		++his_group_it;
	}
}

void ks_notification_center::__ks_notification_center_data::remove_observer(const void* observer) {
	ASSERT(observer != nullptr);
	if (observer == nullptr)
		return;

	std::unique_lock<ks_mutex> lock(m_mutex);

	auto observer_it = m_observer_map.find(observer);
	if (observer_it == m_observer_map.end())
		return;

	_OBSERVER_DATA& observer_data = observer_it->second;

	for (auto& his_group_ex : observer_data.his_group_map) {
		_GROUP_DATA& his_group = his_group_ex.second;

		for (auto& his_entry_ptr : his_group.entry_ptr_list) {
			his_entry_ptr->removed_flag_v = true;
			do_erase_entry_from_indexing_bundle_locked(his_entry_ptr, lock);
		}

		his_group.entry_ptr_list.clear();
	}

	observer_data.his_group_map.clear();
	m_observer_map.erase(observer_it);
	return;
}

void ks_notification_center::__ks_notification_center_data::do_post_notification(const ks_notification& notification) {
	_EXPANDED_NOTIFICATION_NAME_SEQ expanded_notification_name_seq = do_expand_notification_name(notification.get_name());
	if (expanded_notification_name_seq.empty())
		return;

	std::unique_lock<ks_mutex> lock(m_mutex);

	//收集匹配的entries
	//注：若entry以是cancelled状态，则将自动被移除
	auto do_find_group_and_collect_entries_locked = [this](std::deque<std::shared_ptr<_ENTRY_DATA>>* p_matched_entries, _GROUP_MAP& group_map, const std::string& expanded_notification_name, std::unique_lock<ks_mutex>& lock) -> void {
		auto group_it = group_map.find(expanded_notification_name);
		if (group_it != group_map.end()) {
			_GROUP_DATA& group = group_it->second;

			auto entry_ptr_it = group.entry_ptr_list.begin();
			while (entry_ptr_it != group.entry_ptr_list.end()) {
				std::shared_ptr<_ENTRY_DATA>& entry_ptr = *entry_ptr_it;

				if (entry_ptr->context.__check_cancel_ctrl() || entry_ptr->context.__check_owner_expired()) { //cancelled, auto remove entry
					entry_ptr->removed_flag_v = true;
					do_erase_entry_from_observer_map_locked(entry_ptr, lock);
					entry_ptr_it = group.entry_ptr_list.erase(entry_ptr_it);
				}
				else {
					p_matched_entries->push_back(entry_ptr);
					++entry_ptr_it;
				}
			}

			if (group.entry_ptr_list.empty()) {
				ASSERT(group.is_group_empty());
				group_it = group_map.erase(group_it);
			}
		}
	};

	std::deque<std::shared_ptr<_ENTRY_DATA>> matched_entries;
	constexpr int __all_ordered_priority_array[] = { 1, 0, -1 };
	for (auto priority : __all_ordered_priority_array) {
		//just exact group
		if (true) {
			_GROUP_MAP& exact_group_map = m_indexing_bundle.select_indexing_group_map(priority, false);
			do_find_group_and_collect_entries_locked(&matched_entries, exact_group_map, expanded_notification_name_seq.front(), lock);
		}
		//upward wildcard groups
		for (auto& expanded_notification_name : expanded_notification_name_seq) {
			_GROUP_MAP& a_wildcard_group_map = m_indexing_bundle.select_indexing_group_map(priority, true);
			do_find_group_and_collect_entries_locked(&matched_entries, a_wildcard_group_map, expanded_notification_name, lock);
		}
		//just *(spec) groups
		if (true) {
			_GROUP_MAP& a_wildcard_group_map = m_indexing_bundle.select_indexing_group_map(priority, true);
			do_find_group_and_collect_entries_locked(&matched_entries, a_wildcard_group_map, "*", lock);
		}
	}

	//异步触发匹配到的entries各项
	if (!matched_entries.empty()) {
		const ks_async_context& notification_context = notification.get_context();

		__ks_async_raw::ks_raw_living_context_rtstt notification_context_rtstt;
		notification_context_rtstt.apply(notification_context);

		if (!notification_context.__check_cancel_ctrl() && !notification_context.__check_owner_expired()) {
			for (auto& entry_ptr : matched_entries) {
				ks_apartment* entry_partment = entry_ptr->apartment;
				entry_partment->schedule([entry_ptr, notification, notification_context, notification_context_owner_locker = notification_context_rtstt.get_owner_locker()]() {
					const ks_async_context& entry_context = entry_ptr->context;

					__ks_async_raw::ks_raw_living_context_rtstt entry_context_rtstt;
					entry_context_rtstt.apply(entry_context);

					//try {
						if (!entry_ptr->context.__check_cancel_ctrl() && !entry_context.__check_owner_expired())
							entry_ptr->fn(notification);
					//}
					//catch (...) {
					//	//TODO dump exception ...
					//	ASSERT(false);
					//	//abort();
					//	throw;
					//}

					entry_context_rtstt.try_unapply();
				}, 0);
			}
		}

		notification_context_rtstt.try_unapply();
	}
}

void ks_notification_center::__ks_notification_center_data::do_erase_entry_from_observer_map_locked(const std::shared_ptr<_ENTRY_DATA>& entry_ptr, std::unique_lock<ks_mutex>& lock) {
	auto observer_it = m_observer_map.find(entry_ptr->observer);
	if (observer_it == m_observer_map.end())
		return;

	_OBSERVER_DATA& observer_data = observer_it->second;
	auto match_group_it = observer_data.his_group_map.find(entry_ptr->base_name);
	if (match_group_it == observer_data.his_group_map.end())
		return;

	_GROUP_DATA& match_group = match_group_it->second;
	auto match_entry_ptr_it = std::find(match_group.entry_ptr_list.begin(), match_group.entry_ptr_list.end(), entry_ptr);
	if (match_entry_ptr_it == match_group.entry_ptr_list.end())
		return;

	match_group.entry_ptr_list.erase(match_entry_ptr_it);

	if (match_group.entry_ptr_list.empty()) {
		ASSERT(match_group.is_group_empty());
		observer_data.his_group_map.erase(match_group_it);

		if (observer_data.is_observer_empty()) {
			m_observer_map.erase(observer_it);
		}
	}
}

void ks_notification_center::__ks_notification_center_data::do_erase_entry_from_indexing_bundle_locked(const std::shared_ptr<_ENTRY_DATA>& entry_ptr, std::unique_lock<ks_mutex>& lock) {
	_GROUP_MAP& prior_group_map = m_indexing_bundle.select_indexing_group_map(entry_ptr->context.__get_priority(), entry_ptr->is_wildcard_mode);
	auto prior_group_it = prior_group_map.find(entry_ptr->base_name);
	if (prior_group_it == prior_group_map.end())
		return;

	_GROUP_DATA& prior_group = prior_group_it->second;
	prior_group.entry_ptr_list.remove(entry_ptr);

	if (prior_group.is_group_empty()) {
		prior_group_map.erase(prior_group_it);
	}
}

ks_notification_center::__ks_notification_center_data::_PARSED_NOTIFICATION_NAME_PATTERN ks_notification_center::__ks_notification_center_data::do_parse_notification_name_pattern(const char* notification_name) {
	//add_observer和remove_observer时，传入的notification_name支持*通配符后缀
	//notification_name本是用.做分段符，但为满足内部逻辑对notification_name排序的需要，内部将.改为空格进行使用和保存

	std::string alt_notification_name(notification_name != nullptr ? notification_name : "");
	bool is_wildcard_mode = false;
	ASSERT(!alt_notification_name.empty());
	ASSERT(alt_notification_name.find(' ') == -1);
	ASSERT(alt_notification_name == "*" || alt_notification_name.find('.') != -1);

	if (alt_notification_name == "*") {
		alt_notification_name = "*"; //*(spec)
		is_wildcard_mode = true;
	}
	else if (alt_notification_name.length() >= 2 && strncmp(alt_notification_name.data() + alt_notification_name.length() - 2, ".*", 2) == 0) {
		alt_notification_name.resize(alt_notification_name.length() - 2);
		is_wildcard_mode = true;
		ASSERT(alt_notification_name.find('*') == -1);
	}
	else {
		ASSERT(alt_notification_name.find('*') == -1);
	}

	for (auto& ch : alt_notification_name) {
		if (ch == '.')
			ch = ' ';
	}

	return _PARSED_NOTIFICATION_NAME_PATTERN{ std::move(alt_notification_name), is_wildcard_mode };
}

ks_notification_center::__ks_notification_center_data::_EXPANDED_NOTIFICATION_NAME_SEQ ks_notification_center::__ks_notification_center_data::do_expand_notification_name(const char* notification_name) {
	//post_notification时，传入的notification_name（无*通配符后缀）
	//notification_name本是用.做分段符，但为满足内部逻辑对notification_name排序的需要，内部将.改为空格进行使用和保存
	//返回的seq内容形式为：[ "a b c d", "a b c", "a b", "a" ]

	std::string alt_notification_name(notification_name != nullptr ? notification_name : "");
	ASSERT(alt_notification_name.find(' ') == -1);
	ASSERT(alt_notification_name.find('.') != -1);
	ASSERT(alt_notification_name.find('*') == -1);

	for (auto& ch : alt_notification_name) {
		if (ch == '.')
			ch = ' ';
	}

	_EXPANDED_NOTIFICATION_NAME_SEQ ret;

	//首项是name全名，用于完全匹配
	//后接依次截短的上溯名称列表，用于通配
	while (!alt_notification_name.empty()) {
		ret.push_back(alt_notification_name);

		size_t split_pos = alt_notification_name.rfind(' ');
		if (split_pos != -1)
			alt_notification_name.resize(split_pos);
		else
			alt_notification_name.clear();
	}

	return ret;
}


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

ks_notification_center::ks_notification_center(const char* center_name)
	: m_d(std::make_shared<__ks_notification_center_data>(center_name)) {
}


ks_notification_center* ks_notification_center::default_center() {
	static ks_notification_center g_default_center("default_center");
	return &g_default_center;
}


const char* ks_notification_center::name() {
	return m_d->name();
}

uint64_t ks_notification_center::add_observer(const void* observer, const char* notification_name_pattern, ks_apartment* apartment, std::function<void(const ks_notification&)> fn, const ks_async_context& context) {
	return m_d->add_observer(observer, notification_name_pattern, std::move(fn), context, apartment);
}

void ks_notification_center::remove_observer(const void* observer, uint64_t id) {
	m_d->remove_observer(observer, id);
}

void ks_notification_center::remove_observer(const void* observer) {
	m_d->remove_observer(observer);
}

void ks_notification_center::post_notification_indirect(const ks_notification& notification) {
	//将异步在master-sta（或background-sta）中执行，避免阻塞业务，但又可保持notification时序
	ks_apartment* inter_apartment = ks_apartment::master_sta();
	if (inter_apartment == nullptr)
		inter_apartment = ks_apartment::background_sta();

	ASSERT(inter_apartment != nullptr);
	inter_apartment->schedule([d = m_d, notification]() {
		d->do_post_notification(notification);
	}, 0);
}
