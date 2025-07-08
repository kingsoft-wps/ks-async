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

#include <benchmark/benchmark.h>

#include "../ks_future.h"
#include "../ks_promise.h"
#include "../ks_future_util.h"
#include "../ks_async_flow.h"
#include "../ks_notification_center.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>

// 当前cpu线程数
inline int64_t cpu_thread() {
    static int64_t cpu_thread_count = std::thread::hardware_concurrency();
    return cpu_thread_count;
}