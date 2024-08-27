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
#include "ks_raw_future.h"

__KS_ASYNC_RAW_BEGIN


class ks_raw_promise;
using ks_raw_promise_ptr = std::shared_ptr<ks_raw_promise>;

class ks_raw_promise {
protected:
	KS_ASYNC_API ks_raw_promise() = default;
	KS_ASYNC_API virtual ~ks_raw_promise() = default;  //protected
	_DISABLE_COPY_CONSTRUCTOR(ks_raw_promise);

public:
	KS_ASYNC_API static ks_raw_promise_ptr create(ks_apartment* apartment);

public:
	virtual ks_raw_future_ptr get_future() = 0;

	virtual void resolve(const ks_raw_value& value) = 0;
	virtual void reject(const ks_error& error) = 0;
	virtual void try_complete(const ks_raw_result& result) = 0;
};


__KS_ASYNC_RAW_END
