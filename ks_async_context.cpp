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


ks_async_context& ks_async_context::bind_controller(ks_async_controller* controller) {
	if (controller != nullptr) {
		do_prepare_fat_data_cow();
		m_fat_data_p->controller_data_ptr = controller->m_controller_data_ptr;
	}
	else {
		if (m_fat_data_p != nullptr)
			m_fat_data_p->controller_data_ptr.reset();
	}
	return *this;
}

ks_async_context& ks_async_context::__bind_from_source_location(const ks_source_location& from_source_location) {
#if __KS_ASYNC_CONTEXT_FROM_SOURCE_LOCATION_ENABLED
	if (!from_source_location.is_empty()) {
		do_prepare_fat_data_cow();
		m_fat_data_p->from_source_location = from_source_location;
	}
	else {
		if (m_fat_data_p != nullptr)
			m_fat_data_p->from_source_location = ks_source_location(nullptr);
	}
#else
	_UNUSED(from_source_location);
	_NOOP();
#endif
	return *this;
}

ks_async_context& ks_async_context::set_parent(const ks_async_context& parent, bool inherit_attrs) {
	//注：只保存parent.m_fat_data_p，不必保存parent.m_priority（因为无用）
	if (parent.m_fat_data_p != nullptr) {
		do_prepare_fat_data_cow();
		__do_release_fat_data(m_fat_data_p->parent_fat_data_p);
		m_fat_data_p->parent_fat_data_p = parent.m_fat_data_p;
		__do_addref_fat_data(m_fat_data_p->parent_fat_data_p);
	}
	else {
		if (m_fat_data_p != nullptr) {
			__do_release_fat_data(m_fat_data_p->parent_fat_data_p);
			m_fat_data_p->parent_fat_data_p = nullptr;
		}
	}

	if (inherit_attrs) {
		m_priority = parent.__get_priority();
	}

	return *this;
}


bool ks_async_context::__check_owner_expired() const noexcept {
	//注：递归
	for (_FAT_DATA* fat_data_p = m_fat_data_p; fat_data_p != nullptr; fat_data_p = fat_data_p->parent_fat_data_p) {
		if (fat_data_p->owner_ptr_is_weak && fat_data_p->owner_pointer_check_expired_fn())
			return true;
	}
	return false;
}

bool ks_async_context::__check_controller_cancelled() const noexcept {
	//注：递归
	for (_FAT_DATA* fat_data_p = m_fat_data_p; fat_data_p != nullptr; fat_data_p = fat_data_p->parent_fat_data_p) {
		if (fat_data_p->controller_data_ptr != nullptr && fat_data_p->controller_data_ptr->do_check_cancelled())
			return true;
	}
	return false;
}

//void ks_async_context::__increment_pending_count() const noexcept {
//	//注：递归
//	for (_FAT_DATA* fat_data_p = m_fat_data_p; fat_data_p != nullptr; fat_data_p = fat_data_p->parent_fat_data_p) {
//		if (fat_data_p->controller_data_ptr != nullptr)
//			++fat_data_p->controller_data_ptr->pending_count;
//	}
//}
// 
//void ks_async_context::__decrement_pending_count() const noexcept {
//	//注：递归
//	for (_FAT_DATA* fat_data_p = m_fat_data_p; fat_data_p != nullptr; fat_data_p = fat_data_p->parent_fat_data_p) {
//		if (fat_data_p->controller_data_ptr != nullptr)
//			--fat_data_p->controller_data_ptr->pending_count;
//	}
//}


bool ks_async_context::__dodo_check_need_lock_parent_ptr(_FAT_DATA* fat_data_p) noexcept {
	//注：递归
	ASSERT(fat_data_p != nullptr);
	for (_FAT_DATA* parent = fat_data_p->parent_fat_data_p; parent != nullptr; parent = parent->parent_fat_data_p) {
		if (parent->owner_ptr_is_weak)
			return true;
	}
	return false;
}

