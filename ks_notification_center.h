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
#include "ks_notification.h"
#include "ks_apartment.h"
#include <memory>


class ks_notification_center final {
public:
	KS_ASYNC_API explicit ks_notification_center(const char* center_name);
	_DISABLE_COPY_CONSTRUCTOR(ks_notification_center);

	KS_ASYNC_API static ks_notification_center* default_center(); //default-center singleton

public:
	KS_ASYNC_API const char* name() noexcept;

public:
	KS_ASYNC_API uint64_t add_observer(
		const void* observer, const char* notification_name_pattern, 
		ks_apartment* apartment, std::function<void(const ks_notification&)> fn, const ks_async_context& context = {});

	KS_ASYNC_API void remove_observer(const void* observer, uint64_t id);

	KS_ASYNC_API void remove_observer(const void* observer);

public:
	KS_ASYNC_INLINE_API void post_notification(
		const void* sender, 
		const char* notification_name, 
		const ks_async_context& notification_context = {}) {

		return this->post_notification_indirect(
			ks_notification_builder()
				.set_sender(sender)
				.set_name(notification_name)
				.set_context(notification_context)
				.build());
	}

	template <class T, class X = T, class _ = std::enable_if_t<std::is_convertible_v<X, T>>>
	KS_ASYNC_INLINE_API void post_notification_with_payload(
		const void* sender, 
		const char* notification_name, X&& notification_payload,
		const ks_async_context& notification_context = {}) {

		return this->post_notification_indirect(
			ks_notification_builder()
				.set_sender(sender)
				.set_name(notification_name)
				.set_payload<T>(std::forward<X>(notification_payload))
				.set_context(notification_context)
				.build());
	}

	KS_ASYNC_API void post_notification_indirect(const ks_notification& notification);

private:
	class __ks_notification_center_data;
	std::shared_ptr<__ks_notification_center_data> m_d;
};
