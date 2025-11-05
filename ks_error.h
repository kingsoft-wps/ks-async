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
	KS_ASYNC_INLINE_API ks_error() noexcept = default;
	KS_ASYNC_INLINE_API ks_error(const ks_error&) noexcept = default;
	KS_ASYNC_INLINE_API ks_error& operator=(const ks_error&) noexcept = default;
	KS_ASYNC_INLINE_API ks_error(ks_error&&) noexcept = default;
	KS_ASYNC_INLINE_API ks_error& operator=(ks_error&&) noexcept = default;

private:
	__KS_ASYNC_PRIVATE_INLINE_API explicit ks_error(HRESULT code) noexcept : m_code(code) { ASSERT(code != 0); }
	__KS_ASYNC_PRIVATE_INLINE_API explicit ks_error(HRESULT code, ks_any&& payload_any) noexcept : m_code(code), m_payload_any(std::move(payload_any)) {}

public:
	KS_ASYNC_INLINE_API static ks_error of(HRESULT code) noexcept {
		return ks_error(code); 
	}
	template <class T, class X = T, class _ = std::enable_if_t<std::is_convertible_v<X, T>>>
	KS_ASYNC_INLINE_API static ks_error __of(HRESULT code, X&& payload) {
		 return ks_error(code, ks_any::of<T>(std::forward<X>(payload)));
	}

public:
	template <class T, class X = T, class _ = std::enable_if_t<std::is_convertible_v<X, T>>>
	KS_ASYNC_INLINE_API ks_error with_payload(X&& payload) const {
		return ks_error(m_code, ks_any::of<T>(std::forward<X>(payload)));
	}

public:
	KS_ASYNC_INLINE_API bool has_code() const noexcept { return m_code != 0; }
	KS_ASYNC_INLINE_API bool has_code() const volatile noexcept { return m_code != 0; }

	KS_ASYNC_INLINE_API bool has_payload() const noexcept { return m_payload_any.has_value(); }
	KS_ASYNC_INLINE_API bool has_payload() const volatile noexcept { return m_payload_any.has_value(); }

	KS_ASYNC_INLINE_API HRESULT get_code() const noexcept { return m_code; }
	KS_ASYNC_INLINE_API HRESULT get_code() const volatile noexcept { return m_code; } //也额外提供一下get_code的volatile版本实现

	template <class T>
	KS_ASYNC_INLINE_API const T& get_payload() const noexcept { return m_payload_any.get<T>(); }

public:
	KS_ASYNC_INLINE_API void swap(ks_error& r) noexcept {
		if (this != &r) {
			std::swap(m_code, r.m_code);
			m_payload_any.swap(r.m_payload_any);
		}
	}

	KS_ASYNC_INLINE_API void reset() noexcept {
		m_code = 0;
		m_payload_any.reset();
	}

private:
	HRESULT m_code = 0;
	ks_any  m_payload_any;

public: //pre-defined errors
	KS_ASYNC_INLINE_API static ks_error unexpected_error()  noexcept { return ks_error::of(UNEXPECTED_ERROR_CODE); }
	KS_ASYNC_INLINE_API static ks_error timeout_error()     noexcept { return ks_error::of(TIMEOUT_ERROR_CODE); }
	KS_ASYNC_INLINE_API static ks_error cancelled_error()   noexcept { return ks_error::of(CANCELLED_ERROR_CODE); }
	KS_ASYNC_INLINE_API static ks_error interrupted_error() noexcept { return ks_error::of(INTERRUPTED_ERROR_CODE); }
	KS_ASYNC_INLINE_API static ks_error terminated_error()  noexcept { return ks_error::of(TERMINATED_ERROR_CODE); }

	KS_ASYNC_INLINE_API static ks_error general_error()     noexcept { return ks_error::of(GENERAL_ERROR_CODE); }
	KS_ASYNC_INLINE_API static ks_error eof_error()         noexcept { return ks_error::of(EOF_ERROR_CODE); }
	KS_ASYNC_INLINE_API static ks_error arg_error()         noexcept { return ks_error::of(ARG_ERROR_CODE); }
	KS_ASYNC_INLINE_API static ks_error data_error()        noexcept { return ks_error::of(DATA_ERROR_CODE); }
	KS_ASYNC_INLINE_API static ks_error status_error()      noexcept { return ks_error::of(STATUS_ERROR_CODE); }

public: //pre-defined const error-codes
	KS_ASYNC_INLINE_API static constexpr HRESULT UNEXPECTED_ERROR_CODE    = 0xFF338001;
	KS_ASYNC_INLINE_API static constexpr HRESULT TIMEOUT_ERROR_CODE       = 0xFF338002;
	KS_ASYNC_INLINE_API static constexpr HRESULT CANCELLED_ERROR_CODE     = 0xFF338003;
	KS_ASYNC_INLINE_API static constexpr HRESULT INTERRUPTED_ERROR_CODE   = 0xFF338004;
	KS_ASYNC_INLINE_API static constexpr HRESULT TERMINATED_ERROR_CODE    = 0xFF338005;

	KS_ASYNC_INLINE_API static constexpr HRESULT GENERAL_ERROR_CODE       = 0xFF339001;
	KS_ASYNC_INLINE_API static constexpr HRESULT EOF_ERROR_CODE           = 0xFF339002;
	KS_ASYNC_INLINE_API static constexpr HRESULT ARG_ERROR_CODE           = 0xFF339003;
	KS_ASYNC_INLINE_API static constexpr HRESULT DATA_ERROR_CODE          = 0xFF339004;
	KS_ASYNC_INLINE_API static constexpr HRESULT STATUS_ERROR_CODE        = 0xFF339005;
};


namespace std {
	inline void swap(ks_error& l, ks_error& r) noexcept {
		l.swap(r);
	}
}

