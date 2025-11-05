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
	KS_ASYNC_INLINE_API ks_async_context() noexcept 
		: m_fat_data_p(nullptr), m_priority(0) {}

	KS_ASYNC_INLINE_API ks_async_context(const ks_async_context& r) noexcept {
		m_fat_data_p = r.m_fat_data_p;
		__do_addref_fat_data(m_fat_data_p);
		m_priority = r.m_priority;
	}
	KS_ASYNC_INLINE_API ks_async_context(ks_async_context&& r) noexcept {
		m_fat_data_p = r.m_fat_data_p;
		m_priority = r.m_priority;
		r.m_fat_data_p = nullptr;
		r.m_priority = 0;
	}

	KS_ASYNC_INLINE_API _NOINLINE ks_async_context& operator=(const ks_async_context& r) noexcept {
		if (this != &r) {
			if (m_fat_data_p != r.m_fat_data_p) {
				__do_release_fat_data(m_fat_data_p);
				m_fat_data_p = r.m_fat_data_p;
				__do_addref_fat_data(m_fat_data_p);
			}
			m_priority = r.m_priority;
		}
		return *this;
	}
	KS_ASYNC_INLINE_API _NOINLINE ks_async_context& operator=(ks_async_context&& r) noexcept {
		if (this != &r) {
			__do_release_fat_data(m_fat_data_p);
			m_fat_data_p = r.m_fat_data_p;
			m_priority = r.m_priority;
			r.m_fat_data_p = nullptr;
			r.m_priority = 0;
		}
		return *this;
	}

	KS_ASYNC_INLINE_API ~ks_async_context() noexcept {
		__do_release_fat_data(m_fat_data_p);
		//m_fat_data_p = nullptr;
	}

public:
	template <class SMART_PTR, class _ = std::enable_if_t<std::is_null_pointer_v<SMART_PTR> || std::is_shared_pointer_v<SMART_PTR> || std::is_weak_pointer_v<SMART_PTR>>>
	KS_ASYNC_INLINE_API ks_async_context& bind_owner(SMART_PTR&& owner_ptr) {
		static_assert(std::is_null_pointer_v<SMART_PTR> || std::is_shared_pointer_v<SMART_PTR> || std::is_weak_pointer_v<SMART_PTR>, "the type of owner-ptr must be smart-ptr");
		do_bind_owner(std::move(owner_ptr), std::bool_constant<std::is_weak_pointer_v<SMART_PTR> && !std::is_null_pointer_v<SMART_PTR>>{});
		return *this;
	}

	KS_ASYNC_API ks_async_context& bind_controller(ks_async_controller* controller);

	KS_ASYNC_API ks_async_context& __bind_from_source_location(const ks_source_location& from_source_location);

	KS_ASYNC_API ks_async_context& set_parent(const ks_async_context& parent, bool inherit_attrs);

	KS_ASYNC_INLINE_API ks_async_context& set_priority(int priority) {
		m_priority = priority;
		return *this;
	}

private:
	template <class SMART_PTR>
	_NOINLINE void do_bind_owner(SMART_PTR&& owner_ptr, std::false_type owner_ptr_is_weak) {
		do_prepare_fat_data_cow();

		_FAT_DATA* fatData = m_fat_data_p;
		fatData->owner_ptr_any = ks_any::of<SMART_PTR>(std::forward<SMART_PTR>(owner_ptr));
		fatData->owner_ptr_is_weak = false;
	}
	template <class SMART_PTR>
	_NOINLINE void do_bind_owner(SMART_PTR&& owner_ptr, std::true_type owner_ptr_is_weak) {
		auto owner_pointer_check_expired_fn = [owner_ptr]() {
			return std::weak_pointer_traits<SMART_PTR>::check_weak_pointer_expired(owner_ptr);
		};
		auto owner_pointer_try_lock_fn = [owner_ptr]() {
			using typed_locker_t = typename std::weak_pointer_traits<SMART_PTR>::locker_type;
			typed_locker_t typed_locker = std::weak_pointer_traits<SMART_PTR>::try_lock_weak_pointer(owner_ptr);
			if (typed_locker)
				return ks_any::of<typed_locker_t>(std::move(typed_locker));
			std::weak_pointer_traits<SMART_PTR>::unlock_weak_pointer(owner_ptr, typed_locker);
			return ks_any();
		};
		auto owner_pointer_unlock_fn = [owner_ptr](ks_any& locker) {
			if (locker.has_value()) {
				using typed_locker_t = typename std::weak_pointer_traits<SMART_PTR>::locker_type;
				typed_locker_t typed_locker = locker.get<typed_locker_t>();
				std::weak_pointer_traits<SMART_PTR>::unlock_weak_pointer(owner_ptr, typed_locker);
				locker.reset();
			}
		};

		do_prepare_fat_data_cow();

		_FAT_DATA* fatData = m_fat_data_p;
		fatData->owner_ptr_any = ks_any::of<SMART_PTR>(std::forward<SMART_PTR>(owner_ptr));
		fatData->owner_ptr_is_weak = true;

		fatData->owner_pointer_check_expired_fn = std::move(owner_pointer_check_expired_fn);
		fatData->owner_pointer_try_lock_fn = std::move(owner_pointer_try_lock_fn);
		fatData->owner_pointer_unlock_fn = std::move(owner_pointer_unlock_fn);
	}

