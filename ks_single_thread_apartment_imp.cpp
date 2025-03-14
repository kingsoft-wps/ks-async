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
#if defined(_WIN32)
#	include <Windows.h>
#else
#	include <pthread.h>
#endif

void __forcelink_to_ks_single_thread_apartment_imp_cpp() {}


ks_single_thread_apartment_imp::ks_single_thread_apartment_imp(const char* name, uint flags) {
	ASSERT(name != nullptr);
	ASSERT((flags & ~__all_flags) == 0);

	m_d = new _SINGLE_THREAD_APARTMENT_DATA();
	m_d->name = name;
	m_d->flags = flags;

	if ((m_d->flags & auto_register_flag) && !m_d->name.empty()) {
		ks_apartment::register_public_apartment(m_d->name.c_str(), this);
	}
	if (m_d->flags & be_ui_sta_flag) {
		ks_apartment::__set_ui_sta(this);
		if ((m_d->flags & auto_register_flag) && strcmp(m_d->name.c_str(), "ui") != 0)
			ks_apartment::register_public_apartment("ui", this);
	}
	if (m_d->flags & be_master_sta_flag) {
		ks_apartment::__set_master_sta(this);
		if ((m_d->flags & auto_register_flag) && strcmp(m_d->name.c_str(), "master") != 0)
			ks_apartment::register_public_apartment("master", this);
	}
}

ks_single_thread_apartment_imp::~ks_single_thread_apartment_imp() {
	ASSERT(m_d->state_v == _STATE::NOT_START || m_d->state_v == _STATE::STOPPED);
	if (m_d->state_v != _STATE::STOPPED) {
		this->async_stop();
		this->wait();
	}

	if (true) {
		std::unique_lock<ks_mutex> lock(m_d->mutex);

		while (m_d->isolated_thread_opt != nullptr) {
			auto t_isolated_thread_opt = std::move(m_d->isolated_thread_opt);
			m_d->isolated_thread_opt = nullptr;
			lock.unlock();
			t_isolated_thread_opt->join();
			lock.lock();
		}
	}

	if ((m_d->flags & auto_register_flag) && !m_d->name.empty()) {
		ks_apartment::unregister_public_apartment(m_d->name.c_str(), this);
	}
	if (m_d->flags & be_ui_sta_flag) {
		ks_apartment::__set_ui_sta(nullptr);
		if ((m_d->flags & auto_register_flag) && strcmp(m_d->name.c_str(), "ui") != 0)
			ks_apartment::unregister_public_apartment("ui", this);
	}
	if (m_d->flags & be_master_sta_flag) {
		ks_apartment::__set_master_sta(nullptr);
		if ((m_d->flags & auto_register_flag) && strcmp(m_d->name.c_str(), "master") != 0)
			ks_apartment::unregister_public_apartment("master", this);
	}

	delete m_d;
}


const char* ks_single_thread_apartment_imp::name() {
	return m_d->name.c_str();
}

uint ks_single_thread_apartment_imp::features() {
	uint my_features = sequential_feature;
	return my_features;
}


bool ks_single_thread_apartment_imp::start() {
	std::unique_lock<ks_mutex> lock(m_d->mutex);
	if (m_d->state_v != _STATE::NOT_START && m_d->state_v != _STATE::RUNNING)
		return false;

	_try_start_locked(lock);
	if (m_d->flags & inplace_thread_flag)
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

	while (m_d->state_v == _STATE::STOPPING) {
		m_d->stopped_state_cv.wait(lock);
	}

	ASSERT(m_d->state_v == _STATE::STOPPED);
	ASSERT(m_d->now_fn_queue_prior.empty() && m_d->now_fn_queue_normal.empty());
}

bool ks_single_thread_apartment_imp::is_stopped() {
	_STATE state = m_d->state_v;
	return state == _STATE::STOPPED;
}

bool ks_single_thread_apartment_imp::is_stopping_or_stopped() {
	_STATE state = m_d->state_v;
	return state == _STATE::STOPPED || state == _STATE::STOPPING;
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
		if ((m_d->flags & inplace_thread_flag) || m_d->isolated_thread_opt != nullptr) {
			m_d->state_v = _STATE::STOPPING;
			m_d->any_fn_queue_cv.notify_all();
		}
		else {
			m_d->state_v = _STATE::STOPPED;
			m_d->stopped_state_cv.notify_all();
		}
	}
	else if (m_d->state_v == _STATE::NOT_START) {
		m_d->state_v = _STATE::STOPPED;
		m_d->stopped_state_cv.notify_all();
	}
}


void ks_single_thread_apartment_imp::_prepare_single_thread_locked(std::unique_lock<ks_mutex>& lock) {
	if (m_d->flags & inplace_thread_flag)
		return;
	if (m_d->isolated_thread_opt != nullptr)
		return;

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
	}
}

