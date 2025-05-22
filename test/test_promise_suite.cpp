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

#include "test_base.h"
#include <thread>

TEST(test_promise_suite, test_promise) {
    ks_latch work_latch(0);
    work_latch.add(1);

    auto promise_succ = ks_promise<std::string>::create();
    promise_succ.get_future()
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "pass");
        work_latch.count_down();
            });

    std::thread([promise_succ]() {
        promise_succ.resolve("pass");
        }).join();

    work_latch.wait();
    work_latch.add(1);

    auto promise_error = ks_promise<std::string>::create();
    ks_error ret = ks_error().unexpected_error();

    promise_error.get_future()
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        ASSERT_TRUE(result.is_error());
        EXPECT_EQ(result.to_error().get_code(), 0xFF338001);
        work_latch.count_down();
            });

    std::thread([promise_error,&ret]() {
        promise_error.reject(ret);
        }).join();

    work_latch.wait();
    work_latch.add(1);

    auto promise_sett = ks_promise<std::string>::create();
    promise_succ.get_future()
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "pass");
        work_latch.count_down();
            });

    std::thread([promise_sett]() {
        promise_sett.try_settle(ks_result<std::string>("pass"));
        }).join();

    work_latch.wait();
}
