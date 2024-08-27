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
#include "ks_apartment.h"
#include "ks_async_context.h"
#include "ktl/ks_any.h"
#include <string>
#include <memory>

class ks_notification;
class ks_notification_center;


class ks_notification {
public:
	template <class DATA_TYPE>
	explicit ks_notification(const void* sender, const char* notification_name, const ks_async_context& notification_context, DATA_TYPE&& notification_data)
		: m_props(std::make_shared<_NOTIFICATION_PROPS>()) {
		m_props->sender = sender;
		m_props->notification_name = notification_name != nullptr ? notification_name : "";
		m_props->notification_context = notification_context;
		m_props->notification_data_any = ks_any::of(std::forward<DATA_TYPE>(notification_data));
	}

	ks_notification(const ks_notification&) = default;
	ks_notification& operator=(const ks_notification&) = default;
	ks_notification(ks_notification&&) noexcept = default;
	ks_notification& operator=(ks_notification&&) noexcept = default;

public:
	const void* get_sender() const { ASSERT(m_props != nullptr); return m_props->sender; }
	const char* get_notification_name() const { ASSERT(m_props != nullptr); return m_props->notification_name.c_str(); }
	const ks_async_context& get_notification_context() const { ASSERT(m_props != nullptr); return m_props->notification_context; }
	template <class DATA_TYPE>
	const DATA_TYPE& get_notification_data() const { ASSERT(m_props != nullptr); return m_props->notification_data_any.get<DATA_TYPE>(); }

private:
	struct _NOTIFICATION_PROPS {
		const void* sender;
		std::string notification_name;
		ks_async_context notification_context;
		ks_any notification_data_any;
	};

	std::shared_ptr<_NOTIFICATION_PROPS> m_props;
};


class ks_notification_center {
private:
	enum class __raw_ctor { v };

public:
	ks_notification_center() = delete;
	_DISABLE_COPY_CONSTRUCTOR(ks_notification_center);

	explicit ks_notification_center(__raw_ctor, const char* center_name);
	~ks_notification_center();

public:
	KS_ASYNC_API static ks_notification_center* default_center();
	KS_ASYNC_API static std::shared_ptr<ks_notification_center> _create_center(const char* center_name);

public:
	KS_ASYNC_API void add_observer(
		const void* observer, const char* notification_name, 
		ks_apartment* apartment, const ks_async_context& context, std::function<void(const ks_notification&)>&& fn);

	KS_ASYNC_API void remove_observer(const void* observer, const char* notification_name);
	KS_ASYNC_API void remove_observer(const void* observer);

public:
	template <class DATA_TYPE>
	KS_ASYNC_INLINE_API void post_notification(const void* sender, const char* notification_name, const ks_async_context& notification_context, DATA_TYPE&& notification_data) {
		return this->do_post_notification(ks_notification(sender, notification_name, notification_context, std::forward<DATA_TYPE>(notification_data)));
	}

private:
	KS_ASYNC_API void do_post_notification(const ks_notification& notification);

private:
	class __ks_notification_center_data;
	__ks_notification_center_data* m_d;
};
