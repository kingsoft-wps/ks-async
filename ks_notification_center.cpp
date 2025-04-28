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

void __forcelink_to_ks_notification_center_cpp() {}


class ks_notification_center::__ks_notification_center_data final {
public:
	__ks_notification_center_data(const char* center_name) { ASSERT(center_name != nullptr); m_center_name = center_name; }
	~__ks_notification_center_data() {}
	_DISABLE_COPY_CONSTRUCTOR(__ks_notification_center_data);

public:
	const char* name();

public:
	void add_observer(
		const void* observer, const char* notification_name_pattern,
		std::function<void(const ks_notification&)>&& fn, const ks_async_context& context, ks_apartment* apartment);

	void remove_observer(const void* observer, const char* notification_name_pattern);

public:
	void do_post_notification(const ks_notification& notification);

private:
	using _EXPANDED_NOTIFICATION_NAME_SEQ = std::deque<std::string>;

	struct _PARSED_NOTIFICATION_NAME {
		std::string base_name;
		bool is_wildcard_mode;
	};

	struct _ENTRY_DATA : _PARSED_NOTIFICATION_NAME {
		const void* observer;
		std::function<void(const ks_notification&)> fn;
		ks_async_context context;
		ks_apartment* apartment;
		volatile bool removed_flag_v = false;
	};

	using _ENTRY_PTR = std::shared_ptr<_ENTRY_DATA>;

private:
	void do_erase_entry_from_observer_map_locked(const _ENTRY_PTR& entry_ptr, std::unique_lock<ks_mutex>& lock);
	void do_erase_entry_from_indexing_bundle_locked(const _ENTRY_PTR& entry_ptr, std::unique_lock<ks_mutex>& lock);

	static _PARSED_NOTIFICATION_NAME do_parse_notification_name(const char* notification_name);
	static _EXPANDED_NOTIFICATION_NAME_SEQ do_expand_notification_name(const char* notification_name);

private:
	using _ENTRY_LIST = std::list<_ENTRY_PTR>;

	struct _GROUP_DATA {
		_ENTRY_LIST entry_list;

		bool is_group_empty() const { return this->entry_list.empty(); }
	};

	using _GROUP_MAP = std::map<std::string, _GROUP_DATA, std::less<std::string>>;

	struct _OBSERVER_DATA {
		_GROUP_MAP his_group_map;

