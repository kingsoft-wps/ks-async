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
#include <map>

void __forcelink_to_ks_apartment_cpp() {}

#if defined(_WIN32)
	#include <Windows.h>
	static inline void __native_set_current_thread_name(const char* thread_name) {
		typedef HRESULT(WINAPI* PFN_SetThreadDescription)(HANDLE hThread, PCWSTR lpThreadDescription);
		static PFN_SetThreadDescription __pfnSetThreadDescription = (PFN_SetThreadDescription)::GetProcAddress(::GetModuleHandleW(L"Kernel32.dll"), "SetThreadDescription");
		if (__pfnSetThreadDescription != nullptr) { //after Win10
			__pfnSetThreadDescription(::GetCurrentThread(), std::wstring(thread_name, thread_name + strlen(thread_name)).c_str());
		}
		else {
			auto __SetThreadName = [](DWORD dwThreadID, const char* threadName) -> void {
				const DWORD MS_VC_EXCEPTION = 0x406D1388;
				#pragma pack(push, 8)
				typedef struct tagTHREADNAME_INFO {
					DWORD dwType; // Must be 0x1000.
					LPCSTR szName; // Pointer to name (in user addr space).
					DWORD dwThreadID; // Thread ID (-1=caller thread).
					DWORD dwFlags; // Reserved for future use, must be zero.
				} THREADNAME_INFO;
				#pragma pack(pop)
				THREADNAME_INFO info;
				info.dwType = 0x1000;
				info.szName = threadName;
				info.dwThreadID = dwThreadID;
				info.dwFlags = 0;
				#pragma warning(push)
				#pragma warning(disable: 6320 6322)
				__try {
					::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
				}
				#pragma warning(pop)
			};
			__SetThreadName((DWORD)-1, thread_name);
		}
	}

#elif defined(__APPLE__)
	#include <pthread.h>
	static inline void __native_set_current_thread_name(const char* thread_name) {
		pthread_setname_np(thread_name);
	}
#else
	#include <pthread.h>
	static inline void __native_set_current_thread_name(const char* thread_name) {
		pthread_setname_np(pthread_self(), thread_name);
	}
#endif


static ks_apartment* g_ui_sta = nullptr;
static ks_apartment* g_master_sta = nullptr;
static thread_local ks_apartment* tls_current_thread_apartment = nullptr;

static ks_mutex g_public_apartment_mutex {};
static std::map<std::string, ks_apartment*> g_public_apartment_map {};

static size_t g_default_mta_max_thread_count = 0;


ks_apartment* ks_apartment::ui_sta() {
	return g_ui_sta;
}

ks_apartment* ks_apartment::master_sta() {
	return g_master_sta;
}

ks_apartment* ks_apartment::background_sta() {
	static ks_single_thread_apartment_imp g_background_sta(
		"background_sta", 
		ks_single_thread_apartment_imp::auto_register_flag | ks_single_thread_apartment_imp::endless_instance_flag);
	return &g_background_sta;
}

ks_apartment* ks_apartment::default_mta() {
	struct _default_mta_options {
		static size_t max_thread_count() {
			size_t max_thread_count = g_default_mta_max_thread_count;
			if (max_thread_count == 0) {
				int cpu_count = (int)std::thread::hardware_concurrency();
				cpu_count -= 2; //留出2个核给其他线程
				if (cpu_count < 2)
					cpu_count = 2;  //至少2个线程
				else if (cpu_count > 16)
					cpu_count = 16; //至多16个线程

				max_thread_count = (size_t)cpu_count;
			}

			return max_thread_count;
		}
	};

	static ks_thread_pool_apartment_imp g_default_mta(
		"default_mta", 
		_default_mta_options::max_thread_count(), 
		ks_thread_pool_apartment_imp::auto_register_flag | ks_thread_pool_apartment_imp::endless_instance_flag);
	return &g_default_mta;
}

ks_apartment* ks_apartment::current_thread_apartment() {
	return tls_current_thread_apartment;
}

ks_apartment* ks_apartment::current_thread_apartment_or_default_mta() {
	ks_apartment* cur_apartment = tls_current_thread_apartment;
	if (cur_apartment == nullptr)
		cur_apartment = ks_apartment::default_mta();
	ASSERT(cur_apartment != nullptr);
	return cur_apartment;
}

ks_apartment* ks_apartment::current_thread_apartment_or(ks_apartment* or_apartment) {
	ks_apartment* cur_apartment = tls_current_thread_apartment;
	if (cur_apartment == nullptr)
		cur_apartment = or_apartment;
	ASSERT(cur_apartment != nullptr);
	return cur_apartment;
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


void ks_apartment::__set_default_mta_max_thread_count(size_t max_thread_count) {
	ASSERT(ks_apartment::find_public_apartment("default_mta") == nullptr);
	g_default_mta_max_thread_count = max_thread_count;
}


void ks_apartment::__set_ui_sta(ks_apartment* ui_sta) {
	ASSERT(ui_sta == nullptr || (ui_sta->features() & sequential_feature) != 0);
	ASSERT(ui_sta == nullptr || g_ui_sta == nullptr || g_ui_sta == ui_sta);
	g_ui_sta = ui_sta;
}

void ks_apartment::__set_master_sta(ks_apartment* master_sta) {
	ASSERT(master_sta == nullptr || (master_sta->features() & sequential_feature) != 0);
	ASSERT(master_sta == nullptr || g_master_sta == nullptr || g_master_sta == master_sta);
	g_master_sta = master_sta;
}

void ks_apartment::__set_current_thread_apartment(ks_apartment* current_thread_apartment) {
	ASSERT(current_thread_apartment == nullptr || tls_current_thread_apartment == nullptr || tls_current_thread_apartment == current_thread_apartment);
	tls_current_thread_apartment = current_thread_apartment;
}

void ks_apartment::__set_current_thread_name(const char* thread_name) {
	ASSERT(thread_name != nullptr);
	__native_set_current_thread_name(thread_name);
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
	ASSERT(it != g_public_apartment_map.cend());
	if (it != g_public_apartment_map.cend()) {
		ASSERT(it->second == apartment);
		if (it->second == apartment) {
			g_public_apartment_map.erase(it);
		}
	}
}
