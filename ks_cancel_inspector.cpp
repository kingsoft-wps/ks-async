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

#include "ks_cancel_inspector.h"
#include "ks-async-raw/ks_raw_future.h"


void __forcelink_to_ks_cancel_inspector_cpp() {}


class ks_cancel_inspector_for_future final : public ks_cancel_inspector {
public:
	ks_cancel_inspector_for_future() = default;
	_DISABLE_COPY_CONSTRUCTOR(ks_cancel_inspector_for_future);

public:
	virtual bool check_cancelled() override {
		return ks_raw_future::__check_current_future_cancelled();
	}

private:
	using ks_raw_future = __ks_async_raw::ks_raw_future;
};


ks_cancel_inspector* ks_cancel_inspector::__for_future() noexcept {
	static ks_cancel_inspector_for_future g_cancel_inspector_for_future;
	return &g_cancel_inspector_for_future;
}
