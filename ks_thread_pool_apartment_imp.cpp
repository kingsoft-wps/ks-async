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

#include "ks_thread_pool_apartment_imp.h"
#include "ktl/ks_defer.h"
#include <thread>
#include <algorithm>
#include <sstream>

void __forcelink_to_ks_thread_pool_apartment_imp_cpp() {}

static std::atomic<uint64_t> g_last_fn_id{ 0 };

static thread_local size_t tls_current_thread_index_plus = 0;
static thread_local int  tls_current_thread_pump_loop_depth = 0;
static thread_local bool tls_current_thread_pump_loop_busy_for_idle_flag = false;


ks_thread_pool_apartment_imp::ks_thread_pool_apartment_imp(const char* name, size_t max_thread_count, uint flags) 
	: ks_thread_pool_apartment_imp(name, max_thread_count, flags, nullptr, nullptr) {
}

ks_thread_pool_apartment_imp::ks_thread_pool_apartment_imp(const char* name, size_t max_thread_count, uint flags, std::function<void()>&& thread_init_fn, std::function<void()>&& thread_term_fn) 
	: m_d(std::make_shared<_THREAD_POOL_APARTMENT_DATA>()) {
	ASSERT(name != nullptr);
	ASSERT(max_thread_count >= 1);

	m_d->name = name != nullptr ? name : "";
	m_d->max_thread_count = max_thread_count >= 1 ? max_thread_count : 1;
	m_d->flags = flags;
	m_d->thread_init_fn = std::move(thread_init_fn);
	m_d->thread_term_fn = std::move(thread_term_fn);

	if ((m_d->flags & auto_register_flag) && !m_d->name.empty()) {
		ks_apartment::__register_public_apartment(m_d->name.c_str(), this);
	}
}

ks_thread_pool_apartment_imp::~ks_thread_pool_apartment_imp() {
	ASSERT(m_d->state_v == _STATE::NOT_START || m_d->state_v == _STATE::STOPPED);
	if (m_d->state_v != _STATE::STOPPED) {
		std::unique_lock<std::mutex> lock(m_d->mutex);
		this->_try_stop_locked(true, lock, false);
		//this->wait();  //这里不等了，因为在进程退出时导致自动析构的话，可能时机就太晚，work线程已经被杀了
	}

	if ((m_d->flags & endless_instance_flag) == 0) {
		if ((m_d->flags & auto_register_flag) && !m_d->name.empty()) {
			ks_apartment::__unregister_public_apartment(m_d->name.c_str(), this);
		}
	}
}


const char* ks_thread_pool_apartment_imp::name() {
	return m_d->name.c_str();
}

uint ks_thread_pool_apartment_imp::features() {
	return (m_d->max_thread_count == 1 ? sequential_feature : 0)
		 | atfork_aware_future | nested_pump_aware_future;
}

size_t ks_thread_pool_apartment_imp::concurrency() {
	return m_d->max_thread_count;
}


bool ks_thread_pool_apartment_imp::start() {
	std::unique_lock<ks_mutex> lock(m_d->mutex);
	if (m_d->state_v != _STATE::NOT_START && m_d->state_v != _STATE::RUNNING)
		return false;

	_try_start_locked(lock);

	return true;
}

void ks_thread_pool_apartment_imp::async_stop() {
	std::unique_lock<ks_mutex> lock(m_d->mutex);
	return _try_stop_locked(false, lock, false);
}

//注：目前的wait实现暂不支持并发重入
void ks_thread_pool_apartment_imp::wait() {
	ASSERT(this != ks_apartment::current_thread_apartment());

	std::unique_lock<ks_mutex> lock(m_d->mutex);
	this->_try_stop_locked(true, lock, true); //ensure stop

	ASSERT(m_d->state_v == _STATE::STOPPING || m_d->state_v == _STATE::STOPPED);
	while (m_d->state_v == _STATE::STOPPING) {
		m_d->stopped_state_cv.wait(lock);
	}

	ASSERT(m_d->state_v == _STATE::STOPPED);
	ASSERT(m_d->now_fn_queue_prior.empty() && m_d->now_fn_queue_normal.empty());
}

