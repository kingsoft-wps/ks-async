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

TEST(test_future_util_suite, test_repeat) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    std::atomic<int> sn = { 0 };

    auto fn = [p_sn = &sn]() -> ks_result<void> {
        int sn_val = ++(*p_sn);
        if (sn_val <= 5) {
            return nothing;
        }
        else {
            return ks_error::eof_error();
        }
    };

    ks_future_util
        ::repeat(ks_apartment::default_mta(), fn)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
               EXPECT_EQ(_result_to_str(result), "VOID");
               work_wg.done();
            });

    work_wg.wait();
    EXPECT_EQ(sn, 6);

    sn = 0;
    work_wg.add(1);

    auto fn2 = [p_sn = &sn]() -> ks_future<void> {
        int sn_val = ++(*p_sn);
        if (sn_val <= 5) {
            return ks_future<void>::resolved();
        }
        else {
            return ks_future<void>::rejected(ks_error::eof_error());
        }
    };

    ks_future_util
        ::repeat(ks_apartment::default_mta(), fn2)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
               EXPECT_EQ(_result_to_str(result), "VOID");
               work_wg.done();
            });

    work_wg.wait();
    EXPECT_EQ(sn, 6);
}


TEST(test_future_util_suite, test_repeat_periodic) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    std::atomic<int> sn = { 0 };

    auto fn = [p_sn = &sn]() -> ks_result<void> {
        int sn_val = ++(*p_sn);
        if (sn_val <= 5) {
            return nothing;
        }
        else {
            return ks_error::eof_error();
        }
    };

    ks_future_util
        ::repeat_periodic(ks_apartment::default_mta(), fn, 0, 100)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_wg.done();
        });

    work_wg.wait();

    EXPECT_EQ(sn, 6);

     sn = 0;
    work_wg.add(1);

    auto fn2 = [p_sn = &sn]() -> ks_future<void> {
        int sn_val = ++(*p_sn);
        if (sn_val <= 5) {
            return ks_future<void>::resolved();
        }
        else {
            return ks_future<void>::rejected(ks_error::eof_error());
        }
    };

    ks_future_util
        ::repeat_periodic(ks_apartment::default_mta(), fn2, 0, 100)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
               EXPECT_EQ(_result_to_str(result), "VOID");
               work_wg.done();
            });

    work_wg.wait();
    EXPECT_EQ(sn, 6);
}


TEST(test_future_util_suite, test_repeat_productive) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    std::atomic<int> sn = { 0 };
    std::atomic<int> cn = { 0 };

    auto produce_fn = [p_sn = &sn]() -> ks_result<int> {
        int sn_val = ++(*p_sn);
        if (sn_val <= 5) {
            return sn_val;
        }
        else {
            return ks_error::eof_error();
        }
    };

    auto produce_fn2 = [p_sn = &sn]() -> ks_future<int> {
        int sn_val = ++(*p_sn);
        if (sn_val <= 5) {
            return ks_future<int>::resolved(sn_val);
        }
        else {
            return ks_future<int>::rejected(ks_error::eof_error());
        }
    };

    auto consume_fn = [p_cn=&cn](const int& sn_val) -> void {
        ++(*p_cn);
    };

    auto consume_fn2 = [p_cn=&cn](const int& sn_val) -> ks_result<void> {
        ++(*p_cn);
        return ks_result<void>(nothing);
    };

    auto consume_fn3 = [p_cn=&cn](const int& sn_val) -> ks_future<void> {
        ++(*p_cn);
        return ks_future<void>::resolved();
    };

    ks_future_util
        ::repeat_productive<int>(
            ks_apartment::default_mta(), produce_fn,
            ks_apartment::default_mta(), consume_fn)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();

    EXPECT_EQ(sn, 6);
    EXPECT_EQ(cn, 5);

    sn = 0;
    cn = 0;
    work_wg.add(1);

    ks_future_util
        ::repeat_productive<int>(
            ks_apartment::default_mta(), produce_fn2,
            ks_apartment::default_mta(), consume_fn2)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();
    EXPECT_EQ(sn, 6);
    EXPECT_EQ(cn, 5);

    sn = 0;
    cn = 0;
    work_wg.add(1);

    ks_future_util
        ::repeat_productive<int>(
            ks_apartment::default_mta(), produce_fn,
            ks_apartment::default_mta(), consume_fn3)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
           EXPECT_EQ(_result_to_str(result), "VOID");
           work_wg.done();
        });

    work_wg.wait();

    EXPECT_EQ(sn, 6);
    EXPECT_EQ(cn, 5);
}


