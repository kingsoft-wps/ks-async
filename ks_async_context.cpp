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

#include "ks_async_context.h"

void __forcelink_to_ks_async_context_cpp() {}


bool ks_async_context::__do_recur_check_need_lock_owner_ptr(_FAT_DATA* fat_data_p) {
	return fat_data_p != nullptr && (fat_data_p->owner_ptr_is_weak || __do_recur_check_need_lock_owner_ptr(fat_data_p->parent_fat_data_p));
}

ks_any ks_async_context::__do_recur_lock_owner_ptr(_FAT_DATA* fat_data_p) {
	const bool owner_need_lock = fat_data_p != nullptr && fat_data_p->owner_ptr_is_weak;
	const bool parent_need_lock = fat_data_p != nullptr && fat_data_p->parent_fat_data_p != nullptr && __do_recur_check_need_lock_owner_ptr(fat_data_p->parent_fat_data_p);

	if (owner_need_lock && parent_need_lock) {
		ks_any self_locker = fat_data_p->owner_ptr_is_weak ? fat_data_p->owner_pointer_try_lock_fn() : ks_any::of<bool>(true);
		if (self_locker.has_value()) {
			ks_any parent_locker = __do_recur_lock_owner_ptr(fat_data_p->parent_fat_data_p);
			if (parent_locker.has_value())
				return ks_any::of<std::pair<ks_any, ks_any>>(std::make_pair(self_locker, parent_locker));

			__do_recur_unlock_owner_ptr(fat_data_p->parent_fat_data_p, parent_locker);
		}

		fat_data_p->owner_pointer_unlock_fn(self_locker);
		return ks_any();
	}
	else if (parent_need_lock) {
		return __do_recur_lock_owner_ptr(fat_data_p->parent_fat_data_p);
	}
	else if (owner_need_lock) {
		return fat_data_p->owner_pointer_try_lock_fn();
	}
	else {
		return ks_any::of<bool>(true);
	}
}

void ks_async_context::__do_recur_unlock_owner_ptr(_FAT_DATA* fat_data_p, ks_any& locker) {
	if (locker.has_value()) {
		const bool owner_need_lock = fat_data_p != nullptr && fat_data_p->owner_ptr_is_weak;
		const bool parent_need_lock = fat_data_p != nullptr && fat_data_p->parent_fat_data_p != nullptr && __do_recur_check_need_lock_owner_ptr(fat_data_p->parent_fat_data_p);

		if (owner_need_lock && parent_need_lock) {
			auto sub_pair = locker.get<std::pair<ks_any, ks_any>>();
			__do_recur_unlock_owner_ptr(fat_data_p->parent_fat_data_p, sub_pair.second);
			fat_data_p->owner_pointer_unlock_fn(sub_pair.first);
		}
		else if (parent_need_lock) {
			__do_recur_unlock_owner_ptr(fat_data_p->parent_fat_data_p, locker);
		}
		else if (owner_need_lock) {
			fat_data_p->owner_pointer_unlock_fn(locker);
		}

		locker.reset();
	}
}
