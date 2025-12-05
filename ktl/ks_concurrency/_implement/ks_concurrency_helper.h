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

            namespace _WinSynchDefs {
                struct SRWLOCK { PVOID Ptr; };
                typedef void(WINAPI* AcquireSRWLockExclusiveFuncPtr)(struct SRWLOCK* SRWLock);
                typedef BOOLEAN(WINAPI* TryAcquireSRWLockExclusiveFuncPtr)(struct SRWLOCK* SRWLock);
                typedef void(WINAPI* ReleaseSRWLockExclusiveFuncPtr)(struct SRWLOCK* SRWLock);

                typedef void(WINAPI* WaitOnAddressFuncPtr)(volatile void*, void*, size_t, DWORD);
                typedef void(WINAPI* WakeByAddressSingleFuncPtr)(PVOID Address);
                typedef void(WINAPI* WakeByAddressAllFuncPtr)(PVOID Address);
            }

            struct _WinSynchApis {
                _WinSynchDefs::AcquireSRWLockExclusiveFuncPtr pfnAcquireSRWLockExclusive = nullptr;
                _WinSynchDefs::TryAcquireSRWLockExclusiveFuncPtr pfnTryAcquireSRWLockExclusive = nullptr;
                _WinSynchDefs::ReleaseSRWLockExclusiveFuncPtr pfnReleaseSRWLockExclusive = nullptr;

                _WinSynchDefs::WaitOnAddressFuncPtr pfnWaitOnAddress = nullptr;
                _WinSynchDefs::WakeByAddressSingleFuncPtr pfnWakeByAddressSingle = nullptr;
                _WinSynchDefs::WakeByAddressAllFuncPtr pfnWakeByAddressAll = nullptr;
            };

            inline const _WinSynchApis* __getWinSynchApis() {
                static _WinSynchApis* p_apis = []() -> _WinSynchApis* {
                    static _WinSynchApis s_apis;
                    if (true) {
                        HMODULE hModule = ::GetModuleHandleW(L"Kernel32.dll");
                        if (!hModule)
                            hModule = ::LoadLibraryW(L"Kernel32.dll"); //不必释放

                        if (hModule) {
                            s_apis.pfnAcquireSRWLockExclusive = reinterpret_cast<_WinSynchDefs::AcquireSRWLockExclusiveFuncPtr>(::GetProcAddress(hModule, "AcquireSRWLockExclusive"));
                            s_apis.pfnTryAcquireSRWLockExclusive = reinterpret_cast<_WinSynchDefs::TryAcquireSRWLockExclusiveFuncPtr>(::GetProcAddress(hModule, "TryAcquireSRWLockExclusive"));
                            s_apis.pfnReleaseSRWLockExclusive = reinterpret_cast<_WinSynchDefs::ReleaseSRWLockExclusiveFuncPtr>(::GetProcAddress(hModule, "ReleaseSRWLockExclusive"));
                            ASSERT((s_apis.pfnAcquireSRWLockExclusive && s_apis.pfnTryAcquireSRWLockExclusive && s_apis.pfnReleaseSRWLockExclusive)
                                || (!s_apis.pfnAcquireSRWLockExclusive && !s_apis.pfnTryAcquireSRWLockExclusive && !s_apis.pfnReleaseSRWLockExclusive));
                        }
                    }

                    if (true) {
                        HMODULE hModule = ::GetModuleHandleW(L"api-ms-win-core-synch-l1-2-0.dll");
                        if (!hModule)
                            hModule = ::LoadLibraryW(L"api-ms-win-core-synch-l1-2-0.dll"); //不必释放

                        if (hModule) {
                            s_apis.pfnWaitOnAddress = reinterpret_cast<_WinSynchDefs::WaitOnAddressFuncPtr>(::GetProcAddress(hModule, "WaitOnAddress"));
                            s_apis.pfnWakeByAddressSingle = reinterpret_cast<_WinSynchDefs::WakeByAddressSingleFuncPtr>(::GetProcAddress(hModule, "WakeByAddressSingle"));
                            s_apis.pfnWakeByAddressAll = reinterpret_cast<_WinSynchDefs::WakeByAddressAllFuncPtr>(::GetProcAddress(hModule, "WakeByAddressAll"));
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
            namespace _AppleSyncDefs {
                using unfair_lock_lock_with_flags_func_ptr = void (*)(void*, int); //maybe null, while siblings are avail always

                using os_sync_wait_on_address_func_ptr = int (*)(void* addr, uint64_t value, size_t size, int flags);
                using os_sync_wait_on_address_with_timeout_func_ptr = int (*)(void * addr, uint64_t value, size_t size, int flags, int clockid, uint64_t timeout_ns);
                using os_sync_wake_by_address_any_func_ptr = int (*)(void* addr, size_t size, int flags);
                using os_sync_wake_by_address_all_func_ptr = int (*)(void* addr, size_t size, int flags);

                //unfair_lock_lock_with_flags参数
                constexpr int OSUNFAIR_LOCK_FLAG_ADAPTIVE_SPIN = 0x00040000;
                // 阻塞等待标志
                constexpr int OSSYNC_WAIT_ON_ADDRESS_NONE = 0;
                constexpr int OSSYNC_WAIT_ON_ADDRESS_SHARED = 1;
                // 唤醒标志
                constexpr int OSSYNC_WAKE_BY_ADDRESS_NONE = 0;
                constexpr int OSSYNC_WAKE_BY_ADDRESS_SHARED = 1;
                // 绝对时钟ID
                constexpr int OSCLOCK_MACH_ABSOLUTE_TIME = 9;
            }

            struct _AppleSyncApis {
                _AppleSyncDefs::unfair_lock_lock_with_flags_func_ptr unfair_lock_lock_with_flags = nullptr; //maybe null, while siblings are avail always

                _AppleSyncDefs::os_sync_wait_on_address_func_ptr wait_on_address = nullptr;
                _AppleSyncDefs::os_sync_wait_on_address_with_timeout_func_ptr wait_on_address_with_timeout = nullptr;
                _AppleSyncDefs::os_sync_wake_by_address_any_func_ptr wake_by_address_any = nullptr;
                _AppleSyncDefs::os_sync_wake_by_address_all_func_ptr wake_by_address_all = nullptr;
            };

            inline const _AppleSyncApis* __getAppleSyncApis() {
                static _AppleSyncApis* p_apis = []() -> _AppleSyncApis* {
                    static _AppleSyncApis s_apis;
                    if (true) {
                        void* handle = dlopen("/usr/lib/system/libsystem_platform.dylib", RTLD_NOLOAD);
                        if (!handle)
                            handle = dlopen("/usr/lib/system/libsystem_platform.dylib", RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE); //不必释放

                        if (handle) {
                            s_apis.unfair_lock_lock_with_flags = reinterpret_cast<_AppleSyncDefs::unfair_lock_lock_with_flags_func_ptr>(dlsym(handle, "os_unfair_lock_lock_with_flags"));
                            ASSERT((s_apis.unfair_lock_lock_with_flags)
                                || (!s_apis.unfair_lock_lock_with_flags)); //maybe null, while siblings are avail always

                            s_apis.wait_on_address = reinterpret_cast<_AppleSyncDefs::os_sync_wait_on_address_func_ptr>(dlsym(handle, "os_sync_wait_on_address"));
                            s_apis.wait_on_address_with_timeout = reinterpret_cast<_AppleSyncDefs::os_sync_wait_on_address_with_timeout_func_ptr>(dlsym(handle, "os_sync_wait_on_address_with_timeout"));
                            s_apis.wake_by_address_any = reinterpret_cast<_AppleSyncDefs::os_sync_wake_by_address_any_func_ptr>(dlsym(handle, "os_sync_wake_by_address_any"));
                            s_apis.wake_by_address_all = reinterpret_cast<_AppleSyncDefs::os_sync_wake_by_address_all_func_ptr>(dlsym(handle, "os_sync_wake_by_address_all"));
                            ASSERT((s_apis.wait_on_address && s_apis.wait_on_address_with_timeout && s_apis.wake_by_address_any && s_apis.wake_by_address_all)
                                || (!s_apis.wait_on_address && !s_apis.wait_on_address_with_timeout && !s_apis.wake_by_address_any && !s_apis.wake_by_address_all));
                        }
                    }

                    return &s_apis;
                }();
                return p_apis;
            }

        } //namespace _helper
    } // namespace _KSConcurrencyImpl


#elif defined(__linux__)
    #include <unistd.h> 
    #include <errno.h>
    #include <sys/syscall.h>
    #include <linux/futex.h>

    namespace _KSConcurrencyImpl {
        namespace _helper {
            namespace _LinuxFutexDefs {
                constexpr int _FUTEX32_NOT_SUPPORTED = -79791779;  //magic ret-err-value
            }

            struct _LinuxFutexApis {
                static inline int futex32_wait(uint32_t* uaddr, uint32_t val, const struct timespec *timeout) {
                    long res = syscall(SYS_futex, uaddr, FUTEX_WAIT, val, timeout, nullptr, 0);
                    if (res == long(-1) && errno == ENOSYS)
                        res = _LinuxFutexDefs::_FUTEX32_NOT_SUPPORTED;
                    return res;
                }

                static inline void futex32_wake_one(uint32_t* uaddr) {
                    syscall(SYS_futex, uaddr, FUTEX_WAKE, 1, nullptr, nullptr, 0);
                }

                static inline void futex32_wake_all(uint32_t* uaddr) {
                    syscall(SYS_futex, uaddr, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
                }
            };

            static const _LinuxFutexApis* __getLinuxFutexApis() {
                static _LinuxFutexApis s_apis;
                return &s_apis;
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
        //我们自行实现atomic，且额外支持wait_until能力
        template <class T>
        inline bool __atomic_is_wait_efficient(const std::atomic<T>* object) noexcept {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8, "__atomic_is_wait_efficient only supports 32-bit or 64-bit atomic types");
        #   if defined(_WIN32)
                return (bool)(__getWinSynchApis()->pfnWaitOnAddress);
        #   elif defined(__APPLE__)
                return (bool)(__getAppleSyncApis()->wait_on_address);
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
                    if (__getWinSynchApis()->pfnWaitOnAddress) {
                        __getWinSynchApis()->pfnWaitOnAddress((void*)object, (void*)&old, sizeof(T), INFINITE);
                    }
                    else {
                        ASSERT(false);
                        std::this_thread::yield();
                    }
        #       elif defined(__APPLE__)
                    if (__getAppleSyncApis()->wait_on_address) {
                        __getAppleSyncApis()->wait_on_address((void*)object, (uint64_t)old, sizeof(T), _AppleSyncDefs::OSSYNC_WAIT_ON_ADDRESS_NONE);
                    }
                    else {
                        ASSERT(false);
                        std::this_thread::yield();
                    }
        #       elif defined(__linux__)
                //即使T是64位，futex也只操作低32位的地址就够了
                    if (__getLinuxFutexApis()->futex32_wait((uint32_t*)object, (uint32_t)old, nullptr) == _LinuxFutexDefs::_FUTEX32_NOT_SUPPORTED) {
                        ASSERT(false);
                        std::this_thread::yield();
                    }
        #       else
                    ASSERT(false);
                    std::this_thread::yield();
        #       endif
            }
        }

        template <class T, class Clock, class Duration>
        inline bool __atomic_wait_until_explicit(const std::atomic<T>* object, T old, const std::chrono::time_point<Clock, Duration>& abs_time, std::memory_order order) {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8, "__atomic_wait_explicit only supports 32-bit or 64-bit atomic types");
            while (object->load(order) == old) {
                const auto loop_time = Clock::now();
                if (loop_time >= abs_time)
                    return false; //timeout

        #       if defined(_WIN32)
                    if (__getWinSynchApis()->pfnWaitOnAddress) {
                        auto remain_time = abs_time - loop_time;
                        long long remain_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remain_time).count();
                        if (remain_ms < 0)
                            remain_ms = 0;
                        else if (remain_ms >= (long long)MAXDWORD)
                            remain_ms = (long long)MAXDWORD - 1; //可能分为多次wait
                        __getWinSynchApis()->pfnWaitOnAddress((void*)object, (void*)&old, sizeof(T), (DWORD)remain_ms);
                    }
                    else {
                        ASSERT(false);
                        std::this_thread::yield();
                    }
        #       elif defined(__APPLE__)
                    if (__getAppleSyncApis()->wait_on_address) {
                        auto remain_time = abs_time - loop_time;
                        long long remain_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(remain_time).count();
                        if (remain_ns < 0)
                            remain_ns = 0;

                        __getAppleSyncApis()->wait_on_address_with_timeout((void*)object, (uint64_t)old, sizeof(T), 
                            _AppleSyncDefs::OSSYNC_WAIT_ON_ADDRESS_NONE, _AppleSyncDefs::OSCLOCK_MACH_ABSOLUTE_TIME, (uint64_t)remain_ns);
                    }
                    else {
                        ASSERT(false);
                        std::this_thread::yield();
                    }
        #       elif defined(__linux__)
                    //即使T是64位，futex也只操作低32位的地址就够了
                    auto remain_time = abs_time - loop_time;
                    long long remain_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(remain_time).count();
                    if (remain_ns < 0)
                        remain_ns = 0;

                    struct timespec ts;
                    ts.tv_sec = remain_ns / 1000000000;
                    ts.tv_nsec = remain_ns % 1000000000;

                    if (__getLinuxFutexApis()->futex32_wait((uint32_t*)object, (uint32_t)old, &ts) == _LinuxFutexDefs::_FUTEX32_NOT_SUPPORTED) {
                        ASSERT(false);
                        std::this_thread::yield();
                    }
        #       else
                    ASSERT(false);
                    std::this_thread::yield();
        #       endif
            }

            return true;
        }

        template <class T>
        inline void __atomic_notify_one(const std::atomic<T>* object) {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8, "__atomic_notify_one only supports 32-bit or 64-bit atomic types");
        #   if defined(_WIN32)
                if (__getWinSynchApis()->pfnWakeByAddressSingle)
                    __getWinSynchApis()->pfnWakeByAddressSingle((void*)object);
                else
                    (void)0; //noop
        #   elif defined(__APPLE__)
                if (__getAppleSyncApis()->wake_by_address_any)
                    __getAppleSyncApis()->wake_by_address_any((void*)object, sizeof(T), _AppleSyncDefs::OSSYNC_WAKE_BY_ADDRESS_NONE);
                else
                    (void)0; //noop
        #   elif defined(__linux__)
                //即使T是64位，futex也只操作低32位的地址就够了
                __getLinuxFutexApis()->futex32_wake_one((uint32_t*)object);
        #   else
                (void)0; //noop
        #   endif
        }

        template <class T>
        inline void __atomic_notify_all(const std::atomic<T>* object) {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8, "__atomic_notify_all only supports 32-bit or 64-bit atomic types");
        #   if defined(_WIN32)
                if (__getWinSynchApis()->pfnWakeByAddressAll)
                    __getWinSynchApis()->pfnWakeByAddressAll((void*)object);
                else
                    (void)0; //noop
        #   elif defined(__APPLE__)
                if (__getAppleSyncApis()->wake_by_address_all)
                    __getAppleSyncApis()->wake_by_address_all((void*)object, sizeof(T), _AppleSyncDefs::OSSYNC_WAKE_BY_ADDRESS_NONE);
                else
                    (void)0; //noop
        #   elif defined(__linux__)
                //即使T是64位，futex也只操作低32位的地址就够了
                __getLinuxFutexApis()->futex32_wake_all((uint32_t*)object);
        #   else
                (void)0; //noop
        #   endif
        }

    } //namespace _helper
} // namespace _KSConcurrencyImpl


#endif // __KS_CONCURRENCY_HELPER_DEF
