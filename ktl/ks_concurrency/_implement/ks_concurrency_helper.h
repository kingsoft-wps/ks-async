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

#ifndef __KS_CONCURRENCY_HELPER_DEF
#define __KS_CONCURRENCY_HELPER_DEF

#include "../../ks_cxxbase.h"
#include <cstdint>
#include <cstddef>
#include <climits>
#include <atomic>
#include <stdexcept>


#if defined(_WIN32)
    #include <Windows.h>
    #include <synchapi.h>

    namespace _KSConcurrencyImpl {
        namespace _helper {

            struct SRWLOCK { PVOID Ptr; };
            typedef void(WINAPI* AcquireSRWLockExclusiveFuncPtr)(struct SRWLOCK* SRWLock);
            typedef BOOLEAN(WINAPI* TryAcquireSRWLockExclusiveFuncPtr)(struct SRWLOCK* SRWLock);
            typedef void(WINAPI* ReleaseSRWLockExclusiveFuncPtr)(struct SRWLOCK* SRWLock);

            typedef void(WINAPI* WaitOnAddressFuncPtr)(volatile void*, void*, size_t, DWORD);
            typedef void(WINAPI* WakeByAddressSingleFuncPtr)(PVOID Address);
            typedef void(WINAPI* WakeByAddressAllFuncPtr)(PVOID Address);

            struct WinSynchApis {
                AcquireSRWLockExclusiveFuncPtr pfnAcquireSRWLockExclusive = nullptr;
                TryAcquireSRWLockExclusiveFuncPtr pfnTryAcquireSRWLockExclusive = nullptr;
                ReleaseSRWLockExclusiveFuncPtr pfnReleaseSRWLockExclusive = nullptr;

                WaitOnAddressFuncPtr pfnWaitOnAddress = nullptr;
                WakeByAddressSingleFuncPtr pfnWakeByAddressSingle = nullptr;
                WakeByAddressAllFuncPtr pfnWakeByAddressAll = nullptr;
            };

            inline const WinSynchApis* getWinSynchApis() {
                static WinSynchApis* p_apis = []() -> WinSynchApis* {
                    static WinSynchApis s_apis;
                    if (true) {
                        HMODULE hModule = ::GetModuleHandleW(L"Kernel32.dll");
                        if (!hModule)
                            hModule = ::LoadLibraryW(L"Kernel32.dll"); //不必释放

                        if (hModule) {
                            s_apis.pfnAcquireSRWLockExclusive = reinterpret_cast<AcquireSRWLockExclusiveFuncPtr>(::GetProcAddress(hModule, "AcquireSRWLockExclusive"));
                            s_apis.pfnTryAcquireSRWLockExclusive = reinterpret_cast<TryAcquireSRWLockExclusiveFuncPtr>(::GetProcAddress(hModule, "TryAcquireSRWLockExclusive"));
                            s_apis.pfnReleaseSRWLockExclusive = reinterpret_cast<ReleaseSRWLockExclusiveFuncPtr>(::GetProcAddress(hModule, "ReleaseSRWLockExclusive"));
                            ASSERT((s_apis.pfnAcquireSRWLockExclusive && s_apis.pfnTryAcquireSRWLockExclusive && s_apis.pfnReleaseSRWLockExclusive)
                                || (!s_apis.pfnAcquireSRWLockExclusive && !s_apis.pfnTryAcquireSRWLockExclusive && !s_apis.pfnReleaseSRWLockExclusive));
                        }
                    }

                    if (true) {
                        HMODULE hModule = ::GetModuleHandleW(L"api-ms-win-core-synch-l1-2-0.dll");
                        if (!hModule)
                            hModule = ::LoadLibraryW(L"api-ms-win-core-synch-l1-2-0.dll"); //不必释放

                        if (hModule) {
                            s_apis.pfnWaitOnAddress = reinterpret_cast<WaitOnAddressFuncPtr>(::GetProcAddress(hModule, "WaitOnAddress"));
                            s_apis.pfnWakeByAddressSingle = reinterpret_cast<WakeByAddressSingleFuncPtr>(::GetProcAddress(hModule, "WakeByAddressSingle"));
                            s_apis.pfnWakeByAddressAll = reinterpret_cast<WakeByAddressAllFuncPtr>(::GetProcAddress(hModule, "WakeByAddressAll"));
                            ASSERT((s_apis.pfnWaitOnAddress && s_apis.pfnWakeByAddressSingle && s_apis.pfnWakeByAddressAll)
                                || (!s_apis.pfnWaitOnAddress && !s_apis.pfnWakeByAddressSingle && !s_apis.pfnWakeByAddressAll));
                        }
                    }

                    return &s_apis;
                }();
                return p_apis;
            }

        } //namespace _helper
    } // namespace _KSConcurrencyImpl


