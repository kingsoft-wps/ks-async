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
#include "ktl/ks_any.h"
#include "ktl/ks_concurrency.h"
#include "ktl/ks_source_location.h"
#include "ks_async_controller.h"
#include <memory>
#include <vector>


class ks_async_context final {
public:
	//注意：这个默认构造即将被废弃！
	ks_async_context() {
	}

	ks_async_context(const ks_async_context& r) {
		m_fat_data_p = r.m_fat_data_p;
		m_priority = r.m_priority;
		if (m_fat_data_p != nullptr)
			++m_fat_data_p->ref_count;
	}

	ks_async_context(ks_async_context&& r) noexcept {
		m_fat_data_p = r.m_fat_data_p;
		m_priority = r.m_priority;
		r.m_fat_data_p = nullptr;
	}

	ks_async_context& operator=(const ks_async_context& r) {
		if (this != &r && m_fat_data_p != r.m_fat_data_p) {
			do_release_fat_data();
			m_fat_data_p = r.m_fat_data_p;
			m_priority = r.m_priority;
			if (m_fat_data_p != nullptr)
				++m_fat_data_p->ref_count;
		}
		return *this;
	}

	ks_async_context& operator=(ks_async_context&& r) noexcept {
		do_release_fat_data();
		m_fat_data_p = r.m_fat_data_p;
		m_priority = r.m_priority;
		r.m_fat_data_p = nullptr;
		return *this;
	}

	~ks_async_context() {
		do_release_fat_data();
	}

	static ks_async_context __empty_inst() {
		return ks_async_context(ks_source_location::__empty_inst());
	}

private:
	explicit ks_async_context(const ks_source_location& from_source_location) {
		if (!from_source_location.is_empty()) {
			do_prepare_fat_data_cow();
#if __KS_ASYNC_CONTEXT_FROM_SOURCE_LOCATION_ENABLED
			m_fat_data_p->from_source_location = from_source_location;
#endif
		}
	}

public:
	template <class SMART_PTR, class _ = std::enable_if_t<std::is_null_pointer_v<SMART_PTR> || std::is_shared_pointer_v<SMART_PTR> || std::is_weak_pointer_v<SMART_PTR>>>
	ks_async_context& bind_owner(SMART_PTR&& owner_ptr) {
		static_assert(std::is_null_pointer_v<SMART_PTR> || std::is_shared_pointer_v<SMART_PTR> || std::is_weak_pointer_v<SMART_PTR>, "the type of owner-ptr must be smart-ptr");
		do_bind_owner(std::forward<SMART_PTR>(owner_ptr), std::bool_constant<std::is_weak_pointer_v<SMART_PTR> && !std::is_null_pointer_v<SMART_PTR>>{});
		return *this;
	}

	ks_async_context& bind_controller(const ks_async_controller* controller) {
		if (controller != nullptr || do_check_fat_data_avail(true, false)) {
			do_prepare_fat_data_cow();

			if (controller != nullptr)
				m_fat_data_p->controller_data_ptr = controller->m_controller_data_ptr;
			else
				m_fat_data_p->controller_data_ptr.reset();
		}
		else {
			do_release_fat_data();
		}
		return *this;
	}

	ks_async_context& set_priority(int priority) {
		m_priority = priority;
		return *this;
	}

private:
	template <class SMART_PTR>
	void do_bind_owner(SMART_PTR&& owner_ptr, std::true_type owner_ptr_is_weak) {
		do_prepare_fat_data_cow();
		_FAT_DATA* fatData = m_fat_data_p;

		fatData->owner_ptr = ks_any::of(owner_ptr);
		fatData->owner_ptr_is_weak = true;

		fatData->owner_pointer_check_expired_fn = [owner_ptr]() {
			return std::weak_pointer_traits<SMART_PTR>::check_weak_pointer_expired(owner_ptr);
		};

		fatData->owner_pointer_try_lock_fn = [owner_ptr]() {
			auto typed_locker = std::weak_pointer_traits<SMART_PTR>::try_lock_weak_pointer(owner_ptr);
			if (typed_locker)
				return ks_any::of(std::move(typed_locker));
			std::weak_pointer_traits<SMART_PTR>::unlock_weak_pointer(owner_ptr, typed_locker);
			return ks_any();
		};

		fatData->owner_pointer_unlock_fn = [owner_ptr](ks_any& locker) {
			if (locker.has_value()) {
				using typed_locker_t = std::invoke_result_t<decltype(std::weak_pointer_traits<SMART_PTR>::try_lock_weak_pointer), const SMART_PTR&>;
				typed_locker_t& typed_locker = const_cast<typed_locker_t&>(locker.get<typed_locker_t>());
				std::weak_pointer_traits<SMART_PTR>::unlock_weak_pointer(owner_ptr, typed_locker);
				locker.reset();
			}
		};
	}

	template <class SMART_PTR>
	void do_bind_owner(SMART_PTR&& owner_ptr, std::false_type owner_ptr_is_weak) {
		if (owner_ptr != nullptr || do_check_fat_data_avail(false, true)) {
			do_prepare_fat_data_cow();
			_FAT_DATA* fatData = m_fat_data_p;

			if (owner_ptr != nullptr)
				fatData->owner_ptr = ks_any::of(std::forward<SMART_PTR>(owner_ptr));
			else
				fatData->owner_ptr.reset();
			fatData->owner_ptr_is_weak = false;
			fatData->owner_pointer_check_expired_fn = nullptr;
			fatData->owner_pointer_try_lock_fn = nullptr;
			fatData->owner_pointer_unlock_fn = nullptr;
		}
		else {
			do_release_fat_data();
		}
	}

public: //called by ks_raw_future internally
	bool __check_owner_expired() const {
		if (m_fat_data_p != nullptr && m_fat_data_p->owner_ptr_is_weak)
			return m_fat_data_p->owner_pointer_check_expired_fn();
		else
			return false;
	}