bool ks_thread_pool_apartment_imp::is_stopped() {
	_STATE state = m_d->state_v;
	return state == _STATE::STOPPED;
}

bool ks_thread_pool_apartment_imp::is_stopping_or_stopped() {
	_STATE state = m_d->state_v;
	return state == _STATE::STOPPED || state == _STATE::STOPPING;
}


uint64_t ks_thread_pool_apartment_imp::schedule(std::function<void()>&& fn, int priority) {
	std::unique_lock<ks_mutex> lock(m_d->mutex);

	_try_start_locked(lock);
	ASSERT(m_d->state_v == _STATE::RUNNING || m_d->state_v == _STATE::STOPPING);

	if (!(m_d->state_v == _STATE::RUNNING || m_d->state_v == _STATE::STOPPING)) {
		lock.unlock();
		fn = {};
		return 0;
	}
	if (priority < 0 && m_d->state_v != _STATE::RUNNING) {
		lock.unlock();
		fn = {};
		return 0;
	}

	uint64_t fn_id = ++g_last_fn_id;
	ASSERT(fn_id != 0);
	ASSERT(!_check_fn_id_exists_when_debug_locked(m_d, fn_id, lock));

	auto fn_item = std::make_shared<_FN_ITEM>();
	fn_item->fn = std::move(fn);
	fn_item->fn_id = fn_id;
	fn_item->priority = priority;

	_do_put_fn_item_into_now_list_locked(m_d, std::move(fn_item), lock);
	_prepare_work_thread_locked(this, m_d, lock);

	return fn_id;
}

uint64_t ks_thread_pool_apartment_imp::schedule_delayed(std::function<void()>&& fn, int priority, int64_t delay) {
	std::unique_lock<ks_mutex> lock(m_d->mutex);

	_try_start_locked(lock);
	ASSERT(m_d->state_v == _STATE::RUNNING || m_d->state_v == _STATE::STOPPING);

	if (!(m_d->state_v == _STATE::RUNNING)) {
		lock.unlock();
		fn = {};
		return 0;
	}

	uint64_t fn_id = ++g_last_fn_id;
	ASSERT(fn_id != 0);
	ASSERT(!_check_fn_id_exists_when_debug_locked(m_d, fn_id, lock));

	auto fn_item = std::make_shared<_FN_ITEM>();
	fn_item->fn = std::move(fn);
	fn_item->until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay);
	fn_item->fn_id = fn_id;
	fn_item->priority = priority;
	fn_item->delay = delay;
	fn_item->is_delaying_fn = true;

	_do_put_fn_item_into_delaying_list_locked(m_d, std::move(fn_item), lock);
	_prepare_work_thread_locked(this, m_d, lock);

	return fn_id;
}

void ks_thread_pool_apartment_imp::try_unschedule(uint64_t id) {
	if (id == 0)
		return;

	std::unique_lock<ks_mutex> lock(m_d->mutex);

	auto do_erase_fn_from = [](std::deque<std::shared_ptr<_FN_ITEM>>*fn_queue, uint64_t fn_id) -> std::shared_ptr<_FN_ITEM> {
		auto it = std::find_if(fn_queue->begin(), fn_queue->end(),
			[fn_id](const auto& item) {return item->fn_id == fn_id; });
		if (it == fn_queue->end())
			return nullptr;

		std::shared_ptr<_FN_ITEM> found_fn = std::move(*it);
		fn_queue->erase(it);
		return found_fn;
	};

	//检查延时任务队列
	std::shared_ptr<_FN_ITEM> found_fn = do_erase_fn_from(&m_d->delaying_fn_queue, id);
	//检查idle任务队列
	if (found_fn == nullptr)
		found_fn = do_erase_fn_from(&m_d->now_fn_queue_idle, id);
	//对于其他任务队列（normal和prior），没有检查的必要和意义

	//release fn
	lock.unlock();
	if (found_fn != nullptr) {
		found_fn->fn = {};
		found_fn = nullptr;
	}
}


void ks_thread_pool_apartment_imp::_try_start_locked(std::unique_lock<ks_mutex>& lock) {
	if (m_d->state_v == _STATE::NOT_START) {
		m_d->state_v = _STATE::RUNNING;
	}
}

