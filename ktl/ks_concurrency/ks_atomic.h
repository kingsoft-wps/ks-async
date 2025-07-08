/* Copyright 2025 The Kingsoft's ks-async/ktl Authors. All Rights Reserved.

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

#ifndef __KS_ATOMIC_DEF
#define __KS_ATOMIC_DEF

#include "_implement/ks_concurrency_helper.h"
#include <atomic>
#include <type_traits>
#include <thread>


template <class T>
class ks_atomic : private std::atomic<T> {
    //在不同平台上，wait-on-address操作要求T的size必须为:
    //  windows: 1, 2, 4, 8
    //  mac: 4, 8
    //  linix: 4
    //为此，我们限制T的size只能是4或8。
    //特别在linux上，若T的size为8（64位），wait-on-address操作仅在低32位上进行，
    //这样做的逻辑仍是正确的，因为wait有old参数，至多内部可能会出现假醒情况，且这对使用者是无感的。
    static_assert(
        std::is_integral<T>::value && (sizeof(T) == 4 || sizeof(T) == 8),
        "ks_atomic can only be used with 32-bit or 64-bit integral types");

public:
    ks_atomic() noexcept : std::atomic<T>(0) {} //与std不同，我们默认初始化为0
    ks_atomic(T desired) noexcept : std::atomic<T>(desired) {}
    _DISABLE_COPY_CONSTRUCTOR(ks_atomic);

    using std::atomic<T>::operator=;
    using std::atomic<T>::operator+=;
    using std::atomic<T>::operator-=;
    using std::atomic<T>::operator&=;
    using std::atomic<T>::operator|=;
    using std::atomic<T>::operator^=;
    using std::atomic<T>::operator++;
    using std::atomic<T>::operator--;
    using std::atomic<T>::operator T;

    using std::atomic<T>::load;
    using std::atomic<T>::store;
    using std::atomic<T>::exchange;
    using std::atomic<T>::compare_exchange_strong;
    using std::atomic<T>::compare_exchange_weak;
    using std::atomic<T>::fetch_add;
    using std::atomic<T>::fetch_sub;
    using std::atomic<T>::fetch_and;
    using std::atomic<T>::fetch_or;
    using std::atomic<T>::fetch_xor;

    using std::atomic<T>::is_lock_free;

#if __cplusplus < 202002L
    static bool __is_always_wait_efficient() noexcept {
#   if defined(_WIN32)
        return (bool)(_KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWaitOnAddress);
#   elif defined(__APPLE__)
        return (bool)(_KSConcurrencyImpl::_helper::getOSSyncApis()->wait_on_address);
#   elif defined(__linux__)
        return true; //use futex
#   else
        return false;
#   endif
    }

    void __wait(T old, std::memory_order order = std::memory_order_seq_cst) const {
        while (this->load(order) == old) {
#       if defined(_WIN32)
            if (_KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWaitOnAddress)
                _KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWaitOnAddress((void*)this, &old, sizeof(T), INFINITE);
            else
                std::this_thread::yield();
#       elif defined(__APPLE__)
            if (_KSConcurrencyImpl::_helper::getOSSyncApis()->wait_on_address)
                _KSConcurrencyImpl::_helper::getOSSyncApis()->wait_on_address((void*)this, (uint64_t)old, sizeof(T), _KSConcurrencyImpl::_helper::OSSYNC_WAIT_ON_ADDRESS_NONE);
            else
                std::this_thread::yield();
#       elif defined(__linux__)
            //即使T是64位，futex也只操作低32位的地址就够了
            if (_KSConcurrencyImpl::_helper::linux_futex32_wait((uint32_t*)this, (uint32_t)old, nullptr) == _KSConcurrencyImpl::_helper::_LINUX_FUTEX32_NOT_SUPPORTED) 
                std::this_thread::yield();
#       else
            std::this_thread::yield();
#       endif
        }
    }

    void __notify_one() {
#   if defined(_WIN32)
        if (_KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWakeByAddressSingle)
            _KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWakeByAddressSingle((void*)this);
        else
            (void)0; //noop
#   elif defined(__APPLE__)
        if (_KSConcurrencyImpl::_helper::getOSSyncApis()->wake_by_address_any)
            _KSConcurrencyImpl::_helper::getOSSyncApis()->wake_by_address_any((void*)this, sizeof(T), _KSConcurrencyImpl::_helper::OSSYNC_WAKE_BY_ADDRESS_NONE);
        else
            (void)0; //noop
#   elif defined(__linux__)
        //即使T是64位，futex也只操作低32位的地址就够了
        _KSConcurrencyImpl::_helper::linux_futex32_wake_one((uint32_t*)this);
#   else
        (void)0; //noop
#   endif
    }

    void __notify_all() {
#   if defined(_WIN32)
        if (_KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWakeByAddressAll)
            _KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWakeByAddressAll((void*)this);
        else
            (void)0; //noop
#   elif defined(__APPLE__)
        if (_KSConcurrencyImpl::_helper::getOSSyncApis()->wake_by_address_all)
            _KSConcurrencyImpl::_helper::getOSSyncApis()->wake_by_address_all((void*)this, sizeof(T), _KSConcurrencyImpl::_helper::OSSYNC_WAKE_BY_ADDRESS_NONE);
        else
            (void)0; //noop
#   elif defined(__linux__)
        //即使T是64位，futex也只操作低32位的地址就够了
        _KSConcurrencyImpl::_helper::linux_futex32_wake_all((uint32_t*)this);
#   else
        (void)0; //noop
#   endif
    }

#else
    static bool __is_always_wait_efficient() noexcept {
        return true;
    }

    void __wait(T old, std::memory_order order = std::memory_order_seq_cst) const {
        std::atomic<T>::wait(old, order);
    }
    void __notify_one() {
        std::atomic<T>::notify_one();
    }
    void __notify_all() {
        std::atomic<T>::notify_all();
    }

#endif //__cplusplus < 202002L
};


class ks_atomic_flag {
public:
    ks_atomic_flag() noexcept : m_underly_atomic_int(0) {} //与std不同，我们默认初始化为false（即0）
    ks_atomic_flag(bool desired) noexcept : m_underly_atomic_int(desired ? 1 : 0) {} //与std不同，我们还提供指定初值构造
    _DISABLE_COPY_CONSTRUCTOR(ks_atomic_flag);

    void clear(std::memory_order order = std::memory_order_seq_cst) {
        m_underly_atomic_int.store(0, order);
    }

    bool test(std::memory_order order = std::memory_order_seq_cst) const {
        return m_underly_atomic_int.load(order) != 0;
    }

    bool test_and_set(std::memory_order order = std::memory_order_seq_cst) {
        return m_underly_atomic_int.exchange(1, order) != 0;
    }

    static bool __is_always_wait_efficient() noexcept {
        return ks_atomic<int>::__is_always_wait_efficient();
    }

    void __wait(bool old, std::memory_order order = std::memory_order_seq_cst) const {
        m_underly_atomic_int.__wait(old ? 1 : 0, order);
    }

    void __notify_one() {
        m_underly_atomic_int.__notify_one();
    }

    void __notify_all() {
        m_underly_atomic_int.__notify_all();
    }

private:
    ks_atomic<int> m_underly_atomic_int;
};


#endif // __KS_ATOMIC_DEF
