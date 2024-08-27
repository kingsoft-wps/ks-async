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
#include "ks_raw_internal_helper.h"
#include "../ktl/ks_concurrency.h"
#include <algorithm>

template <class FN>
using function = std::function<FN>;

void __forcelink_to_ks_raw_future_cpp() {}


__KS_ASYNC_RAW_BEGIN

static thread_local ks_raw_future* tls_current_thread_running_future = nullptr;


enum class ks_raw_future_mode {
	DX, PROMISE,  //promise
	TASK, TASK_DELAYED,  //task
	THEN, TRAP, TRANSFORM, FORWARD,  //pipe
	FLATTEN_THEN, FLATTEN_TRAP, FLATTEN_TRANSFORM,  //flatten
	ALL, ALL_COMPLETED, ANY, //aggr
};

class ks_raw_future_baseimp : public ks_raw_future, public std::enable_shared_from_this<ks_raw_future> {
protected:
	explicit ks_raw_future_baseimp(ks_raw_future_mode mode, bool cancelable, ks_apartment* spec_apartment, const ks_async_context& living_context)
		: m_spec_apartment(spec_apartment), m_create_time(std::chrono::steady_clock::now()), m_living_context(living_context)
		, m_mode(mode), m_cancelable(cancelable), m_living_context_controller_available_v(living_context.__is_controller_present()) {}

	_DISABLE_COPY_CONSTRUCTOR(ks_raw_future_baseimp);

public:
	virtual ks_raw_future_ptr then(function<ks_raw_result(const ks_raw_value &)>&& fn, const ks_async_context& context, ks_apartment* apartment) override;
	virtual ks_raw_future_ptr trap(function<ks_raw_result(const ks_error &)>&& fn, const ks_async_context& context, ks_apartment* apartment) override;
	virtual ks_raw_future_ptr transform(function<ks_raw_result(const ks_raw_result &)>&& fn, const ks_async_context& context, ks_apartment* apartment) override;

