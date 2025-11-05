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
#include "ks_raw_value.h"
#include "../ks_error.h"

__KS_ASYNC_RAW_BEGIN

class ks_raw_result final {
public:
	KS_ASYNC_INLINE_API ks_raw_result() noexcept 
		: m_state(_STATE::NOT_COMPLETED) {}

	KS_ASYNC_INLINE_API ks_raw_result(const ks_raw_value& value) noexcept
		: m_state(_STATE::JUST_VALUE), m_value_u(value) {
		ASSERT(m_value_u.has_value());
	}
	KS_ASYNC_INLINE_API ks_raw_result(ks_raw_value&& value) noexcept
		: m_state(_STATE::JUST_VALUE), m_value_u(std::move(value)) {
		ASSERT(m_value_u.has_value());
	}

	KS_ASYNC_INLINE_API ks_raw_result(const ks_error& error) noexcept
		: m_state(_STATE::JUST_ERROR), m_error_u(error) {
		ASSERT(m_error_u.has_code());
	}
	KS_ASYNC_INLINE_API ks_raw_result(ks_error&& error) noexcept
		: m_state(_STATE::JUST_ERROR), m_error_u(std::move(error)) {
		ASSERT(m_error_u.has_code());
	}

public:
	KS_ASYNC_API ks_raw_result(const ks_raw_result& r) noexcept;
	KS_ASYNC_API ks_raw_result(ks_raw_result&& r) noexcept;

	KS_ASYNC_API ks_raw_result& operator=(const ks_raw_result& r) noexcept;
	KS_ASYNC_API ks_raw_result& operator=(ks_raw_result && r) noexcept;

	KS_ASYNC_INLINE_API ~ks_raw_result() noexcept { 
		this->reset(); 
	}

public:
	KS_ASYNC_INLINE_API bool is_completed() const noexcept { return m_state != _STATE::NOT_COMPLETED; }
	KS_ASYNC_INLINE_API bool is_completed() const volatile noexcept { return m_state != _STATE::NOT_COMPLETED; }

	KS_ASYNC_INLINE_API bool is_value() const noexcept { return m_state == _STATE::JUST_VALUE; }
	KS_ASYNC_INLINE_API bool is_value() const volatile noexcept { return m_state == _STATE::JUST_VALUE; }

	KS_ASYNC_INLINE_API bool is_error() const noexcept { return m_state == _STATE::JUST_ERROR; }
	KS_ASYNC_INLINE_API bool is_error() const volatile noexcept { return m_state == _STATE::JUST_ERROR; }

	KS_ASYNC_API const ks_raw_value& to_value() const;
	KS_ASYNC_API ks_error to_error() const;
	KS_ASYNC_API ks_raw_result require_completed_or_error() const;

public:
	KS_ASYNC_API void swap(ks_raw_result& r) noexcept;
	KS_ASYNC_API void reset() noexcept;

private:
	enum class _STATE { NOT_COMPLETED, JUST_VALUE, JUST_ERROR };
	_STATE m_state;
	union {
		ks_raw_value m_value_u;
		ks_error m_error_u;
	};
};

__KS_ASYNC_RAW_END


namespace std {
	inline void swap(__ks_async_raw::ks_raw_result& l, __ks_async_raw::ks_raw_result& r) noexcept {
		l.swap(r);
	}
}