void ks_thread_pool_apartment_imp::_try_stop_locked(bool mark_waiting, std::unique_lock<ks_mutex>& lock, bool must_keep_locked) {
	ASSERT(lock.owns_lock());
	//must_keep_locked may be false.

	std::deque<std::shared_ptr<_FN_ITEM>> t_now_fn_queue_idle;
	std::deque<std::shared_ptr<_FN_ITEM>> t_delaying_fn_queue;
	std::function<void()> t_thread_init_fn;
	std::function<void()> t_thread_term_fn;

	if (m_d->state_v == _STATE::RUNNING) {
		if (!m_d->thread_pool.empty()) {
			m_d->state_v = _STATE::STOPPING;
			m_d->any_fn_queue_cv.notify_all(); //trigger threads
			m_d->now_fn_queue_idle.swap(t_now_fn_queue_idle); //cleanup idles at once
			m_d->delaying_fn_queue.swap(t_delaying_fn_queue); //cleanup delayings at once
		}
		else {
			m_d->state_v = _STATE::STOPPED;
			m_d->stopped_state_cv.notify_all();
			m_d->thread_init_fn.swap(t_thread_init_fn); //final cleanup
			m_d->thread_term_fn.swap(t_thread_term_fn); //final cleanup
		}
	}
	else if (m_d->state_v == _STATE::NOT_START) {
		m_d->state_v = _STATE::STOPPED;
		m_d->stopped_state_cv.notify_all();
		m_d->thread_init_fn.swap(t_thread_init_fn); //final cleanup
		m_d->thread_term_fn.swap(t_thread_term_fn); //final cleanup
	}

	if (mark_waiting && !m_d->waiting_v) {
		m_d->waiting_v = true;
		m_d->any_fn_queue_cv.notify_all();
	}

	if (!t_now_fn_queue_idle.empty() || !t_delaying_fn_queue.empty() || t_thread_init_fn || t_thread_term_fn) {
		lock.unlock();
		t_now_fn_queue_idle.clear();
		t_delaying_fn_queue.clear();
		t_thread_init_fn = nullptr;
		t_thread_term_fn = nullptr;
		if (must_keep_locked)
			lock.lock();
	}
}


void ks_thread_pool_apartment_imp::_prepare_work_thread_locked(ks_thread_pool_apartment_imp* self, const std::shared_ptr<_THREAD_POOL_APARTMENT_DATA>& d, std::unique_lock<ks_mutex>& lock) {
	if (d->thread_pool.size() >= d->max_thread_count)
		return;

	size_t needed_thread_count = 0;
	if (d->state_v == _STATE::RUNNING) 
		needed_thread_count = d->busy_thread_count + d->now_fn_queue_prior.size() + d->now_fn_queue_normal.size() + d->now_fn_queue_idle.size();
	else if (d->state_v == _STATE::STOPPING) 
		needed_thread_count = d->busy_thread_count + d->now_fn_queue_prior.size() + d->now_fn_queue_normal.size();

	if (needed_thread_count > d->max_thread_count)
		needed_thread_count = d->max_thread_count;

	if (needed_thread_count == 0 && d->state_v == _STATE::RUNNING)
		needed_thread_count = 1;

	for (size_t i = d->thread_pool.size(); i < needed_thread_count; ++i) {
		d->thread_pool.push_back(std::make_shared<_THREAD_ITEM>());
		d->living_thread_count++;

		std::thread([self, d, thread_index = d->thread_pool.size() - 1]() {
			_work_thread_proc(self, d, thread_index);
		}).detach();
	}
}

