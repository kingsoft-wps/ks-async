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
	ks_error() : m_code(0), m_payload_any() {}

	ks_error(const ks_error&) = default;
	ks_error(ks_error&&) noexcept = default;

	ks_error& operator=(const ks_error&) = default;
	ks_error& operator=(ks_error&&) noexcept = default;

public:
	static ks_error of(HRESULT code) {
		ks_error ret;
		ASSERT(code != 0);
		ret.m_code = code;
		return ret;
	}

	template <class T, class X = T, class _ = std::enable_if_t<std::is_convertible_v<X, T>>>
	static ks_error __of(HRESULT code, X&& payload) {
		ks_error ret;
		ASSERT(code != 0);
		ret.m_code = code;
		ret.m_payload_any = ks_any::of<T>(std::forward<X>(payload));
		return ret;
	}

public:
	template <class T, class X = T, class _ = std::enable_if_t<std::is_convertible_v<X, T>>>
	ks_error with_payload(X&& payload) const {
		ks_error ret = *this;
		ASSERT(ret.m_code != 0);
		ret.m_payload_any = ks_any::of<T>(std::forward<X>(payload));
		return ret;
	}

public:
	bool has_code() const { return m_code != 0; }
	bool has_code() const volatile { return m_code != 0; }

	HRESULT get_code() const { return m_code; }
	HRESULT get_code() const volatile { return m_code; } //也额外提供get_code的volatile版本实现

	bool has_payload() const { return m_payload_any.has_value(); }
	bool has_payload() const volatile { return m_payload_any.has_value(); }

	template <class T>
	const T& get_payload() const {
		return m_payload_any.get<T>();
	}
	
private:
	HRESULT m_code;
	ks_any  m_payload_any;

public:
	static ks_error unexpected_error() { return ks_error::of(UNEXPECTED_ERROR_CODE); }
	static ks_error timeout_error()    { return ks_error::of(TIMEOUT_ERROR_CODE); }
	static ks_error cancelled_error()  { return ks_error::of(CANCELLED_ERROR_CODE); }
	static ks_error interrupted_error() { return ks_error::of(INTERRUPTED_ERROR_CODE); }
	static ks_error terminated_error() { return ks_error::of(TERMINATED_ERROR_CODE); }

	static ks_error general_error()    { return ks_error::of(GENERAL_ERROR_CODE); }
	static ks_error eof_error()        { return ks_error::of(EOF_ERROR_CODE); }
	static ks_error arg_error()        { return ks_error::of(ARG_ERROR_CODE); }
	static ks_error data_error()       { return ks_error::of(DATA_ERROR_CODE); }
	static ks_error status_error()     { return ks_error::of(STATUS_ERROR_CODE); }

public:
	static constexpr HRESULT UNEXPECTED_ERROR_CODE    = 0xFF338001;
	static constexpr HRESULT TIMEOUT_ERROR_CODE       = 0xFF338002;
	static constexpr HRESULT CANCELLED_ERROR_CODE     = 0xFF338003;
	static constexpr HRESULT INTERRUPTED_ERROR_CODE   = 0xFF338004;
	static constexpr HRESULT TERMINATED_ERROR_CODE    = 0xFF338005;

	static constexpr HRESULT GENERAL_ERROR_CODE       = 0xFF339001;
	static constexpr HRESULT EOF_ERROR_CODE           = 0xFF339002;
	static constexpr HRESULT ARG_ERROR_CODE           = 0xFF339003;
	static constexpr HRESULT DATA_ERROR_CODE          = 0xFF339004;
	static constexpr HRESULT STATUS_ERROR_CODE        = 0xFF339005;
};