ks_any ks_async_context::__do_lock_owner_ptr_recursively(_FAT_DATA* fat_data_p) noexcept {
	//注：递归
	const bool owner_need_lock = fat_data_p != nullptr && fat_data_p->owner_ptr_is_weak;
	const bool parent_need_lock = fat_data_p != nullptr && __dodo_check_need_lock_parent_ptr(fat_data_p);

	if (owner_need_lock && parent_need_lock) {
		ks_any self_locker = fat_data_p->owner_ptr_is_weak ? fat_data_p->owner_pointer_try_lock_fn() : ks_any::of<bool>(true);
		if (self_locker.has_value()) {
			ks_any parent_locker = __do_lock_owner_ptr_recursively(fat_data_p->parent_fat_data_p);
			if (parent_locker.has_value())
				return ks_any::of<std::pair<ks_any, ks_any>>(std::make_pair(self_locker, parent_locker));

			__do_unlock_owner_ptr_recursively(fat_data_p->parent_fat_data_p, parent_locker);
		}

		fat_data_p->owner_pointer_unlock_fn(self_locker);
		return ks_any();
	}
	else if (parent_need_lock) {
		return __do_lock_owner_ptr_recursively(fat_data_p->parent_fat_data_p);
	}
	else if (owner_need_lock) {
		return fat_data_p->owner_pointer_try_lock_fn();
	}
	else {
		return ks_any::of<bool>(true);
	}
}

void ks_async_context::__do_unlock_owner_ptr_recursively(_FAT_DATA* fat_data_p, ks_any& locker) noexcept {
	//注：递归
	if (locker.has_value()) {
		const bool owner_need_lock = fat_data_p != nullptr && fat_data_p->owner_ptr_is_weak;
		const bool parent_need_lock = fat_data_p != nullptr && __dodo_check_need_lock_parent_ptr(fat_data_p);

		if (owner_need_lock && parent_need_lock) {
			auto sub_pair = locker.get<std::pair<ks_any, ks_any>>();
			__do_unlock_owner_ptr_recursively(fat_data_p->parent_fat_data_p, sub_pair.second);
			fat_data_p->owner_pointer_unlock_fn(sub_pair.first);
		}
		else if (parent_need_lock) {
			__do_unlock_owner_ptr_recursively(fat_data_p->parent_fat_data_p, locker);
		}
		else if (owner_need_lock) {
			fat_data_p->owner_pointer_unlock_fn(locker);
		}

		locker.reset();
	}
}

void ks_async_context::do_prepare_fat_data_cow() {
	if (m_fat_data_p == nullptr) {
		m_fat_data_p = new _FAT_DATA();
	}
	else if (m_fat_data_p->ref_count.peek_value() != 1) { //纯粹的COW判定，无需acquire
		_FAT_DATA* fatDataOrig = m_fat_data_p;
		_FAT_DATA* fatDataCopy = new _FAT_DATA();
		fatDataCopy->owner_ptr_any = fatDataOrig->owner_ptr_any;
		fatDataCopy->owner_ptr_is_weak = fatDataOrig->owner_ptr_is_weak;
		fatDataCopy->owner_pointer_check_expired_fn = fatDataOrig->owner_pointer_check_expired_fn;
		fatDataCopy->owner_pointer_try_lock_fn = fatDataOrig->owner_pointer_try_lock_fn;
		fatDataCopy->owner_pointer_unlock_fn = fatDataOrig->owner_pointer_unlock_fn;
		fatDataCopy->controller_data_ptr = fatDataOrig->controller_data_ptr;
		fatDataCopy->parent_fat_data_p = fatDataOrig->parent_fat_data_p;
		__do_addref_fat_data(fatDataCopy->parent_fat_data_p);

		__do_release_fat_data(m_fat_data_p);
		m_fat_data_p = fatDataCopy;
	}
}
