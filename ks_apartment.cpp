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

#include "ks_apartment.h"
#include "ks_single_thread_apartment_imp.h"
#include "ks_thread_pool_apartment_imp.h"
#include <thread>
#include <unordered_map>

void __forcelink_to_ks_apartment_cpp() {}


static ks_apartment* g_ui_sta = nullptr;
static ks_apartment* g_master_sta = nullptr;

static ks_mutex g_public_apartment_mutex = {};
static std::unordered_map<std::string, ks_apartment*> g_public_apartment_map = {};

static thread_local ks_apartment* tls_current_thread_apartment = nullptr;


ks_apartment* ks_apartment::ui_sta() {
	return g_ui_sta;
}

ks_apartment* ks_apartment::master_sta() {
	return g_master_sta;
}

ks_apartment* ks_apartment::background_sta() {
	static ks_single_thread_apartment_imp g_background_sta("background_sta", 0);
	return &g_background_sta;
}

ks_apartment* ks_apartment::default_mta() {
	struct _default_mta_options {
		static size_t max_thread_count() {
#ifdef __KS_DEFAULT_MTA_MAX_THREAD_COUNT_CUSTOM
			return __KS_DEFAULT_MTA_MAX_THREAD_COUNT_CUSTOM;
#else
			size_t cpu_count = (size_t)std::thread::hardware_concurrency();
			size_t max_thread_count = cpu_count + 1;
			if (max_thread_count < 3)
				max_thread_count = 3;
			return max_thread_count;
#endif
		}
	};

	static ks_thread_pool_apartment_imp g_default_mta("default_mta", _default_mta_options::max_thread_count(), 0);
	return &g_default_mta;
}


ks_apartment* ks_apartment::find_public_apartment(const char* name) {
	std::unique_lock<ks_mutex> lock(g_public_apartment_mutex);
	ASSERT(name != nullptr);

	auto it = g_public_apartment_map.find(name);
	if (it != g_public_apartment_map.cend())
		return it->second;

	if (name[0] == '#') {
		int ord = atoi(name + 1);
		if (ord != 0) {
			int index = ord > 0 ? ord - 1 : (int)g_public_apartment_map.size() + ord;
			if (index >= 0 && index < g_public_apartment_map.size()) {
				if (index <= g_public_apartment_map.size() / 2)
					return std::next(g_public_apartment_map.cbegin(), index)->second;
				else
					return std::prev(g_public_apartment_map.cend(), g_public_apartment_map.size() - index)->second;
			}
		}
	}

	return nullptr;
}


ks_apartment* ks_apartment::current_thread_apartment() {
	return tls_current_thread_apartment;
}

ks_apartment* ks_apartment::current_thread_apartment_or_default_mta() {
	ks_apartment* cur_apartment = tls_current_thread_apartment;
	if (cur_apartment == nullptr)
		cur_apartment = ks_apartment::default_mta();
	return cur_apartment;
}

ks_apartment* ks_apartment::current_thread_apartment_or(ks_apartment* or_apartment) {
	ks_apartment* cur_apartment = tls_current_thread_apartment;
	if (cur_apartment == nullptr)
		cur_apartment = or_apartment;
	return cur_apartment;
}

bool ks_apartment::__current_thread_apartment_try_pump_once() {
	ks_apartment* cur_apartment = tls_current_thread_apartment;
	ASSERT(cur_apartment != nullptr);
	return cur_apartment->__try_pump_once();
}


void ks_apartment::__set_ui_sta(ks_apartment* ui_sta) {
	ASSERT(g_ui_sta == nullptr || ui_sta == nullptr);
	ASSERT(g_ui_sta != nullptr || ui_sta != nullptr);
	ASSERT(ui_sta == nullptr || strcmp(ui_sta->name(), "ui_sta") == 0);
	g_ui_sta = ui_sta;
}

void ks_apartment::__set_master_sta(ks_apartment* master_sta) {
	ASSERT(g_master_sta == nullptr || master_sta == nullptr);
	ASSERT(g_master_sta != nullptr || master_sta != nullptr);
	ASSERT(master_sta == nullptr || strcmp(master_sta->name(), "master_sta") == 0);
	g_master_sta = master_sta;
}

void ks_apartment::__register_public_apartment(const char* name, ks_apartment* apartment) {
	std::unique_lock<ks_mutex> lock(g_public_apartment_mutex);
	ASSERT(name != nullptr && apartment != nullptr);
	ASSERT(g_public_apartment_map.find(name) == g_public_apartment_map.cend());
	g_public_apartment_map[name] = apartment;
}

void ks_apartment::__unregister_public_apartment(const char* name, ks_apartment* apartment) {
	std::unique_lock<ks_mutex> lock(g_public_apartment_mutex);
	ASSERT(name != nullptr && apartment != nullptr);
	auto it = g_public_apartment_map.find(name);
	if (it != g_public_apartment_map.cend() && it->second == apartment)
		g_public_apartment_map.erase(it);
}

void ks_apartment::__tls_set_current_thread_apartment(ks_apartment* current_thread_apartment) {
	ASSERT(tls_current_thread_apartment == nullptr || current_thread_apartment == nullptr);
	ASSERT(tls_current_thread_apartment != nullptr || current_thread_apartment != nullptr);
	tls_current_thread_apartment = current_thread_apartment;
}
