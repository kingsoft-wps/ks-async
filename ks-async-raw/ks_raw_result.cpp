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

#include "ks_raw_result.h"

void __forcelink_to_ks_raw_result_cpp() {}


__KS_ASYNC_RAW_BEGIN

ks_raw_result::ks_raw_result(const ks_raw_result& r) noexcept
	: m_state(r.m_state) {
	switch (m_state) {
	case _STATE::JUST_VALUE: ::new (&m_value_u) ks_raw_value(r.m_value_u); break;
	case _STATE::JUST_ERROR: ::new (&m_error_u) ks_error(r.m_error_u); break;
	default: ASSERT(m_state == _STATE::NOT_COMPLETED); break;
	}
}

ks_raw_result::ks_raw_result(ks_raw_result&& r) noexcept
	: m_state(r.m_state) {
	switch (m_state) {
	case _STATE::JUST_VALUE: ::new (&m_value_u) ks_raw_value(std::move(r.m_value_u)); break;
	case _STATE::JUST_ERROR: ::new (&m_error_u) ks_error(std::move(r.m_error_u)); break;
	default: ASSERT(m_state == _STATE::NOT_COMPLETED); break;
	}
}

ks_raw_result& ks_raw_result::operator=(const ks_raw_result& r) noexcept {
	if (this != &r) {
		this->~ks_raw_result();
		::new (this) ks_raw_result(r);
	}
	return *this;
}

ks_raw_result& ks_raw_result::operator=(ks_raw_result&& r) noexcept {
	if (this != &r) {
		this->~ks_raw_result();
		::new (this) ks_raw_result(std::move(r));
	}
	return *this;
}


const ks_raw_value& ks_raw_result::to_value() const {
	if (m_state == _STATE::JUST_VALUE) {
		return m_value_u;
	}
	else {
		ASSERT(false);
		throw m_state == _STATE::JUST_ERROR ? m_error_u : ks_error::unexpected_error();
	}
}

ks_error ks_raw_result::to_error() const {
	if (m_state == _STATE::JUST_ERROR) {
		return m_error_u;
	}
	else {
		ASSERT(false);
		throw ks_error::unexpected_error();
	}
}

ks_raw_result ks_raw_result::require_completed_or_error() const {
	switch (m_state) {
	case _STATE::JUST_VALUE:
		if (this->to_value().has_value())
			return *this;

		ASSERT(false);
		break;

	case _STATE::JUST_ERROR:
		if (this->to_error().has_code())
			return *this;

		ASSERT(false);
		break;

	default:
		ASSERT(m_state == _STATE::NOT_COMPLETED);
		break;
	}

	ASSERT(false);
	return ks_error::unexpected_error();
}


void ks_raw_result::swap(ks_raw_result& r) noexcept {
	if (this != &r) {
		ks_raw_result tmp = std::move(*this);
		*this = std::move(r);
		r = std::move(tmp);
	}
}

void ks_raw_result::reset() noexcept {
	switch (m_state) {
	case _STATE::JUST_VALUE: m_value_u.~ks_raw_value(); m_state = _STATE::NOT_COMPLETED; break;
	case _STATE::JUST_ERROR: m_error_u.~ks_error(); m_state = _STATE::NOT_COMPLETED; break;
	default: ASSERT(m_state == _STATE::NOT_COMPLETED); break;
	}
}


__KS_ASYNC_RAW_END
