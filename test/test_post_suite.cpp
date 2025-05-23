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

TEST(test_post_suite, test_post) {
    ks_latch work_latch(0);
    work_latch.add(1);

    auto future_T = ks_future<std::string>::post(ks_apartment::default_mta(), make_async_context(), []() {
        return std::string("pass");
     });

    future_T.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "pass");
        work_latch.count_down();
     });

    work_latch.wait();
    work_latch.add(1);


    auto future_result = ks_future<std::string>::post(ks_apartment::default_mta(), make_async_context(), []() {
        auto result = ks_result<std::string>("pass");
        return result;
     });

    future_result.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "pass");
        work_latch.count_down();
     });

    work_latch.wait();
    work_latch.add(1);

    auto future_fu = ks_future<std::string>::post(ks_apartment::default_mta(), make_async_context(), []() {
        auto future= ks_future<std::string>::resolved("pass");
        return future;
     });

    future_fu.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "pass");
        work_latch.count_down();
     });

    work_latch.wait();
    work_latch.add(1);

    auto future_T_insp = ks_future<void>::post(ks_apartment::default_mta(), make_async_context(), [](ks_cancel_inspector* inspector) -> ks_result<void>{
        if (inspector->check_cancelled())
            return ks_error::cancelled_error();
        else
            return nothing;
     });

    future_T_insp.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        if(result.is_error())
            EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        else
            EXPECT_EQ(_result_to_str(result), "VOID");
        work_latch.count_down();
     });

    work_latch.wait();
    work_latch.add(1);

    auto future_fu_insp = ks_future<void>::post(ks_apartment::default_mta(), make_async_context(), [](ks_cancel_inspector* inspector) -> ks_future<void>{
        if (inspector->check_cancelled())
            return ks_future<void>::rejected(ks_error::cancelled_error());
        else
            return ks_future<void>::resolved();
     });

    future_fu_insp.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        if(result.is_error())
            EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        else
            EXPECT_EQ(_result_to_str(result), "VOID");
        work_latch.count_down();
     });

    work_latch.wait();
}

TEST(test_post_suite, test_post_pending) {
    ks_latch work_latch(0);
    work_latch.add(1);

    ks_pending_trigger trigger1;
    ks_pending_trigger trigger2;
    ks_pending_trigger trigger3;
    ks_pending_trigger trigger4;
    ks_pending_trigger trigger5;
    ks_pending_trigger trigger6;
    auto future_T = ks_future<std::string>::post_pending(ks_apartment::default_mta(), make_async_context(), []() {
        return std::string("pass");
     },&trigger1);

    future_T.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "pass");
        work_latch.count_down();
     });

    trigger1.start();

    work_latch.wait();
    work_latch.add(1);


    auto future_result = ks_future<std::string>::post_pending(ks_apartment::default_mta(), make_async_context(), []() {
        auto result = ks_result<std::string>("pass");
        return result;
     },&trigger2);

    future_result.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "pass");
        work_latch.count_down();
     });

    trigger2.start();

    work_latch.wait();
    work_latch.add(1);

    auto future_fu = ks_future<std::string>::post_pending(ks_apartment::default_mta(), make_async_context(), []() {
        auto future= ks_future<std::string>::resolved("pass");
        return future;
     },&trigger3);

    future_fu.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "pass");
        work_latch.count_down();
     });

    trigger3.start();

    work_latch.wait();
    work_latch.add(1);

    auto future_T_insp = ks_future<void>::post_pending(ks_apartment::default_mta(), make_async_context(), [](ks_cancel_inspector* inspector) -> ks_result<void>{
        if (inspector->check_cancelled())
            return ks_error::cancelled_error();
        else
            return nothing;
     },&trigger4);

    future_T_insp.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        if(result.is_error())
            EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        else
            EXPECT_EQ(_result_to_str(result), "VOID");
        work_latch.count_down();
     });

    trigger4.start();

    work_latch.wait();
    work_latch.add(1);

    auto future_fu_insp = ks_future<void>::post_pending(ks_apartment::default_mta(), make_async_context(), [](ks_cancel_inspector* inspector) -> ks_future<void>{
        if (inspector->check_cancelled())
            return ks_future<void>::rejected(ks_error::cancelled_error());
        else
            return ks_future<void>::resolved();
     },&trigger5);

    future_fu_insp.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        if(result.is_error())
            EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        else
            EXPECT_EQ(_result_to_str(result), "VOID");
        work_latch.count_down();
     });

    trigger5.start();

    work_latch.wait();
    work_latch.add(1);

    auto future_cancel = ks_future<void>::post_pending(ks_apartment::default_mta(), make_async_context(), []() -> ks_future<void>{
            return ks_future<void>::resolved();
     },&trigger6);

    future_cancel.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        if(result.is_error())
            EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        else
            EXPECT_EQ(_result_to_str(result), "VOID");
        work_latch.count_down();
     });

    trigger6.cancel();

    work_latch.wait();
}