		bool is_observer_empty() const { return this->his_group_map.empty(); }
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
	ks_mutex m_mutex;
	std::string m_center_name;
	_OBSERVER_MAP m_observer_map;
	_INDEXING_BUNDLE m_indexing_bundle;
};


const char* ks_notification_center::__ks_notification_center_data::name() {
	return m_center_name.c_str();
}

void ks_notification_center::__ks_notification_center_data::add_observer(const void* observer, const char* notification_name_pattern, std::function<void(const ks_notification&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	ASSERT(observer != nullptr);
	ASSERT(apartment != nullptr);
	if (apartment == nullptr)
		apartment = ks_apartment::current_thread_apartment_or_default_mta();
	if (notification_name_pattern == nullptr || notification_name_pattern[0] == 0)
		notification_name_pattern = "*";

	_ENTRY_PTR entry_ptr = std::make_shared<_ENTRY_DATA>();
	entry_ptr->observer = observer;
	entry_ptr->fn = std::move(fn);
	entry_ptr->context = context;
	entry_ptr->apartment = apartment;
	static_cast<_PARSED_NOTIFICATION_NAME&>(*entry_ptr) = do_parse_notification_name(notification_name_pattern);

	m_observer_map[observer]
		.his_group_map[entry_ptr->base_name]
		.entry_list.push_back(entry_ptr);

	m_indexing_bundle
		.select_indexing_group_map(entry_ptr->context.__get_priority(), entry_ptr->is_wildcard_mode)[entry_ptr->base_name]
		.entry_list.push_back(entry_ptr);
}

void ks_notification_center::__ks_notification_center_data::remove_observer(const void* observer, const char* notification_name_pattern) {
	std::unique_lock<ks_mutex> lock(m_mutex);

	ASSERT(observer != 0);
	if (notification_name_pattern == nullptr || notification_name_pattern[0] == 0)
		notification_name_pattern = "*";

	auto observer_it = m_observer_map.find(observer);
	if (observer_it == m_observer_map.end())
		return;

	_OBSERVER_DATA& observer_data = observer_it->second;
	const _PARSED_NOTIFICATION_NAME parsed_notification_name = do_parse_notification_name(notification_name_pattern);

	//匹配所有(*)，observer的全部entry将被移除
	if (parsed_notification_name.base_name == "*") {
		ASSERT(parsed_notification_name.is_wildcard_mode);

		for (auto& group_ex : observer_data.his_group_map) {
			_GROUP_DATA& group = group_ex.second;
			//const auto&  group_name = group_ex.first;

			for (auto& entry_ptr : group.entry_list) {
				do_erase_entry_from_indexing_bundle_locked(entry_ptr, lock);
				entry_ptr->removed_flag_v = true;
			}

			group.entry_list.clear();
		}

		observer_data.his_group_map.clear();
		m_observer_map.erase(observer_it);
		return;
	}

	//无*后缀：仅精确匹配name的entry被移除
	if (!parsed_notification_name.is_wildcard_mode) {
		auto match_group_it = observer_data.his_group_map.find(parsed_notification_name.base_name);
		if (match_group_it == observer_data.his_group_map.end())
			return;

		_GROUP_DATA& match_group = match_group_it->second;
		//const auto& match_group_name = match_group_it->first;

		auto entry_it = match_group.entry_list.begin();
		while (entry_it != match_group.entry_list.end()) {
			_ENTRY_PTR& entry_ptr = *entry_it;
			if (entry_ptr->is_wildcard_mode) {
				++entry_it;
				continue;
			}

			do_erase_entry_from_indexing_bundle_locked(entry_ptr, lock);
			entry_ptr->removed_flag_v = true;
			entry_it = match_group.entry_list.erase(entry_it);
		}

		if (match_group.is_group_empty()) {
			observer_data.his_group_map.erase(match_group_it);

			if (observer_data.is_observer_empty()) {
				m_observer_map.erase(observer_it);
			}
		}

		return;
	}

	//有*后缀：精确匹配或通配name的entry均被移除...
	//因为map项是lex排序，故这里遍历wildcard-group-map的算法为从lower_bound处向后遍历，直至name不匹配
	if (true) {
		auto group_it = parsed_notification_name.base_name.empty() ? observer_data.his_group_map.begin() : observer_data.his_group_map.lower_bound(parsed_notification_name.base_name);
		while (group_it != observer_data.his_group_map.end()) {
			_GROUP_DATA& group = group_it->second;
			const auto& group_name = group_it->first;

			//检查group名称是否匹配
			bool is_group_name_matched = false;
			if (group_name.empty())
				is_group_name_matched = true;
			if (group_name.length() == parsed_notification_name.base_name.length() && group_name == parsed_notification_name.base_name)
				is_group_name_matched = true;
			else if (group_name.length() > parsed_notification_name.base_name.length() && group_name[parsed_notification_name.base_name.length()] == ' ' 
				&& memcmp(group_name.data(), parsed_notification_name.base_name.data(), parsed_notification_name.base_name.length() * sizeof(*parsed_notification_name.base_name.data())) == 0)
				is_group_name_matched = true;

			//因为name有序，所以当遇到首个不匹配项时可立即break
			if (!is_group_name_matched)
				break;

			//确定是匹配的，移除group内包含的各entry
			for (auto& entry_ptr : group.entry_list) {
				do_erase_entry_from_indexing_bundle_locked(entry_ptr, lock);
				entry_ptr->removed_flag_v = true;
			}

			//移除group
			group.entry_list.clear();
			group_it = observer_data.his_group_map.erase(group_it);
		}

		if (observer_data.is_observer_empty()) {
			m_observer_map.erase(observer_it);
		}

		return;
	}
}

void ks_notification_center::__ks_notification_center_data::do_post_notification(const ks_notification& notification) {
	const _EXPANDED_NOTIFICATION_NAME_SEQ expanded_notification_name_seq = do_expand_notification_name(notification.get_name());
	ASSERT(!expanded_notification_name_seq.empty());

	std::unique_lock<ks_mutex> lock(m_mutex);

	//收集匹配的entries
	//注：若entry以是cancelled状态，则将自动被移除
	std::deque<_ENTRY_PTR> matched_entries;

	auto do_find_group_and_collect_entries_locked = [this, &matched_entries](_GROUP_MAP& group_map, const std::string& expanded_notification_name, std::unique_lock<ks_mutex>& lock) -> void {
		auto group_it = group_map.find(expanded_notification_name);
		if (group_it != group_map.end()) {
			_GROUP_DATA& group = group_it->second;

			auto entry_it = group.entry_list.begin();
			while (entry_it != group.entry_list.end()) {
				_ENTRY_PTR& entry_ptr = *entry_it;

				if (entry_ptr->context.__check_cancel_all_ctrl() || entry_ptr->context.__check_owner_expired()) { //cancelled, auto remove entry
					do_erase_entry_from_observer_map_locked(entry_ptr, lock);
					entry_ptr->removed_flag_v = true;
					entry_it = group.entry_list.erase(entry_it);
				}
				else {
					matched_entries.push_back(entry_ptr);
					++entry_it;
				}
			}

			if (group.is_group_empty()) {
				group_map.erase(group_it);
			}
		}
	};

	constexpr int __all_ordered_priority_array[] = { 1, 0, -1 };
	for (auto priority : __all_ordered_priority_array) {
		//just exact group
		if (true) {
			_GROUP_MAP& exact_group_map = m_indexing_bundle.select_indexing_group_map(priority, false);
			do_find_group_and_collect_entries_locked(exact_group_map, expanded_notification_name_seq.front(), lock);
		}

		//upward wildcard groups
		for (auto& expanded_notification_name : expanded_notification_name_seq) {
			_GROUP_MAP& a_wildcard_group_map = m_indexing_bundle.select_indexing_group_map(priority, true);
			do_find_group_and_collect_entries_locked(a_wildcard_group_map, expanded_notification_name, lock);
		}

		//any-wildcard (*) group
		if (true) {
			_GROUP_MAP& any_wildcard_group_map = m_indexing_bundle.select_indexing_group_map(priority, true);
			do_find_group_and_collect_entries_locked(any_wildcard_group_map, "*", lock);
		}
	}

	//异步触发匹配到的entries各项
	if (!matched_entries.empty()) {
		const ks_async_context& notification_context = notification.get_context();

		__ks_async_raw::ks_raw_living_context_rtstt notification_context_rtstt;
		notification_context_rtstt.apply(notification_context);

		if (!notification_context.__check_cancel_all_ctrl() && !notification_context.__check_owner_expired()) {
			for (auto& entry_ptr : matched_entries) {
				ks_apartment* entry_partment = entry_ptr->apartment;
				entry_partment->schedule([entry_ptr, notification, notification_context, notification_context_owner_locker = notification_context_rtstt.get_owner_locker()]() {
					const ks_async_context& entry_context = entry_ptr->context;

					__ks_async_raw::ks_raw_living_context_rtstt entry_context_rtstt;
					entry_context_rtstt.apply(entry_context);

					//try {
						if (!entry_ptr->context.__check_cancel_all_ctrl() && !entry_context.__check_owner_expired())
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

void ks_notification_center::__ks_notification_center_data::do_erase_entry_from_observer_map_locked(const _ENTRY_PTR& entry_ptr, std::unique_lock<ks_mutex>& lock) {
	auto observer_it = m_observer_map.find(entry_ptr->observer);
	if (observer_it == m_observer_map.end())
		return;

	_OBSERVER_DATA& observer_data = observer_it->second;
	auto match_group_it = observer_data.his_group_map.find(entry_ptr->base_name);
	if (match_group_it == observer_data.his_group_map.end())
		return;

	_GROUP_DATA& match_group = match_group_it->second;
	match_group.entry_list.remove(entry_ptr);

	if (match_group.is_group_empty()) {
		observer_data.his_group_map.erase(match_group_it);

		if (observer_data.is_observer_empty()) {
			m_observer_map.erase(observer_it);
		}
	}
}

void ks_notification_center::__ks_notification_center_data::do_erase_entry_from_indexing_bundle_locked(const _ENTRY_PTR& entry_ptr, std::unique_lock<ks_mutex>& lock) {
	_GROUP_MAP& prior_group_map = m_indexing_bundle.select_indexing_group_map(entry_ptr->context.__get_priority(), entry_ptr->is_wildcard_mode);
	auto prior_group_it = prior_group_map.find(entry_ptr->base_name);
	if (prior_group_it == prior_group_map.end())
		return;

	_GROUP_DATA& prior_group = prior_group_it->second;
	prior_group.entry_list.remove(entry_ptr);

	if (prior_group.is_group_empty()) {
		prior_group_map.erase(prior_group_it);
	}
}

ks_notification_center::__ks_notification_center_data::_PARSED_NOTIFICATION_NAME ks_notification_center::__ks_notification_center_data::do_parse_notification_name(const char* notification_name) {
	//add_observer和remove_observer时，传入的notification_name支持*通配符后缀
	//notification_name本是用.做分段符，但为满足内部逻辑对notification_name排序的需要，内部将.改为空格进行使用和保存

	std::string alt_notification_name(notification_name != nullptr ? notification_name : "");
	bool is_wildcard_mode = false;
	ASSERT(!alt_notification_name.empty());
	ASSERT(alt_notification_name.find(' ') == -1);
	ASSERT(alt_notification_name.find('.') != -1);

	if (alt_notification_name == "*") {
		alt_notification_name = "*";
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

	return _PARSED_NOTIFICATION_NAME{ std::move(alt_notification_name), is_wildcard_mode };
}

ks_notification_center::__ks_notification_center_data::_EXPANDED_NOTIFICATION_NAME_SEQ ks_notification_center::__ks_notification_center_data::do_expand_notification_name(const char* notification_name) {
	//post_notification时，传入的notification_name（无*通配符后缀）
	//notification_name本是用.做分段符，但为满足内部逻辑对notification_name排序的需要，内部将.改为空格进行使用和保存
	//返回的seq内容形式为：[ "a b c d", "a b c", "a b", "a", "" ]
	//但出于性能考虑，目前至多仅保留前两项，其他更短项全部被丢弃（相当于只上溯至父级通配）
	//虽然这会使得更短的祖先通配项未能匹配，但通常这已能够满足实现业务逻辑的需要了
	//而且，我们还额外增加支持了特殊的任意通配形式（*）

	std::string alt_notification_name(notification_name != nullptr ? notification_name : "");
	ASSERT(!alt_notification_name.empty());
	ASSERT(alt_notification_name.find(' ') == -1);
	ASSERT(alt_notification_name.find('.') != -1);
	ASSERT(alt_notification_name.find('*') == -1);

	for (auto& ch : alt_notification_name) {
		if (ch == '.')
			ch = ' ';
	}

	_EXPANDED_NOTIFICATION_NAME_SEQ ret;

	//首项是name全名，用于完全匹配
	ret.push_back(alt_notification_name);

	//后接依次截短的上溯名称列表，用于通配（目前策略实际上相当于仅上溯至父级通配）
	while (!alt_notification_name.empty() && ret.size() < 2) {
		size_t split_pos = alt_notification_name.rfind(' ');
		if (split_pos == -1)
			split_pos = 0;

		alt_notification_name.resize(split_pos);
		ret.push_back(alt_notification_name);
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

void ks_notification_center::add_observer(const void* observer, const char* notification_name_pattern, ks_apartment* apartment, std::function<void(const ks_notification&)> fn, const ks_async_context& context) {
	m_d->add_observer(observer, notification_name_pattern, std::move(fn), context, apartment);
}

void ks_notification_center::remove_observer(const void* observer, const char* notification_name_pattern) {
	m_d->remove_observer(observer, notification_name_pattern);
}

void ks_notification_center::remove_observer(const void* observer) {
	m_d->remove_observer(observer, "*");
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
