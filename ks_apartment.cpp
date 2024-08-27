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

void __forcelink_to_ks_apartment_cpp() {}


static ks_apartment* g_ui_sta = nullptr;
static ks_apartment* g_master_sta = nullptr;
static thread_local ks_apartment* tls_current_thread_apartment = nullptr;


ks_apartment* ks_apartment::ui_sta() {
	ASSERT(g_ui_sta != nullptr);
	return g_ui_sta;
}

ks_apartment* ks_apartment::master_sta() {
	ASSERT(g_master_sta != nullptr);
	return g_master_sta;
}

ks_apartment* ks_apartment::background_sta() {
	static ks_single_thread_apartment_imp g_background_sta(true);
	return &g_background_sta;
}

ks_apartment* ks_apartment::default_mta() {
	struct _default_mta_options {
		static size_t max_thread_count() {
			size_t cpu_count = (size_t)std::thread::hardware_concurrency();
			size_t max_thread_count = cpu_count * 2;
			if (max_thread_count < 4)
				max_thread_count = 4;
			return max_thread_count;
		}
	};

	static ks_thread_pool_apartment_imp g_default_mta(_default_mta_options::max_thread_count());
	return &g_default_mta;
}

ks_apartment* ks_apartment::current_thread_apartment() {
	return tls_current_thread_apartment;
}

ks_apartment* ks_apartment::current_thread_apartment_or_default_mta() {
	ks_apartment* cur_apartment = tls_current_thread_apartment;
	if (cur_apartment == nullptr)
		cur_apartment = default_mta();
	return cur_apartment;
}

ks_apartment* ks_apartment::current_thread_apartment_or(ks_apartment* or_apartment) {
	ks_apartment* cur_apartment = tls_current_thread_apartment;
	if (cur_apartment == nullptr)
		cur_apartment = or_apartment;
	return cur_apartment;
}


void ks_apartment::__set_ui_sta(ks_apartment* ui_sta) {
	ASSERT(g_ui_sta == nullptr || ui_sta == nullptr);
	ASSERT(g_ui_sta != nullptr || ui_sta != nullptr);
	g_ui_sta = ui_sta;
}

void ks_apartment::__set_master_sta(ks_apartment* master_sta) {
	ASSERT(g_master_sta == nullptr || master_sta == nullptr);
	ASSERT(g_master_sta != nullptr || master_sta != nullptr);
	g_master_sta = master_sta;
}

void ks_apartment::__tls_set_current_thread_apartment(ks_apartment* current_thread_apartment) {
	ASSERT(tls_current_thread_apartment == nullptr || current_thread_apartment == nullptr);
	ASSERT(tls_current_thread_apartment != nullptr || current_thread_apartment != nullptr);
	tls_current_thread_apartment = current_thread_apartment;
}

ks_apartment* ks_apartment::__get_ui_sta() {
	return g_ui_sta;
}

ks_apartment* ks_apartment::__get_master_sta() {
	return g_master_sta;
}

ks_apartment* ks_apartment::__tls_get_current_thread_apartment() {
	return tls_current_thread_apartment;
}