void ks_thread_pool_apartment_imp::_work_thread_proc(ks_thread_pool_apartment_imp* self, const std::shared_ptr<_THREAD_POOL_APARTMENT_DATA>& d, size_t thread_index) {
	ASSERT(ks_apartment::current_thread_apartment() == nullptr);
	ASSERT(tls_current_thread_index_plus == 0);
	ks_apartment::__set_current_thread_apartment(self);
	tls_current_thread_index_plus = thread_index + 1;

	if (true) {
		std::stringstream thread_name_ss;
		thread_name_ss << d->name << "'s work-thread [" << thread_index << "/" << d->max_thread_count << "]";
		ks_apartment::__set_current_thread_name(thread_name_ss.str().c_str());
	}

	if (d->thread_init_fn) {
		d->thread_init_fn();
	}

	ASSERT(tls_current_thread_pump_loop_depth == 0);
	++tls_current_thread_pump_loop_depth;

	while (true) {
		std::unique_lock<ks_mutex> lock(d->mutex);

#if __KS_APARTMENT_ATFORK_ENABLED
		while (d->atforking_flag_v) {
			d->atforking_done_cv.wait(lock);
		}
#endif

#if __KS_APARTMENT_ATFORK_ENABLED
		++d->working_rc_v;
		
		ks_defer defer_dec_working_rc([&d, &lock]() {
			ASSERT(lock.owns_lock());
			if (--d->working_rc_v == 0)
				d->working_done_cv.notify_all();
		});
#endif

		//try next now_fn
		if (true) {
			auto* now_fn_queue_sel = !d->now_fn_queue_prior.empty() ? &d->now_fn_queue_prior : &d->now_fn_queue_normal;
			if (now_fn_queue_sel->empty() && !d->now_fn_queue_idle.empty() && d->busy_thread_count_for_idle + 1 < d->thread_pool.size() && d->state_v == _STATE::RUNNING)
				now_fn_queue_sel = &d->now_fn_queue_idle; //保留1个线程不去执行idle任务（除非是单线程套间）

			if (!now_fn_queue_sel->empty()) {
				//pop and exec a fn
				bool is_now_fn_from_idle = now_fn_queue_sel == &d->now_fn_queue_idle;
				auto now_fn_item = std::move(now_fn_queue_sel->front());
				now_fn_queue_sel->pop_front();

				ASSERT(d->busy_thread_count < d->thread_pool.size());
				++d->busy_thread_count;
				if (is_now_fn_from_idle) {
					ASSERT(d->busy_thread_count_for_idle + 1 < d->thread_pool.size());
					++d->busy_thread_count_for_idle;
					ASSERT(!tls_current_thread_pump_loop_busy_for_idle_flag);
					tls_current_thread_pump_loop_busy_for_idle_flag = true;
				}

				ks_defer defer_dec_busy_thread_count([&d, is_now_fn_from_idle, &lock]() {
					ASSERT(lock.owns_lock());
					ASSERT(d->busy_thread_count >= 1);
					--d->busy_thread_count;
					if (is_now_fn_from_idle) {
						ASSERT(d->busy_thread_count_for_idle >= 1);
						--d->busy_thread_count_for_idle;
						ASSERT(tls_current_thread_pump_loop_busy_for_idle_flag);
						tls_current_thread_pump_loop_busy_for_idle_flag = false;
					}
				});

				lock.unlock();
				now_fn_item->fn();
				now_fn_item->fn = {};
				now_fn_item.reset();

				lock.lock(); //for working_rc and busy_thread_count
				continue;
			}
		}

		//try next delaying_fn
		if (!d->delaying_fn_queue.empty() && d->state_v == _STATE::RUNNING) {
			size_t moved_fn_count = 0;
			const auto now = std::chrono::steady_clock::now();
			while (!d->delaying_fn_queue.empty() && d->delaying_fn_queue.front()->until_time <= now) {
				//直接将到期的delaying项移入idle队列（忽略priority）
				_do_put_fn_item_into_now_list_locked(d, std::move(d->delaying_fn_queue.front()), lock);
				d->delaying_fn_queue.pop_front();
				++moved_fn_count;
			}

			if (moved_fn_count != 0) {
				if (d->max_thread_count > 1)
					_prepare_work_thread_locked(self, d, lock);
				continue;
			}
		}

		//pump-idle
		if (d->state_v == _STATE::STOPPING && d->waiting_v) {
			break; //end
		}

		//waiting
		if (!d->delaying_fn_queue.empty() && !d->delaying_fn_queue.front()->is_waiting_until_flag) {
			const auto waiting_fn_item = d->delaying_fn_queue.front();
			waiting_fn_item->is_waiting_until_flag = true;
			d->any_fn_queue_cv.wait_until(lock, waiting_fn_item->until_time); //waiting
			waiting_fn_item->is_waiting_until_flag = false;
		}
		else {
			d->any_fn_queue_cv.wait(lock);
		}
	}

	--tls_current_thread_pump_loop_depth;
	ASSERT(tls_current_thread_pump_loop_depth == 0);

	std::function<void()> t_thread_init_fn;
	std::function<void()> t_thread_term_fn;
	if (true) {
		std::unique_lock<ks_mutex> lock(d->mutex);
		ASSERT(d->living_thread_count > 0);
		d->living_thread_count--;
		if (d->state_v == _STATE::STOPPING && d->living_thread_count == 0) {
			d->state_v = _STATE::STOPPED;
			d->stopped_state_cv.notify_all();
			d->thread_init_fn.swap(t_thread_init_fn); //final cleanup
			d->thread_term_fn.swap(t_thread_term_fn); //final cleanup
		}
		else {
			t_thread_init_fn = d->thread_init_fn;
			t_thread_term_fn = d->thread_term_fn;
		}
	}

	if (t_thread_term_fn) {
		t_thread_term_fn();
	}

	t_thread_init_fn = nullptr;
	t_thread_term_fn = nullptr;
	ASSERT(ks_apartment::current_thread_apartment() == self);
	ASSERT(tls_current_thread_index_plus == thread_index + 1);
}

