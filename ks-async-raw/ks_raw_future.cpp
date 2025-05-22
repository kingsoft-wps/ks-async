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

#include "ks_raw_future.h"
#include "ks_raw_promise.h"
#include "ks_raw_internal_helper.hpp"
#include "../ktl/ks_concurrency.h"
#include "../ktl/ks_defer.h"
#include <vector>
#include <algorithm>

void __forcelink_to_ks_raw_future_cpp() {}

#if (!__KS_APARTMENT_ATFORK_ENABLED)
	using __native_pid_t = int;
	static inline __native_pid_t __native_get_current_pid() { return -1; }  //pseudo
	static constexpr __native_pid_t __native_pid_none = 0;
#elif defined(_WIN32)
	#include <Windows.h>
	#include <processthreadsapi.h>
	using __native_pid_t = DWORD;
	static inline __native_pid_t __native_get_current_pid() { return ::GetCurrentProcessId(); }
	static constexpr __native_pid_t __native_pid_none = 0;
#else
	#include <unistd.h>
	using __native_pid_t = pid_t;
	static inline __native_pid_t __native_get_current_pid() { return getpid(); }
	static constexpr __native_pid_t __native_pid_none = 0;
#endif


__KS_ASYNC_RAW_BEGIN

static thread_local ks_raw_future* tls_current_thread_running_future = nullptr;

#if __KS_ASYNC_RAW_FUTURE_GLOBAL_MUTEX_ENABLED || __KS_ASYNC_DX_RAW_FUTURE_GLOBAL_MUTEX_ENABLED
static ks_mutex g_raw_future_global_mutex;  //global mutex
#endif

class ks_raw_future_lock final { //like std::unique_lock, with supporting pseudo
public:
	ks_raw_future_lock(ks_mutex* mutex, bool pseudo) 
		: m_mutex(mutex), m_pseudo(pseudo), m_owns_lock(false) {
		ASSERT(m_mutex != nullptr || m_pseudo);
		this->lock(); 
	}

	ks_raw_future_lock(ks_mutex* mutex, bool pseudo, std::defer_lock_t) 
		: m_mutex(mutex), m_pseudo(pseudo), m_owns_lock(false) {
		ASSERT(m_mutex != nullptr || m_pseudo);
	}

	~ks_raw_future_lock() {
		if (m_owns_lock)
			this->unlock();
	}

	_DISABLE_COPY_CONSTRUCTOR(ks_raw_future_lock);

public:
	void lock() {
		ASSERT(!m_owns_lock);
		if (!m_pseudo)
			m_mutex->lock();
		m_owns_lock = true;
	}
	void unlock() {
		ASSERT(m_owns_lock);
		if (!m_pseudo) 
			m_mutex->unlock();
		m_owns_lock = false;
	}

public:
	ks_mutex* mutex() const { return m_mutex; }
	bool is_pseudo() const { return m_pseudo; }
	bool owns_lock() const { return m_owns_lock; }

private:
	ks_mutex* m_mutex;
	bool m_pseudo;
	bool m_owns_lock;
};


enum class ks_raw_future_mode {
	DX, PROMISE,  //promise
	TASK, TASK_DELAYED,  //task
	THEN, TRAP, TRANSFORM,  //pipe
	ON_SUCCESS, ON_FAILURE, ON_COMPLETION,  //on-pipe
	FORWARD, //forward-pipe
	FLATTEN_THEN, FLATTEN_TRAP, FLATTEN_TRANSFORM,  //flatten
	ALL, ALL_COMPLETED, ANY, //aggr
};

#define __REAL_IMP

