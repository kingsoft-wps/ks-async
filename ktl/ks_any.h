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
#include "ks_cxxbase.h"
#include "ks_type_traits.h"
#include <atomic>
#include <cstring>
#include <functional>
#include <stdlib.h>

#ifndef __KS_ANY_DEF
#define __KS_ANY_DEF

//与c++17的std::any相比，ks_any约束要更多一点，用法也稍有不同，但大差不差
class ks_any {
public:
	ks_any() {}
	~ks_any() { this->reset(); }

	ks_any(const ks_any& r) {
		m_data_p = r.m_data_p;
		m_embed_tiny_trivial_x_mem = r.m_embed_tiny_trivial_x_mem;
		if (m_data_p != nullptr && m_data_p != (void*)(-1))
			++m_data_p->ref_count;
#ifdef _DEBUG
		m_x_typeinfo = r.m_x_typeinfo;
		m_x_sizeof = r.m_x_sizeof;
#endif
	}

	ks_any& operator=(const ks_any& r) {
		if (this != &r) {
			if (m_data_p != r.m_data_p) {
				this->reset();
				m_data_p = r.m_data_p;
				m_embed_tiny_trivial_x_mem = r.m_embed_tiny_trivial_x_mem;
				if (m_data_p != nullptr && m_data_p != (void*)(-1))
					++m_data_p->ref_count;
			}
			else {
				m_embed_tiny_trivial_x_mem = r.m_embed_tiny_trivial_x_mem;
			}
#ifdef _DEBUG
			m_x_typeinfo = r.m_x_typeinfo;
			m_x_sizeof = r.m_x_sizeof;
#endif
		}
		return *this;
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

	ks_any& operator=(ks_any&& r) noexcept {
		this->reset();
		m_data_p = r.m_data_p;
		m_embed_tiny_trivial_x_mem = r.m_embed_tiny_trivial_x_mem;
		r.m_data_p = nullptr;
		r.m_embed_tiny_trivial_x_mem = {};
#ifdef _DEBUG
		m_x_typeinfo = r.m_x_typeinfo;
		m_x_sizeof = r.m_x_sizeof;
		r.m_data_p = nullptr;
		r.m_x_sizeof = 0;
#endif
return *this;
	}

private:
	template <class T>
	explicit ks_any(T&& x, int) {
		using XT = std::remove_cvref_t<T>;
		if (__can_embed_tiny_trivial_x<XT>()) {
			*(XT*)(void*)(&m_embed_tiny_trivial_x_mem) = x;
			m_data_p = (_DATA_HEADER*)(void*)(-1);
		}
		else {
			constexpr size_t x_offset = (sizeof(_DATA_HEADER) + alignof(XT) - 1) / alignof(XT) * alignof(XT);
			_DATA_HEADER* data_p = (_DATA_HEADER*)malloc(x_offset + sizeof(XT));
			::new ((void*)data_p) _DATA_HEADER();
			data_p->x_offset = int(x_offset);
			data_p->x_dtor = [](void* px) { ((XT*)px)->~XT(); };
			::new (data_p->x_addr()) XT(std::forward<T>(x));
			m_data_p = data_p;
		}

#ifdef _DEBUG
		m_x_typeinfo = &typeid(XT);
		m_x_sizeof = sizeof(XT);
#endif
	}

	template <class T>
	static constexpr bool __can_embed_tiny_trivial_x() {
		using XT = std::remove_cvref_t<T>;
		return std::is_trivially_copy_assignable_v<XT>
			&& std::is_trivially_destructible_v<XT>
			&& sizeof(XT) <= sizeof(m_embed_tiny_trivial_x_mem);
	}

public:
	template <class T>
	static ks_any of(T&& x) {
		return ks_any(std::forward<T>(x), 0);
	}

	bool has_value() const {
		return m_data_p != nullptr;
	}

	template <class T>
	const T& get() const {
		return this->cast<T>();
	}

	template <class T>
	T& cast() {
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

	template <class T>
	const T& cast() const {
		return const_cast<ks_any*>(this)->cast<T>();
	}

	void reset() {
		if (m_data_p != nullptr && m_data_p != (void*)(-1)) {
			if (--m_data_p->ref_count == 0) {
				m_data_p->x_dtor(m_data_p->x_addr());
				m_data_p->~_DATA_HEADER();
				free((void*)m_data_p);
			}
		}

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

		void* x_addr() const { return (void*)(uintptr_t(this) + this->x_offset); }
	};

	_DATA_HEADER* m_data_p = nullptr;
	long long m_embed_tiny_trivial_x_mem = {}; //仅当m_data_p为-1时有效

#ifdef _DEBUG
	const std::type_info* m_x_typeinfo = nullptr;
	size_t m_x_sizeof = 0;
#endif
};

#endif //__KS_ANY_DEF
