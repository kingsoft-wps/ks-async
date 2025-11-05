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

class ks_raw_value : private ks_any {
public:
	KS_ASYNC_INLINE_API ks_raw_value() noexcept = default;
	KS_ASYNC_INLINE_API ks_raw_value(const ks_raw_value& v) noexcept = default;
	KS_ASYNC_INLINE_API ks_raw_value& operator=(const ks_raw_value& v) noexcept = default;
	KS_ASYNC_INLINE_API ks_raw_value(ks_raw_value&& v) noexcept = default;
	KS_ASYNC_INLINE_API ks_raw_value& operator=(ks_raw_value&& v) noexcept = default;

private:
	__KS_ASYNC_PRIVATE_INLINE_API explicit ks_raw_value(ks_any&& r) noexcept : ks_any(std::move(r)) {}

public:
	template <class T, class X = T, class _ = std::enable_if_t<std::is_convertible_v<X, T>>>
	KS_ASYNC_INLINE_API static ks_raw_value of(X&& x) noexcept {
		return ks_raw_value(ks_any::of<T>(std::forward<X>(x))); 
	}

public:
	KS_ASYNC_INLINE_API bool has_value() const noexcept { 
		return ks_any::has_value(); 
	}
	KS_ASYNC_INLINE_API bool has_value() const volatile noexcept { 
		return ks_any::has_value(); 
	}

	template <class T>
	KS_ASYNC_INLINE_API const T& get() const noexcept { 
		return ks_any::template get<T>(); 
	}

public:
	KS_ASYNC_INLINE_API void swap(ks_raw_value& r) noexcept {
		ks_any::swap(r);
	}

	KS_ASYNC_INLINE_API void reset() noexcept {
		ks_any::reset();
	}
};

__KS_ASYNC_RAW_END


namespace std {
	inline void swap(__ks_async_raw::ks_raw_value& l, __ks_async_raw::ks_raw_value& r) noexcept {
		l.swap(r);
	}
}
