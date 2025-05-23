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

#include "ks_async_controller.h"
#include <thread>

void __forcelink_to_ks_async_controller_cpp() {}


//慎用，使用不当可能会造成死锁或卡顿！
_DECL_DEPRECATED void ks_async_controller::__wait_all() const {
	//暂以最简陋的手段实现，最小化内存占用
	while (m_controller_data_ptr->pending_count != 0) {
		std::this_thread::yield(); 
	}
}
