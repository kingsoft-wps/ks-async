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



TEST(test_event_suite, test_manual_event) {
    ks_event ev(true, true);
    EXPECT_TRUE(ev.try_wait());
    EXPECT_TRUE(ev.try_wait());

    ev.reset_event();
    EXPECT_FALSE(ev.try_wait());

    std::thread t([&ev]() {
        EXPECT_FALSE(ev.try_wait());
        ev.set_event();
    });

    ev.wait();
    EXPECT_TRUE(ev.try_wait());
    EXPECT_TRUE(ev.try_wait());

    t.join();
}

TEST(test_event_suite, test_auto_event) {
    ks_event ev(true, false);
    EXPECT_TRUE(ev.try_wait());
    EXPECT_FALSE(ev.try_wait());

    ev.set_event();
    ev.reset_event();
    EXPECT_FALSE(ev.try_wait());

    std::thread t([&ev]() {
        ev.set_event();
    });

    ev.wait();
    EXPECT_FALSE(ev.try_wait());

    t.join();
}
