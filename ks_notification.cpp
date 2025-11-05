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

#include "ks_notification.h"

void __forcelink_to_ks_notification_cpp() {}

ks_notification& ks_notification::operator=(const ks_notification&) noexcept = default;
ks_notification& ks_notification::operator=(ks_notification&&) noexcept = default;

ks_notification::~ks_notification() noexcept = default;


ks_notification ks_notification_builder::build() {
	ASSERT(m_data.sender != nullptr);
	ASSERT(!m_data.name.empty());
	return ks_notification(std::make_shared<__NOTIFICATION_DATA>(std::move(m_data)));
}
