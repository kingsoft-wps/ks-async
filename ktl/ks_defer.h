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

#ifndef __KS_DEFER_DEF
#define __KS_DEFER_DEF

#include "ks_cxxbase.h"
#include <functional>
#include <vector>


class ks_defer {
public:
	ks_defer() noexcept {}
	explicit ks_defer(std::function<void()>&& fn) noexcept : m_pri_fn(std::move(fn)) {}

	ks_defer(const ks_defer&) = delete;
	ks_defer(ks_defer&& other) noexcept { //仅支持移动构造
		m_pri_fn.swap(other.m_pri_fn);
		m_more_fns.swap(other.m_more_fns);
	}

	ks_defer& operator=(const ks_defer&) = delete;
	ks_defer& operator=(ks_defer&&) noexcept = delete;

	~ks_defer() { this->apply(); }

public:
	template <class FN, class _ = std::enable_if_t<std::is_convertible_v<FN, std::function<void()>>>>
	ks_defer& add(FN&& fn) {
		if (!m_pri_fn)
			m_pri_fn = std::forward<FN>(fn);
		else
			m_more_fns.push_back(std::forward<FN>(fn));

		return *this;
	}

public:
	void apply() {
		for (auto it = m_more_fns.crbegin(); it != m_more_fns.crend(); ++it)
			(*it)();
		if (m_pri_fn)
			m_pri_fn();

		m_more_fns.clear();
		m_pri_fn = {};
	}

	void reset() noexcept {
		m_more_fns.clear();
		m_pri_fn = {};
	}

private:
	std::function<void()> m_pri_fn;
	std::vector<std::function<void()>> m_more_fns;
};

#endif //__KS_DEFER_DEF