TEST(test_post_suite, test_post_delayed) {
    ks_latch work_latch(0);
    work_latch.add(1);

    const auto post_time1 = std::chrono::steady_clock::now();
    auto future_T = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), [post_time1]() {
        auto duration = std::chrono::steady_clock::now() - post_time1;
        int64_t real_delay = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        EXPECT_LE(real_delay, 230);
        return std::string("pass");
     }, 200);

    future_T.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "pass");
        work_latch.count_down();
     });

    work_latch.wait();
    work_latch.add(1);

    const auto post_time2 = std::chrono::steady_clock::now();
    auto future_result = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), [post_time2]() {
        auto duration = std::chrono::steady_clock::now() - post_time2;
        int64_t real_delay = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        EXPECT_LE(real_delay, 230);
        auto result = ks_result<std::string>("pass");
        return result;
     }, 200);

    future_result.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "pass");
        work_latch.count_down();
     });

    work_latch.wait();
    work_latch.add(1);

    const auto post_time3 = std::chrono::steady_clock::now();
    auto future_fu = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), [post_time3]() {
        auto duration = std::chrono::steady_clock::now() - post_time3;
        int64_t real_delay = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        EXPECT_LE(real_delay, 230);
        auto future= ks_future<std::string>::resolved("pass");
        return future;
     }, 200);

    future_fu.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "pass");
        work_latch.count_down();
     });

    work_latch.wait();
    work_latch.add(1);

    const auto post_time4 = std::chrono::steady_clock::now();
    auto future_T_insp = ks_future<void>::post_delayed(ks_apartment::default_mta(), make_async_context(), [post_time4](ks_cancel_inspector* inspector) -> ks_result<void>{
        auto duration = std::chrono::steady_clock::now() - post_time4;
        int64_t real_delay = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        EXPECT_LE(real_delay, 230);
        if (inspector->check_cancelled())
            return ks_error::cancelled_error();
        else
            return nothing;
     }, 200);

    future_T_insp.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        if(result.is_error())
            EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        else
            EXPECT_EQ(_result_to_str(result), "VOID");
        work_latch.count_down();
     });

    work_latch.wait();
    work_latch.add(1);

    const auto post_time5 = std::chrono::steady_clock::now();
    auto future_fu_insp = ks_future<void>::post_delayed(ks_apartment::default_mta(), make_async_context(), [post_time5](ks_cancel_inspector* inspector) -> ks_future<void>{
        auto duration = std::chrono::steady_clock::now() - post_time5;
        int64_t real_delay = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        EXPECT_LE(real_delay, 230);
        if (inspector->check_cancelled())
            return ks_future<void>::rejected(ks_error::cancelled_error());
        else
            return ks_future<void>::resolved();
     }, 200);

    future_fu_insp.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        if(result.is_error())
            EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        else
            EXPECT_EQ(_result_to_str(result), "VOID");
        work_latch.count_down();
     });

    work_latch.wait();
}