TEST(test_future_util_suite, test_all) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    auto f1 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "a";
        }, 100);
    auto f2 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "b";
        }, 80);
    auto f3 = ks_future<int>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> int {
        return 3;
        }, 120);

    ks_future_util::all(f1, f2, f3)
        .then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::tuple<std::string, std::string, int>& valueTuple) -> std::string {
        std::stringstream ss;
        ss << std::get<0>(valueTuple) << std::get<1>(valueTuple) << std::get<2>(valueTuple);
        return ss.str();
            })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
                EXPECT_EQ(_result_to_str(result), "ab3");
                work_wg.done();
            });

    work_wg.wait();
}

TEST(test_future_util_suite, test_all_vector) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    auto f1 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "a";
        }, 100);
    auto f2 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "b";
        }, 80);
    auto f3 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "c";
        }, 120);

    std::vector<ks_future<std::string>> f_vec;
    f_vec.push_back(f1);
    f_vec.push_back(f2);
    f_vec.push_back(f3);

    ks_future_util::all(f_vec)
        .then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::vector<std::string> &valueVector) -> std::string {
        std::stringstream ss;
        ss << valueVector[0] << valueVector[1] << valueVector[2];
        return ss.str();
            })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
                EXPECT_EQ(_result_to_str(result), "abc");
                work_wg.done();
            });

    work_wg.wait();
}

TEST(test_future_util_suite, test_all_void) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    auto f1 = ks_future<void>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() {
        
        }, 100);
    auto f2 = ks_future<void>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() {
        
        }, 80);
    auto f3 = ks_future<void>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() {
        
        }, 120);

    ks_future_util::all(f1, f2, f3)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
                EXPECT_EQ(_result_to_str(result), "VOID");
                work_wg.done();
            });

    work_wg.wait();
    work_wg.add(1);

    auto f4 = ks_future<void>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() {
        
        }, 100);
    auto f5 = ks_future<void>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() {
        
        }, 80);
    auto f6 = ks_future<void>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() {
        
        }, 120);

    std::vector<ks_future<void>> f_vec;
    f_vec.push_back(f4);
    f_vec.push_back(f5);
    f_vec.push_back(f6);

    ks_future_util::all(f_vec)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
                EXPECT_EQ(_result_to_str(result), "VOID");
                work_wg.done();
            });

    work_wg.wait();
}


TEST(test_future_util_suite, test_any) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    auto f1 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "a";
        }, 100);
    auto f2 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "b";
        }, 50);
    auto f3 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "c";
        }, 120);

    ks_future_util::any(f1, f2, f3)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "b");
        work_wg.done();
            });

    work_wg.wait();
}

TEST(test_future_util_suite, test_any_vector) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    auto f1 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "a";
        }, 100);
    auto f2 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "b";
        }, 50);
    auto f3 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "c";
        }, 120);

    std::vector<ks_future<std::string>> f_vec;
    f_vec.push_back(f1);
    f_vec.push_back(f2);
    f_vec.push_back(f3);

    ks_future_util::any(f_vec)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg](const auto& result) {
        EXPECT_EQ(_result_to_str(result), "b");
        work_wg.done();
            });

    work_wg.wait();
}