_ABSTRACT class ks_raw_future_baseimp : public ks_raw_future, public std::enable_shared_from_this<ks_raw_future> {
protected:
	explicit ks_raw_future_baseimp() = default;
	_DISABLE_COPY_CONSTRUCTOR(ks_raw_future_baseimp);

	struct __INTERMEDIATE_DATA;
	void do_init_base_locked(ks_apartment* spec_apartment, const ks_async_context& living_context, const std::shared_ptr<__INTERMEDIATE_DATA>& intermediate_data_ptr, ks_raw_future_lock& lock) {
		ASSERT(intermediate_data_ptr != nullptr);
		ASSERT(intermediate_data_ptr == __get_intermediate_data_ptr(lock));

		if (spec_apartment == nullptr && __is_head_future())
			spec_apartment = ks_apartment::default_mta();

		intermediate_data_ptr->m_spec_apartment = spec_apartment;
		intermediate_data_ptr->m_living_context = living_context;
		intermediate_data_ptr->m_create_time = std::chrono::steady_clock::now();
	}

	void do_init_with_result_locked(ks_apartment* spec_apartment, const ks_raw_result& completed_result, ks_raw_future_lock& lock, bool must_keep_locked) {
		ASSERT(lock.owns_lock() && !must_keep_locked);

		if (spec_apartment == nullptr && __is_head_future())
			spec_apartment = ks_apartment::default_mta();

		ASSERT(__get_intermediate_data_ptr(lock) == nullptr);
		this->do_complete_locked(completed_result, spec_apartment, true, lock, must_keep_locked);
	}

public:
	virtual ks_raw_future_ptr then(std::function<ks_raw_result(const ks_raw_value&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override final;
	virtual ks_raw_future_ptr trap(std::function<ks_raw_result(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override final;
	virtual ks_raw_future_ptr transform(std::function<ks_raw_result(const ks_raw_result&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override final;

	virtual ks_raw_future_ptr flat_then(std::function<ks_raw_future_ptr(const ks_raw_value&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override final;
	virtual ks_raw_future_ptr flat_trap(std::function<ks_raw_future_ptr(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override final;
	virtual ks_raw_future_ptr flat_transform(std::function<ks_raw_future_ptr(const ks_raw_result&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override final;

	virtual ks_raw_future_ptr on_success(std::function<void(const ks_raw_value&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override final;
	virtual ks_raw_future_ptr on_failure(std::function<void(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override final;
	virtual ks_raw_future_ptr on_completion(std::function<void(const ks_raw_result&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override final;

	virtual ks_raw_future_ptr noop(ks_apartment* apartment) override final;

public:
	virtual bool is_completed() override final {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		return m_completed_result.is_completed();
	}

	virtual ks_raw_result peek_result() override final {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		return m_completed_result;
	}

protected:
	virtual bool is_cancelable_self() override = 0;

	virtual bool do_check_cancel() override final {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		return this->do_check_cancel_locked(lock);
	}

	virtual ks_error do_acquire_cancel_error(const ks_error& def_error) override final {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		return this->do_acquire_cancel_error_locked(def_error, lock);
	}

	__REAL_IMP bool do_check_cancel_locked(ks_raw_future_lock& lock) {
		if (!this->is_cancelable_self())
			return false;

		if (!m_completed_result.is_completed()) {
			auto intermediate_data_ptr = __get_intermediate_data_ptr(lock);
			ASSERT(intermediate_data_ptr != nullptr);

			if (intermediate_data_ptr->m_cancel_error.get_code() != 0)
				return true;

			if (intermediate_data_ptr->m_living_context.__is_controller_present()) {
				if (intermediate_data_ptr->m_living_context.__check_cancel_ctrl() || intermediate_data_ptr->m_living_context.__check_owner_expired())
					return true;
			}

			if (intermediate_data_ptr->m_timeout_time != std::chrono::steady_clock::time_point{} && intermediate_data_ptr->m_timeout_time <= std::chrono::steady_clock::now()) {
				return true;
			}
		}
		else if (m_completed_result.is_error()) {
			return true;
		}

		return false;
	}

	__REAL_IMP ks_error do_acquire_cancel_error_locked(const ks_error& def_error, ks_raw_future_lock& lock) {
		if (!this->is_cancelable_self())
			return ks_error();

		if (!m_completed_result.is_completed()) {
			auto intermediate_data_ptr = __get_intermediate_data_ptr(lock);
			ASSERT(intermediate_data_ptr != nullptr);

			if (intermediate_data_ptr->m_cancel_error.get_code() != 0)
				return intermediate_data_ptr->m_cancel_error;

			if (intermediate_data_ptr->m_living_context.__is_controller_present()) {
				if (intermediate_data_ptr->m_living_context.__check_cancel_ctrl() || intermediate_data_ptr->m_living_context.__check_owner_expired())
					return ks_error::cancelled_error();
			}

			if (intermediate_data_ptr->m_timeout_time != std::chrono::steady_clock::time_point{} && (intermediate_data_ptr->m_timeout_time <= std::chrono::steady_clock::now())) {
				return ks_error::timeout_error();
			}
		}
		else if (m_completed_result.is_error()) {
			return m_completed_result.to_error();
		}

		return def_error;
	}

	virtual bool do_wait() override final {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		if (m_completed_result.is_completed())
			return true;

		auto intermediate_data_ptr = __get_intermediate_data_ptr(lock);
		ASSERT(intermediate_data_ptr != nullptr);

		ks_apartment* cur_apartment = ks_apartment::current_thread_apartment();
		ASSERT(cur_apartment == nullptr || (cur_apartment->features() & ks_apartment::nested_pump_aware_future) != 0);
		if (cur_apartment != nullptr && (cur_apartment->features() & ks_apartment::nested_pump_suppressed_future) == 0) {
			//若嵌套loop可能会（但常规用法并不会）遭遇相同项，但也不必重复记录，因为一次awaken就触发全部
			//另外，wait后也不必将cur_apartment从m_waiting_for_me_apartment_1st和m_waiting_for_me_apartment_more中移除，因为do_complete时自会在awaken后全部清除
			if (intermediate_data_ptr->m_waiting_for_me_apartment_1st != cur_apartment &&
				std::find(intermediate_data_ptr->m_waiting_for_me_apartment_more.cbegin(), intermediate_data_ptr->m_waiting_for_me_apartment_more.cend(), cur_apartment) == intermediate_data_ptr->m_waiting_for_me_apartment_more.cend()) {
				if (intermediate_data_ptr->m_waiting_for_me_apartment_1st == nullptr)
					intermediate_data_ptr->m_waiting_for_me_apartment_1st = cur_apartment;
				else
					intermediate_data_ptr->m_waiting_for_me_apartment_more.push_back(cur_apartment);
			}

			lock.unlock();
			bool was_satisfied = cur_apartment->__run_nested_pump_loop_for_extern_waiting(
				this,
				[this, this_shared = this->shared_from_this()]() -> bool { return m_completed_result.is_completed(); });
			ASSERT(was_satisfied ? m_completed_result.is_completed() : true);

			lock.lock();
			if (!m_completed_result.is_completed()) {
				//若nested_pump_loop已退出，但又非completed，理论上是在atforking了，那么我们只能立即结束future的wait，
				//又因wait结束，那么也只好将当前future标记为失败了，因为后续此future理所当然会被认为是completed状态了，
				//但实际上我们期望不要发生此情形，即在有future在wait时不期望进行进程fork，因此这里ASSERT(false)。
				ASSERT(false);
				this->do_complete_locked(ks_error::interrupted_error(), cur_apartment, false, lock, false);
				return false;
			}

			return true;
		}
		else {
			if (intermediate_data_ptr->m_completion_cv_waiting_rc == 0) {
				ASSERT(intermediate_data_ptr->m_completion_cv_belong_pid == __native_pid_none);
				intermediate_data_ptr->m_completion_cv_belong_pid = __native_get_current_pid();
			}
			else if (intermediate_data_ptr->m_completion_cv_belong_pid != __native_get_current_pid()) {
				ASSERT(false);
				intermediate_data_ptr->m_completion_cv_belong_pid = __native_get_current_pid();
				::new (&intermediate_data_ptr->m_completion_cv) ks_condition_variable(); //重建cv，在子进程中
			}

			++intermediate_data_ptr->m_completion_cv_waiting_rc;

			if (true) {
				ASSERT(lock.owns_lock() && !lock.is_pseudo());
				std::unique_lock<ks_mutex> adopt_lock(*lock.mutex(), std::adopt_lock);

				while (!m_completed_result.is_completed()) {
					intermediate_data_ptr->m_completion_cv.wait(adopt_lock);
				}

				ASSERT(adopt_lock.owns_lock());
				adopt_lock.release();
			}

			if (--intermediate_data_ptr->m_completion_cv_waiting_rc == 0) {
				if (intermediate_data_ptr->m_completion_cv_belong_pid != __native_get_current_pid()) {
					ASSERT(false);
					intermediate_data_ptr->m_completion_cv_belong_pid = __native_get_current_pid();
					::new (&intermediate_data_ptr->m_completion_cv) ks_condition_variable(); //重建cv，马上就要析构了
				}
			}

			return true;
		}
	}

protected:
	virtual void do_add_next(const ks_raw_future_ptr& next_future) override final {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		return this->do_add_next_locked(next_future, lock, false);
	}

	virtual void do_add_next_multi(const std::vector<ks_raw_future_ptr>& next_futures) override final {
		if (next_futures.empty()) {
			ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
			return this->do_add_next_multi_locked(next_futures, lock, false);
		}
	}

	virtual void do_complete(const ks_raw_result& result, ks_apartment* prefer_apartment, bool from_internal) override final {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		return this->do_complete_locked(result, prefer_apartment, from_internal, lock, false);
	}

	virtual void do_add_next_locked(const ks_raw_future_ptr& next_future, ks_raw_future_lock& lock, bool must_keep_locked) {
		ASSERT(lock.owns_lock() && !must_keep_locked);

		if (!m_completed_result.is_completed()) {
			auto intermediate_data_ptr = __get_intermediate_data_ptr(lock);
			ASSERT(intermediate_data_ptr != nullptr);

			ASSERT(intermediate_data_ptr->m_next_future_1st != next_future
				&& std::find(intermediate_data_ptr->m_next_future_more.cbegin(), intermediate_data_ptr->m_next_future_more.cend(), next_future) == intermediate_data_ptr->m_next_future_more.cend());
			if (intermediate_data_ptr->m_next_future_1st == nullptr)
				intermediate_data_ptr->m_next_future_1st = next_future;
			else
				intermediate_data_ptr->m_next_future_more.push_back(next_future);
		}
		else {
			ks_raw_result const my_completed_result = m_completed_result;
			ks_apartment* const prefer_completed_apartment = m_completed_apartment;
			uint64_t act_schedule_id = prefer_completed_apartment->schedule(
				[this, this_shared = this->shared_from_this(), next_future, my_completed_result, prefer_completed_apartment]() {
				next_future->on_feeded_by_prev(my_completed_result, this, prefer_completed_apartment);
			}, 0);

			if (act_schedule_id == 0) {
				lock.unlock();
				next_future->do_complete(ks_error::terminated_error(), prefer_completed_apartment, false);
				if (must_keep_locked)
					lock.lock();
			}
		}
	}

	__REAL_IMP void do_add_next_multi_locked(const std::vector<ks_raw_future_ptr>& next_futures, ks_raw_future_lock& lock, bool must_keep_locked) {
		ASSERT(lock.owns_lock() && !must_keep_locked);

		if (!next_futures.empty()) {
			if (!m_completed_result.is_completed()) {
				auto intermediate_data_ptr = __get_intermediate_data_ptr(lock);
				ASSERT(intermediate_data_ptr != nullptr);

				auto next_future_it = next_futures.cbegin();
				if (intermediate_data_ptr->m_next_future_1st == nullptr)
					intermediate_data_ptr->m_next_future_1st = *next_future_it++;
				if (next_future_it != next_futures.cend()) {
					intermediate_data_ptr->m_next_future_more.insert(
						intermediate_data_ptr->m_next_future_more.end(),
						next_future_it, next_futures.cend());
				}
			}
			else {
				ks_raw_result const my_completed_result = m_completed_result;
				ks_apartment* const prefer_completed_apartment = m_completed_apartment;
				uint64_t act_schedule_id = prefer_completed_apartment->schedule(
					[this, this_shared = this->shared_from_this(), next_futures, my_completed_result, prefer_completed_apartment]() {
					for (auto& next_future : next_futures)
						next_future->on_feeded_by_prev(my_completed_result, this, prefer_completed_apartment);
				}, 0);

				if (act_schedule_id == 0) {
					lock.unlock();
					for (auto& next_future : next_futures)
						next_future->do_complete(ks_error::terminated_error(), prefer_completed_apartment, false);
					if (must_keep_locked)
						lock.lock();
				}
			}
		}
	}

	__REAL_IMP void do_complete_locked(const ks_raw_result& completed_result, ks_apartment* hint_apartment, bool from_internal, ks_raw_future_lock& lock, bool must_keep_locked) {
		ASSERT(lock.owns_lock() && !must_keep_locked);
		ASSERT(completed_result.is_completed());

		if (m_completed_result.is_completed()) 
			return; //repeat complete?

		auto intermediate_data_ptr = __get_intermediate_data_ptr(lock);
		//here, intermediate_data_ptr maybe nullptr (when dx)!

		ks_raw_result const my_completed_result = completed_result.require_completed_or_error();
		ks_apartment* const my_completed_apartment = do_determine_completed_apartment(intermediate_data_ptr != nullptr ? intermediate_data_ptr->m_spec_apartment : nullptr, hint_apartment);
		m_completed_result = my_completed_result;
		m_completed_apartment = my_completed_apartment;
		if (__get_mode() == ks_raw_future_mode::DX) {
			//dx-future是在init时立即do_complete的，状态初始化为completed后就没什么其他要做的事儿了
			ASSERT(intermediate_data_ptr == nullptr);
			return;
		}

		//除了dx-future以外，其他类型future还需要在状态转为completed时feed其所有下游
		ASSERT(intermediate_data_ptr != nullptr);

		ks_raw_future_ptr t_next_future_1st = nullptr;
		std::vector<ks_raw_future_ptr> t_next_future_more{};
		ks_apartment* t_waiting_for_me_apartment_1st = nullptr;
		std::vector<ks_apartment*> t_waiting_for_me_apartment_more{};
		if (intermediate_data_ptr != nullptr) {
			if (intermediate_data_ptr->m_timeout_schedule_id != 0) {
				ASSERT(intermediate_data_ptr->m_timeout_apartment != nullptr);
				intermediate_data_ptr->m_timeout_apartment->try_unschedule(intermediate_data_ptr->m_timeout_schedule_id);
				intermediate_data_ptr->m_timeout_schedule_id = 0;
			}

			//cv notify
			if (intermediate_data_ptr->m_completion_cv_waiting_rc != 0) {
				if (intermediate_data_ptr->m_completion_cv_belong_pid == __native_get_current_pid())
					intermediate_data_ptr->m_completion_cv.notify_all();
				else
					ASSERT(false);  //子进程不要notify，会有几率卡死！（若子进程内执行wait，则会更新子进程内记录的belong_pid的，那么就会正常notify）
			}

			//take next-futures
			t_next_future_1st = std::move(intermediate_data_ptr->m_next_future_1st);
			t_next_future_more.swap(intermediate_data_ptr->m_next_future_more);
			intermediate_data_ptr->m_next_future_1st = nullptr;
			intermediate_data_ptr->m_next_future_more.clear();

			//take waiting-apartments
			t_waiting_for_me_apartment_1st = std::move(intermediate_data_ptr->m_waiting_for_me_apartment_1st);
			t_waiting_for_me_apartment_more.swap(intermediate_data_ptr->m_waiting_for_me_apartment_more);
			intermediate_data_ptr->m_waiting_for_me_apartment_1st = nullptr;
			intermediate_data_ptr->m_waiting_for_me_apartment_more.clear();

			//完毕，自此刻起，本future进入completed稳态，可清除intermediate-data了
			intermediate_data_ptr->m_living_context = {};
			__clear_intermediate_data_ptr(intermediate_data_ptr, lock);
			intermediate_data_ptr = nullptr;
		}

		//触发流水线后续事项
		const bool has_some_next_futures = t_next_future_1st != nullptr || !t_next_future_more.empty();
		const bool has_some_waiting_apartments = t_waiting_for_me_apartment_1st != nullptr || !t_waiting_for_me_apartment_more.empty();
		if (has_some_next_futures || has_some_waiting_apartments) {
			//注：按说from_internal流程无需解锁（除非外部不合理乱用0x10000优先级），但我们这里无差别处理，没啥损失且更安全
			lock.unlock();

			//feed next-futures
			if (has_some_next_futures) {
				if (from_internal) {
					if (t_next_future_1st != nullptr)
						t_next_future_1st->on_feeded_by_prev(my_completed_result, this, my_completed_apartment);
					for (auto& next_future : t_next_future_more)
						next_future->on_feeded_by_prev(my_completed_result, this, my_completed_apartment);
				}
				else {
					uint64_t act_schedule_id = my_completed_apartment->schedule(
						[this, this_shared = this->shared_from_this(),
						t_next_future_1st, t_next_future_more,  //因为失败时还需要处理，所以不可以右值引用传递
						my_completed_result, my_completed_apartment]() {
						if (t_next_future_1st != nullptr)
							t_next_future_1st->on_feeded_by_prev(my_completed_result, this, my_completed_apartment);
						for (auto& next_future : t_next_future_more)
							next_future->on_feeded_by_prev(my_completed_result, this, my_completed_apartment);
					}, 0);

					if (act_schedule_id == 0) {
						if (t_next_future_1st != nullptr)
							t_next_future_1st->on_feeded_by_prev(ks_error::terminated_error(), this, my_completed_apartment);
						for (auto& next_future : t_next_future_more)
							next_future->on_feeded_by_prev(ks_error::terminated_error(), this, my_completed_apartment);
					}
				}
			}

			//awaken waiting-apartments
			if (has_some_waiting_apartments) {
				if (t_waiting_for_me_apartment_1st != nullptr)
					t_waiting_for_me_apartment_1st->__awaken_nested_pump_loop_for_extern_waiting_once(this);
				for (ks_apartment* waiting_apartment : t_waiting_for_me_apartment_more)
					waiting_apartment->__awaken_nested_pump_loop_for_extern_waiting_once(this);
			}

			if (must_keep_locked)
				lock.lock();
		}
	}

	virtual void do_set_timeout(int64_t timeout, const ks_error& error, bool backtrack) override final {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		if (m_completed_result.is_completed())
			return;

		auto intermediate_data_ptr = __get_intermediate_data_ptr(lock);
		ASSERT(intermediate_data_ptr != nullptr);

		//cancel prev-timeout
		if (intermediate_data_ptr->m_timeout_schedule_id != 0) {
			ASSERT(intermediate_data_ptr->m_timeout_apartment != nullptr);
			intermediate_data_ptr->m_timeout_apartment->try_unschedule(intermediate_data_ptr->m_timeout_schedule_id);
			intermediate_data_ptr->m_timeout_schedule_id = 0;
		}

		//update timeout_time
		intermediate_data_ptr->m_timeout_time = timeout > 0 ? intermediate_data_ptr->m_create_time + std::chrono::milliseconds(timeout) : std::chrono::steady_clock::time_point{};
		if (timeout <= 0) 
			return; //infinity (no-timeout forever)

		//check timeout, at once
		const std::chrono::steady_clock::time_point now_time = std::chrono::steady_clock::now();
		const int64_t timeout_remain_ms = std::chrono::duration_cast<std::chrono::milliseconds>(intermediate_data_ptr->m_timeout_time - now_time).count();
		if (timeout_remain_ms <= 0) {
			if (!m_completed_result.is_completed()) {
				lock.unlock();
				this->do_try_cancel(error, backtrack);
			}
			return;
		}

		//schedule timeout
		intermediate_data_ptr->m_timeout_apartment = do_determine_timeout_apartment(intermediate_data_ptr->m_spec_apartment);
		intermediate_data_ptr->m_timeout_schedule_id = intermediate_data_ptr->m_timeout_apartment->schedule_delayed(
			[this, this_shared = this->shared_from_this(), intermediate_data_ptr, schedule_id = intermediate_data_ptr->m_timeout_schedule_id, error, backtrack]() -> void {
			ks_raw_future_lock lock2(__get_mutex(), __is_using_pseudo_mutex());
			if (m_completed_result.is_completed())
				return;

			ASSERT(intermediate_data_ptr != nullptr);
			if (schedule_id != intermediate_data_ptr->m_timeout_schedule_id)
				return;

			intermediate_data_ptr->m_timeout_schedule_id = 0; //reset

			lock2.unlock();
			this->do_try_cancel(error, backtrack); //will become timeout
		}, 0, timeout_remain_ms);

		if (intermediate_data_ptr->m_timeout_schedule_id == 0) {
			//we can just ignore scheduling timeout-fn failure, simply.
			ASSERT(false);
			return;
		}
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override = 0;

protected:
	static inline ks_apartment* do_determine_prefer_apartment(ks_apartment* spec_apartment) {
		if (spec_apartment != nullptr)
			return spec_apartment;
		else
			return ks_apartment::default_mta();
	}

	static inline ks_apartment* do_determine_prefer_apartment_2(ks_apartment* spec_apartment, ks_apartment* prev_advice_apartment) {
		if (spec_apartment != nullptr)
			return spec_apartment;
		else if (prev_advice_apartment != nullptr)
			return prev_advice_apartment;
		else
			return ks_apartment::current_thread_apartment_or_default_mta();
	}

	static inline ks_apartment* do_determine_completed_apartment(ks_apartment* spec_apartment, ks_apartment* hint_apartment) {
		if (spec_apartment != nullptr)
			return spec_apartment;
		else if (hint_apartment != nullptr)
			return hint_apartment;
		else
			return ks_apartment::current_thread_apartment_or_default_mta();
	}

	static inline ks_apartment* do_determine_timeout_apartment(ks_apartment* spec_apartment) {
		if (spec_apartment != nullptr)
			return spec_apartment;
		else
			return ks_apartment::default_mta();
	}

protected:
	ks_raw_result m_completed_result{};
	ks_apartment* m_completed_apartment = nullptr;

	struct __INTERMEDIATE_DATA {
		ks_apartment* m_spec_apartment = nullptr;                  //const-like
		ks_async_context m_living_context = {};                    //const-like
		std::chrono::steady_clock::time_point m_create_time = {};  //const-like

		std::chrono::steady_clock::time_point m_timeout_time = {};
		ks_apartment* m_timeout_apartment = nullptr;
		uint64_t m_timeout_schedule_id = 0;

		ks_raw_future_ptr m_next_future_1st = nullptr;
		std::vector<ks_raw_future_ptr> m_next_future_more{};

		ks_apartment* m_waiting_for_me_apartment_1st = nullptr;
		std::vector<ks_apartment*> m_waiting_for_me_apartment_more{};

		ks_error m_cancel_error{}; //volatile-like

		//fork子进程中操作cv有几率死锁，故子进程中不要使用它！
		//记录cv所属pid，保证进程内操作cv的一致性
		//必要时会重建cv，避免子进程卡死（只是尽量容错而已，并不绝对安全，尤其是逻辑上的死等）
		ks_condition_variable m_completion_cv{};
		__native_pid_t m_completion_cv_belong_pid = __native_pid_none;
		int m_completion_cv_waiting_rc = 0;
	};

	virtual ks_raw_future_mode __get_mode() = 0;
	virtual bool __is_head_future() = 0;

	virtual ks_mutex* __get_mutex() = 0;
	virtual bool __is_using_pseudo_mutex() = 0;

	virtual std::shared_ptr<__INTERMEDIATE_DATA> __get_intermediate_data_ptr(ks_raw_future_lock& lock) = 0;
	virtual void __clear_intermediate_data_ptr(const std::shared_ptr<__INTERMEDIATE_DATA>& intermediate_data_ptr, ks_raw_future_lock& lock) = 0; //completed后被清除

	friend class ks_raw_future;
};


class ks_raw_dx_future final : public ks_raw_future_baseimp {
public:
	//注：默认apartment原设计使用current_thread_apartment，现已改为使用default_mta
	explicit ks_raw_dx_future(ks_raw_future_mode dx_mode) {
		ASSERT(dx_mode == ks_raw_future_mode::DX);
	}
	_DISABLE_COPY_CONSTRUCTOR(ks_raw_dx_future);

	void init(ks_apartment* spec_apartment, const ks_raw_result& completed_result) {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		do_init_with_result_locked(spec_apartment, completed_result, lock, false);
	}

protected:
	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) override {
		//ks_raw_dx_future的此方法不应被调用，而是在init时就已立即do_complete
		ASSERT(false);
	}

	virtual bool is_cancelable_self() override {
		//dx-future无cancel行为
		return false; 
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override {
		//dx-future无cancel行为
		ASSERT(error.get_code() != 0);
		ASSERT(this->is_completed());
		_NOOP();
	}

private:
	virtual ks_raw_future_mode __get_mode() override { return ks_raw_future_mode::DX; }
	virtual bool __is_head_future() override { return true; }

#if __KS_ASYNC_DX_RAW_FUTURE_PSEUDO_MUTEX_ENABLED
	virtual ks_mutex* __get_mutex() override { return nullptr; }
	virtual bool __is_using_pseudo_mutex() override { return true; }
#elif __KS_ASYNC_DX_RAW_FUTURE_GLOBAL_MUTEX_ENABLED
	virtual ks_mutex* __get_mutex() override { return &g_raw_future_global_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#else
	ks_mutex m_self_mutex;
	virtual ks_mutex* __get_mutex() override { return &m_self_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#endif

	virtual std::shared_ptr<__INTERMEDIATE_DATA> __get_intermediate_data_ptr(ks_raw_future_lock& lock) override { return nullptr; }
	virtual void __clear_intermediate_data_ptr(const std::shared_ptr<__INTERMEDIATE_DATA>& intermediate_data_ptr, ks_raw_future_lock& lock) override { ASSERT(false); }
};


class ks_raw_promise_future final : public ks_raw_future_baseimp {
public:
	//注：默认apartment原设计使用current_thread_apartment，现已改为使用default_mta
	explicit ks_raw_promise_future(ks_raw_future_mode promise_mode) 
		: m_intermediate_data_ex_ptr(std::make_shared<__INTERMEDIATE_DATA_EX>()) {
		ASSERT(promise_mode == ks_raw_future_mode::PROMISE);
	}
	_DISABLE_COPY_CONSTRUCTOR(ks_raw_promise_future);

	void init(ks_apartment* spec_apartment) {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		do_init_base_locked(spec_apartment, ks_async_context{}, m_intermediate_data_ex_ptr, lock);
	}

	ks_raw_promise_ptr create_promise_representative() {
		return std::make_shared<ks_raw_promise_representative>(
			std::static_pointer_cast<ks_raw_promise_future>(this->shared_from_this()));
	}

protected:
	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) override {
		//ks_raw_promise_future的此方法不应被调用，而是应直接do_complete
		ASSERT(false);
	}

	virtual bool is_cancelable_self() override {
		return true; 
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override {
		//promise-future立即reject即可
		ASSERT(error.get_code() != 0);
		this->do_complete(error, nullptr, false);
	}

private:
	virtual ks_raw_future_mode __get_mode() override { return ks_raw_future_mode::PROMISE; }
	virtual bool __is_head_future() override { return true; }

#if __KS_ASYNC_RAW_FUTURE_GLOBAL_MUTEX_ENABLED
	virtual ks_mutex* __get_mutex() override { return &g_raw_future_global_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#else
	ks_mutex m_self_mutex;
	virtual ks_mutex* __get_mutex() override { return &m_self_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#endif

	using __INTERMEDIATE_DATA_EX = __INTERMEDIATE_DATA;

	std::shared_ptr<__INTERMEDIATE_DATA_EX> m_intermediate_data_ex_ptr;
	virtual std::shared_ptr<__INTERMEDIATE_DATA> __get_intermediate_data_ptr(ks_raw_future_lock& lock) override { return m_intermediate_data_ex_ptr; }
	inline  std::shared_ptr<__INTERMEDIATE_DATA_EX> __get_intermediate_data_ex_ptr(ks_raw_future_lock& lock) const { return m_intermediate_data_ex_ptr; }

	virtual void __clear_intermediate_data_ptr(const std::shared_ptr<__INTERMEDIATE_DATA>& intermediate_data_ptr, ks_raw_future_lock& lock) override {
		ASSERT(intermediate_data_ptr == m_intermediate_data_ex_ptr);
		ASSERT(m_intermediate_data_ex_ptr != nullptr);

		m_intermediate_data_ex_ptr.reset();
	}

private:
	class ks_raw_promise_representative final : public ks_raw_promise, public std::enable_shared_from_this<ks_raw_promise> {
	public:
		explicit ks_raw_promise_representative(std::shared_ptr<ks_raw_promise_future>&& promise_future)
			: m_promise_future(std::move(promise_future)) {
		}

		~ks_raw_promise_representative() {
			if (m_promise_future != nullptr && !m_promise_future->m_completed_result.is_completed()) {
				//若最终未被settle过，则自动reject，以确保future最终completed
				ASSERT(false);
				m_promise_future->do_complete(ks_error::unexpected_error(), nullptr, false);
			}
		}

		_DISABLE_COPY_CONSTRUCTOR(ks_raw_promise_representative);

	public: //override ks_raw_promise's methods
		virtual ks_raw_future_ptr get_future() override {
			return m_promise_future;
		}

		virtual void resolve(const ks_raw_value & value) override {
			m_promise_future->do_complete(value, nullptr, false);
		}

		virtual void reject(const ks_error & error) override {
			m_promise_future->do_complete(error, nullptr, false);
		}

		virtual void try_settle(const ks_raw_result & result) override {
			ASSERT(result.is_completed());
			if (result.is_completed())
				m_promise_future->do_complete(result, nullptr, false);
		}

	private:
		const std::shared_ptr<ks_raw_promise_future> m_promise_future;
	};
};


class ks_raw_task_future final : public ks_raw_future_baseimp {
public:
	explicit ks_raw_task_future(ks_raw_future_mode task_mode)
		: m_task_mode(task_mode), m_intermediate_data_ex_ptr(std::make_shared<__INTERMEDIATE_DATA_EX>()) {
		ASSERT(task_mode == ks_raw_future_mode::TASK || task_mode == ks_raw_future_mode::TASK_DELAYED);
	}
	_DISABLE_COPY_CONSTRUCTOR(ks_raw_task_future);

	void init(ks_apartment* spec_apartment, std::function<ks_raw_result()>&& task_fn, const ks_async_context& living_context, int64_t delay) {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		do_init_base_locked(spec_apartment, living_context, m_intermediate_data_ex_ptr, lock);
		do_submit_locked(std::move(task_fn), delay, m_intermediate_data_ex_ptr, lock, false);
	}

private:
	struct __INTERMEDIATE_DATA_EX;
	void do_submit_locked(std::function<ks_raw_result()>&& task_fn, int64_t delay, const std::shared_ptr<__INTERMEDIATE_DATA_EX>& intermediate_data_ex_ptr, ks_raw_future_lock& lock, bool must_keep_locked) {
		ASSERT(intermediate_data_ex_ptr != nullptr);
		ASSERT(intermediate_data_ex_ptr == __get_intermediate_data_ex_ptr(lock));
		ASSERT(lock.owns_lock() && !must_keep_locked);
		ASSERT(!m_completed_result.is_completed());

		intermediate_data_ex_ptr->m_task_fn = std::move(task_fn);
		intermediate_data_ex_ptr->m_delay = delay;

		ks_apartment* prefer_apartment = this->do_determine_prefer_apartment(intermediate_data_ex_ptr->m_spec_apartment);
		int priority = intermediate_data_ex_ptr->m_living_context.__get_priority();
		bool could_run_locally = (priority >= 0x10000) && (intermediate_data_ex_ptr->m_spec_apartment == nullptr || intermediate_data_ex_ptr->m_spec_apartment == prefer_apartment);

		//pending_schedule_fn不对context进行捕获。
		//这样做的意图是：对于delayed任务，当try_cancel时，即使apartment::try_unschedule失败，也不影响context的及时释放。
		std::function<void()> pending_schedule_fn = [this, this_shared = this->shared_from_this(), intermediate_data_ex_ptr, prefer_apartment, context = intermediate_data_ex_ptr->m_living_context]() mutable -> void {
			ks_raw_future_lock lock2(__get_mutex(), __is_using_pseudo_mutex());
			if (m_completed_result.is_completed())
				return; //pre-check cancelled

			intermediate_data_ex_ptr->m_pending_schedule_id = 0; //这个变量第一时间被清0

			ASSERT(!intermediate_data_ex_ptr->m_pending_touched_flag);
			intermediate_data_ex_ptr->m_pending_touched_flag = true;

			ks_raw_running_future_rtstt running_future_rtstt;
			ks_raw_living_context_rtstt living_context_rtstt;
			running_future_rtstt.apply(this, &tls_current_thread_running_future);
			living_context_rtstt.apply(context);

			ks_raw_result result;
			try {
				if (this->do_check_cancel_locked(lock2))
					result = this->do_acquire_cancel_error_locked(ks_error::cancelled_error(), lock2);
				else {
					std::function<ks_raw_result()> task_fn = std::move(intermediate_data_ex_ptr->m_task_fn);
					lock2.unlock();
					ks_defer defer_relock2([&lock2]() { lock2.lock(); });
					result = task_fn().require_completed_or_error();
					task_fn = {};
					defer_relock2.apply();
				}
			}
			catch (ks_error error) {
				result = error;
			}

			this->do_complete_locked(result, prefer_apartment, true, lock2, false);
		};

		if (could_run_locally) {
			lock.unlock();
			pending_schedule_fn(); //超高优先级、且spec_partment为nullptr，则立即执行，省掉schedule过程
			if (must_keep_locked)
				lock.lock();
		}
		else {
			intermediate_data_ex_ptr->m_pending_aparrment = prefer_apartment;
			intermediate_data_ex_ptr->m_pending_schedule_id = (m_task_mode == ks_raw_future_mode::TASK)
				? intermediate_data_ex_ptr->m_pending_aparrment->schedule(std::move(pending_schedule_fn), priority)
				: intermediate_data_ex_ptr->m_pending_aparrment->schedule_delayed(std::move(pending_schedule_fn), priority, intermediate_data_ex_ptr->m_delay);
			if (intermediate_data_ex_ptr->m_pending_schedule_id == 0) {
				//schedule失败，则立即将this标记为错误即可
				return this->do_complete_locked(ks_error::terminated_error(), nullptr, false, lock, false);
			}
		}
	}

protected:
	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) override {
		//ks_raw_promise_future的此方法不应被调用，而是应直接do_complete
		ASSERT(false);
	}

	virtual bool is_cancelable_self() override {
		return true; 
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		if (m_completed_result.is_completed())
			return;

		auto intermediate_data_ex_ptr = __get_intermediate_data_ex_ptr(lock);
		ASSERT(intermediate_data_ex_ptr != nullptr);

		//task-future标记cancel
		ASSERT(error.get_code() != 0);
		intermediate_data_ex_ptr->m_cancel_error = error;

		//若为未到期的延时task-future，则立即do_complete
		if (m_task_mode == ks_raw_future_mode::TASK_DELAYED && intermediate_data_ex_ptr->m_create_time + std::chrono::milliseconds(intermediate_data_ex_ptr->m_delay) > std::chrono::steady_clock::now()) {
			this->do_complete_locked(error, nullptr, false, lock, false);
		}
	}

private:
	const ks_raw_future_mode m_task_mode;  //const-like
	virtual ks_raw_future_mode __get_mode() override { return m_task_mode; }
	virtual bool __is_head_future() override { return true; }

#if __KS_ASYNC_RAW_FUTURE_GLOBAL_MUTEX_ENABLED
	virtual ks_mutex* __get_mutex() override { return &g_raw_future_global_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#else
	ks_mutex m_self_mutex;
	virtual ks_mutex* __get_mutex() override { return &m_self_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#endif

	struct __INTERMEDIATE_DATA_EX : __INTERMEDIATE_DATA {
		std::function<ks_raw_result()> m_task_fn; //在complete后被自动清除
		int64_t m_delay = 0;  //const-like
		ks_apartment* m_pending_aparrment = nullptr;
		uint64_t m_pending_schedule_id = 0;
		bool m_pending_touched_flag = false;
	};

	std::shared_ptr<__INTERMEDIATE_DATA_EX> m_intermediate_data_ex_ptr;
	virtual std::shared_ptr<__INTERMEDIATE_DATA> __get_intermediate_data_ptr(ks_raw_future_lock& lock) override { return m_intermediate_data_ex_ptr; }
	inline  std::shared_ptr<__INTERMEDIATE_DATA_EX> __get_intermediate_data_ex_ptr(ks_raw_future_lock& lock) const { return m_intermediate_data_ex_ptr; }

	virtual void __clear_intermediate_data_ptr(const std::shared_ptr<__INTERMEDIATE_DATA>& intermediate_data_ptr, ks_raw_future_lock& lock) override {
		ASSERT(intermediate_data_ptr == m_intermediate_data_ex_ptr);
		ASSERT(m_intermediate_data_ex_ptr != nullptr);

		if (m_intermediate_data_ex_ptr->m_pending_schedule_id != 0) {
			ASSERT(m_intermediate_data_ex_ptr->m_pending_aparrment != nullptr);
			m_intermediate_data_ex_ptr->m_pending_aparrment->try_unschedule(m_intermediate_data_ex_ptr->m_pending_schedule_id);
			m_intermediate_data_ex_ptr->m_pending_schedule_id = 0;
		}

		m_intermediate_data_ex_ptr->m_task_fn = {};

		m_intermediate_data_ex_ptr.reset();
	}
};


class ks_raw_pipe_future final : public ks_raw_future_baseimp {
public:
	explicit ks_raw_pipe_future(ks_raw_future_mode pipe_mode) 
		: m_pipe_mode(pipe_mode), m_intermediate_data_ex_ptr(std::make_shared<__INTERMEDIATE_DATA_EX>()) {
		ASSERT(pipe_mode == ks_raw_future_mode::THEN
			|| pipe_mode == ks_raw_future_mode::TRAP
			|| pipe_mode == ks_raw_future_mode::TRANSFORM
			|| pipe_mode == ks_raw_future_mode::ON_SUCCESS
			|| pipe_mode == ks_raw_future_mode::ON_FAILURE
			|| pipe_mode == ks_raw_future_mode::ON_COMPLETION
			|| pipe_mode == ks_raw_future_mode::FORWARD);
	}
	_DISABLE_COPY_CONSTRUCTOR(ks_raw_pipe_future);

	void init(ks_apartment* spec_apartment, std::function<ks_raw_result(const ks_raw_result&)>&& fn_ex, const ks_async_context& living_context, const ks_raw_future_ptr& prev_future) {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		do_init_base_locked(spec_apartment, living_context, m_intermediate_data_ex_ptr, lock);
		do_connect_locked(std::move(fn_ex), prev_future, m_intermediate_data_ex_ptr, lock, false);
	}

private:
	struct __INTERMEDIATE_DATA_EX;
	void do_connect_locked(std::function<ks_raw_result(const ks_raw_result&)>&& fn_ex, const ks_raw_future_ptr& prev_future, const std::shared_ptr<__INTERMEDIATE_DATA_EX>& intermediate_data_ex_ptr, ks_raw_future_lock& lock, bool must_keep_locked) {
		ASSERT(intermediate_data_ex_ptr != nullptr);
		ASSERT(intermediate_data_ex_ptr == __get_intermediate_data_ex_ptr(lock));
		ASSERT(lock.owns_lock() && !must_keep_locked);
		ASSERT(!m_completed_result.is_completed());
		ASSERT(prev_future != nullptr);

		intermediate_data_ex_ptr->m_fn_ex = std::move(fn_ex);
		intermediate_data_ex_ptr->m_prev_future_weak = prev_future;

		lock.unlock();

		prev_future->do_add_next(this->shared_from_this());

		if (must_keep_locked)
			lock.lock();
	}

protected:
	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) override {
		ASSERT(prev_result.is_completed());

		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		if (m_completed_result.is_completed())
			return; 

		auto intermediate_data_ex_ptr = __get_intermediate_data_ex_ptr(lock);
		ASSERT(intermediate_data_ex_ptr != nullptr);

		ASSERT(!intermediate_data_ex_ptr->m_prev_future_completed_flag);
		intermediate_data_ex_ptr->m_prev_future_completed_flag = true;

		ks_raw_result alt_prev_result = prev_result;
		if (prev_result.is_value() && this->do_check_cancel_locked(lock)) {
			//若this已被cancel，则将prev_result立即强制改为cancelled
			alt_prev_result = this->do_acquire_cancel_error_locked(ks_error::cancelled_error(), lock);
		}

		bool could_skip_run = false;
		switch (m_pipe_mode) {
		case ks_raw_future_mode::THEN:
		case ks_raw_future_mode::ON_SUCCESS:
			could_skip_run = !alt_prev_result.is_value();
			break;
		case ks_raw_future_mode::TRAP:
		case ks_raw_future_mode::ON_FAILURE:
			could_skip_run = !alt_prev_result.is_error();
			break;
		case ks_raw_future_mode::FORWARD:
			could_skip_run = true;
			break;
		}

		if (could_skip_run) {
			//可直接skip-run，则立即将this进行settle即可
			ks_apartment* prefer_apartment = do_determine_prefer_apartment_2(intermediate_data_ex_ptr->m_spec_apartment, prev_advice_apartment);
			this->do_complete_locked(alt_prev_result, prefer_apartment, true, lock, false);
			return;
		}

		int priority = intermediate_data_ex_ptr->m_living_context.__get_priority();
		ks_apartment* prefer_apartment = do_determine_prefer_apartment_2(intermediate_data_ex_ptr->m_spec_apartment, prev_advice_apartment);
		bool could_run_locally = (priority >= 0x10000) && (intermediate_data_ex_ptr->m_spec_apartment == nullptr || intermediate_data_ex_ptr->m_spec_apartment == prefer_apartment);

		std::function<void()> run_fn = [this, this_shared = this->shared_from_this(), intermediate_data_ex_ptr, alt_prev_result, prefer_apartment, context = intermediate_data_ex_ptr->m_living_context]() mutable -> void {
			ks_raw_future_lock lock2(__get_mutex(), __is_using_pseudo_mutex());
			if (m_completed_result.is_completed())
				return; //pre-check cancelled

			ks_raw_running_future_rtstt running_future_rtstt;
			ks_raw_living_context_rtstt living_context_rtstt;
			running_future_rtstt.apply(this, &tls_current_thread_running_future);
			living_context_rtstt.apply(context);

			ks_raw_result result;
			try {
				std::function<ks_raw_result(const ks_raw_result&)> fn_ex = std::move(intermediate_data_ex_ptr->m_fn_ex);
				lock2.unlock();
				ks_defer defer_relock2([&lock2]() { lock2.lock(); });
				result = fn_ex(alt_prev_result).require_completed_or_error();
				fn_ex = {};
				defer_relock2.apply();
			}
			catch (ks_error error) {
				result = error;
			}

			this->do_complete_locked(result, prefer_apartment, true, lock2, false);
		};

		if (could_run_locally) {
			lock.unlock();
			run_fn(); //超高优先级、且spec_partment为nullptr，则立即执行，省掉schedule过程
			return;
		}

		uint64_t act_schedule_id = prefer_apartment->schedule(std::move(run_fn), priority);
		if (act_schedule_id == 0) {
			//schedule失败，则立即将this标记为错误即可
			return this->do_complete_locked(ks_error::terminated_error(), prefer_apartment, true, lock, false);
		}
	}

	virtual bool is_cancelable_self() override {
		//pipe-future部分是非cancelable的（on_xxxx和forward）
		return __my_cancelable_flag();
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		if (m_completed_result.is_completed())
			return;

		auto intermediate_data_ex_ptr = __get_intermediate_data_ex_ptr(lock);
		ASSERT(intermediate_data_ex_ptr != nullptr);

		//pipe-future部分是非cancelable的（on_xxxx和forward）
		ks_raw_future_ptr not_completed_prev_future = !intermediate_data_ex_ptr->m_prev_future_completed_flag ? intermediate_data_ex_ptr->m_prev_future_weak.lock() : nullptr;

		//pipe-future标记cancel
		ASSERT(error.get_code() != 0);
		if (__my_cancelable_flag())
			intermediate_data_ex_ptr->m_cancel_error = error;

		if (!backtrack) {
			if (__my_cancelable_flag()) {
				_NOOP();
			}
			else {
				//非backtrack，但!cancelable，则回溯一层
				lock.unlock();
				if (not_completed_prev_future != nullptr)
					not_completed_prev_future->do_try_cancel(error, false);
			}
		}
		else {
			lock.unlock();
			if (not_completed_prev_future != nullptr)
				not_completed_prev_future->do_try_cancel(error, true);
		}
	}

private:
	bool __my_cancelable_flag() const {
		//pipe-future部分是非cancelable的（on_xxxx和forward）
		switch (m_pipe_mode) {
		case ks_raw_future_mode::ON_SUCCESS:
		case ks_raw_future_mode::ON_FAILURE:
		case ks_raw_future_mode::ON_COMPLETION:
		case ks_raw_future_mode::FORWARD:
			return false;
		default:
			return true;
		}
	}

private:
	const ks_raw_future_mode m_pipe_mode;  //const-like
	virtual ks_raw_future_mode __get_mode() override { return m_pipe_mode; }
	virtual bool __is_head_future() override { return false; }

#if __KS_ASYNC_RAW_FUTURE_GLOBAL_MUTEX_ENABLED
	virtual ks_mutex* __get_mutex() override { return &g_raw_future_global_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#else
	ks_mutex m_self_mutex;
	virtual ks_mutex* __get_mutex() override { return &m_self_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#endif

	struct __INTERMEDIATE_DATA_EX : __INTERMEDIATE_DATA {
		std::function<ks_raw_result(const ks_raw_result&)> m_fn_ex;  //在complete后被自动清除
		std::weak_ptr<ks_raw_future> m_prev_future_weak;             //在complete后被自动清除
		bool m_prev_future_completed_flag = false;
	};

	std::shared_ptr<__INTERMEDIATE_DATA_EX> m_intermediate_data_ex_ptr;
	virtual std::shared_ptr<__INTERMEDIATE_DATA> __get_intermediate_data_ptr(ks_raw_future_lock& lock) override { return m_intermediate_data_ex_ptr; }
	inline  std::shared_ptr<__INTERMEDIATE_DATA_EX> __get_intermediate_data_ex_ptr(ks_raw_future_lock& lock) const { return m_intermediate_data_ex_ptr; }

	virtual void __clear_intermediate_data_ptr(const std::shared_ptr<__INTERMEDIATE_DATA>& intermediate_data_ptr, ks_raw_future_lock& lock) override {
		ASSERT(intermediate_data_ptr == m_intermediate_data_ex_ptr);
		ASSERT(m_intermediate_data_ex_ptr != nullptr);

		m_intermediate_data_ex_ptr->m_fn_ex = {};
		m_intermediate_data_ex_ptr->m_prev_future_weak.reset();

		m_intermediate_data_ex_ptr.reset();
	}
};


class ks_raw_flatten_future final : public ks_raw_future_baseimp {
public:
	explicit ks_raw_flatten_future(ks_raw_future_mode flatten_mode) 
		: m_flatten_mode(flatten_mode), m_intermediate_data_ex_ptr(std::make_shared<__INTERMEDIATE_DATA_EX>()) {
		ASSERT(flatten_mode == ks_raw_future_mode::FLATTEN_THEN
			|| flatten_mode == ks_raw_future_mode::FLATTEN_TRAP
			|| flatten_mode == ks_raw_future_mode::FLATTEN_TRANSFORM);
	}
	_DISABLE_COPY_CONSTRUCTOR(ks_raw_flatten_future);
		
	void init(ks_apartment* spec_apartment, std::function<ks_raw_future_ptr(const ks_raw_result&)>&& afn_ex, const ks_async_context& living_context, const ks_raw_future_ptr& prev_future) {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		do_init_base_locked(spec_apartment, living_context, m_intermediate_data_ex_ptr, lock);
		do_connect_locked(std::move(afn_ex), prev_future, m_intermediate_data_ex_ptr, lock, false);
	}

private:
	struct __INTERMEDIATE_DATA_EX;
	void do_connect_locked(std::function<ks_raw_future_ptr(const ks_raw_result&)>&& afn_ex, const ks_raw_future_ptr& prev_future, const std::shared_ptr<__INTERMEDIATE_DATA_EX>& intermediate_data_ex_ptr, ks_raw_future_lock& lock, bool must_keep_locked) {
		ASSERT(intermediate_data_ex_ptr != nullptr);
		ASSERT(intermediate_data_ex_ptr == __get_intermediate_data_ex_ptr(lock));
		ASSERT(lock.owns_lock() && !must_keep_locked);
		ASSERT(!m_completed_result.is_completed());
		ASSERT(prev_future != nullptr);

		intermediate_data_ex_ptr->m_afn_ex = std::move(afn_ex);
		intermediate_data_ex_ptr->m_prev_future_weak = prev_future;

		auto this_shared = this->shared_from_this();
		auto context = intermediate_data_ex_ptr->m_living_context;

		lock.unlock();

		prev_future->do_add_next(this->shared_from_this());

		if (must_keep_locked)
			lock.lock();
	}

protected:
	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) override {
		ASSERT(prev_result.is_completed());

		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		if (m_completed_result.is_completed())
			return;

		auto intermediate_data_ex_ptr = __get_intermediate_data_ex_ptr(lock);
		ASSERT(intermediate_data_ex_ptr != nullptr);

		ASSERT(!intermediate_data_ex_ptr->m_prev_future_completed_flag);
		intermediate_data_ex_ptr->m_prev_future_completed_flag = true;

		ks_raw_result alt_prev_result = prev_result;
		if (prev_result.is_value() && this->do_check_cancel_locked(lock)) {
			//若this已被cancel，则将prev_result立即强制改为cancelled
			alt_prev_result = this->do_acquire_cancel_error_locked(ks_error::cancelled_error(), lock);
		}

		bool could_skip_run = false;
		switch (m_flatten_mode) {
		case ks_raw_future_mode::FLATTEN_THEN:
			could_skip_run = !alt_prev_result.is_value();
			break;
		case ks_raw_future_mode::FLATTEN_TRAP:
			could_skip_run = !alt_prev_result.is_error();
			break;
		}

		if (could_skip_run) {
			//可直接skip-run，则立即将this进行settle即可
			ks_apartment* prefer_apartment = do_determine_prefer_apartment_2(intermediate_data_ex_ptr->m_spec_apartment, prev_advice_apartment);
			this->do_complete_locked(alt_prev_result, prefer_apartment, true, lock, false);
			return;
		}

		int priority = intermediate_data_ex_ptr->m_living_context.__get_priority();
		ks_apartment* prefer_apartment = do_determine_prefer_apartment_2(intermediate_data_ex_ptr->m_spec_apartment, prev_advice_apartment);
		bool could_run_locally = (priority >= 0x10000) && (intermediate_data_ex_ptr->m_spec_apartment == nullptr || intermediate_data_ex_ptr->m_spec_apartment == prefer_apartment);

		std::function<void()> run_fn = [this, this_shared = this->shared_from_this(), intermediate_data_ex_ptr, alt_prev_result, prefer_apartment, context = intermediate_data_ex_ptr->m_living_context]() mutable -> void {
			ks_raw_future_lock lock2(__get_mutex(), __is_using_pseudo_mutex());
			if (m_completed_result.is_completed())
				return; //pre-check cancelled

			ks_raw_running_future_rtstt running_future_rtstt;
			ks_raw_living_context_rtstt living_context_rtstt;
			running_future_rtstt.apply(this, &tls_current_thread_running_future);
			living_context_rtstt.apply(context);

			ks_raw_future_ptr extern_future;
			try {
				std::function<ks_raw_future_ptr(const ks_raw_result&)> afn_ex = std::move(intermediate_data_ex_ptr->m_afn_ex);
				lock2.unlock();
				ks_defer defer_relock2([&lock2]() { lock2.lock(); });
				extern_future = afn_ex(alt_prev_result);
				if (extern_future == nullptr) {
					ASSERT(false);
					extern_future = ks_raw_future::rejected(ks_error::unexpected_error(), prefer_apartment);
				}
				afn_ex = {};
				defer_relock2.apply();
			}
			catch (ks_error error) {
				extern_future = ks_raw_future::rejected(error, prefer_apartment);
			}

			ASSERT(extern_future != nullptr);
			intermediate_data_ex_ptr->m_extern_future_weak = extern_future;

			lock2.unlock();
			extern_future->on_completion([this, this_shared, intermediate_data_ex_ptr, prefer_apartment](const ks_raw_result& extern_result) {
				//注：在extern_future完成回调中，直接调用this的do_complete方法
				ks_raw_future_lock lock3(__get_mutex(), __is_using_pseudo_mutex());

				ASSERT(!intermediate_data_ex_ptr->m_extern_future_completed_flag);
				intermediate_data_ex_ptr->m_extern_future_completed_flag = true;

				this->do_complete_locked(extern_result, prefer_apartment, false, lock3, false);
			}, make_async_context().set_priority(0x10000), prefer_apartment);
		};

		if (could_run_locally) {
			lock.unlock();
			run_fn(); //超高优先级、且spec_partment为nullptr，则立即执行，省掉schedule过程
			return;
		}

		uint64_t act_schedule_id = prefer_apartment->schedule(std::move(run_fn), priority);
		if (act_schedule_id == 0) {
			//schedule失败，则立即将this标记为错误即可
			return this->do_complete_locked(ks_error::terminated_error(), prefer_apartment, true, lock, false);
		}
	}

	virtual bool is_cancelable_self() override {
		return true; 
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		if (m_completed_result.is_completed())
			return;

		auto intermediate_data_ex_ptr = __get_intermediate_data_ex_ptr(lock);
		ASSERT(intermediate_data_ex_ptr != nullptr);

		//对于extern_future，因其物理上不在future链中，故若其已存在，则始终要无条件try-cancel
		ks_raw_future_ptr not_completed_prev_future = !intermediate_data_ex_ptr->m_prev_future_completed_flag ? intermediate_data_ex_ptr->m_prev_future_weak.lock() : nullptr;
		ks_raw_future_ptr not_completed_extern_future = !intermediate_data_ex_ptr->m_extern_future_completed_flag ? intermediate_data_ex_ptr->m_extern_future_weak.lock() : nullptr;

		//flatten-future标记cancel（都是cancelable的）
		ASSERT(error.get_code() != 0);
		intermediate_data_ex_ptr->m_cancel_error = error;

		lock.unlock();
		//无条件对extern做cancel
		if (not_completed_extern_future != nullptr)
			not_completed_extern_future->do_try_cancel(error, backtrack);
		//当backtrack时，对prev做cancel
		if (backtrack && not_completed_prev_future != nullptr)
			not_completed_prev_future->do_try_cancel(error, true);
	}

private:
	const ks_raw_future_mode m_flatten_mode;  //const-like
	virtual ks_raw_future_mode __get_mode() override { return m_flatten_mode; }
	virtual bool __is_head_future() override { return false; }

#if __KS_ASYNC_RAW_FUTURE_GLOBAL_MUTEX_ENABLED
	virtual ks_mutex* __get_mutex() override { return &g_raw_future_global_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#else
	ks_mutex m_self_mutex;
	virtual ks_mutex* __get_mutex() override { return &m_self_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#endif

	struct __INTERMEDIATE_DATA_EX : __INTERMEDIATE_DATA {
		std::function<ks_raw_future_ptr(const ks_raw_result&)> m_afn_ex; //在complete后被自动清除
		std::weak_ptr<ks_raw_future> m_prev_future_weak;                 //在complete后被自动清除
		std::weak_ptr<ks_raw_future> m_extern_future_weak;               //中间过程中初始化，在complete后被自动清除
		bool m_prev_future_completed_flag = false;
		bool m_extern_future_completed_flag = false;
	};

	std::shared_ptr<__INTERMEDIATE_DATA_EX> m_intermediate_data_ex_ptr;
	virtual std::shared_ptr<__INTERMEDIATE_DATA> __get_intermediate_data_ptr(ks_raw_future_lock& lock) override { return m_intermediate_data_ex_ptr; }
	inline  std::shared_ptr<__INTERMEDIATE_DATA_EX> __get_intermediate_data_ex_ptr(ks_raw_future_lock& lock) const { return m_intermediate_data_ex_ptr; }

	virtual void __clear_intermediate_data_ptr(const std::shared_ptr<__INTERMEDIATE_DATA>& intermediate_data_ptr, ks_raw_future_lock& lock) override {
		ASSERT(intermediate_data_ptr == m_intermediate_data_ex_ptr);
		ASSERT(m_intermediate_data_ex_ptr != nullptr);

		m_intermediate_data_ex_ptr->m_afn_ex = {};
		m_intermediate_data_ex_ptr->m_prev_future_weak.reset();
		m_intermediate_data_ex_ptr->m_extern_future_weak.reset();

		m_intermediate_data_ex_ptr.reset();
	}
};


class ks_raw_aggr_future final : public ks_raw_future_baseimp {
public:
	explicit ks_raw_aggr_future(ks_raw_future_mode aggr_mode) 
		: m_aggr_mode(aggr_mode), m_intermediate_data_ex_ptr(std::make_shared<__INTERMEDIATE_DATA_EX>()) {
		ASSERT(aggr_mode == ks_raw_future_mode::ALL
			|| aggr_mode == ks_raw_future_mode::ALL_COMPLETED
			|| aggr_mode == ks_raw_future_mode::ANY);
	}
	_DISABLE_COPY_CONSTRUCTOR(ks_raw_aggr_future);

	void init(ks_apartment* spec_apartment, const std::vector<ks_raw_future_ptr>& prev_futures) {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		do_init_base_locked(spec_apartment, ks_async_context{}, m_intermediate_data_ex_ptr, lock);
		do_connect_locked(prev_futures, m_intermediate_data_ex_ptr, lock, false);
	}

private:
	struct __INTERMEDIATE_DATA_EX;
	void do_connect_locked(const std::vector<ks_raw_future_ptr>& prev_futures, const std::shared_ptr<__INTERMEDIATE_DATA_EX>& intermediate_data_ex_ptr, ks_raw_future_lock& lock, bool must_keep_locked) {
		ASSERT(intermediate_data_ex_ptr != nullptr);
		ASSERT(intermediate_data_ex_ptr == __get_intermediate_data_ex_ptr(lock));
		ASSERT(lock.owns_lock() && !must_keep_locked);
		ASSERT(!m_completed_result.is_completed());
		ASSERT(!prev_futures.empty());

		intermediate_data_ex_ptr->m_prev_future_weak_seq.reserve(prev_futures.size());
		intermediate_data_ex_ptr->m_not_completed_prev_future_raw_p_seq.reserve(prev_futures.size());
		for (auto& prev_future : prev_futures) {
			intermediate_data_ex_ptr->m_prev_future_weak_seq.push_back(prev_future);
			intermediate_data_ex_ptr->m_not_completed_prev_future_raw_p_seq.push_back(prev_future.get());
		}
		intermediate_data_ex_ptr->m_prev_total_count = prev_futures.size();
		intermediate_data_ex_ptr->m_prev_completed_count = 0;

		intermediate_data_ex_ptr->m_prev_result_seq_cache.resize(prev_futures.size(), ks_raw_result());
		intermediate_data_ex_ptr->m_prev_prefer_apartment_seq_cache.resize(prev_futures.size(), nullptr);
		intermediate_data_ex_ptr->m_prev_first_resolved_index = -1;
		intermediate_data_ex_ptr->m_prev_first_rejected_index = -1;

		lock.unlock();

		ks_raw_future_ptr this_shared = this->shared_from_this();
		for (auto& prev_future : prev_futures)
			prev_future->do_add_next(this_shared);

		if (must_keep_locked)
			lock.lock();
	}

protected:
	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) override {
		ASSERT(prev_result.is_completed());

		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		if (m_completed_result.is_completed())
			return;

		auto intermediate_data_ex_ptr = __get_intermediate_data_ex_ptr(lock);
		ASSERT(intermediate_data_ex_ptr != nullptr);

		size_t prev_index_just = -1;
		ASSERT(intermediate_data_ex_ptr->m_prev_completed_count < intermediate_data_ex_ptr->m_prev_total_count);
		while (intermediate_data_ex_ptr->m_prev_completed_count < intermediate_data_ex_ptr->m_prev_total_count) { //此处while为了支持future重复出现
			auto not_completed_prev_future_iter = std::find(
				intermediate_data_ex_ptr->m_not_completed_prev_future_raw_p_seq.cbegin(),
				intermediate_data_ex_ptr->m_not_completed_prev_future_raw_p_seq.cend(),
				prev_future);
			if (not_completed_prev_future_iter == intermediate_data_ex_ptr->m_not_completed_prev_future_raw_p_seq.cend())
				break; //miss prev_future (unexpected)

			size_t prev_index = not_completed_prev_future_iter - intermediate_data_ex_ptr->m_not_completed_prev_future_raw_p_seq.cbegin();
			if (prev_index_just == -1)
				prev_index_just = prev_index;

			ASSERT(!intermediate_data_ex_ptr->m_prev_result_seq_cache[prev_index].is_completed());

			intermediate_data_ex_ptr->m_not_completed_prev_future_raw_p_seq[prev_index] = nullptr;
			intermediate_data_ex_ptr->m_prev_result_seq_cache[prev_index] = prev_result.require_completed_or_error();
			intermediate_data_ex_ptr->m_prev_prefer_apartment_seq_cache[prev_index] = prev_advice_apartment;

			intermediate_data_ex_ptr->m_prev_completed_count++;
			if (intermediate_data_ex_ptr->m_prev_first_resolved_index == -1 && prev_result.is_value())
				intermediate_data_ex_ptr->m_prev_first_resolved_index = prev_index;
			if (intermediate_data_ex_ptr->m_prev_first_rejected_index == -1 && !prev_result.is_value())
				intermediate_data_ex_ptr->m_prev_first_rejected_index = prev_index;
		}

		if (prev_index_just == -1) {
			ASSERT(false);
			return;
		}

		do_check_and_try_settle_me_locked(prev_result, prev_advice_apartment, lock, false);
	}

	virtual bool is_cancelable_self() override {
		//aggr-future实质上都是forward，都可认为是非cancelable的
		return false;
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override {
		ks_raw_future_lock lock(__get_mutex(), __is_using_pseudo_mutex());
		if (m_completed_result.is_completed())
			return;

		auto intermediate_data_ex_ptr = __get_intermediate_data_ex_ptr(lock);
		ASSERT(intermediate_data_ex_ptr != nullptr);

		//aggr-future实质上都是forward，都可认为是非cancelable的
		ASSERT(error.get_code() != 0);
		_NOOP();

		//既然是forward，那么始终要无条件backtrack
		std::vector<ks_raw_future_ptr> not_completed_prev_future_vec;
		not_completed_prev_future_vec.reserve(intermediate_data_ex_ptr->m_prev_future_weak_seq.size());
		for (size_t i = 0; i < intermediate_data_ex_ptr->m_prev_future_weak_seq.size(); ++i) {
			if (intermediate_data_ex_ptr->m_not_completed_prev_future_raw_p_seq[i] != nullptr) {
				auto prev_future_opt = intermediate_data_ex_ptr->m_prev_future_weak_seq[i].lock();
				if (prev_future_opt != nullptr)
					not_completed_prev_future_vec.push_back(std::move(prev_future_opt));
			}
		}

		lock.unlock();
		for (auto& prev_fut : not_completed_prev_future_vec) {
			if (prev_fut != nullptr)
				prev_fut->do_try_cancel(error, backtrack);
		}
	}

private:
	void do_check_and_try_settle_me_locked(const ks_raw_result& prev_result, ks_apartment* prev_advice_apartment, ks_raw_future_lock& lock, bool must_keep_locked) {
		ASSERT(lock.owns_lock() && !must_keep_locked);

		ASSERT(!m_completed_result.is_completed());

		auto intermediate_data_ex_ptr = __get_intermediate_data_ex_ptr(lock);
		ASSERT(intermediate_data_ex_ptr != nullptr);

		//aggr-future是非cancelable的
		ASSERT(!this->do_check_cancel_locked(lock));

		//check and try settle me ...
		switch (m_aggr_mode) {
		case ks_raw_future_mode::ALL:
			if (intermediate_data_ex_ptr->m_prev_first_rejected_index != -1) {
				//前序任务出现失败
				ks_apartment* prefer_apartment = do_determine_prefer_apartment_2(intermediate_data_ex_ptr->m_spec_apartment, prev_advice_apartment);
				ks_error prev_error_first = intermediate_data_ex_ptr->m_prev_result_seq_cache[intermediate_data_ex_ptr->m_prev_first_rejected_index].to_error();
				this->do_complete_locked(prev_error_first, prefer_apartment, true, lock, must_keep_locked);
				return;
			}
			else if (intermediate_data_ex_ptr->m_prev_completed_count == intermediate_data_ex_ptr->m_prev_total_count) {
				//前序任务全部成功
				ks_apartment* prefer_apartment = do_determine_prefer_apartment_2(intermediate_data_ex_ptr->m_spec_apartment, prev_advice_apartment);
				std::vector<ks_raw_value> prev_value_seq;
				prev_value_seq.reserve(intermediate_data_ex_ptr->m_prev_result_seq_cache.size());
				for (auto& prev_result : intermediate_data_ex_ptr->m_prev_result_seq_cache)
					prev_value_seq.push_back(prev_result.to_value());
				this->do_complete_locked(ks_raw_value::of<std::vector<ks_raw_value>>(std::move(prev_value_seq)), prefer_apartment, true, lock, must_keep_locked);
				return;
			}
			else {
				break;
			}

		case ks_raw_future_mode::ALL_COMPLETED:
			if (intermediate_data_ex_ptr->m_prev_completed_count == intermediate_data_ex_ptr->m_prev_total_count) {
				//前序任务全部完成（无论成功/失败）
				ks_apartment* prefer_apartment = do_determine_prefer_apartment_2(intermediate_data_ex_ptr->m_spec_apartment, prev_advice_apartment);
				std::vector<ks_raw_result> prev_result_seq = intermediate_data_ex_ptr->m_prev_result_seq_cache;
				this->do_complete_locked(ks_raw_value::of<std::vector<ks_raw_result>>(std::move(prev_result_seq)), prefer_apartment, true, lock, must_keep_locked);
				return;
			}
			else {
				break;
			}

		case ks_raw_future_mode::ANY:
			if (intermediate_data_ex_ptr->m_prev_first_resolved_index != -1) {
				//前序任务出现成功
				ks_apartment* prefer_apartment = do_determine_prefer_apartment_2(intermediate_data_ex_ptr->m_spec_apartment, prev_advice_apartment);
				ks_raw_value prev_value_first = intermediate_data_ex_ptr->m_prev_result_seq_cache[intermediate_data_ex_ptr->m_prev_first_resolved_index].to_value();
				this->do_complete_locked(prev_value_first, prefer_apartment, true, lock, must_keep_locked);
				return;
			}
			else if (intermediate_data_ex_ptr->m_prev_completed_count == intermediate_data_ex_ptr->m_prev_total_count) {
				//前序任务全部失败
				ASSERT(intermediate_data_ex_ptr->m_prev_first_rejected_index != -1);
				ks_apartment* prefer_apartment = do_determine_prefer_apartment_2(intermediate_data_ex_ptr->m_spec_apartment, prev_advice_apartment);
				ks_error prev_error_first = intermediate_data_ex_ptr->m_prev_result_seq_cache[intermediate_data_ex_ptr->m_prev_first_rejected_index].to_error();
				this->do_complete_locked(prev_error_first, prefer_apartment, true, lock, must_keep_locked);
				return;
			}
			else {
				break;
			}

		default:
			ASSERT(false);
			break;
		}
	}

private:
	const ks_raw_future_mode m_aggr_mode;  //const-like
	virtual ks_raw_future_mode __get_mode() override { return m_aggr_mode; }
	virtual bool __is_head_future() override { return false; }

#if __KS_ASYNC_RAW_FUTURE_GLOBAL_MUTEX_ENABLED
	virtual ks_mutex* __get_mutex() override { return &g_raw_future_global_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#else
	ks_mutex m_self_mutex;
	virtual ks_mutex* __get_mutex() override { return &m_self_mutex; }
	virtual bool __is_using_pseudo_mutex() override { return false; }
#endif

	struct __INTERMEDIATE_DATA_EX : __INTERMEDIATE_DATA {
		std::vector<std::weak_ptr<ks_raw_future>> m_prev_future_weak_seq;  //在complete后被自动清除
		std::vector<ks_raw_future*> m_not_completed_prev_future_raw_p_seq; //在complete后被自动清除
		size_t m_prev_total_count = 0;
		size_t m_prev_completed_count = 0;

		std::vector<ks_raw_result> m_prev_result_seq_cache;           //在complete后被自动清除
		std::vector<ks_apartment*> m_prev_prefer_apartment_seq_cache; //在complete后被自动清除
		size_t m_prev_first_resolved_index = -1; //在complete后被自动清除
		size_t m_prev_first_rejected_index = -1; //在complete后被自动清除
	};

	std::shared_ptr<__INTERMEDIATE_DATA_EX> m_intermediate_data_ex_ptr;
	virtual std::shared_ptr<__INTERMEDIATE_DATA> __get_intermediate_data_ptr(ks_raw_future_lock& lock) override { return m_intermediate_data_ex_ptr; }
	inline  std::shared_ptr<__INTERMEDIATE_DATA_EX> __get_intermediate_data_ex_ptr(ks_raw_future_lock& lock) const { return m_intermediate_data_ex_ptr; }

	virtual void __clear_intermediate_data_ptr(const std::shared_ptr<__INTERMEDIATE_DATA>& intermediate_data_ptr, ks_raw_future_lock& lock) override {
		ASSERT(intermediate_data_ptr == m_intermediate_data_ex_ptr);
		ASSERT(m_intermediate_data_ex_ptr != nullptr);

		m_intermediate_data_ex_ptr->m_prev_future_weak_seq.clear();
		m_intermediate_data_ex_ptr->m_not_completed_prev_future_raw_p_seq.clear();
		m_intermediate_data_ex_ptr->m_prev_result_seq_cache.clear();
		m_intermediate_data_ex_ptr->m_prev_prefer_apartment_seq_cache.clear();

		m_intermediate_data_ex_ptr.reset();
	}
};


//ks_raw_future静态方法实现
ks_raw_future_ptr ks_raw_future::resolved(const ks_raw_value& value, ks_apartment* apartment) {
	auto dx_future = std::make_shared<ks_raw_dx_future>(ks_raw_future_mode::DX);
	dx_future->init(apartment, ks_raw_result(value));
	return dx_future;
}

ks_raw_future_ptr ks_raw_future::rejected(const ks_error& error, ks_apartment* apartment) {
	auto dx_future = std::make_shared<ks_raw_dx_future>(ks_raw_future_mode::DX);
	dx_future->init(apartment, ks_raw_result(error));
	return dx_future;
}

ks_raw_future_ptr ks_raw_future::__from_result(const ks_raw_result& result, ks_apartment* apartment) {
	ASSERT(result.is_completed());
	auto dx_future = std::make_shared<ks_raw_dx_future>(ks_raw_future_mode::DX);
	dx_future->init(apartment, result.is_completed() ? result : ks_raw_result(ks_error::unexpected_error()));
	return dx_future;
}


ks_raw_future_ptr ks_raw_future::post(std::function<ks_raw_result()>&& task_fn, const ks_async_context& context, ks_apartment* apartment) {
	auto task_future = std::make_shared<ks_raw_task_future>(ks_raw_future_mode::TASK);
	task_future->init(apartment, std::move(task_fn), context, 0);
	return task_future;
}

ks_raw_future_ptr ks_raw_future::post_delayed(std::function<ks_raw_result()>&& task_fn, const ks_async_context& context, ks_apartment* apartment, int64_t delay) {
	auto task_future = std::make_shared<ks_raw_task_future>(ks_raw_future_mode::TASK_DELAYED);
	task_future->init(apartment, std::move(task_fn), context, delay);
	return task_future;
}


ks_raw_future_ptr ks_raw_future::all(const std::vector<ks_raw_future_ptr>& futures, ks_apartment* apartment) {
	if (futures.empty())
		return ks_raw_future::resolved(ks_raw_value::of<std::vector<ks_raw_value>>(std::vector<ks_raw_value>()), apartment);

	auto aggr_future = std::make_shared<ks_raw_aggr_future>(ks_raw_future_mode::ALL);
	aggr_future->init(apartment, futures);
	return aggr_future;
}

ks_raw_future_ptr ks_raw_future::all_completed(const std::vector<ks_raw_future_ptr>& futures, ks_apartment* apartment) {
	if (futures.empty())
		return ks_raw_future::resolved(ks_raw_value::of<std::vector<ks_raw_result>>(std::vector<ks_raw_result>()), apartment);

	auto aggr_future = std::make_shared<ks_raw_aggr_future>(ks_raw_future_mode::ALL_COMPLETED);
	aggr_future->init(apartment, futures);
	return aggr_future;
}

ks_raw_future_ptr ks_raw_future::any(const std::vector<ks_raw_future_ptr>& futures, ks_apartment* apartment) {
	if (futures.empty())
		return ks_raw_future::rejected(ks_error::unexpected_error(), apartment);
	if (futures.size() == 1)
		return futures.at(0);

	auto aggr_future = std::make_shared<ks_raw_aggr_future>(ks_raw_future_mode::ANY);
	aggr_future->init(apartment, futures);
	return aggr_future;
}

void ks_raw_future::set_timeout(int64_t timeout, bool backtrack) {
	return this->do_set_timeout(timeout, ks_error::timeout_error(), backtrack);
}

void ks_raw_future::__try_cancel(bool backtrack) {
	this->do_try_cancel(ks_error::cancelled_error(), backtrack);
}

bool ks_raw_future::__check_current_future_cancel(bool with_extra) {
	ks_raw_future* cur_future = tls_current_thread_running_future;
	if (cur_future == nullptr || !cur_future->is_cancelable_self())
		return false;

	ASSERT(!cur_future->is_completed());
	if (cur_future->do_check_cancel())
		return true;

	if (with_extra) {
		ks_apartment* cur_apartment = ks_apartment::current_thread_apartment();
		if (cur_apartment != nullptr) {
			if (cur_apartment->is_stopping_or_stopped())
				return true;
		}
	}

	return false;
}

ks_error ks_raw_future::__acquire_current_future_cancel_error(const ks_error& def_error, bool with_extra) {
	ks_raw_future* cur_future = tls_current_thread_running_future;
	if (cur_future == nullptr || !cur_future->is_cancelable_self())
		return ks_error();

	ASSERT(!cur_future->is_completed());
	ks_error error = cur_future->do_acquire_cancel_error(ks_error());
	if (error.get_code() != 0)
		return error;

	if (with_extra) {
		ks_apartment* cur_apartment = ks_apartment::current_thread_apartment();
		if (cur_apartment != nullptr) {
			if (cur_apartment->is_stopping_or_stopped())
				return ks_error::terminated_error();
		}
	}

	return def_error;
}

void ks_raw_future::__wait() {
	return (void)this->do_wait();
}

ks_raw_promise_ptr ks_raw_promise::create(ks_apartment* apartment) {
	auto promise_future = std::make_shared<ks_raw_promise_future>(ks_raw_future_mode::PROMISE);
	promise_future->init(apartment);
	return promise_future->create_promise_representative();
}



//ks_raw_future基础pipe方法实现
ks_raw_future_ptr ks_raw_future_baseimp::then(std::function<ks_raw_result(const ks_raw_value &)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	std::function<ks_raw_result(const ks_raw_result&)> fn_ex = [fn = std::move(fn)](const ks_raw_result& input)->ks_raw_result {
		if (input.is_value())
			return fn(input.to_value());
		else
			return input;
	};

	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::THEN);
	pipe_future->init(apartment, std::move(fn_ex), context, this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::trap(std::function<ks_raw_result(const ks_error &)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	std::function<ks_raw_result(const ks_raw_result&)> fn_ex = [fn = std::move(fn)](const ks_raw_result& input)->ks_raw_result {
		if (input.is_error())
			return fn(input.to_error());
		else
			return input;
	};

	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::TRAP);
	pipe_future->init(apartment, std::move(fn_ex), context, this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::transform(std::function<ks_raw_result(const ks_raw_result &)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::TRANSFORM);
	pipe_future->init(apartment, std::move(fn), context, this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::flat_then(std::function<ks_raw_future_ptr(const ks_raw_value&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	std::function<ks_raw_future_ptr(const ks_raw_result&)> afn_ex = [fn = std::move(fn), apartment](const ks_raw_result& input)->ks_raw_future_ptr {
		if (!input.is_value())
			return ks_raw_future::rejected(input.to_error(), apartment);

		ks_raw_future_ptr extern_future = fn(input.to_value());
		ASSERT(extern_future != nullptr);
		return extern_future;
	};

	auto flatten_future = std::make_shared<ks_raw_flatten_future>(ks_raw_future_mode::FLATTEN_THEN);
	flatten_future->init(apartment, std::move(afn_ex), context, this->shared_from_this());
	return flatten_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::flat_trap(std::function<ks_raw_future_ptr(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	std::function<ks_raw_future_ptr(const ks_raw_result&)> afn_ex = [fn = std::move(fn), apartment](const ks_raw_result& input)->ks_raw_future_ptr {
		if (!input.is_error())
			return ks_raw_future::resolved(input.to_value(), apartment);

		ks_raw_future_ptr extern_future = fn(input.to_error());
		ASSERT(extern_future != nullptr);
		return extern_future;
	};

	auto flatten_future = std::make_shared<ks_raw_flatten_future>(ks_raw_future_mode::FLATTEN_TRAP);
	flatten_future->init(apartment, std::move(afn_ex), context, this->shared_from_this());
	return flatten_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::flat_transform(std::function<ks_raw_future_ptr(const ks_raw_result&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	auto flatten_future = std::make_shared<ks_raw_flatten_future>(ks_raw_future_mode::FLATTEN_TRANSFORM);
	flatten_future->init(apartment, std::move(fn), context, this->shared_from_this());
	return flatten_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::on_success(std::function<void(const ks_raw_value&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	auto fn_ex = [fn = std::move(fn)](const ks_raw_result& input)->ks_raw_result {
		if (input.is_value())
			fn(input.to_value());
		return input;
	};

	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::ON_SUCCESS);
	pipe_future->init(apartment, std::move(fn_ex), context, this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::on_failure(std::function<void(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	auto fn_ex = [fn = std::move(fn)](const ks_raw_result& input)->ks_raw_result {
		if (input.is_error())
			fn(input.to_error());
		return input;
	};

	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::ON_FAILURE);
	pipe_future->init(apartment, std::move(fn_ex), context, this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::on_completion(std::function<void(const ks_raw_result&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	auto fn_ex = [fn = std::move(fn)](const ks_raw_result& input)->ks_raw_result {
		fn(input);
		return input;
	};

	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::ON_COMPLETION);
	pipe_future->init(apartment, std::move(fn_ex), context, this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::noop(ks_apartment* apartment) {
	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::FORWARD);
	pipe_future->init(apartment,
		[](const auto& input) -> auto { return input; },
		make_async_context().set_priority(0x10000), 
		this->shared_from_this());
	return pipe_future;
}


__KS_ASYNC_RAW_END
