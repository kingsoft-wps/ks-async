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

#pragma once

#include "../ks_async_base.h"
#include "../ktl/ks_any.h"

__KS_ASYNC_RAW_BEGIN


class ks_raw_value final {
public:
	ks_raw_value() : m_any() {}

	ks_raw_value(const ks_raw_value&) = default;
	ks_raw_value& operator=(const ks_raw_value&) = default;
	ks_raw_value(ks_raw_value&&) noexcept = default;
	ks_raw_value& operator=(ks_raw_value&&) noexcept = default;

public:
	template <class T>
	static ks_raw_value of(T&& x) {
		return ks_raw_value(std::forward<T>(x), 0);
	}

private:
	template <class T>
	explicit ks_raw_value(T&& x, int) : m_any(ks_any::of(std::forward<T>(x))) {}

public:
	template <class T>
	const T& get() const {
		return m_any.get<T>();
	}

private:
	ks_any m_any;
};


__KS_ASYNC_RAW_END
