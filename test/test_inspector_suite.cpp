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

TEST(test_inspector_suite, test_inspector) {
     ks_latch work_latch(0);
    work_latch.add(1);

    ks_pending_trigger trigger;
    auto future = ks_future<void>::post_pending(ks_apartment::default_mta(), make_async_context(), [](ks_cancel_inspector* inspector) -> ks_result<void>{
        if (inspector->check_cancel())
            return ks_error::cancelled_error();
        else
            return nothing;
     },&trigger);

    future.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        ASSERT_TRUE(result.is_error());
        EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        work_latch.count_down();
     });

    future.try_cancel();

    work_latch.wait();
}