void ks_thread_pool_apartment_imp::_do_put_fn_item_into_now_list_locked(const std::shared_ptr<_THREAD_POOL_APARTMENT_DATA>& d, std::shared_ptr<_FN_ITEM>&& fn_item, std::unique_lock<ks_mutex>& lock) {
	auto* now_fn_queue_sel = 
		fn_item->is_delaying_fn ? &d->now_fn_queue_idle :
		fn_item->priority == 0 ? &d->now_fn_queue_normal :  //priority=0为普通优先级
		fn_item->priority > 0 ? &d->now_fn_queue_prior :    //priority>0为高优先级
		&d->now_fn_queue_idle;                             //priority<0为低优先级，简单地加入到idle队列
	now_fn_queue_sel->push_back(std::move(fn_item));
	d->any_fn_queue_cv.notify_one();
}

void ks_thread_pool_apartment_imp::_do_put_fn_item_into_delaying_list_locked(const std::shared_ptr<_THREAD_POOL_APARTMENT_DATA>& d, std::shared_ptr<_FN_ITEM>&& fn_item, std::unique_lock<ks_mutex>& lock) {
	auto where_it = d->delaying_fn_queue.end();
	if (!d->delaying_fn_queue.empty()) {
		if (fn_item->until_time >= d->delaying_fn_queue.back()->until_time) {
			//最大延时项：插在队尾
			where_it = d->delaying_fn_queue.end();
		}
		else if (fn_item->until_time < d->delaying_fn_queue.front()->until_time) {
			//最小延时项：插在队头
			where_it = d->delaying_fn_queue.begin();
		}
		else {
			//新项的目标时点落在当前队列的时段区间内。。。
			//忽略priority
			where_it = std::upper_bound(d->delaying_fn_queue.begin(), d->delaying_fn_queue.end(), fn_item,
				[](const auto& a, const auto& b) { return a->until_time < b->until_time; });
		}
	}

	bool should_notify = d->delaying_fn_queue.empty() || fn_item->until_time < d->delaying_fn_queue.front()->until_time;
	d->delaying_fn_queue.insert(where_it, std::move(fn_item));

	if (should_notify) {
		//只需notify_one即可，即使有多项。
		//这是因为调度时到期的delayed项会被先移至now队列，即使瞬间由一个线程处理多项也没什么负担。
		//参见_thread_proc中对于delayed的调度算法。
		d->any_fn_queue_cv.notify_one();
	}
}

