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

TEST(test_parallel_suite, test_parallel) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    auto c = std::make_shared<std::atomic<int>>(0);
    ks_future_util
        ::parallel(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>{5, [c]() { ++(*c); }})
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

    *c = 0;

    work_wg.add(1);

    ks_future_util
        ::parallel(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>{5,[c]() { ++(*c);
                 auto result = ks_result<void>(nothing);
                 return result;
            }})
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

     *c = 0;

    work_wg.add(1);

    ks_future_util
        ::parallel(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>{5,[c]() { ++(*c);
                   return ks_future<void>::resolved(nothing);
            }})
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

    *c = 0;

    work_wg.add(1);

    ks_future_util
        ::parallel(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>(0,[c]() { ++(*c);
            }))
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 0);

    work_wg.add(1);

    ks_future_util
        ::parallel(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>{1, [c]() { ++(*c); }
            })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 1);
}

TEST(test_parallel_suite, test_parallel_n) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    auto c = std::make_shared<std::atomic<int>>(0);
    ks_future_util
        ::parallel_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c); },
            5)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

    *c = 0;

    work_wg.add(1);

    ks_future_util
        ::parallel_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c);
               auto result = ks_result<void>(nothing);
               return result;
            },
            5)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

     *c = 0;

    work_wg.add(1);

    ks_future_util
        ::parallel_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c);
             return ks_future<void>::resolved(nothing);
            },
            5)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

    *c = 0;

    work_wg.add(1);

    ks_future_util
        ::parallel_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c);
            },
            0)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 0);

    work_wg.add(1);

    ks_future_util
        ::parallel_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c);
            },
            1)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 1);
}

TEST(test_sequential_suite, test_sequential) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    auto c = std::make_shared<std::atomic<int>>(0);
    ks_future_util
        ::sequential(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>{5, [c]() { ++(*c); }})
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

    *c = 0;

    work_wg.add(1);

    ks_future_util
        ::sequential(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>{5,[c]() { ++(*c);
                 auto result = ks_result<void>(nothing);
                 return result;
            }})
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

     *c = 0;

    work_wg.add(1);

    ks_future_util
        ::sequential(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>{5,[c]() { ++(*c);
                   return ks_future<void>::resolved(nothing);
            }})
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

    *c = 0;

    work_wg.add(1);

    ks_future_util
        ::sequential(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>(0,[c]() { ++(*c);
            }))
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 0);

    work_wg.add(1);

    ks_future_util
        ::sequential(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>{1, [c]() { ++(*c); }
            })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 1);

 }

TEST(test_sequential_suite, test_sequential_n) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    auto c = std::make_shared<std::atomic<int>>(0);
    ks_future_util
        ::sequential_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c); },
            5)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

    *c = 0;

    work_wg.add(1);

    ks_future_util
        ::sequential_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c);
               auto result = ks_result<void>(nothing);
               return result;
            },
            5)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

     *c = 0;

    work_wg.add(1);

    ks_future_util
        ::sequential_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c);
             return ks_future<void>::resolved(nothing);
            },
            5)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 5);

    *c = 0;

    work_wg.add(1);

    ks_future_util
        ::sequential_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c);
            },
            0)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 0);

    work_wg.add(1);

    ks_future_util
        ::sequential_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c);
            },
            1)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    ASSERT_EQ(*c, 1);
}
