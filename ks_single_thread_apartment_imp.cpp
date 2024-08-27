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

#include "ks_single_thread_apartment_imp.h"
#include <algorithm>

void __forcelink_to_ks_single_thread_apartment_imp_cpp() {}


ks_single_thread_apartment_imp::ks_single_thread_apartment_imp(bool isolated_thread_mode)
	: m_d(new _SINGLE_THREAD_APARTMENT_DATA()) {
	m_d->isolated_thread_mode = isolated_thread_mode;
}

ks_single_thread_apartment_imp::~ks_single_thread_apartment_imp() {
	ASSERT(m_d->state_v == _STATE::NOT_START || m_d->state_v == _STATE::STOPPED);
	if (m_d->state_v == _STATE::RUNNING)
		this->async_stop();
	if (m_d->state_v == _STATE::STOPPING)
		this->wait();

	ASSERT(m_d->state_v == _STATE::NOT_START || m_d->state_v == _STATE::STOPPED);
	delete m_d;
}


bool ks_single_thread_apartment_imp::start() {
	std::unique_lock<ks_mutex> lock(m_d->mutex);
	if (m_d->state_v != _STATE::NOT_START && m_d->state_v != _STATE::RUNNING)
		return false;

	_try_start_locked(lock);
	if (!m_d->isolated_thread_mode)
		this->_single_thread_proc();

	return true;
}

void ks_single_thread_apartment_imp::async_stop() {
	std::unique_lock<ks_mutex> lock(m_d->mutex);
	return _try_stop_locked(lock);
}

//注：目前的wait实现暂不支持并发重入
void ks_single_thread_apartment_imp::wait() {
	ASSERT(this != ks_apartment::__tls_get_current_thread_apartment());

	std::unique_lock<ks_mutex> lock(m_d->mutex);
	this->_try_stop_locked(lock); //ensure stop

	if (m_d->state_v == _STATE::STOPPING) {
		if (m_d->prime_waiting_thread_id == std::thread::id{}) {
			m_d->prime_waiting_thread_id = std::this_thread::get_id();

			m_d->isolated_thread_presented_flag = true; //disable more threads presented
			if (m_d->isolated_thread_opt != nullptr) {
				std::shared_ptr<std::thread> t_isolated_thread;
				t_isolated_thread.swap(m_d->isolated_thread_opt);
				m_d->isolated_thread_opt = nullptr;

				lock.unlock();
				if (t_isolated_thread != nullptr)
					t_isolated_thread->join();
				lock.lock();
			}

			ASSERT(m_d->state_v == _STATE::STOPPING);
			ASSERT(m_d->now_fn_queue_prior.empty() && m_d->now_fn_queue_normal.empty());
			ASSERT(m_d->prime_waiting_thread_id == std::this_thread::get_id());
			m_d->state_v = _STATE::STOPPED;

			m_d->prime_waiting_thread_id = std::thread::id{};
			m_d->prime_waiting_thread_cv.notify_all();
		}
		else {
			while (m_d->prime_waiting_thread_id != std::thread::id{}) {
				m_d->prime_waiting_thread_cv.wait(lock);
			}

			ASSERT(m_d->state_v == _STATE::STOPPED);
		}
	}
}

bool ks_single_thread_apartment_imp::is_stopping_or_stopped() {
	_STATE state = m_d->state_v;
	return state == _STATE::STOPPED || state == _STATE::STOPPING;
}

bool ks_single_thread_apartment_imp::is_stopped() {
	_STATE state = m_d->state_v;
	return state == _STATE::STOPPED;
}


uint64_t ks_single_thread_apartment_imp::schedule(std::function<void()>&& fn, int priority) {
	std::unique_lock<ks_mutex> lock(m_d->mutex);

	_try_start_locked(lock);
	if (m_d->state_v != _STATE::RUNNING && m_d->state_v != _STATE::STOPPING) {
		ASSERT(false);
		return 0;
	}

	uint64_t id = ++m_d->atomic_last_fn_id;
	ASSERT(id != 0);
	ASSERT(!_debug_check_fn_id_exists_locked(id, lock));

	_FN_ITEM fn_item;
	fn_item.fn = std::move(fn);
	fn_item.priority = priority;
	fn_item.delay = 0;
	fn_item.is_delaying_fn = false;
	fn_item.until_time = std::chrono::steady_clock::time_point{};
	fn_item.fn_id = id;

	_do_put_fn_item_into_now_list_locked(std::move(fn_item), lock);
	_prepare_single_thread_locked(lock);

	return id;
}