void ks_single_thread_apartment_imp::_single_thread_proc() {
	ASSERT(ks_apartment::__tls_get_current_thread_apartment() == nullptr);
	ks_apartment::__tls_set_current_thread_apartment(this);

	std::string& thread_name = m_d->name;
#if defined(_WIN32)
	typedef HRESULT (WINAPI* PFN_SetThreadDescription)(HANDLE, PCWSTR);
	static PFN_SetThreadDescription _pfnSetThreadDescription = (PFN_SetThreadDescription)::GetProcAddress(::GetModuleHandleW(L"Kernel32.dll"), "SetThreadDescription");
	if (_pfnSetThreadDescription != nullptr) 
		_pfnSetThreadDescription(::GetCurrentThread(), std::wstring(thread_name.cbegin(), thread_name.cend()).c_str());
#elif defined(__APPLE__)
	pthread_setname_np(thread_name.c_str());
#else
	pthread_setname_np(pthread_self(), thread_name.c_str());
#endif

	while (true) {
#if __KS_APARTMENT_ATFORK_ENABLED
		std::unique_lock<ks_mutex> busy_unique_lock(m_d->busy_unique_mutex);
#endif
		std::unique_lock<ks_mutex> lock(m_d->mutex);

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

				//try {
					now_fn_item.fn();
				//}
				//catch (...) {
				//	//TODO dump exception ...
				//	ASSERT(false);
				//	//abort();
				//	throw;
				//}

				continue;
			}
		}

		//try next delaying_fn
		if (m_d->state_v != _STATE::RUNNING)
			break; //check stop, end

		if (m_d->delaying_fn_queue.empty()) {
			//wait
#if __KS_APARTMENT_ATFORK_ENABLED
			busy_unique_lock.unlock();
#endif
			m_d->any_fn_queue_cv.wait(lock);
			continue;
		}
		else if (m_d->delaying_fn_queue.front().until_time > std::chrono::steady_clock::now()) {
			//wait until
#if __KS_APARTMENT_ATFORK_ENABLED
			busy_unique_lock.unlock();
#endif
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

	if (true) {
		std::unique_lock<ks_mutex> lock(m_d->mutex);
		if (m_d->state_v == _STATE::STOPPING) {
			m_d->state_v = _STATE::STOPPED;
			m_d->stopped_state_cv.notify_all();
		}
	}

	ASSERT(ks_apartment::__tls_get_current_thread_apartment() == this);
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


bool ks_single_thread_apartment_imp::__try_pump_once() {
	ASSERT(ks_apartment::__tls_get_current_thread_apartment() == this);

	std::unique_lock<ks_mutex> lock(m_d->mutex);

	//try next now_fn
	//注：主动泵不必去执行idle任务
	auto* now_fn_queue_sel = !m_d->now_fn_queue_prior.empty() ? &m_d->now_fn_queue_prior : &m_d->now_fn_queue_normal;
	if (now_fn_queue_sel->empty()) 
		return false; //check stop, end

	//pop and exec a fn
	_FN_ITEM now_fn_item = std::move(now_fn_queue_sel->front());
	now_fn_queue_sel->pop_front();

	lock.unlock();

	//try {
		now_fn_item.fn();
	//}
	//catch (...) {
	//	//TODO dump exception ...
	//	ASSERT(false);
	//	//abort();
	//	throw;
	//}

	//注：无事待做，无需重锁
	//lock.lock();

	return true;
}


#if __KS_APARTMENT_ATFORK_ENABLED
void ks_single_thread_apartment_imp::atfork_prepare() {
	ASSERT(m_d->state_v != _STATE::STOPPING && m_d->state_v != _STATE::STOPPED);

	if (m_d->atfork_prepared_flag_v) 
		return; //重复prepare

	bool atfork_calling_in_my_thread_flag = (ks_apartment::__tls_get_current_thread_apartment() == this);

	//若是在本套间线程中调用fork，则busy_unique_mutex已锁，继续保持即可...
	if (!atfork_calling_in_my_thread_flag) {
		m_d->busy_unique_mutex.lock();
	}

	//unlock不保证时序，所以这里需要对mutex加锁，已保证一致性
	std::unique_lock<ks_mutex> lock(m_d->mutex);

	ASSERT(!m_d->atfork_prepared_flag_v);
	m_d->atfork_prepared_flag_v = true;
	m_d->atfork_calling_in_my_thread_flag = atfork_calling_in_my_thread_flag;
}

void ks_single_thread_apartment_imp::atfork_parent() {
	ASSERT(m_d->state_v != _STATE::STOPPING && m_d->state_v != _STATE::STOPPED);

	if (!m_d->atfork_prepared_flag_v)
		return;

	bool atfork_calling_in_my_thread_flag = (ks_apartment::__tls_get_current_thread_apartment() == this);
	ASSERT(atfork_calling_in_my_thread_flag == m_d->atfork_calling_in_my_thread_flag);

	//这里实际上不会有竞争者，因为fork是不应该出现竞争的
	std::unique_lock<ks_mutex> lock(m_d->mutex);

	m_d->atfork_prepared_flag_v = false;
	m_d->atfork_calling_in_my_thread_flag = false;

	if (!atfork_calling_in_my_thread_flag) {
		m_d->busy_unique_mutex.unlock();
	}
}

void ks_single_thread_apartment_imp::atfork_child() {
	ASSERT(m_d->state_v != _STATE::STOPPING && m_d->state_v != _STATE::STOPPED);

	if (!m_d->atfork_prepared_flag_v)
		return;

	bool atfork_calling_in_my_thread_flag = (ks_apartment::__tls_get_current_thread_apartment() == this);
	ASSERT(atfork_calling_in_my_thread_flag == m_d->atfork_calling_in_my_thread_flag);

	//这里实际上不会有竞争者，因为fork是不应该出现竞争的
	std::unique_lock<ks_mutex> lock(m_d->mutex);

	//重建线程
	if (!atfork_calling_in_my_thread_flag && m_d->isolated_thread_opt) {
		m_d->isolated_thread_opt->detach();
		m_d->isolated_thread_opt = std::make_shared<std::thread>(
			[this]() { this->_single_thread_proc(); });
	}

	m_d->atfork_prepared_flag_v = false;
	m_d->atfork_calling_in_my_thread_flag = false;

	if (!atfork_calling_in_my_thread_flag) {
		m_d->busy_unique_mutex.unlock();
	}
}
#endif
