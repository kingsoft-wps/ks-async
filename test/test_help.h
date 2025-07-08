/* Copyright 2025 The Kingsoft's ks-async Authors. All Rights Reserved.

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

#ifndef TEST_HELP_H
#define TEST_HELP_H

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#endif

#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#endif


class CpuUsage {
public:
    static double GetCpuUsage() {
#if defined(_WIN32) || defined(_WIN64)
        return GetCpuUsageWindows();
#elif defined(__linux__)
        return GetCpuUsageLinux();
#elif defined(__APPLE__)
        return GetCpuUsageMac();
#else
        return -1.0; // 不支持的平台
#endif
    }

private:
#if defined(_WIN32) || defined(_WIN64)
    static double GetCpuUsageWindows() {
        static bool initialized = false;
        static FILETIME prevIdleTime = {0}, prevKernelTime = {0}, prevUserTime = {0};

        FILETIME idleTime, kernelTime, userTime;
        if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
            // Error retrieving times
            return -1.0;
        }

        if (!initialized) {
            // Initialize on first call
            prevIdleTime = idleTime;
            prevKernelTime = kernelTime;
            prevUserTime = userTime;
            initialized = true;
            return 0.0;
        }

        // Convert FILETIME (100-nanosecond intervals) to 64-bit values
        auto FileTimeToULL = [](const FILETIME &ft) {
            return (static_cast<unsigned long long>(ft.dwHighDateTime) << 32) |
                   ft.dwLowDateTime;
        };

        unsigned long long prevIdle = FileTimeToULL(prevIdleTime);
        unsigned long long prevKernel = FileTimeToULL(prevKernelTime);
        unsigned long long prevUser = FileTimeToULL(prevUserTime);

        unsigned long long currIdle = FileTimeToULL(idleTime);
        unsigned long long currKernel = FileTimeToULL(kernelTime);
        unsigned long long currUser = FileTimeToULL(userTime);

        unsigned long long idleDiff = currIdle - prevIdle;
        unsigned long long kernelDiff = currKernel - prevKernel;
        unsigned long long userDiff = currUser - prevUser;

        // Total system time is kernel + user
        unsigned long long totalSystem = kernelDiff + userDiff;
        if (totalSystem == 0) {
            return 0.0;
        }

        // CPU usage is non-idle time divided by total system time
        double cpuUsage = (static_cast<double>(totalSystem - idleDiff) * 100.0) /
                          static_cast<double>(totalSystem);

        // Store the current times for next calculation
        prevIdleTime = idleTime;
        prevKernelTime = kernelTime;
        prevUserTime = userTime;

        return cpuUsage;
    }
#elif defined(__linux__)
    static double GetCpuUsageLinux() {
        static bool initialized = false;
        static uint64_t prevIdle = 0, prevTotal = 0;
    
        std::ifstream statFile("/proc/stat");
        if (!statFile.is_open()) {
            return -1.0;
        }
    
        std::string line;
        std::getline(statFile, line);
        statFile.close();
    
        std::istringstream ss(line);
        std::string cpu;
        uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
        ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
        uint64_t idleAll = idle + iowait;
        uint64_t nonIdle = user + nice + system + irq + softirq + steal;
        uint64_t total = idleAll + nonIdle;
    
        if (!initialized) {
            prevIdle = idleAll;
            prevTotal = total;
            initialized = true;
            return 0.0;
        }
    
        uint64_t idleDiff = idleAll - prevIdle;
        uint64_t totalDiff = total - prevTotal;
    
        prevIdle = idleAll;
        prevTotal = total;
    
        if (totalDiff == 0) return 0.0;
        return (static_cast<double>(totalDiff - idleDiff) * 100.0) / static_cast<double>(totalDiff);
    }
#elif defined(__APPLE__)
    static double GetCpuUsageMac() {
        static bool initialized = false;
        static uint64_t prevIdle = 0, prevTotal = 0;

        host_cpu_load_info_data_t cpuinfo;
        mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
        if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, reinterpret_cast<host_info_t>(&cpuinfo), &count) != KERN_SUCCESS) {
            return -1.0;
        }

        uint64_t user = cpuinfo.cpu_ticks[CPU_STATE_USER];
        uint64_t system = cpuinfo.cpu_ticks[CPU_STATE_SYSTEM];
        uint64_t idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
        uint64_t nice = cpuinfo.cpu_ticks[CPU_STATE_NICE];

        uint64_t total = user + system + idle + nice;

        if (!initialized) {
            prevIdle = idle;
            prevTotal = total;
            initialized = true;
            return 0.0;
        }

        uint64_t idleDiff = idle - prevIdle;
        uint64_t totalDiff = total - prevTotal;

        prevIdle = idle;
        prevTotal = total;

        if (totalDiff == 0) return 0.0;
        return (static_cast<double>(totalDiff - idleDiff) * 100.0) / static_cast<double>(totalDiff);
    }
#endif
};


#endif // TEST_HELP_H