uint64_t ks_single_thread_apartment_imp::schedule_delayed(std::function<void()>&& fn, int priority, int64_t delay) {
	std::unique_lock<ks_mutex> lock(m_d->mutex);

	_try_start_locked(lock);
	if (m_d->state_v != _STATE::RUNNING) {
		ASSERT(false);
		return 0;
	}

	uint64_t id = ++m_d->atomic_last_fn_id;
	ASSERT(id != 0);
	ASSERT(!_debug_check_fn_id_exists_locked(id, lock));

	_FN_ITEM fn_item;
	fn_item.fn = std::move(fn);
	fn_item.priority = priority;
	fn_item.delay = delay;
	fn_item.is_delaying_fn = true;
	fn_item.until_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay);
	fn_item.fn_id = id;

	if (delay <= 0) {
		_do_put_fn_item_into_now_list_locked(std::move(fn_item), lock);
		_prepare_single_thread_locked(lock);
	}
	else {
		_do_put_fn_item_into_delaying_list_locked(std::move(fn_item), lock);
		_prepare_single_thread_locked(lock);
	}

	return id;
}

void ks_single_thread_apartment_imp::try_unschedule(uint64_t id) {
	if (id == 0)
		return;

	std::unique_lock<ks_mutex> lock(m_d->mutex);

	auto do_erase_fn_from = [](std::deque<_FN_ITEM>* fn_queue, uint64_t fn_id) -> bool {
		auto it = std::find_if(fn_queue->begin(), fn_queue->end(),
			[fn_id](auto& item) {return item.fn_id == fn_id; });
		if (it != fn_queue->end()) {
			fn_queue->erase(it);
			return true;
		}
		else {
			return false;
		}
	};

	//检查延时任务队列
	if (do_erase_fn_from(&m_d->delaying_fn_queue, id))
		return;
	//检查idle任务队列
	if (do_erase_fn_from(&m_d->now_fn_queue_idle, id))
		return;

	//对于其他任务队列（normal和prior），没有检查的必要和意义
	return;
}

void ks_single_thread_apartment_imp::_try_start_locked(std::unique_lock<ks_mutex>& lock) {
	if (m_d->state_v == _STATE::NOT_START) {
		m_d->state_v = _STATE::RUNNING;
	}
}

void ks_single_thread_apartment_imp::_try_stop_locked(std::unique_lock<ks_mutex>& lock) {
	if (m_d->state_v == _STATE::RUNNING) {
		m_d->state_v = _STATE::STOPPING;
		m_d->any_fn_queue_cv.notify_all();
	}
	else if (m_d->state_v == _STATE::NOT_START) {
		m_d->state_v = _STATE::STOPPED;
	}
}


void ks_single_thread_apartment_imp::_prepare_single_thread_locked(std::unique_lock<ks_mutex>& lock) {
	if (!m_d->isolated_thread_mode)
		return;
	if (m_d->isolated_thread_presented_flag)
		return;

	ASSERT(m_d->isolated_thread_opt == nullptr);

	bool need_thread_flag = false;
	if (m_d->state_v == _STATE::RUNNING)
		need_thread_flag = !m_d->now_fn_queue_prior.empty() || !m_d->now_fn_queue_normal.empty() || !m_d->now_fn_queue_idle.empty() || !m_d->delaying_fn_queue.empty();
	else if (m_d->state_v == _STATE::STOPPING)
		need_thread_flag = !m_d->now_fn_queue_prior.empty() || !m_d->now_fn_queue_normal.empty();
	else 
		need_thread_flag = false;

	if (need_thread_flag) {
		m_d->isolated_thread_opt = std::make_shared<std::thread>(
			[this]() { this->_single_thread_proc(); });
		m_d->isolated_thread_presented_flag = true;
	}
}

