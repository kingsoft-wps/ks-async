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
#include "ks_error.h"

_ABSTRACT class ks_cancel_inspector {
public:
	virtual bool check_cancel() = 0;

protected:
	ks_cancel_inspector() = default;
	~ks_cancel_inspector() = default;
	_DISABLE_COPY_CONSTRUCTOR(ks_cancel_inspector);

private:
	KS_ASYNC_API static ks_cancel_inspector* __for_future();
	template <class T2> friend class ks_future;
	friend class ks_future_util;
};
