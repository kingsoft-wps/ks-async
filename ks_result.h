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
#include "ks-async-raw/ks_raw_result.h"


template <class T>
class ks_result final {
public:
	ks_result(const T& value) : m_raw_result(ks_raw_value::of(value)) {}
	ks_result(T&& value) : m_raw_result(ks_raw_value::of(std::move(value))) {}

	ks_result(const ks_error& error) : m_raw_result(error) {}
	ks_result(ks_error&& error) : m_raw_result(std::move(error)) {}

	ks_result(const ks_result&) = default;
	ks_result(ks_result&&) noexcept = default;
	ks_result& operator=(const ks_result&) = default;
	ks_result& operator=(ks_result&&) noexcept = default;

	using value_type = T;
	using this_result_type = ks_result<T>;

public:
	bool is_completed() const { return m_raw_result.is_completed(); }
	bool is_value() const { return m_raw_result.is_value(); }
	bool is_error() const { return m_raw_result.is_error(); }

	const T& to_value() const noexcept(false) { return m_raw_result.to_value().template get<T>(); }
	const ks_error& to_error() const noexcept(false) { return m_raw_result.to_error(); }

	template <class R>
	ks_result<R> cast() const {
		constexpr __cast_mode_t cast_mode = __determine_cast_mode<R>();
		return __do_cast<R>(std::integral_constant<__cast_mode_t, cast_mode>());
	}

private:
	enum class __cast_mode_t { invalid, to_same, to_nothing, to_other };

	template <class R>
	static constexpr __cast_mode_t __determine_cast_mode() {
		using XT = std::remove_cvref_t<T>;
		using XR = std::remove_cvref_t<R>;
		using PROXT = std::conditional_t<std::is_void_v<XT>, nothing_t, XT>;
		using PROXR = std::conditional_t<std::is_void_v<XR>, nothing_t, XR>;

		if (std::is_same_v<PROXR, PROXT>)
			return __cast_mode_t::to_same;
		else if (std::is_nothing_v<PROXR>)
			return __cast_mode_t::to_nothing;
		else if (std::is_convertible_v<PROXT, PROXR>)
			return __cast_mode_t::to_other;
		else
			return __cast_mode_t::invalid;
	}

	template <class R>
	ks_result<R> __do_cast(std::integral_constant<__cast_mode_t, __cast_mode_t::to_same> __cast_mode) const {
		using XT = std::remove_cvref_t<T>;
		using XR = std::remove_cvref_t<R>;
		using PROXT = std::conditional_t<std::is_void_v<XT>, nothing_t, XT>;
		using PROXR = std::conditional_t<std::is_void_v<XR>, nothing_t, XR>;
		static_assert(std::is_same_v<PROXR, PROXT>, "ks_result::cast_to mode 1 error");

		return ks_result<R>::__from_raw(m_raw_result);
	}

	template <class R>
	ks_result<R> __do_cast(std::integral_constant<__cast_mode_t, __cast_mode_t::to_nothing> __cast_mode) const {
		//using XT = std::remove_cvref_t<T>;
		using XR = std::remove_cvref_t<R>;
		//using PROXT = std::conditional_t<std::is_void_v<XT>, nothing_t, XT>;
		using PROXR = std::conditional_t<std::is_void_v<XR>, nothing_t, XR>;
		static_assert(std::is_nothing_v<PROXR>, "ks_result::cast_to mode 2 error");

		ks_raw_result raw_result2 = m_raw_result.is_value() ? ks_raw_value::of(nothing) : m_raw_result;
		return ks_result<R>::__from_raw(raw_result2);
	}

	template <class R>
	ks_result<R> __do_cast(std::integral_constant<__cast_mode_t, __cast_mode_t::to_other> __cast_mode) const {
		using XT = std::remove_cvref_t<T>;
		using XR = std::remove_cvref_t<R>;
		using PROXT = std::conditional_t<std::is_void_v<XT>, nothing_t, XT>;
		using PROXR = std::conditional_t<std::is_void_v<XR>, nothing_t, XR>;
		static_assert(std::is_convertible_v<PROXT, PROXR>, "ks_result::cast_to mode 3 error");

		ks_raw_result raw_result2 = m_raw_result.is_value() ? ks_raw_value::of(static_cast<PROXR>(m_raw_result.to_value().template get<PROXT>())) : m_raw_result;
		return ks_result<R>::__from_raw(raw_result2);
	}

private:
	using ks_raw_result = __ks_async_raw::ks_raw_result;
	using ks_raw_value = __ks_async_raw::ks_raw_value;

	enum class __raw_ctor { v };
	explicit ks_result(__raw_ctor) : m_raw_result() {}

	explicit ks_result(const ks_raw_result& raw_result, int) : m_raw_result(raw_result) {}
	explicit ks_result(ks_raw_result&& raw_result, int) : m_raw_result(std::move(raw_result)) {}

	static ks_result __from_raw(const ks_raw_result& raw_result) {
		return ks_result(raw_result, 0);
	}

	static ks_result __from_raw(ks_raw_result&& raw_result) {
		return ks_result(std::move(raw_result), 0);
	}

	ks_raw_result __get_raw() const {
		return m_raw_result;
	}

	template <class T2> friend class ks_result;
	template <class T2> friend class ks_future;
	template <class T2> friend class ks_promise;
	friend class ks_future_util;

private:
	ks_raw_result m_raw_result;
};


#include "ks_result_void.inl"
