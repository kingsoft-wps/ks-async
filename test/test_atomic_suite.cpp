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

#include "test_base.h"
#include "test_help.h"

#include <chrono>
#include <cmath>
#include <thread>



TEST(test_atomic_suite, test_atomic_int_wait) {
    ks_atomic<int> x = ATOMIC_VAR_INIT(0);

    std::thread t1([&x]() {
        x.store(1);
        x.__notify_one();
    });
    std::thread t2([&x]() {
        x.store(2);
        x.__notify_all();
    });

    x.__wait(0);
    x.__wait(1);
    auto v = x.load();
    EXPECT_EQ(v, 2);

    t1.join();
    t2.join();
}

TEST(test_atomic_suite, test_atomic_flag_wait) {
    ks_atomic_flag x = { false };

    std::thread t1([&x]() {
        x.test_and_set();
        x.__notify_one();
        });

    x.__wait(false);
    auto v = x.test();
    EXPECT_EQ(v, true);

    t1.join();
}