public:
	KS_ASYNC_INLINE_API ks_source_location __get_from_source_location() const noexcept {
#if __KS_ASYNC_CONTEXT_FROM_SOURCE_LOCATION_ENABLED
		return m_fat_data_p != nullptr ? m_fat_data_p->from_source_location : ks_source_location(nullptr);
#else
		return ks_source_location(nullptr);
#endif
	}

	KS_ASYNC_INLINE_API int __get_priority() const noexcept {
		return m_priority;
	}

public: //called by ks_raw_future internally
	KS_ASYNC_API bool __check_owner_expired() const noexcept;
	KS_ASYNC_INLINE_API ks_any __lock_owner_ptr() const noexcept { return __do_lock_owner_ptr_recursively(m_fat_data_p); }
	KS_ASYNC_INLINE_API void __unlock_owner_ptr(ks_any& locker) const noexcept { return __do_unlock_owner_ptr_recursively(m_fat_data_p, locker); }

	KS_ASYNC_API bool __check_controller_cancelled() const noexcept;

	//KS_ASYNC_API void __increment_pending_count() const noexcept;
	//KS_ASYNC_API void __decrement_pending_count() const noexcept;

private:
	struct _FAT_DATA;

	static bool __dodo_check_need_lock_parent_ptr(_FAT_DATA* fat_data_p) noexcept;
	static ks_any __do_lock_owner_ptr_recursively(_FAT_DATA* fat_data_p) noexcept;
	static void __do_unlock_owner_ptr_recursively(_FAT_DATA* fat_data_p, ks_any& locker) noexcept;

public:
	KS_ASYNC_INLINE_API _NOINLINE void swap(ks_async_context& r) noexcept {
		//if (this != &r) {
			std::swap(m_fat_data_p, r.m_fat_data_p);
			std::swap(m_priority, r.m_priority);
		//}
	}

private:
	struct _FAT_DATA {
		//关于owner
		ks_any owner_ptr_any;
		bool   owner_ptr_is_weak = false;
		std::function<bool()>        owner_pointer_check_expired_fn; //only when weak
		std::function<ks_any()>      owner_pointer_try_lock_fn;      //only when weak
		std::function<void(ks_any&)> owner_pointer_unlock_fn;        //only when weak

		//关于controller
		std::shared_ptr<ks_async_controller::_CONTROLLER_DATA> controller_data_ptr;

		//关于parent
		_FAT_DATA* parent_fat_data_p = nullptr;

#if __KS_ASYNC_CONTEXT_FROM_SOURCE_LOCATION_ENABLED
		//关于from_source_location
		ks_source_location from_source_location = ks_source_location(nullptr);
#endif

		//引用计数
		ks_refcount<int> ref_count = { 1 };
	};


	__KS_ASYNC_PRIVATE_API void do_prepare_fat_data_cow();

	inline void __do_addref_fat_data(_FAT_DATA* fata_data_p) noexcept {
		if (fata_data_p != nullptr) {
			++fata_data_p->ref_count;
		}
	}
	_NOINLINE void __do_release_fat_data(_FAT_DATA* fata_data_p) noexcept {
		if (fata_data_p != nullptr) {
			if (--fata_data_p->ref_count == 0) {
				__do_release_fat_data(fata_data_p->parent_fat_data_p);
				//fata_data_p->parent_fat_data_p = nullptr;
				delete fata_data_p;
			}
		}
	}

private:
	_FAT_DATA* m_fat_data_p; //_FAT_DATA结构体有点大，采用COW技术优化
	int m_priority;
};


namespace std {
	inline void swap(ks_async_context& l, ks_async_context& r) noexcept {
		l.swap(r);
	}
}


#define make_async_context(...)  (ks_async_context().__bind_from_source_location(current_source_location(__VA_ARGS__)))
