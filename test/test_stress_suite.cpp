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

TEST(test_stress_suite, test_parallel_excess) {
     ks_latch work_latch(0);
    work_latch.add(1);

    auto c = std::make_shared<std::atomic<int>>(0);
    ks_future_util
        ::parallel_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c); },
            FLAGS_parallel_num)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_latch.count_down();
        });

    work_latch.wait();
    ASSERT_EQ(*c, FLAGS_parallel_num);
}

TEST(test_stress_suite, test_burst_load) {
    ks_latch work_latch(0);
    work_latch.add(FLAGS_future_num);

    auto shared_counter = std::make_shared<std::atomic<int>>(0);
    for (int i = 0; i < FLAGS_future_num; ++i) {
        auto future = ks_future<std::string>::post(ks_apartment::default_mta(), make_async_context(), [shared_counter]() {
             ++(*shared_counter);
             return std::string("pass");
        });

        future.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
             EXPECT_EQ(_result_to_str(result), "pass");
             work_latch.count_down();
         });
    }

    work_latch.wait();
    ASSERT_EQ(*shared_counter, FLAGS_future_num);
}
