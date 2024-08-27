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

#include "ks_async_base.h"
#include "ktl/ks_any.h"


class ks_error final {
public:
	ks_error() : m_code(0) {}
	explicit ks_error(HRESULT code) : m_code(code) {}

	ks_error(const ks_error&) = default;
	ks_error& operator=(const ks_error&) = default;
	ks_error(ks_error&&) noexcept = default;
	ks_error& operator=(ks_error&&) noexcept = default;

public:
	static ks_error of(HRESULT code) {
		return ks_error(code); 
	}

	template <class T>
	ks_error with_payload(T&& payload) const {
		ks_error ret = *this;
		ret.m_payload_any = ks_any::of(std::forward<T>(payload));
		return ret;
	}

public:
	HRESULT get_code() const {
		return m_code;
	}

	template <class T>
	const T& get_payload() const { 
		return m_payload_any.get<T>();
	}

private:
	HRESULT m_code;
	ks_any  m_payload_any;

public:
	static ks_error unexpected_error()     { return ks_error::of(UNEXPECTED_ERROR_CODE); }
	static ks_error was_timeout_error()    { return ks_error::of(WAS_TIMEOUT_ERROR_CODE); }
	static ks_error was_cancelled_error()  { return ks_error::of(WAS_CANCELLED_ERROR_CODE); }
	static ks_error was_terminated_error() { return ks_error::of(WAS_TERMINATED_ERROR_CODE); }

public:
	static constexpr HRESULT UNEXPECTED_ERROR_CODE     = 0xFF3C0001;
	static constexpr HRESULT WAS_TIMEOUT_ERROR_CODE    = 0xFF3C0002;
	static constexpr HRESULT WAS_CANCELLED_ERROR_CODE  = 0xFF3C0003;
	static constexpr HRESULT WAS_TERMINATED_ERROR_CODE = 0xFF3C0004;
};