	bool __check_cancel_all_ctrl() const {
		if (m_fat_data_p != nullptr && m_fat_data_p->controller_data_ptr != nullptr)
			return m_fat_data_p->controller_data_ptr->cancel_all_ctrl_v;
		else
			return false;
	}

	ks_source_location __get_from_source_location() const {
		if (m_fat_data_p != nullptr && true)
#if __KS_ASYNC_CONTEXT_FROM_SOURCE_LOCATION_ENABLED
			return m_fat_data_p->from_source_location;
#else
			return ks_source_location::__empty_inst();
#endif
		else
			return ks_source_location::__empty_inst();
	}

	int  __get_priority() const {
		return m_priority;
	}

	bool __is_controller_present() const {
		return (m_fat_data_p != nullptr && m_fat_data_p->controller_data_ptr != nullptr);
	}

public: //called by ks_raw_future internally
	ks_any __lock_owner_ptr() const {
		if (m_fat_data_p != nullptr && m_fat_data_p->owner_ptr_is_weak)
			return m_fat_data_p->owner_pointer_try_lock_fn();
		else
			return ks_any::of(true);
	}
	void __unlock_owner_ptr(ks_any& locker) const {
		if (m_fat_data_p != nullptr && m_fat_data_p->owner_ptr_is_weak)
			m_fat_data_p->owner_pointer_unlock_fn(locker);
		else
			locker.reset();
	}

	void __increment_pending_count() const {
		if (m_fat_data_p != nullptr && m_fat_data_p->controller_data_ptr != nullptr)
			m_fat_data_p->controller_data_ptr->pending_latch.add(1);
	}
	void __decrement_pending_count() const {
		if (m_fat_data_p != nullptr && m_fat_data_p->controller_data_ptr != nullptr)
			m_fat_data_p->controller_data_ptr->pending_latch.count_down(1);
	}

private:
	bool do_check_fat_data_avail(bool check_owner, bool check_controller) const {
		if (m_fat_data_p != nullptr) {
			if (check_owner && m_fat_data_p->owner_ptr.has_value())
				return true;
			if (check_controller && m_fat_data_p->controller_data_ptr != nullptr)
				return true;
#if __KS_ASYNC_CONTEXT_FROM_SOURCE_LOCATION_ENABLED
			if (true && !m_fat_data_p->from_source_location.is_empty())
				return true;
#endif
		}
		return false;
	}

	void do_prepare_fat_data_cow() {
		if (m_fat_data_p == nullptr) {
			m_fat_data_p = new _FAT_DATA();
		}
		else if (m_fat_data_p->ref_count >= 2) {
			_FAT_DATA* fatDataOrig = m_fat_data_p;
			_FAT_DATA* fatDataCopy = new _FAT_DATA();
			fatDataCopy->owner_ptr = fatDataOrig->owner_ptr;
			fatDataCopy->owner_ptr_is_weak = fatDataOrig->owner_ptr_is_weak;
			fatDataCopy->owner_pointer_check_expired_fn = fatDataOrig->owner_pointer_check_expired_fn;
			fatDataCopy->owner_pointer_try_lock_fn = fatDataOrig->owner_pointer_try_lock_fn;
			fatDataCopy->owner_pointer_unlock_fn = fatDataOrig->owner_pointer_unlock_fn;
			fatDataCopy->controller_data_ptr = fatDataOrig->controller_data_ptr;
			do_release_fat_data();
			m_fat_data_p = fatDataCopy;
		}
	}

	void do_release_fat_data() {
		if (m_fat_data_p != nullptr) {
			if (--m_fat_data_p->ref_count == 0)
				delete m_fat_data_p;
			m_fat_data_p = nullptr;
		}
	}

private:
	struct _FAT_DATA {
		//关于owner
		ks_any owner_ptr;
		//bool owner_ptr_is_weak = false;  //被移动位置，使内存更紧凑
		std::function<bool()>        owner_pointer_check_expired_fn; //only when weak
		std::function<ks_any()>      owner_pointer_try_lock_fn;      //only when weak
		std::function<void(ks_any&)> owner_pointer_unlock_fn;        //only when weak

		//关于controller
		std::shared_ptr<ks_async_controller::_CONTROLLER_DATA> controller_data_ptr;

		//关于from_source_location
#if __KS_ASYNC_CONTEXT_FROM_SOURCE_LOCATION_ENABLED
		ks_source_location from_source_location = ks_source_location::__empty_inst();
#endif

		//引用计数
		//std::atomic<int> ref_count = { 1 };  //被移动位置，使内存更紧凑

		//为了使内存布局更紧凑，将部分成员变量集中安置
		std::atomic<int> ref_count = { 1 };
		bool owner_ptr_is_weak = false;
	};

	_FAT_DATA* m_fat_data_p = nullptr; //_FAT_DATA结构体有点大，采用COW技术优化

	int m_priority = 0;

	friend ks_async_context __make_async_context_from(const ks_source_location& from_source_location);
};


inline 
ks_async_context __make_async_context_from(const ks_source_location& from_source_location) {
	return ks_async_context(from_source_location);
}

#define make_async_context()  \
	(__make_async_context_from(current_source_location()))