#ifdef _DEBUG
bool ks_thread_pool_apartment_imp::_check_fn_id_exists_when_debug_locked(const std::shared_ptr<_THREAD_POOL_APARTMENT_DATA>& d, uint64_t fn_id, std::unique_lock<ks_mutex>& lock) {
	auto do_check_fn_exists = [](std::deque<std::shared_ptr<_FN_ITEM>>* fn_queue, uint64_t a_fn_id) -> bool {
		return std::find_if(fn_queue->cbegin(), fn_queue->cend(),
			[a_fn_id](const auto& item) {return item->fn_id == a_fn_id; }) != fn_queue->cend();
	};

	return do_check_fn_exists(&d->now_fn_queue_prior, fn_id)
		|| do_check_fn_exists(&d->now_fn_queue_normal, fn_id)
		|| do_check_fn_exists(&d->now_fn_queue_idle, fn_id)
		|| do_check_fn_exists(&d->delaying_fn_queue, fn_id);
}
#endif


#if __KS_APARTMENT_ATFORK_ENABLED
void ks_thread_pool_apartment_imp::atfork_prepare() {
	ASSERT(m_d->state_v == _STATE::NOT_START || m_d->state_v == _STATE::RUNNING);
	if (m_d->atforking_flag_v)
		return; //重复prepare

	const bool atfork_calling_in_my_thread_flag = (ks_apartment::current_thread_apartment() == this);
	_UNUSED(atfork_calling_in_my_thread_flag); //即使在该mta某个线程内调用fork，也要照样lock，因为至少还有delaying线程呢

	std::unique_lock<ks_mutex> lock(m_d->mutex);
	m_d->atforking_flag_v = true;

	m_d->any_fn_queue_cv.notify_all();

	while (m_d->working_rc_v != 0)
		m_d->working_done_cv.wait(lock);

#ifdef _DEBUG
	for (auto& fn_item : m_d->delaying_fn_queue) 
		ASSERT(!fn_item->is_waiting_until_flag);
#endif

	lock.release();
}

void ks_thread_pool_apartment_imp::atfork_parent() {
	ASSERT(m_d->state_v == _STATE::NOT_START || m_d->state_v == _STATE::RUNNING);
	if (!m_d->atforking_flag_v)
		return;

	const bool atfork_calling_in_my_thread_flag = (ks_apartment::current_thread_apartment() == this);
	_UNUSED(atfork_calling_in_my_thread_flag); //即使在该mta某个线程内调用fork，也要照样lock，因为至少还有delaying线程呢

	m_d->atforking_flag_v = false;
	m_d->atforking_done_cv.notify_all();
	m_d->mutex.unlock();
}

void ks_thread_pool_apartment_imp::atfork_child() {
	ASSERT(m_d->state_v == _STATE::NOT_START || m_d->state_v == _STATE::RUNNING);
	if (!m_d->atforking_flag_v)
		return;

	const bool atfork_calling_in_my_thread_flag = (ks_apartment::current_thread_apartment() == this);
	const size_t atfork_calling_in_my_thread_index = atfork_calling_in_my_thread_flag ? tls_current_thread_index_plus - 1 : size_t(-1);

	//重建线程
	for (size_t i = 0; i < m_d->thread_pool.size(); ++i) {
		if (!atfork_calling_in_my_thread_flag || i != atfork_calling_in_my_thread_index) {
			std::thread([self = this, d = m_d, thread_index = i]() {
				_work_thread_proc(self, d, thread_index);
			}).detach();
		}
	}

	m_d->atforking_flag_v = false;
	m_d->atforking_done_cv.notify_all();
	m_d->mutex.unlock();
}
#endif