	virtual ks_raw_future_ptr flat_then(function<ks_raw_future_ptr(const ks_raw_value&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override;
	virtual ks_raw_future_ptr flat_trap(function<ks_raw_future_ptr(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override;
	virtual ks_raw_future_ptr flat_transform(function<ks_raw_future_ptr(const ks_raw_result&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override;

	virtual ks_raw_future_ptr on_success(function<void(const ks_raw_value&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override;
	virtual ks_raw_future_ptr on_failure(function<void(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override;
	virtual ks_raw_future_ptr on_completion(function<void(const ks_raw_result&)>&& fn, const ks_async_context& context, ks_apartment* apartment) override;

	virtual ks_raw_future_ptr noop(ks_apartment* apartment) override;

public:
	virtual bool is_completed() override {
		std::unique_lock<ks_mutex> lock(m_mutex);
		return m_completed_result.is_completed();
	}

	virtual ks_raw_result peek_result() override {
		std::unique_lock<ks_mutex> lock(m_mutex);
		return m_completed_result;
	}

protected:
	virtual bool do_check_cancel() override {
		std::unique_lock<ks_mutex> lock(m_mutex, std::defer_lock);
		return this->do_check_cancel_locking(lock);
	}

	virtual ks_error do_acquire_cancel_error(const ks_error& def_error) override {
		std::unique_lock<ks_mutex> lock(m_mutex, std::defer_lock);
		return this->do_acquire_cancel_error_locking(def_error, lock);
	}

	bool do_check_cancel_locking(std::unique_lock<ks_mutex>& lock) {
		auto cancel_error_code = m_cancel_error_code_v;
		if (cancel_error_code != 0) {
			return true;
		}

		if (m_living_context_controller_available_v) {
			if (!lock.owns_lock())
				lock.lock();
			if (m_living_context_controller_available_v && (m_living_context.__check_cancel_all_ctrl() || m_living_context.__check_owner_expired()))
				return true;
		}

		if (m_timeout_schedule_id != 0) {
			if (!lock.owns_lock())
				lock.lock();
			if (m_timeout_schedule_id != 0 && (m_timeout_time <= std::chrono::steady_clock::now()))
				return true;
		}

		return false;
	}

	ks_error do_acquire_cancel_error_locking(const ks_error& def_error, std::unique_lock<ks_mutex>& lock) {
		auto cancel_error_code = m_cancel_error_code_v;
		if (cancel_error_code != 0) {
			return ks_error::of(cancel_error_code);
		}

		if (m_living_context_controller_available_v) {
			if (!lock.owns_lock())
				lock.lock();
			if (m_living_context_controller_available_v && (m_living_context.__check_cancel_all_ctrl() || m_living_context.__check_owner_expired()))
				return ks_error::was_cancelled_error();
		}

		if (m_timeout_schedule_id != 0) {
			if (!lock.owns_lock())
				lock.lock();
			if (m_timeout_schedule_id != 0 && (m_timeout_time <= std::chrono::steady_clock::now()))
				return ks_error::was_timeout_error();
		}

		return def_error;
	}

	virtual bool do_wait() override {
#ifdef _DEBUG
		ks_apartment* cur_apartment = ks_apartment::current_thread_apartment();
		if (cur_apartment != nullptr && cur_apartment == m_spec_apartment) {
			//特别说明：
			//要求在不同套间之间才允许wait，以避免死锁。
			//但即便进行了如此检查，仍不能完全避免死锁，因为this的前序未完成的future仍可能要求在当前套间中执行，
			//若此套间的线程已全部处于wait状态，那么就没有机会去调度前序future了，自然也就永远等不到this的完成信号了（死锁）。
			//如果要完美支持wait，就要求apartment实现真正的协程机制，这是个大挑战，宜单独立项。
			//或者退而求其次采用一个流氓方案，在wait期间临时允许线程池扩容，不过这个办法并不适用于sta套间。
			//但目前我们没有做任何特别支持。
			ASSERT(false);
			return false;
		}
#endif

		std::unique_lock<ks_mutex> lock(m_mutex);
		while (!m_completed_result.is_completed())
			m_completed_result_cv.wait(lock);
		return true;
	}

	virtual ks_apartment* get_spec_apartment() override {
		return m_spec_apartment;
	}

protected:
	virtual void do_add_next(const ks_raw_future_ptr& next_future) override {
		std::unique_lock<ks_mutex> lock(m_mutex);
		return this->do_add_next_locked(next_future, lock);
	}

	virtual void do_add_next_multi(const std::vector<ks_raw_future_ptr>& next_futures) override {
		if (!next_futures.empty())
			return;

		std::unique_lock<ks_mutex> lock(m_mutex);
		return this->do_add_next_multi_locked(next_futures, lock);
	}

	virtual void do_complete(const ks_raw_result& result, ks_apartment* prefer_apartment, bool from_internal) override {
		std::unique_lock<ks_mutex> lock(m_mutex);
		return this->do_complete_locked<false>(result, prefer_apartment, from_internal, lock);
	}

	void do_add_next_locked(const ks_raw_future_ptr& next_future, std::unique_lock<ks_mutex>& lock) {
		if (!m_completed_result.is_completed()) {
			if (m_next_future_0 == nullptr)
				m_next_future_0 = next_future;
			else
				m_next_future_more.push_back(next_future);
		}
		else {
			ks_raw_result result = m_completed_result;
			ks_apartment* prefer_apartment = m_completed_prefer_apartment;

			uint64_t act_schedule_id = prefer_apartment->schedule([this, this_shared = this->shared_from_this(), next_future, result, prefer_apartment]() {
				next_future->on_feeded_by_prev(result, this, prefer_apartment);
			}, 0);
			if (act_schedule_id == 0) {
				lock.unlock();
				next_future->do_complete(ks_error::was_terminated_error(), prefer_apartment, false);
				lock.lock();
			}
		}
	}

	void do_add_next_multi_locked(const std::vector<ks_raw_future_ptr>& next_futures, std::unique_lock<ks_mutex>& lock) {
		if (next_futures.empty())
			return;

		if (!m_completed_result.is_completed()) {
			auto next_future_it = next_futures.cbegin();
			if (m_next_future_0 == nullptr) 
				m_next_future_0 = *next_future_it++;
			m_next_future_more.insert(m_next_future_more.end(), next_future_it, next_futures.cend());
		}
		else {
			ks_raw_result result = m_completed_result;
			ks_apartment* prefer_apartment = m_completed_prefer_apartment;

			uint64_t act_schedule_id = prefer_apartment->schedule([this, this_shared = this->shared_from_this(), next_futures, result, prefer_apartment]() {
				for (auto& next_future : next_futures)
					next_future->on_feeded_by_prev(result, this, prefer_apartment);
			}, 0);
			if (act_schedule_id == 0) {
				lock.unlock();
				for (auto& next_future : next_futures)
					next_future->do_complete(ks_error::was_terminated_error(), prefer_apartment, false);
				lock.lock();
			}
		}
	}

	template <bool must_keep_locked>
	void do_complete_locked(const ks_raw_result& result, ks_apartment* prefer_apartment, bool from_internal, std::unique_lock<ks_mutex>& lock) {
		static_assert(!must_keep_locked, "the must_keep_locked arg is expected false always, for optimization");
		ASSERT(result.is_completed());

		if (m_completed_result.is_completed()) 
			return; //repeat complete?

		m_pending_flag_v = false;

		if (prefer_apartment == nullptr)
			prefer_apartment = this->do_determine_prefer_apartment(nullptr);

		m_completed_result = result.require_completed_or_error();
		m_completed_prefer_apartment = prefer_apartment;
		m_completed_result_cv.notify_all();

		if (m_timeout_schedule_id != 0) {
			uint64_t timeout_schedule_id = m_timeout_schedule_id;
			m_timeout_schedule_id = 0;
			m_timeout_time = {};

			ks_apartment* timeout_apartment = this->do_determine_timeout_apartment();
			timeout_apartment->try_unschedule(timeout_schedule_id);
		}

		this->do_reset_extra_data_locked(lock);

		m_living_context = {}; //clear
		m_living_context_controller_available_v = false;

		if (m_next_future_0 != nullptr || !m_next_future_more.empty()) {
			ks_raw_future_ptr t_next_future_0;
			std::vector<ks_raw_future_ptr> t_next_future_more;
			t_next_future_0.swap(m_next_future_0);
			t_next_future_more.swap(m_next_future_more);
			m_next_future_0 = nullptr;
			m_next_future_more.clear();

			if (from_internal) {
				ASSERT(lock.owns_lock());
				lock.unlock(); //内部流程无需解锁（除非外部不合理乱用0x10000优先级）
				if (t_next_future_0 != nullptr)
					t_next_future_0->on_feeded_by_prev(result, this, prefer_apartment);
				for (auto& next_future : t_next_future_more)
					next_future->on_feeded_by_prev(result, this, prefer_apartment);
				if (must_keep_locked)
					lock.lock();
			}
			else {
				uint64_t act_schedule_id = prefer_apartment->schedule(
					[this, this_shared = this->shared_from_this(), 
					t_next_future_0, t_next_future_more,  //因为失败时还需要处理，所以不可以右值引用传递
					//t_next_future_0 = std::move(t_next_future_0), t_next_future_more = std::move(t_next_future_more),
					result = m_completed_result, prefer_apartment]() {
						if (t_next_future_0 != nullptr)
							t_next_future_0->on_feeded_by_prev(result, this, prefer_apartment);
						for (auto& next_future : t_next_future_more)
							next_future->on_feeded_by_prev(result, this, prefer_apartment);
				}, 0);

				if (act_schedule_id == 0) {
					ASSERT(lock.owns_lock());
					lock.unlock();
					if (t_next_future_0 != nullptr)
						t_next_future_0->on_feeded_by_prev(ks_error::was_terminated_error(), this, prefer_apartment);
					for (auto& next_future : t_next_future_more)
						next_future->on_feeded_by_prev(ks_error::was_terminated_error(), this, prefer_apartment);
					if (must_keep_locked)
						lock.lock();
				}
			}
		}
	}

	virtual void do_reset_extra_data_locked(std::unique_lock<ks_mutex>& lock) {}

	virtual void do_set_timeout(int64_t timeout, const ks_error& error, bool backtrack) override {
		std::unique_lock<ks_mutex> lock(m_mutex);
		if (m_completed_result.is_completed())
			return;

		if (m_timeout_schedule_id != 0) {
			ks_apartment* timeout_apartment = this->do_determine_timeout_apartment();
			timeout_apartment->try_unschedule(m_timeout_schedule_id);
			m_timeout_schedule_id = 0;
			m_timeout_time = {};
		}

		if (timeout <= 0) 
			return; //infinity

		//not infinity
		m_timeout_time = m_create_time + std::chrono::milliseconds(timeout);

		const std::chrono::steady_clock::time_point now_time = std::chrono::steady_clock::now();
		const int64_t timeout_remain_ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_timeout_time - now_time).count();
		if (timeout_remain_ms <= 0) {
			if (!m_completed_result.is_completed()) {
				lock.unlock();
				this->do_try_cancel(error, backtrack);
			}
		}
		else {
			//schedule timeout
			ks_apartment* timeout_apartment = this->do_determine_timeout_apartment();
			m_timeout_schedule_id = timeout_apartment->schedule_delayed(
				[this, this_shared = this->shared_from_this(), schedule_id = m_timeout_schedule_id, error, backtrack]() -> void {
				std::unique_lock<ks_mutex> lock2(m_mutex);
				if (schedule_id != m_timeout_schedule_id)
					return;

				m_timeout_schedule_id = 0; //reset
				if (!m_pending_flag_v || m_completed_result.is_completed())
					return;

				lock2.unlock();
				this->do_try_cancel(error, backtrack); //will become timeout
			}, 0, timeout_remain_ms);
			if (m_timeout_schedule_id == 0) {
				ASSERT(false);
				return;
			}
		}
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override = 0;

protected:
	ks_apartment* do_determine_prefer_apartment(ks_apartment* advice_apartment) const {
		if (m_spec_apartment != nullptr)
			return m_spec_apartment;
		else if (advice_apartment != nullptr)
			return advice_apartment;
		else
			return ks_apartment::current_thread_apartment_or_default_mta();
	}

	ks_apartment* do_determine_timeout_apartment() const {
		return ks_apartment::default_mta();
	}

protected:
	//const ks_raw_future_mode m_mode;  //const-like  //被移动位置，使内存更紧凑
	//const bool m_cancelable;  //const-like  //被移动位置，使内存更紧凑
	const std::add_const_t<ks_apartment*> m_spec_apartment;  //const-like
	const std::chrono::steady_clock::time_point m_create_time;  //const-like

	ks_mutex m_mutex;

	ks_raw_result m_completed_result;
	ks_condition_variable m_completed_result_cv{};
	ks_apartment* m_completed_prefer_apartment = nullptr;

	ks_async_context m_living_context; //在complete后被自动清除
	//volatile bool m_living_context_controller_available_v = false;  //被移动位置，使内存更紧凑

	ks_raw_future_ptr m_next_future_0; //在complete后被自动清除
	std::vector<ks_raw_future_ptr> m_next_future_more; //在complete后被自动清除

	uint64_t m_timeout_schedule_id = 0;
	std::chrono::steady_clock::time_point m_timeout_time = {};

	//volatile HRESULT m_cancel_error_code_v = 0;  //被移动位置，使内存更紧凑
	//volatile bool m_pending_flag_v = true;  //被移动位置，使内存更紧凑

	//为了使内存布局更紧凑，将部分成员变量集中安置
	volatile HRESULT m_cancel_error_code_v = 0;
	const ks_raw_future_mode m_mode;  //const-like
	const bool m_cancelable;  //const-like
	volatile bool m_pending_flag_v = true;
	volatile bool m_living_context_controller_available_v = false;

	friend class ks_raw_future;
};


class ks_raw_promise_future final : public ks_raw_future_baseimp, public ks_raw_promise {
public:
	//注：默认apartment原设计使用current_thread_apartment，现已改为使用default_mta
	explicit ks_raw_promise_future(ks_raw_future_mode mode, ks_apartment* spec_apartment)
		: ks_raw_future_baseimp(mode, true, spec_apartment != nullptr ? spec_apartment : ks_apartment::default_mta(), ks_async_context()) {}

	_DISABLE_COPY_CONSTRUCTOR(ks_raw_promise_future);

public: //override ks_raw_promise's methods
	virtual ks_raw_future_ptr get_future() override {
		return this->shared_from_this();
	}

	virtual void resolve(const ks_raw_value& value) override {
		this->do_complete(value, nullptr, false);
	}

	virtual void reject(const ks_error& error) override {
		this->do_complete(error, nullptr, false);
	}

	virtual void try_complete(const ks_raw_result& result) override {
		if (result.is_completed())
			this->do_complete(result, nullptr, false);
		else
			ASSERT(false);
	}


protected:
	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) override {
		//ks_raw_promise_future的此方法不应被调用，而是应直接do_complete
		ASSERT(false);
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override {
		ASSERT(m_cancelable);

		std::unique_lock<ks_mutex> lock(m_mutex);
		m_cancel_error_code_v = error.get_code();
		this->do_complete_locked<false>(error, nullptr, false, lock);
	}

	virtual bool is_with_upstream_future() override {
		return false;
	}

};


class ks_raw_task_future final : public ks_raw_future_baseimp {
public:
	explicit ks_raw_task_future(ks_raw_future_mode mode, ks_apartment* spec_apartment, function<ks_raw_result()>&& task_fn, const ks_async_context& living_context, int64_t delay)
		: ks_raw_future_baseimp(mode, true, spec_apartment != nullptr ? spec_apartment : ks_apartment::default_mta(), living_context)
		, m_task_fn(std::move(task_fn)), m_delay(delay) {}

	_DISABLE_COPY_CONSTRUCTOR(ks_raw_task_future);

	void submit() {
		std::unique_lock<ks_mutex> lock(m_mutex);
		int priority = m_living_context.__get_priority();
		ks_apartment* prefer_apartment = this->do_determine_prefer_apartment(nullptr);
		bool could_run_locally = m_mode == ks_raw_future_mode::TASK && priority >= 0x10000 && (m_spec_apartment == nullptr || m_spec_apartment == prefer_apartment);

		//pending_schedule_fn不对context进行捕获。
		//这样做的意图是：对于delayed任务，当try_cancel时，即使apartment::try_unschedule失败，也不影响context的及时释放。
		function<void()> pending_schedule_fn = [this, this_shared = this->shared_from_this(), prefer_apartment, context = m_living_context]() mutable -> void {
			std::unique_lock<ks_mutex> lock2(m_mutex);
			m_pending_schedule_id = 0; //这个变量第一时间被清0
			if (m_completed_result.is_completed())
				return; //pre-check cancelled

			m_pending_flag_v = false;

			ks_raw_running_future_rtstt running_future_rtstt;
			ks_raw_living_context_rtstt living_context_rtstt;
			running_future_rtstt.apply(this, &tls_current_thread_running_future);
			living_context_rtstt.apply(context);

			ks_raw_result result;
			try {
				if (this->do_check_cancel_locking(lock2))
					result = this->do_acquire_cancel_error_locking(ks_error::was_cancelled_error(), lock2);
				else {
					std::function<ks_raw_result()> task_fn = std::move(m_task_fn);
					lock2.unlock();
					result = task_fn().require_completed_or_error();
					lock2.lock();
					task_fn = {};
				}
			}
			catch (ks_error error) {
				result = error;
			}
			catch (...) {
				ASSERT(false);
				result = ks_error::unexpected_error();
			}

			this->do_complete_locked<false>(result, prefer_apartment, true, lock2);

			running_future_rtstt.try_unapply();
			living_context_rtstt.try_unapply();
		};

		if (could_run_locally) {
			lock.unlock();
			pending_schedule_fn(); //超高优先级、且spec_partment为nullptr，则立即执行，省掉schedule过程
		}
		else {
			m_pending_schedule_id = (m_mode == ks_raw_future_mode::TASK)
				? prefer_apartment->schedule(std::move(pending_schedule_fn), priority)
				: prefer_apartment->schedule_delayed(std::move(pending_schedule_fn), priority, m_delay);
			if (m_pending_schedule_id == 0) {
				//schedule失败，则立即将this标记为错误即可
				this->do_complete_locked<false>(ks_error::was_terminated_error(), nullptr, false, lock);
				return;
			}
		}
	}

protected:
	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) override {
		//ks_raw_promise_future的此方法不应被调用，而是应直接do_complete
		ASSERT(false);
	}

	virtual void do_reset_extra_data_locked(std::unique_lock<ks_mutex>& lock) override {
		m_task_fn = {};
		m_pending_schedule_id = 0;
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override {
		ASSERT(m_cancelable);

		std::unique_lock<ks_mutex> lock(m_mutex);
		m_cancel_error_code_v = error.get_code();

		//若为延时任务，则执行unschedule，task-future结果状态将成为rejected
		const bool is_delaying_schedule_task = (m_mode == ks_raw_future_mode::TASK_DELAYED && m_delay >= 0);
		if (is_delaying_schedule_task) {
			if (m_pending_schedule_id != 0) {
				m_spec_apartment->try_unschedule(m_pending_schedule_id);
				m_pending_schedule_id = 0;
			}

			this->do_complete_locked<false>(error, nullptr, false, lock);
		}
	}

	virtual bool is_with_upstream_future() override {
		return false;
	}

private:
	function<ks_raw_result()> m_task_fn; //在complete后被自动清除
	const int64_t m_delay;  //const-like
	uint64_t m_pending_schedule_id = 0;
};


class ks_raw_pipe_future final : public ks_raw_future_baseimp {
public:
	explicit ks_raw_pipe_future(ks_raw_future_mode mode, ks_apartment* spec_apartment, function<ks_raw_result(const ks_raw_result&)>&& fn_ex, const ks_async_context& living_context, bool cancelable)
		: ks_raw_future_baseimp(mode, cancelable, spec_apartment, living_context), m_fn_ex(std::move(fn_ex)) {}

	_DISABLE_COPY_CONSTRUCTOR(ks_raw_pipe_future);

	void connect(const ks_raw_future_ptr& prev_future) {
		if (m_spec_apartment == nullptr) 
			const_cast<ks_apartment*&>(m_spec_apartment) = prev_future->get_spec_apartment();

		m_prev_future_weak = prev_future;
		prev_future->do_add_next(this->shared_from_this());
	}

protected:
	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) override {
		ASSERT(prev_result.is_completed());

		std::unique_lock<ks_mutex> lock(m_mutex);
		if (m_completed_result.is_completed())
			return; 

		bool could_skip_run = false;
		switch (m_mode) {
		case ks_raw_future_mode::THEN:
			could_skip_run = !prev_result.is_value();
			break;
		case ks_raw_future_mode::TRAP:
			could_skip_run = !prev_result.is_error();
			break;
		case ks_raw_future_mode::TRANSFORM:
			could_skip_run = false;
			break;
		case ks_raw_future_mode::FORWARD:
			could_skip_run = true;
			break;
		default:
			ASSERT(false);
			break;
		}

		if (could_skip_run) {
			//可直接skip-run，则立即将this进行settle即可
			ks_apartment* prefer_apartment = this->do_determine_prefer_apartment(prev_advice_apartment);
			this->do_complete_locked<false>(prev_result, prefer_apartment, true, lock);
			return;
		}

		if (m_cancelable && this->do_check_cancel_locking(lock)) {
			//若this已被cancel，则立即将this进行settle即可
			ks_apartment* prefer_apartment = this->do_determine_prefer_apartment(prev_advice_apartment);
			this->do_complete_locked<false>(
				prev_result.is_error() ? prev_result.to_error() : this->do_acquire_cancel_error_locking(ks_error::was_cancelled_error(), lock),
				prefer_apartment, true, lock);
			return;
		}

		int priority = m_living_context.__get_priority();
		ks_apartment* prefer_apartment = this->do_determine_prefer_apartment(prev_advice_apartment);
		bool could_run_locally = priority >= 0x10000 && (m_spec_apartment == nullptr || m_spec_apartment == prefer_apartment);

		function<void()> run_fn = [this, this_shared = this->shared_from_this(), prev_result, prefer_apartment, context = m_living_context]() mutable -> void {
			std::unique_lock<ks_mutex> lock2(m_mutex);
			if (m_completed_result.is_completed())
				return; //pre-check cancelled

			m_pending_flag_v = false;

			ks_raw_running_future_rtstt running_future_rtstt;
			ks_raw_living_context_rtstt living_context_rtstt;
			running_future_rtstt.apply(this, &tls_current_thread_running_future);
			living_context_rtstt.apply(context);

			ks_raw_result result;
			try {
				if (m_cancelable && this->do_check_cancel_locking(lock2))
					result = prev_result.is_error() ? prev_result.to_error() : this->do_acquire_cancel_error_locking(ks_error::was_cancelled_error(), lock2);
				else {
					function<ks_raw_result(const ks_raw_result&)> fn_ex = std::move(m_fn_ex);
					lock2.unlock();
					result = fn_ex(prev_result).require_completed_or_error();
					lock2.lock();
					fn_ex = {};
				}
			}
			catch (ks_error error) {
				result = error;
			}
			catch (...) {
				ASSERT(false);
				result = ks_error::unexpected_error();
			}

			this->do_complete_locked<false>(result, prefer_apartment, true, lock2);

			running_future_rtstt.try_unapply();
			living_context_rtstt.try_unapply();
		};

		if (could_run_locally) {
			lock.unlock();
			run_fn(); //超高优先级、且spec_partment为nullptr，则立即执行，省掉schedule过程
			return;
		}

		uint64_t act_schedule_id = prefer_apartment->schedule(std::move(run_fn), priority);
		if (act_schedule_id == 0) {
			//schedule失败，则立即将this标记为错误即可
			this->do_complete_locked<false>(ks_error::was_terminated_error(), prefer_apartment, true, lock);
			return;
		}
	}

	virtual void do_reset_extra_data_locked(std::unique_lock<ks_mutex>& lock) override {
		m_fn_ex = {};
		m_prev_future_weak.reset();
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override {
		std::unique_lock<ks_mutex> lock(m_mutex);
		if (m_cancelable) {
			m_cancel_error_code_v = error.get_code();
			//pipe-future无需主动标记结果
		}

		if (backtrack) {
			ks_raw_future_ptr prev_future = m_prev_future_weak.lock();
			m_prev_future_weak.reset();

			lock.unlock();
			if (prev_future != nullptr) 
				prev_future->do_try_cancel(error, true);
		}
	}

	virtual bool is_with_upstream_future() override {
		return true;
	}

private:
	function<ks_raw_result(const ks_raw_result&)> m_fn_ex;  //在complete后被自动清除
	std::weak_ptr<ks_raw_future> m_prev_future_weak;    //在complete后被自动清除
};


class ks_raw_flatten_future final : public ks_raw_future_baseimp {
public:
	explicit ks_raw_flatten_future(ks_raw_future_mode mode, ks_apartment* spec_apartment, function<ks_raw_future_ptr(const ks_raw_result&)>&& afn_ex, const ks_async_context& living_context)
		: ks_raw_future_baseimp(mode, true, spec_apartment, living_context), m_afn_ex(afn_ex) {}

	void connect(const ks_raw_future_ptr& prev_future) {
		if (m_spec_apartment == nullptr)
			const_cast<ks_apartment*&>(m_spec_apartment) = prev_future->get_spec_apartment();

		std::unique_lock<ks_mutex> lock(m_mutex);

		auto this_shared = this->shared_from_this();
		auto context = m_living_context;

		m_medium_future = prev_future->on_completion(
			[this, this_shared, context](const ks_raw_result& prev_result) mutable -> void {
			std::unique_lock<ks_mutex> lock2(m_mutex);
			if (m_completed_result.is_completed()) 
				return;

			m_pending_flag_v = false;

			ks_raw_running_future_rtstt running_future_rtstt;
			ks_raw_living_context_rtstt living_context_rtstt;
			running_future_rtstt.apply(this, &tls_current_thread_running_future);
			living_context_rtstt.apply(context);

			ks_apartment* prefer_apartment = this->do_determine_prefer_apartment(nullptr);

			ks_raw_future_ptr extern_future = nullptr;
			ks_error else_error = {};
			try {
				if (this->do_check_cancel_locking(lock2))
					else_error = prev_result.is_error() ? prev_result.to_error() : this->do_acquire_cancel_error_locking(ks_error::was_cancelled_error(), lock2);
				else {
					function<ks_raw_future_ptr(const ks_raw_result&)> afn_ex = std::move(m_afn_ex);
					lock2.unlock();
					extern_future = afn_ex(prev_result);
					if (extern_future == nullptr)
						else_error = ks_error::unexpected_error();
					lock2.lock();
					afn_ex = {};
				}
			}
			catch (ks_error error) {
				else_error = error;
			}
			catch (...) {
				ASSERT(false);
				else_error = ks_error::unexpected_error();
			}

			if (extern_future != nullptr) {
				m_extern_future = extern_future;
				m_extern_future->on_completion([this, this_shared, context, prefer_apartment](const ks_raw_result& extern_result) {
					this->do_complete(extern_result, prefer_apartment, false);
				}, context, prefer_apartment);
			}
			else {
				this->do_complete_locked<false>(else_error, prefer_apartment, false, lock2);
			}

			running_future_rtstt.try_unapply();
			living_context_rtstt.try_unapply();
		}, context, m_spec_apartment);
	}

protected:
	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) override {
		//ks_raw_promise_future的此方法不应被调用，而是应直接do_complete
		ASSERT(false);
	}

	virtual void do_reset_extra_data_locked(std::unique_lock<ks_mutex>& lock) override {
		m_afn_ex = {};
		m_medium_future = nullptr;
		m_extern_future = nullptr;
	}

	virtual void do_set_timeout(int64_t timeout, const ks_error& error, bool backtrack) override {
		std::unique_lock<ks_mutex> lock(m_mutex);
		ks_raw_future_ptr medium_future = m_medium_future;
		ks_raw_future_ptr extern_future = m_extern_future;
		lock.unlock();

		if (medium_future != nullptr) {
			medium_future->do_set_timeout(timeout, error, backtrack);
		}
		if (extern_future != nullptr) {
			extern_future->set_timeout(timeout, backtrack);
		}
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override {
		ASSERT(m_cancelable);

		std::unique_lock<ks_mutex> lock(m_mutex);
		m_cancel_error_code_v = error.get_code();
		ks_raw_future_ptr medium_future = m_medium_future;
		ks_raw_future_ptr extern_future = m_extern_future;
		lock.unlock();

		if (medium_future != nullptr) {
			medium_future->do_try_cancel(error, backtrack);
		}
		if (extern_future != nullptr) {
			extern_future->try_cancel(backtrack);
		}
	}

	virtual bool is_with_upstream_future() override {
		return true;
	}

private:
	function<ks_raw_future_ptr(const ks_raw_result&)> m_afn_ex;
	std::shared_ptr<ks_raw_future> m_medium_future;
	std::shared_ptr<ks_raw_future> m_extern_future;
};



class ks_raw_aggr_future final : public ks_raw_future_baseimp {
public:
	explicit ks_raw_aggr_future(ks_raw_future_mode mode, ks_apartment* spec_apartment) 
		: ks_raw_future_baseimp(mode, true, spec_apartment, ks_async_context()) {}

	_DISABLE_COPY_CONSTRUCTOR(ks_raw_aggr_future);

	void connect(const std::vector<ks_raw_future_ptr>& prev_futures) {
		m_prev_future_weak_seq.reserve(prev_futures.size());
		m_prev_future_raw_pointer_seq.reserve(prev_futures.size());
		for (auto& prev_future : prev_futures) {
			m_prev_future_weak_seq.push_back(prev_future);
			m_prev_future_raw_pointer_seq.push_back(prev_future.get());
		}
		m_prev_total_count = prev_futures.size();
		m_prev_completed_count = 0;

		m_prev_result_seq_cache.resize(prev_futures.size(), ks_raw_result());
		m_prev_prefer_apartment_seq_cache.resize(prev_futures.size(), nullptr);
		m_prev_first_resolved_index = -1;
		m_prev_first_rejected_index = -1;

		ks_raw_future_ptr this_shared = this->shared_from_this();
		for (auto& prev_future : prev_futures) {
			prev_future->do_add_next(this_shared);
		}
	}

protected:
	virtual void on_feeded_by_prev(const ks_raw_result& prev_result, ks_raw_future* prev_future, ks_apartment* prev_advice_apartment) override {
		ASSERT(prev_result.is_completed());

		std::unique_lock<ks_mutex> lock(m_mutex);
		if (m_completed_result.is_completed())
			return;

		size_t prev_index_just = -1;
		ASSERT(m_prev_completed_count < m_prev_total_count);
		while (true) { //此处while为了支持future重复出现
			auto prev_future_iter = std::find(m_prev_future_raw_pointer_seq.cbegin(), m_prev_future_raw_pointer_seq.cend(), prev_future);
			if (prev_future_iter == m_prev_future_raw_pointer_seq.cend())
				break; //miss prev_future (unexpected)
			size_t prev_index = prev_future_iter - m_prev_future_raw_pointer_seq.cbegin();
			if (m_prev_result_seq_cache[prev_index].is_completed())
				break; //the prev_future has been completed (unexpected)
			if (prev_index_just == -1)
				prev_index_just = prev_index;

			m_prev_future_raw_pointer_seq[prev_index] = nullptr;
			m_prev_result_seq_cache[prev_index] = prev_result.require_completed_or_error();
			m_prev_prefer_apartment_seq_cache[prev_index] = prev_advice_apartment;
			m_prev_completed_count++;
			if (m_prev_first_resolved_index == -1 && prev_result.is_value())
				m_prev_first_resolved_index = prev_index;
			if (m_prev_first_rejected_index == -1 && !prev_result.is_value())
				m_prev_first_rejected_index = prev_index;
		}

		if (prev_index_just == -1) {
			ASSERT(false);
			return;
		}

		do_check_and_try_settle_me_locked<false>(prev_result, prev_advice_apartment, lock);
	}

	virtual void do_reset_extra_data_locked(std::unique_lock<ks_mutex>& lock) override {
		m_prev_future_weak_seq.clear();
		m_prev_future_raw_pointer_seq.clear();
		m_prev_result_seq_cache.clear();
		m_prev_prefer_apartment_seq_cache.clear();
	}

	virtual void do_try_cancel(const ks_error& error, bool backtrack) override {
		ASSERT(m_cancelable);

		std::unique_lock<ks_mutex> lock(m_mutex);
		m_cancel_error_code_v = error.get_code();
		//aggr-future无需主动标记结果

		if (backtrack) {
			std::vector<ks_raw_future_ptr> prev_future_seq;
			prev_future_seq.reserve(m_prev_future_weak_seq.size());
			for (auto& prev_future_weak : m_prev_future_weak_seq) {
				ks_raw_future_ptr prev_fut = prev_future_weak.lock();
				if (prev_fut != nullptr)
					prev_future_seq.push_back(prev_fut);
			}
			m_prev_future_weak_seq.clear();

			lock.unlock();
			for (auto& prev_fut : prev_future_seq) 
				prev_fut->do_try_cancel(error, true);
		}
	}

	virtual bool is_with_upstream_future() override {
		return true;
	}

private:
	template <bool must_keep_locked>
	void do_check_and_try_settle_me_locked(const ks_raw_result& prev_result, ks_apartment* prev_advice_apartment, std::unique_lock<ks_mutex>& lock) {
		static_assert(!must_keep_locked, "the must_keep_locked arg is expected false always, for optimization");

		//check and try settle me ...
		switch (m_mode) {
		case ks_raw_future_mode::ALL:
			if (m_prev_first_rejected_index != -1) {
				//前序任务出现失败
				ks_apartment* prefer_apartment = this->do_determine_prefer_apartment_from_prev_seq_locked(prev_advice_apartment, lock);
				ks_error prev_error_first = m_prev_result_seq_cache[m_prev_first_rejected_index].to_error();
				this->do_complete_locked<must_keep_locked>(prev_error_first, prefer_apartment, true, lock);
				return;
			}
			else if (m_prev_completed_count == m_prev_total_count) {
				//前序任务全部成功
				ks_apartment* prefer_apartment = this->do_determine_prefer_apartment_from_prev_seq_locked(prev_advice_apartment, lock);
				std::vector<ks_raw_value> prev_value_seq;
				prev_value_seq.reserve(m_prev_result_seq_cache.size());
				for (auto& prev_result : m_prev_result_seq_cache)
					prev_value_seq.push_back(prev_result.to_value());
				this->do_complete_locked<must_keep_locked>(ks_raw_value::of(std::move(prev_value_seq)), prefer_apartment, true, lock);
				return;
			}
			else {
				break;
			}

		case ks_raw_future_mode::ALL_COMPLETED:
			if (m_prev_completed_count == m_prev_total_count) {
				//前序任务全部完成（无论成功/失败）
				ks_apartment* prefer_apartment = this->do_determine_prefer_apartment_from_prev_seq_locked(prev_advice_apartment, lock);
				std::vector<ks_raw_result> prev_result_seq = m_prev_result_seq_cache;
				this->do_complete_locked<must_keep_locked>(ks_raw_value::of(std::move(prev_result_seq)), prefer_apartment, true, lock);
				return;
			}
			else {
				break;
			}

		case ks_raw_future_mode::ANY:
			if (m_prev_first_resolved_index != -1) {
				//前序任务出现成功
				ks_apartment* prefer_apartment = this->do_determine_prefer_apartment_from_prev_seq_locked(prev_advice_apartment, lock);
				ks_raw_value prev_value_first = m_prev_result_seq_cache[m_prev_first_resolved_index].to_value();
				this->do_complete_locked<must_keep_locked>(prev_value_first, prefer_apartment, true, lock);
				return;
			}
			else if (m_prev_completed_count == m_prev_total_count) {
				//前序任务全部失败
				ASSERT(m_prev_first_rejected_index != -1);
				ks_apartment* prefer_apartment = this->do_determine_prefer_apartment_from_prev_seq_locked(prev_advice_apartment, lock);
				ks_error prev_error_first = m_prev_result_seq_cache[m_prev_first_rejected_index].to_error();
				this->do_complete_locked<must_keep_locked>(prev_error_first, prefer_apartment, true, lock);
				return;
			}
			else {
				break;
			}

		default:
			ASSERT(false);
			break;
		}

		if (this->do_check_cancel_locking(lock)) {
			//还有前序future仍未完成，但若this已被cancel，则立即将this进行settle即可，不再继续等待
			ks_apartment* prefer_apartment = this->do_determine_prefer_apartment_from_prev_seq_locked(prev_advice_apartment, lock);
			this->do_complete_locked<must_keep_locked>(
				prev_result.is_error() ? prev_result.to_error() : this->do_acquire_cancel_error_locking(ks_error::was_cancelled_error(), lock),
				prefer_apartment, true, lock);
			return;
		}
	}

	ks_apartment* do_determine_prefer_apartment_from_prev_seq_locked(ks_apartment* prev_advice_apartment, std::unique_lock<ks_mutex>& lock) const {
		if (m_spec_apartment != nullptr)
			return m_spec_apartment;

		if (prev_advice_apartment != nullptr)
			return prev_advice_apartment;

		for (auto* prev_prefer_apartment : m_prev_prefer_apartment_seq_cache) {
			if (prev_prefer_apartment != nullptr)
				return prev_prefer_apartment;
		}

		return ks_apartment::current_thread_apartment_or_default_mta();
	}

private:
	std::vector<std::weak_ptr<ks_raw_future>> m_prev_future_weak_seq;       //在触发后被自动清除
	std::vector<ks_raw_future*> m_prev_future_raw_pointer_seq;              //在触发后被自动清除
	size_t m_prev_total_count = 0;
	size_t m_prev_completed_count = 0;

	std::vector<ks_raw_result> m_prev_result_seq_cache;                     //在触发后被自动清除
	std::vector<ks_apartment*> m_prev_prefer_apartment_seq_cache; //在触发后被自动清除
	size_t m_prev_first_resolved_index = -1; //在触发后被自动清除
	size_t m_prev_first_rejected_index = -1; //在触发后被自动清除
};


//ks_raw_future静态方法实现
ks_raw_future_ptr ks_raw_future::resolved(const ks_raw_value& value, ks_apartment* apartment) {
	auto promise_future = std::make_shared<ks_raw_promise_future>(ks_raw_future_mode::DX, apartment);
	promise_future->do_complete(value, nullptr, false);
	return promise_future;
}

ks_raw_future_ptr ks_raw_future::rejected(const ks_error& error, ks_apartment* apartment) {
	auto promise_future = std::make_shared<ks_raw_promise_future>(ks_raw_future_mode::DX, apartment);
	promise_future->do_complete(error, nullptr, false);
	return promise_future;
}

ks_raw_future_ptr ks_raw_future::post(function<ks_raw_result()>&& task_fn, const ks_async_context& context, ks_apartment* apartment) {
	auto task_future = std::make_shared<ks_raw_task_future>(ks_raw_future_mode::TASK, apartment, std::move(task_fn), context, 0);
	task_future->submit();
	return task_future;
}

ks_raw_future_ptr ks_raw_future::post_delayed(function<ks_raw_result()>&& task_fn, const ks_async_context& context, ks_apartment* apartment, int64_t delay) {
	auto task_future = std::make_shared<ks_raw_task_future>(ks_raw_future_mode::TASK_DELAYED, apartment, std::move(task_fn), context, delay);
	task_future->submit();
	return task_future;
}


ks_raw_future_ptr ks_raw_future::all(const std::vector<ks_raw_future_ptr>& futures, ks_apartment* apartment) {
	if (futures.empty())
		return ks_raw_future::resolved(ks_raw_value::of(std::vector<ks_raw_value>()), apartment);

	auto aggr_future = std::make_shared<ks_raw_aggr_future>(ks_raw_future_mode::ALL, apartment);
	aggr_future->connect(futures);
	return aggr_future;
}

ks_raw_future_ptr ks_raw_future::all_completed(const std::vector<ks_raw_future_ptr>& futures, ks_apartment* apartment) {
	if (futures.empty())
		return ks_raw_future::resolved(ks_raw_value::of(std::vector<ks_raw_result>()), apartment);

	auto aggr_future = std::make_shared<ks_raw_aggr_future>(ks_raw_future_mode::ALL_COMPLETED, apartment);
	aggr_future->connect(futures);
	return aggr_future;
}

ks_raw_future_ptr ks_raw_future::any(const std::vector<ks_raw_future_ptr>& futures, ks_apartment* apartment) {
	if (futures.empty())
		return ks_raw_future::rejected(ks_error::unexpected_error(), apartment);
	if (futures.size() == 1)
		return futures.at(0);

	auto aggr_future = std::make_shared<ks_raw_aggr_future>(ks_raw_future_mode::ANY, apartment);
	aggr_future->connect(futures);
	return aggr_future;
}

void ks_raw_future::try_cancel(bool backtrack) {
	this->do_try_cancel(ks_error::was_cancelled_error(), backtrack); 
}

bool ks_raw_future::check_current_future_cancel(bool with_extra) {
	ks_raw_future* cur_future = tls_current_thread_running_future;
	if (cur_future != nullptr) {
		if (cur_future->do_check_cancel())
			return true;
		if (with_extra) {
			ks_apartment* cur_apartment = cur_future->get_spec_apartment();
			if (cur_apartment != nullptr) {
				if (cur_apartment->is_stopping_or_stopped())
					return true;
			}
		}
	}
	return false;
}

ks_error ks_raw_future::get_current_future_cancel_error(bool with_extra) {
	ks_raw_future* cur_future = tls_current_thread_running_future;
	if (cur_future != nullptr) {
		ks_error error = cur_future->do_acquire_cancel_error(ks_error());
		if (error.get_code() != 0)
			return error;
		if (with_extra) {
			ks_apartment* cur_apartment = cur_future->get_spec_apartment();
			if (cur_apartment != nullptr) {
				if (cur_apartment->is_stopping_or_stopped())
					return ks_error::was_terminated_error();
			}
		}
	}
	return ks_error();
}

void ks_raw_future::set_timeout(int64_t timeout, bool backtrack) {
	this->do_set_timeout(timeout, ks_error::was_timeout_error(), backtrack); 
}

ks_raw_promise_ptr ks_raw_promise::create(ks_apartment* apartment) {
	return std::make_shared<ks_raw_promise_future>(ks_raw_future_mode::PROMISE, apartment);
}



//ks_raw_future基础pipe方法实现
ks_raw_future_ptr ks_raw_future_baseimp::then(function<ks_raw_result(const ks_raw_value &)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	function<ks_raw_result(const ks_raw_result&)> fn_ex = [fn = std::move(fn)](const ks_raw_result& input)->ks_raw_result {
		if (input.is_value())
			return fn(input.to_value());
		else
			return input;
	};

	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::THEN, apartment, std::move(fn_ex), context, true);
	pipe_future->connect(this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::trap(function<ks_raw_result(const ks_error &)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	function<ks_raw_result(const ks_raw_result&)> fn_ex = [fn = std::move(fn)](const ks_raw_result& input)->ks_raw_result {
		if (input.is_error())
			return fn(input.to_error());
		else
			return input;
	};

	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::TRAP, apartment, std::move(fn_ex), context, true);
	pipe_future->connect(this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::transform(function<ks_raw_result(const ks_raw_result &)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::TRANSFORM, apartment, std::move(fn), context, true);
	pipe_future->connect(this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::flat_then(function<ks_raw_future_ptr(const ks_raw_value&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	function<ks_raw_future_ptr(const ks_raw_result&)> afn_ex = [fn = std::move(fn), apartment](const ks_raw_result& input)->ks_raw_future_ptr {
		if (!input.is_value())
			return ks_raw_future::rejected(input.to_error(), apartment);

		ks_raw_future_ptr extern_future = fn(input.to_value());
		ASSERT(extern_future != nullptr);
		return extern_future;
	};

	auto flatten_future = std::make_shared<ks_raw_flatten_future>(ks_raw_future_mode::FLATTEN_THEN, apartment, std::move(afn_ex), context);
	flatten_future->connect(this->shared_from_this());
	return flatten_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::flat_trap(function<ks_raw_future_ptr(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	function<ks_raw_future_ptr(const ks_raw_result&)> afn_ex = [fn = std::move(fn), apartment](const ks_raw_result& input)->ks_raw_future_ptr {
		if (!input.is_error())
			return ks_raw_future::resolved(input.to_value(), apartment);

		ks_raw_future_ptr extern_future = fn(input.to_error());
		ASSERT(extern_future != nullptr);
		return extern_future;
	};

	auto flatten_future = std::make_shared<ks_raw_flatten_future>(ks_raw_future_mode::FLATTEN_TRAP, apartment, std::move(afn_ex), context);
	flatten_future->connect(this->shared_from_this());
	return flatten_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::flat_transform(function<ks_raw_future_ptr(const ks_raw_result&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	auto flatten_future = std::make_shared<ks_raw_flatten_future>(ks_raw_future_mode::FLATTEN_TRANSFORM, apartment, std::move(fn), context);
	flatten_future->connect(this->shared_from_this());
	return flatten_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::on_success(function<void(const ks_raw_value&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	auto fn_ex = [fn = std::move(fn)](const ks_raw_result& input)->ks_raw_result {
		if (input.is_value())
			fn(input.to_value());
		return input;
	};

	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::THEN, apartment, std::move(fn_ex), context, false);
	pipe_future->connect(this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::on_failure(function<void(const ks_error&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	auto fn_ex = [fn = std::move(fn)](const ks_raw_result& input)->ks_raw_result {
		if (input.is_error())
			fn(input.to_error());
		return input;
	};

	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::TRAP, apartment, std::move(fn_ex), context, false);
	pipe_future->connect(this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::on_completion(function<void(const ks_raw_result&)>&& fn, const ks_async_context& context, ks_apartment* apartment) {
	auto fn_ex = [fn = std::move(fn)](const ks_raw_result& input)->ks_raw_result {
		fn(input);
		return input;
	};

	auto pipe_future = std::make_shared<ks_raw_pipe_future>(ks_raw_future_mode::TRANSFORM, apartment, std::move(fn_ex), context, false);
	pipe_future->connect(this->shared_from_this());
	return pipe_future;
}

ks_raw_future_ptr ks_raw_future_baseimp::noop(ks_apartment* apartment) {
	auto pipe_future = std::make_shared<
		ks_raw_pipe_future>(
			ks_raw_future_mode::FORWARD,
			apartment,
			[](auto& input) { return input; },
			ks_async_context().set_priority(0x10000),
			false);
	pipe_future->connect(this->shared_from_this());
	return pipe_future;
}


__KS_ASYNC_RAW_END
