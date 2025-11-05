/* Copyright 2024 The Kingsoft's ks-async/ktl Authors. All Rights Reserved.

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

#pragma once

#ifndef __KS_ANY_DEF
#define __KS_ANY_DEF

#include "ks_cxxbase.h"
#include "ks_type_traits.h"
#include <atomic>
#include <functional>
#include <cstring>
#include <string.h>


//与c++17的std::any相比，ks_any约束要更多一点，用法也稍有不同，但大差不差
class ks_any {
public:
	ks_any() noexcept {
		m_data_p = nullptr;
		m_embed_tiny_trivial_x_mem = {};
#ifdef _DEBUG
		m_x_typeinfo = nullptr;
		m_x_sizeof = 0;
#endif
	}

	ks_any(const ks_any& r) noexcept {
		m_data_p = r.m_data_p;
		__do_addref_data(m_data_p);
		m_embed_tiny_trivial_x_mem = r.m_embed_tiny_trivial_x_mem;
#ifdef _DEBUG
		m_x_typeinfo = r.m_x_typeinfo;
		m_x_sizeof = r.m_x_sizeof;
#endif
	}

	ks_any(ks_any&& r) noexcept {
		m_data_p = r.m_data_p;
		m_embed_tiny_trivial_x_mem = r.m_embed_tiny_trivial_x_mem;
		r.m_data_p = nullptr;
		r.m_embed_tiny_trivial_x_mem = {};
#ifdef _DEBUG
		m_x_typeinfo = r.m_x_typeinfo;
		m_x_sizeof = r.m_x_sizeof;
		r.m_x_typeinfo = nullptr;
		r.m_x_sizeof = 0;
#endif
	}

	_NOINLINE ks_any& operator=(const ks_any& r) noexcept {
		if (this != &r) {
			if (m_data_p != r.m_data_p) {
				__do_release_data(m_data_p);
				m_data_p = r.m_data_p;
				__do_addref_data(m_data_p);
			}
			m_embed_tiny_trivial_x_mem = r.m_embed_tiny_trivial_x_mem;
#ifdef _DEBUG
			m_x_typeinfo = r.m_x_typeinfo;
			m_x_sizeof = r.m_x_sizeof;
#endif
		}
		return *this;
	}

	_NOINLINE ks_any& operator=(ks_any&& r) noexcept {
		if (this != &r) {
			__do_release_data(m_data_p);
			m_data_p = r.m_data_p;
			m_embed_tiny_trivial_x_mem = r.m_embed_tiny_trivial_x_mem;
			r.m_data_p = nullptr;
			r.m_embed_tiny_trivial_x_mem = {};
#ifdef _DEBUG
			m_x_typeinfo = r.m_x_typeinfo;
			m_x_sizeof = r.m_x_sizeof;
			r.m_x_typeinfo = nullptr;
			r.m_x_sizeof = 0;
#endif
		}
		return *this;
	}

	~ks_any() noexcept {
		__do_release_data(m_data_p); //instead of this->reset()
		//m_data_p = nullptr;
	}

public:
	template <class T, class X = T, class _ = std::enable_if_t<std::is_convertible_v<X, T>>>
	static ks_any of(X&& x) {
		constexpr bool can_embed = __can_embed_tiny_trivial_x<T>();
		return ks_any((T*)nullptr, std::forward<X>(x), std::bool_constant<can_embed>());
	}

private:
	template <class T>
	static constexpr bool __can_embed_tiny_trivial_x() noexcept {
		using XT = std::remove_cvref_t<T>;
		return std::is_trivial_v<XT> && std::is_standard_layout_v<XT>
			&& sizeof(XT) <= sizeof(m_embed_tiny_trivial_x_mem)
			&& alignof(XT) <= alignof(decltype(m_embed_tiny_trivial_x_mem));
	}

	template <class T, class X>
	_NOINLINE explicit ks_any(T*, X&& x, std::bool_constant<false>) {
		using XT = std::remove_cvref_t<T>;
		constexpr size_t x_offset = (sizeof(_DATA_HEADER) + alignof(XT) - 1) / alignof(XT) * alignof(XT);
		_DATA_HEADER* data_p = (_DATA_HEADER*)(new unsigned char[x_offset + sizeof(XT)]);
		::new ((void*)data_p) _DATA_HEADER();
		data_p->x_offset = int(x_offset);
		data_p->x_dtor = [](void* px) { ((XT*)px)->~XT(); };
		::new (data_p->x_addr()) XT(std::forward<X>(x));
		m_data_p = data_p;

#ifdef _DEBUG
		m_x_typeinfo = &typeid(XT);
		m_x_sizeof = sizeof(XT);
#endif
	}

	template <class T, class X>
	_NOINLINE explicit ks_any(T*, X&& x, std::bool_constant<true>) {
		using XT = std::remove_cvref_t<T>;
		m_data_p = (_DATA_HEADER*)(void*)(-1);
		::new ((XT*)(void*)(&m_embed_tiny_trivial_x_mem)) XT(std::forward<X>(x));
#ifdef _DEBUG
		m_x_typeinfo = &typeid(XT);
		m_x_sizeof = sizeof(XT);
#endif
	}

public:
	bool has_value() const noexcept { return m_data_p != nullptr; }
	bool has_value() const volatile noexcept { return m_data_p != nullptr; }

	template <class T>
	const T& get() const noexcept {
		return this->do_get<T>();
	}

private:
	template <class T>
	const T& do_get() const noexcept {
		using XT = std::remove_cvref_t<T>;

		ASSERT(this->has_value());
#ifdef _DEBUG
		ASSERT(m_x_typeinfo != nullptr && (*m_x_typeinfo == typeid(XT) || strcmp(m_x_typeinfo->name(), typeid(XT).name()) == 0));
		ASSERT(m_x_sizeof == sizeof(XT));
#endif

		if (m_data_p == nullptr) {
			ASSERT(false);
			return *((XT*)(void*)(nullptr) + 0); // NOLINT
		}
		else if (m_data_p == (void*)(-1)) {
			ASSERT(__can_embed_tiny_trivial_x<XT>());
			return *(XT*)(void*)(&m_embed_tiny_trivial_x_mem);
		}
		else {
			ASSERT(!__can_embed_tiny_trivial_x<XT>());
			return *(XT*)m_data_p->x_addr();
		}
	}

public:
	void swap(ks_any& r) noexcept {
		if (this != &r) {
			std::swap(m_data_p, r.m_data_p);
			std::swap(m_embed_tiny_trivial_x_mem, r.m_embed_tiny_trivial_x_mem);
#ifdef _DEBUG
			std::swap(m_x_typeinfo, r.m_x_typeinfo);
			std::swap(m_x_sizeof, r.m_x_sizeof);
#endif
		}
	}

	void reset() noexcept {
		__do_release_data(m_data_p);
		m_data_p = nullptr;
		m_embed_tiny_trivial_x_mem = {};
#ifdef _DEBUG
		m_x_typeinfo = nullptr;
		m_x_sizeof = 0;
#endif
	}

private:
	struct _DATA_HEADER {
		int x_offset;  //const-like
		std::atomic<int> ref_count = { 1 };
		std::function<void(void*)> x_dtor;

		void* x_addr() const noexcept { return (void*)(uintptr_t(this) + this->x_offset); }
	};

	inline static void __do_addref_data(_DATA_HEADER* data_p) noexcept {
		if (data_p != nullptr && data_p != (void*)(-1)) {
			data_p->ref_count.fetch_add(1, std::memory_order_relaxed);
		}
	}
	_NOINLINE static void __do_release_data(_DATA_HEADER* data_p) noexcept {
		if (data_p != nullptr && data_p != (void*)(-1)) {
			if (data_p->ref_count.fetch_sub(1, std::memory_order_release) == 1) {
				std::atomic_thread_fence(std::memory_order_acquire);
				data_p->x_dtor(data_p->x_addr());
				data_p->~_DATA_HEADER();
				delete[](unsigned char*)data_p;
			}
		}
	}

private:
	_DATA_HEADER* m_data_p;
	long long m_embed_tiny_trivial_x_mem; //仅当m_data_p为-1时有效
#ifdef _DEBUG
	const std::type_info* m_x_typeinfo;
	size_t m_x_sizeof;
#endif
};


namespace std {
	inline void swap(ks_any& l, ks_any& r) noexcept {
		l.swap(r);
	}
}


#endif //__KS_ANY_DEF