void ks_single_thread_apartment_imp::_single_thread_proc() {
	ASSERT(ks_apartment::__tls_get_current_thread_apartment() == nullptr);
	ks_apartment::__tls_set_current_thread_apartment(this);

	std::unique_lock<ks_mutex> lock(m_d->mutex);
	while (true) {
		//try next now_fn
		if (true) {
			auto* now_fn_queue_sel = !m_d->now_fn_queue_prior.empty() ? &m_d->now_fn_queue_prior : &m_d->now_fn_queue_normal;
			if (now_fn_queue_sel->empty()) {
				if (m_d->state_v != _STATE::RUNNING && m_d->now_fn_queue_prior.empty() && m_d->now_fn_queue_normal.empty())
					break; //check stop, end

				if (!m_d->now_fn_queue_idle.empty())
					now_fn_queue_sel = &m_d->now_fn_queue_idle;
			}

			if (!now_fn_queue_sel->empty()) {
				//pop and exec a fn
				_FN_ITEM now_fn_item = std::move(now_fn_queue_sel->front());
				now_fn_queue_sel->pop_front();

				lock.unlock();

				try {
					now_fn_item.fn();
				}
				catch (...) {
					//TODO dump exception ...
					ASSERT(false);
					abort();
				}

				lock.lock();
				continue;
			}
		}

		//try next delaying_fn
		if (m_d->state_v != _STATE::RUNNING)
			break; //check stop, end

		if (m_d->delaying_fn_queue.empty()) {
			//wait
			m_d->any_fn_queue_cv.wait(lock);
			continue;
		}
		else if (m_d->delaying_fn_queue.front().until_time > std::chrono::steady_clock::now()) {
			//wait until
			m_d->any_fn_queue_cv.wait_until(lock, m_d->delaying_fn_queue.front().until_time);
			continue;
		}
		else {
			//直接将到期的delaying项移入idle队列（忽略priority）
			_do_put_fn_item_into_now_list_locked(std::move(m_d->delaying_fn_queue.front()), lock);
			m_d->delaying_fn_queue.pop_front();
			continue;
		}
	}

	ASSERT(ks_apartment::__tls_get_current_thread_apartment() == this);
	ks_apartment::__tls_set_current_thread_apartment(nullptr);
}

bool ks_single_thread_apartment_imp::_debug_check_fn_id_exists_locked(uint64_t id, std::unique_lock<ks_mutex>& lock) const {
	auto do_check_fn_exists = [](std::deque<_FN_ITEM>* fn_queue, uint64_t fn_id) -> bool {
		return std::find_if(fn_queue->cbegin(), fn_queue->cend(),
			[fn_id](auto& item) {return item.fn_id == fn_id; }) != fn_queue->cend();
	};

	return do_check_fn_exists(&m_d->now_fn_queue_prior, id)
		|| do_check_fn_exists(&m_d->now_fn_queue_normal, id)
		|| do_check_fn_exists(&m_d->now_fn_queue_idle, id)
		|| do_check_fn_exists(&m_d->delaying_fn_queue, id);
}

void ks_single_thread_apartment_imp::_do_put_fn_item_into_now_list_locked(_FN_ITEM&& fn_item, std::unique_lock<ks_mutex>& lock) {
	auto* now_fn_queue_sel = 
		fn_item.is_delaying_fn ? &m_d->now_fn_queue_idle :
		fn_item.priority == 0 ? &m_d->now_fn_queue_normal :  //priority=0为普通优先级
		fn_item.priority > 0 ? &m_d->now_fn_queue_prior :    //priority>0为高优先级
		&m_d->now_fn_queue_idle;                             //priority<0为低优先级，简单地加入到idle队列
	now_fn_queue_sel->push_back(std::move(fn_item));
	m_d->any_fn_queue_cv.notify_one();
}

void ks_single_thread_apartment_imp::_do_put_fn_item_into_delaying_list_locked(_FN_ITEM&& fn_item, std::unique_lock<ks_mutex>& lock) {
	auto where_it = m_d->delaying_fn_queue.end();
	if (!m_d->delaying_fn_queue.empty()) {
		if (fn_item.until_time >= m_d->delaying_fn_queue.back().until_time) {
			//最大延时项：插在队尾
			where_it = m_d->delaying_fn_queue.end();
		}
		else if (fn_item.until_time < m_d->delaying_fn_queue.front().until_time) {
			//最小延时项：插在队头
			where_it = m_d->delaying_fn_queue.begin();
		}
		else {
			//新项的目标时点落在当前队列的时段区间内。。。
			//忽略priority
			where_it = std::upper_bound(m_d->delaying_fn_queue.begin(), m_d->delaying_fn_queue.end(), fn_item,
				[](auto& a, auto& b) { return a.until_time < b.until_time; });
		}
	}

	bool should_notify = 
		(m_d->delaying_fn_queue.empty() || fn_item.until_time < m_d->delaying_fn_queue.front().until_time) &&
		(m_d->now_fn_queue_prior.empty() && m_d->now_fn_queue_normal.empty() && m_d->now_fn_queue_idle.empty());
	m_d->delaying_fn_queue.insert(where_it, std::move(fn_item));

	if (should_notify) {
		//只需notify_one即可，即使有多项。
		//这是因为调度时到期的delayed项会被先移至now队列，即使瞬间由一个线程处理多项也没什么负担。
		//参见_thread_proc中对于delayed的调度算法。
		m_d->any_fn_queue_cv.notify_one();
	}
}

