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
#include "ks_async_controller.h"
#include <memory>
#include <vector>


class ks_async_context final {
public:
	ks_async_context() {
		this->do_init();
	}

	template <class SMART_PTR, class _ = std::enable_if_t<std::is_null_pointer_v<SMART_PTR> || std::is_shared_pointer_v<SMART_PTR> || std::is_weak_pointer_v<SMART_PTR>>>
	explicit ks_async_context(const SMART_PTR& owner_ptr, const ks_async_controller* controller) {
		static_assert(std::is_null_pointer_v<SMART_PTR> || std::is_shared_pointer_v<SMART_PTR> || std::is_weak_pointer_v<SMART_PTR>, "the type of owner-ptr must be smart-ptr");
		this->do_init(owner_ptr, controller, std::bool_constant<std::is_weak_pointer_v<SMART_PTR> && !std::is_null_pointer_v<SMART_PTR>>{});
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

private:
	void do_init() {
	}

	template <class SMART_PTR>
	void do_init(const SMART_PTR& owner_ptr, const ks_async_controller* controller, std::false_type owner_ptr_is_weak) {
		if (owner_ptr != nullptr && controller != nullptr) {
			m_fat_data_p = new _FAT_DATA();

			if (owner_ptr != nullptr) {
				m_fat_data_p->owner_ptr = ks_any::of(owner_ptr);
				m_fat_data_p->owner_ptr_is_weak = false;
			}

			if (controller != nullptr) {
				m_fat_data_p->controller_data_ptr = controller->m_controller_data_ptr;
			}
		}
	}

	template <class SMART_PTR>
	void do_init(const SMART_PTR& owner_ptr, const ks_async_controller* controller, std::true_type owner_ptr_is_weak) {
		if (true && controller != nullptr) {
			m_fat_data_p = new _FAT_DATA();

			if (true) { //owner为weak指针时，即使此时即已为expired也要正常初始化，以便未来在异步过程执行时可进行正常检查（理论上彼时仍会为expired）
				m_fat_data_p->owner_ptr = ks_any::of(owner_ptr);
				m_fat_data_p->owner_ptr_is_weak = true;

				m_fat_data_p->owner_pointer_check_expired_fn = [owner_ptr]() {
					return std::weak_pointer_traits<SMART_PTR>::check_weak_pointer_expired(owner_ptr);
				};
				m_fat_data_p->owner_pointer_try_lock_fn = [owner_ptr]() {
					auto typed_locker = std::weak_pointer_traits<SMART_PTR>::try_lock_weak_pointer(owner_ptr);
					if (typed_locker)
						return ks_any::of(std::move(typed_locker));
					std::weak_pointer_traits<SMART_PTR>::unlock_weak_pointer(owner_ptr, typed_locker);
					return ks_any();
				};
				m_fat_data_p->owner_pointer_unlock_fn = [owner_ptr](ks_any& locker) {
					if (locker.has_value()) {
						using typed_locker_t = std::invoke_result_t<decltype(std::weak_pointer_traits<SMART_PTR>::try_lock_weak_pointer), const SMART_PTR&>;
						typed_locker_t& typed_locker = const_cast<typed_locker_t&>(locker.get<typed_locker_t>());
						std::weak_pointer_traits<SMART_PTR>::unlock_weak_pointer(owner_ptr, typed_locker);
						locker.reset();
					}
				};
			}

			if (controller != nullptr) {
				m_fat_data_p->controller_data_ptr = controller->m_controller_data_ptr;
			}
		}
	}

public:
	ks_async_context& set_priority(int priority) {
		m_priority = priority;
		return *this;
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
	void do_prepare_fat_data_cow() {
		if (m_fat_data_p == nullptr) {
			m_fat_data_p = new _FAT_DATA();
		}
		else if (m_fat_data_p->ref_count >= 2) {
			_FAT_DATA* fatDataCopy = new _FAT_DATA();
			fatDataCopy->owner_ptr = m_fat_data_p->owner_ptr;
			fatDataCopy->owner_ptr_is_weak = m_fat_data_p->owner_ptr_is_weak;
			fatDataCopy->owner_pointer_check_expired_fn = m_fat_data_p->owner_pointer_check_expired_fn;
			fatDataCopy->owner_pointer_try_lock_fn = m_fat_data_p->owner_pointer_try_lock_fn;
			fatDataCopy->owner_pointer_unlock_fn = m_fat_data_p->owner_pointer_unlock_fn;
			fatDataCopy->controller_data_ptr = m_fat_data_p->controller_data_ptr;
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

		//引用计数
		//std::atomic<int> ref_count = { 1 };  //被移动位置，使内存更紧凑

		//为了使内存布局更紧凑，将部分成员变量集中安置
		std::atomic<int> ref_count = { 1 };
		bool owner_ptr_is_weak = false;
	};

	_FAT_DATA* m_fat_data_p = nullptr; //_FAT_DATA结构体有点大，采用COW技术优化

	int m_priority = 0;
};
