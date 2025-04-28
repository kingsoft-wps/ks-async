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
#include "ks_async_context.h"
#include "ktl/ks_any.h"
#include <string>
#include <memory>


class ks_notification;
class ks_notification_builder;


_ABSTRACT class ks_notification {
public:
	ks_notification(const ks_notification&) = default;
	ks_notification(ks_notification&&) noexcept = default;

	ks_notification& operator=(const ks_notification&) = default;
	ks_notification& operator=(ks_notification&& r) noexcept = default; 

public:
	const void* get_sender() const {
		return m_data_ptr->sender;
	}

	const char* get_name() const {
		return m_data_ptr->name.c_str();
	}

	bool has_payload() const {
		return m_data_ptr->payload_any.has_value();
	}

	template <class T>
	const T& get_payload() const {
		return m_data_ptr->payload_any.get<T>();
	}

	const ks_async_context& get_context() const {
		return m_data_ptr->context;
	}

private:
	struct __NOTIFICATION_DATA {
		const void* sender = nullptr;
		std::string name;
		ks_any payload_any;
		ks_async_context context;
	};

	explicit ks_notification() = delete;
	explicit ks_notification(std::shared_ptr<__NOTIFICATION_DATA>&& data_ptr) : m_data_ptr(std::move(data_ptr)) {}

	friend class ks_notification_builder;

private:
	std::shared_ptr<__NOTIFICATION_DATA> m_data_ptr;
};


class ks_notification_builder final {
public:
	ks_notification_builder() : m_data{} {}
	_DISABLE_COPY_CONSTRUCTOR(ks_notification_builder);

public:
	ks_notification_builder& set_sender(const void* sender) {
		m_data.sender = sender;
		return *this;
	}

	ks_notification_builder& set_name(const char* name) {
		m_data.name = name;
		return *this;
	}

	template <class T, class X = T, class _ = std::enable_if_t<std::is_convertible_v<X, T>>>
	ks_notification_builder& set_payload(X&& payload) {
		m_data.payload_any = ks_any::of<T>(std::forward<X>(payload));
		return *this;
	}

	ks_notification_builder& set_context(const ks_async_context& context) {
		m_data.context = context;
		return *this;
	}

public:
	ks_notification build() {
		ASSERT(m_data.sender != nullptr);
		ASSERT(!m_data.name.empty());

		std::shared_ptr<__NOTIFICATION_DATA> data_ptr = std::make_shared<__NOTIFICATION_DATA>(std::move(m_data));
#ifdef _DEBUG
		m_data = __NOTIFICATION_DATA{};
#endif

		return ks_notification(std::move(data_ptr));
	}

private:
	using __NOTIFICATION_DATA = ks_notification::__NOTIFICATION_DATA;

	__NOTIFICATION_DATA m_data;
};