#elif defined(__APPLE__)
    #include <dlfcn.h>
    #include <os/lock.h>
    #include <dispatch/dispatch.h>

    namespace _KSConcurrencyImpl {
        namespace _helper {

            using unfair_lock_lock_with_flags_func_ptr = void (*)(void*, uint32_t); //maybe null, while siblings are avail always

            using os_sync_wait_on_address_func_ptr = int (*)(void* addr, uint64_t value, size_t size, uint32_t flags);
            using os_sync_wake_by_address_any_func_ptr = int (*)(void* addr, size_t size, uint32_t flags);
            using os_sync_wake_by_address_all_func_ptr = int (*)(void* addr, size_t size, uint32_t flags);

            struct OSSyncApis {
                unfair_lock_lock_with_flags_func_ptr unfair_lock_lock_with_flags = nullptr; //maybe null, while siblings are avail always

                os_sync_wait_on_address_func_ptr wait_on_address = nullptr;
                os_sync_wake_by_address_any_func_ptr wake_by_address_any = nullptr;
                os_sync_wake_by_address_all_func_ptr wake_by_address_all = nullptr;
            };

            inline const OSSyncApis* getOSSyncApis() {
                static OSSyncApis* p_apis = []() -> OSSyncApis* {
                    static OSSyncApis s_apis;
                    if (true) {
                        void* handle = dlopen("/usr/lib/system/libsystem_platform.dylib", RTLD_NOLOAD);
                        if (!handle)
                            handle = dlopen("/usr/lib/system/libsystem_platform.dylib", RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE); //不必释放

                        if (handle) {
                            s_apis.unfair_lock_lock_with_flags = reinterpret_cast<unfair_lock_lock_with_flags_func_ptr>(dlsym(handle, "os_unfair_lock_lock_with_flags"));
                            ASSERT((s_apis.unfair_lock_lock_with_flags)
                                || (!s_apis.unfair_lock_lock_with_flags)); //maybe null, while siblings are avail always

                            s_apis.wait_on_address = reinterpret_cast<os_sync_wait_on_address_func_ptr>(dlsym(handle, "os_sync_wait_on_address"));
                            s_apis.wake_by_address_any = reinterpret_cast<os_sync_wake_by_address_any_func_ptr>(dlsym(handle, "os_sync_wake_by_address_any"));
                            s_apis.wake_by_address_all = reinterpret_cast<os_sync_wake_by_address_all_func_ptr>(dlsym(handle, "os_sync_wake_by_address_all"));
                            ASSERT((s_apis.wait_on_address && s_apis.wake_by_address_any && s_apis.wake_by_address_all)
                                || (!s_apis.wait_on_address && !s_apis.wake_by_address_any && !s_apis.wake_by_address_all));
                        }
                    }

                    return &s_apis;
                }();
                return p_apis;
            }

            //unfair_lock_lock_with_flags参数
            constexpr uint32_t OS_UNFAIR_LOCK_FLAG_ADAPTIVE_SPIN = 0x00040000;

            // 阻塞等待标志
            constexpr uint32_t OSSYNC_WAIT_ON_ADDRESS_NONE = 0x0;
            constexpr uint32_t OSSYNC_WAIT_ON_ADDRESS_SHARED = 0x1;
            // 唤醒标志
            constexpr uint32_t OSSYNC_WAKE_BY_ADDRESS_NONE = 0x0;
            constexpr uint32_t OSSYNC_WAKE_BY_ADDRESS_SHARED = 0x1;

        } //namespace _helper
    } // namespace _KSConcurrencyImpl