bool ks_thread_pool_apartment_imp::__run_nested_pump_loop_for_extern_waiting(void* extern_obj, std::function<bool()>&& extern_pred_fn) {
	if (ks_apartment::current_thread_apartment() != this) {
		ASSERT(false);
		return false;
	}

	ASSERT(tls_current_thread_index_plus != 0);
	const size_t thread_index = tls_current_thread_index_plus - 1;
	_UNUSED(thread_index);

	ASSERT(tls_current_thread_pump_loop_depth >= 1);
	++tls_current_thread_pump_loop_depth;

	ks_defer defer_dec_tls_current_thread_pump_loop_depth([]() {
		--tls_current_thread_pump_loop_depth;
		ASSERT(tls_current_thread_pump_loop_depth >= 1);
	});

	auto d = m_d;
	bool was_satisified = false;

	while (true) {
		if (extern_pred_fn()) {
			was_satisified = true;
			break; //waiting was satisfied, ok
		}

		std::unique_lock<ks_mutex> lock(d->mutex);
		ASSERT(d->busy_thread_count >= 1);

#if __KS_APARTMENT_ATFORK_ENABLED
		if (d->atforking_flag_v)
			break; //atforking, broken
#endif

#if __KS_APARTMENT_ATFORK_ENABLED
		ASSERT(d->working_rc_v != 0);
#endif

		//try next now_fn
		if (true) {
			auto* now_fn_queue_sel = !d->now_fn_queue_prior.empty() ? &d->now_fn_queue_prior : &d->now_fn_queue_normal;
			if (now_fn_queue_sel->empty() && !d->now_fn_queue_idle.empty() && d->busy_thread_count_for_idle + 1 < d->thread_pool.size() && d->state_v == _STATE::RUNNING)
				now_fn_queue_sel = &d->now_fn_queue_idle; //保留1个线程不去执行idle任务（除非是单线程套间）

			if (!now_fn_queue_sel->empty()) {
				//pop and exec a fn
				auto now_fn_item = std::move(now_fn_queue_sel->front());
				now_fn_queue_sel->pop_front();

				lock.unlock();
				now_fn_item->fn();
				now_fn_item->fn = {};
				now_fn_item.reset();

				lock.lock(); //for working_rc and busy_thread_count
				continue;
			}
		}

		//try next delaying_fn
		if (!d->delaying_fn_queue.empty() && d->state_v == _STATE::RUNNING) {
			size_t moved_fn_count = 0;
			const auto now = std::chrono::steady_clock::now();
			while (!d->delaying_fn_queue.empty() && d->delaying_fn_queue.front()->until_time <= now) {
				//直接将到期的delaying项移入idle队列（忽略priority）
				_do_put_fn_item_into_now_list_locked(d, std::move(d->delaying_fn_queue.front()), lock);
				d->delaying_fn_queue.pop_front();
				++moved_fn_count;
			}

			if (moved_fn_count != 0) {
				if (d->max_thread_count > 1)
					_prepare_work_thread_locked(this, m_d, lock);
				continue;
			}
		}

		//pump-idle
		if (d->state_v != _STATE::RUNNING) {
			break; //end
		}

		//nested waiting
		const bool pre_busy_for_idle_flag = tls_current_thread_pump_loop_busy_for_idle_flag;
		ASSERT(d->busy_thread_count >= 1);
		--d->busy_thread_count;
		if (pre_busy_for_idle_flag) {
			ASSERT(d->busy_thread_count_for_idle >= 1);
			--d->busy_thread_count_for_idle;
			ASSERT(tls_current_thread_pump_loop_busy_for_idle_flag);
			tls_current_thread_pump_loop_busy_for_idle_flag = false;
		}

		ks_defer defer_recover_busy_thread_count([&d, pre_busy_for_idle_flag, &lock]() {
			ASSERT(lock.owns_lock());
			ASSERT(d->busy_thread_count < d->thread_pool.size());
			++d->busy_thread_count;
			if (pre_busy_for_idle_flag) {
				ASSERT(d->busy_thread_count_for_idle + 1 < d->thread_pool.size());
				++d->busy_thread_count_for_idle;
				ASSERT(!tls_current_thread_pump_loop_busy_for_idle_flag);
				tls_current_thread_pump_loop_busy_for_idle_flag = true;
			}
		});

		if (!d->delaying_fn_queue.empty() && !d->delaying_fn_queue.front()->is_waiting_until_flag) {
			const auto waiting_fn_item = d->delaying_fn_queue.front();
			waiting_fn_item->is_waiting_until_flag = true;
			d->any_fn_queue_cv.wait_until(lock, waiting_fn_item->until_time); //waiting
			waiting_fn_item->is_waiting_until_flag = false;
		}
		else {
			d->any_fn_queue_cv.wait(lock);
		}
	}

	return was_satisified;
}

void ks_thread_pool_apartment_imp::__awaken_nested_pump_loop_for_extern_waiting_once(void* extern_obj) {
	std::unique_lock<ks_mutex> lock(m_d->mutex);
	m_d->any_fn_queue_cv.notify_all();
}
