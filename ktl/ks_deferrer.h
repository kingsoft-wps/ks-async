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
#include <functional>
#include <vector>


#ifndef __KS_DEFERRER_DEF
#define __KS_DEFERRER_DEF

class ks_deferrer {
public:
	ks_deferrer() {}
	ks_deferrer(std::function<void()>&& fn) { this->add(std::move(fn)); }
	_DISABLE_COPY_CONSTRUCTOR(ks_deferrer);

	~ks_deferrer() { this->apply(); }

public:
	ks_deferrer& add(std::function<void()>&& fn) {
		if (fn) {
			if (!m_pri_fn)
				m_pri_fn = std::move(fn);
			else
				m_more_fns.push_back(std::move(fn));
		}

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

	void reset() {
		m_more_fns.clear();
		m_pri_fn = {};
	}

private:
	std::function<void()> m_pri_fn;
	std::vector<std::function<void()>> m_more_fns;
};

#endif //__KS_DEFERRER_DEF