#elif defined(__linux__)
    #include <unistd.h> 
    #include <errno.h>
    #include <sys/syscall.h>
    #include <linux/futex.h>

    namespace _KSConcurrencyImpl {
        namespace _helper {

            constexpr int _LINUX_FUTEX32_NOT_SUPPORTED = -79791779;  //magic ret-err-value

            //即使是64位系统，futex也只能操作32位的地址；
            //不过，就算对于uint64变量，通常在其低32位上进行futex操作也足够了。
            inline int linux_futex32_wait(uint32_t* uaddr, uint32_t val, const struct timespec *timeout) {
                long res = syscall(SYS_futex, uaddr, FUTEX_WAIT, val, timeout, nullptr, 0);
                if (res == long(-1) && errno == ENOSYS)
                    res = _LINUX_FUTEX32_NOT_SUPPORTED;
                return res;
            }

            inline void linux_futex32_wake_one(uint32_t* uaddr) {
                syscall(SYS_futex, uaddr, FUTEX_WAKE, 1, nullptr, nullptr, 0);
            }

            inline void linux_futex32_wake_all(uint32_t* uaddr) {
                syscall(SYS_futex, uaddr, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
            }

        } //namespace _helper
    } // namespace _KSConcurrencyImpl


#else
#   error "Unsupported platform for concurrency helper"
#endif


//for atomic
#include <thread>

namespace _KSConcurrencyImpl {
    namespace _helper {

    #if __cplusplus < 202002L
        template <class T>
        inline bool __atomic_is_wait_efficient(const std::atomic<T>* object) noexcept {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8, "__atomic_is_wait_efficient only supports 32-bit or 64-bit atomic types");
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

        template <class T>
        inline void __atomic_wait_explicit(const std::atomic<T>* object, T old, std::memory_order order) {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8, "__atomic_wait_explicit only supports 32-bit or 64-bit atomic types");
            while (object->load(order) == old) {
        #       if defined(_WIN32)
                if (_KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWaitOnAddress)
                    _KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWaitOnAddress((void*)object, (void*)&old, sizeof(T), INFINITE);
                else
                    std::this_thread::yield();
        #       elif defined(__APPLE__)
                if (_KSConcurrencyImpl::_helper::getOSSyncApis()->wait_on_address)
                    _KSConcurrencyImpl::_helper::getOSSyncApis()->wait_on_address((void*)object, (uint64_t)old, sizeof(T), _KSConcurrencyImpl::_helper::OSSYNC_WAIT_ON_ADDRESS_NONE);
                else
                    std::this_thread::yield();
        #       elif defined(__linux__)
                //即使T是64位，futex也只操作低32位的地址就够了
                if (_KSConcurrencyImpl::_helper::linux_futex32_wait((uint32_t*)object, (uint32_t)old, nullptr) == _KSConcurrencyImpl::_helper::_LINUX_FUTEX32_NOT_SUPPORTED)
                    std::this_thread::yield();
        #       else
                std::this_thread::yield();
        #       endif
            }
        }

        template <class T>
        inline void __atomic_notify_one(const std::atomic<T>* object) {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8, "__atomic_notify_one only supports 32-bit or 64-bit atomic types");
        #   if defined(_WIN32)
            if (_KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWakeByAddressSingle)
                _KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWakeByAddressSingle((void*)object);
            else
                (void)0; //noop
        #   elif defined(__APPLE__)
            if (_KSConcurrencyImpl::_helper::getOSSyncApis()->wake_by_address_any)
                _KSConcurrencyImpl::_helper::getOSSyncApis()->wake_by_address_any((void*)object, sizeof(T), _KSConcurrencyImpl::_helper::OSSYNC_WAKE_BY_ADDRESS_NONE);
            else
                (void)0; //noop
        #   elif defined(__linux__)
            //即使T是64位，futex也只操作低32位的地址就够了
            _KSConcurrencyImpl::_helper::linux_futex32_wake_one((uint32_t*)object);
        #   else
            (void)0; //noop
        #   endif
        }

        template <class T>
        inline void __atomic_notify_all(const std::atomic<T>* object) {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8, "__atomic_notify_all only supports 32-bit or 64-bit atomic types");
        #   if defined(_WIN32)
            if (_KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWakeByAddressAll)
                _KSConcurrencyImpl::_helper::getWinSynchApis()->pfnWakeByAddressAll((void*)object);
            else
                (void)0; //noop
        #   elif defined(__APPLE__)
            if (_KSConcurrencyImpl::_helper::getOSSyncApis()->wake_by_address_all)
                _KSConcurrencyImpl::_helper::getOSSyncApis()->wake_by_address_all((void*)object, sizeof(T), _KSConcurrencyImpl::_helper::OSSYNC_WAKE_BY_ADDRESS_NONE);
            else
                (void)0; //noop
        #   elif defined(__linux__)
            //即使T是64位，futex也只操作低32位的地址就够了
            _KSConcurrencyImpl::_helper::linux_futex32_wake_all((uint32_t*)object);
        #   else
            (void)0; //noop
        #   endif
        }

    #else //__cplusplus < 202002L

        template <class T>
        inline bool __atomic_is_wait_efficient(const std::atomic<T>* object) noexcept {
            return true;
        }

        template <class T>
        inline void __atomic_wait_explicit(const std::atomic<T>* object, T old, std::memory_order order) const {
            object->wait(old, order);
        }

        template <class T>
        inline void __atomic_notify_one(const std::atomic<T>* object) {
            object->notify_one();
        }

        template <class T>
        inline void __atomic_notify_all(const std::atomic<T>* object) {
            object->notify_all();
        }

    #endif //__cplusplus < 202002L

    } //namespace _helper
} // namespace _KSConcurrencyImpl


#endif // __KS_CONCURRENCY_HELPER_DEF
