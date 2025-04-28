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
#include "../ks_error.h"
#include "ks_raw_value.h"

__KS_ASYNC_RAW_BEGIN


class ks_raw_result final {
public:
	ks_raw_result() : m_state(_STATE::NOT_COMPLETED) {}

	ks_raw_result(const ks_raw_value& value) : m_state(_STATE::JUST_VALUE) {
		::new (this->__raw_value_data_ptr()) ks_raw_value(value);
	}
	ks_raw_result(ks_raw_value&& value) noexcept : m_state(_STATE::JUST_VALUE) {
		::new (this->__raw_value_data_ptr()) ks_raw_value(std::move(value));
	}

	ks_raw_result(const ks_error& error) : m_state(_STATE::JUST_ERROR) {
		ASSERT(error.get_code() != 0);
		::new (this->__error_data_ptr()) ks_error(error);
	}
	ks_raw_result(ks_error&& error) noexcept : m_state(_STATE::JUST_ERROR) {
		ASSERT(error.get_code() != 0);
		::new (this->__error_data_ptr()) ks_error(std::move(error));
	}

	ks_raw_result(const ks_raw_result& r) : m_state(r.m_state) {
		switch (m_state) {
		case _STATE::JUST_VALUE: ::new (this->__raw_value_data_ptr()) ks_raw_value(*r.__raw_value_data_ptr()); break;
		case _STATE::JUST_ERROR: ::new (this->__error_data_ptr()) ks_error(*r.__error_data_ptr()); break;
		default: break;
		}
	}
	ks_raw_result(ks_raw_result&& r) noexcept : m_state(r.m_state) {
		switch (m_state) {
		case _STATE::JUST_VALUE: ::new (this->__raw_value_data_ptr()) ks_raw_value(std::move(*r.__raw_value_data_ptr())); break;
		case _STATE::JUST_ERROR: ::new (this->__error_data_ptr()) ks_error(std::move(*r.__error_data_ptr())); break;
		default: break;
		}
	}
	ks_raw_result& operator=(const ks_raw_result& r) {
		if (this != &r) {
			this->~ks_raw_result();
			::new (this) ks_raw_result(r);
		}
		return *this;
	}
	ks_raw_result& operator=(ks_raw_result&& r) noexcept {
		ASSERT(this != &r);
		this->~ks_raw_result();
		::new (this) ks_raw_result(std::move(r));
		return *this;
	}

	~ks_raw_result() {
		switch (m_state) {
		case _STATE::JUST_VALUE: this->__raw_value_data_ptr()->~ks_raw_value(); break;
		case _STATE::JUST_ERROR: this->__error_data_ptr()->~ks_error(); break;
		default: break;
		}
	}

public:
	bool is_completed() const { return (volatile _STATE&)m_state != _STATE::NOT_COMPLETED; }
	bool is_value() const { return (volatile _STATE&)m_state == _STATE::JUST_VALUE; }
	bool is_error() const { return (volatile _STATE&)m_state == _STATE::JUST_ERROR; }

	const ks_raw_value& to_value() const noexcept(false) {
		if (m_state == _STATE::JUST_VALUE)
			return *this->__raw_value_data_ptr();
		else
			throw m_state == _STATE::JUST_ERROR ? *this->__error_data_ptr() : ks_error::unexpected_error();
	}

	ks_error to_error() const noexcept(false) {
		if (m_state == _STATE::JUST_ERROR)
			return *this->__error_data_ptr();
		else {
			ASSERT(false);
			return ks_error();
		}
	}

	ks_raw_result require_completed_or_error() const { 
		if (this->is_value())
			return *this;
		else if (this->is_error() && this->to_error().get_code() != 0)
			return *this;
		else 
			return ks_error::unexpected_error();
	}

private:
	ks_raw_value* __raw_value_data_ptr() { return (ks_raw_value*)(&m_data_memory); }
	ks_error* __error_data_ptr() { return (ks_error*)(&m_data_memory); }

	const ks_raw_value* __raw_value_data_ptr() const { return (ks_raw_value*)(&m_data_memory); }
	const ks_error* __error_data_ptr() const { return (ks_error*)(&m_data_memory); }

private:
	enum class _STATE { NOT_COMPLETED, JUST_VALUE, JUST_ERROR };
	_STATE m_state;
	std::aligned_union_t<0, ks_raw_value, ks_error> m_data_memory;
};


__KS_ASYNC_RAW_END
