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

TEST(test_future_suite, test_resolve) {
    ks_latch work_latch(0);
    work_latch.add(1);

    auto future_T= ks_future<int>::resolved(1);

    future_T.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        ASSERT_TRUE(result.is_value());
        EXPECT_EQ(result.to_value(), 1);
        work_latch.count_down();
    });

    work_latch.wait();
    work_latch.add(1);

    auto future_void= ks_future<void>::resolved();

    future_void.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        ASSERT_TRUE(result.is_value());
        EXPECT_EQ(_result_to_str(result),"VOID");
        work_latch.count_down();
    });

    work_latch.wait();
}

TEST(test_future_suite, test_reject) {
    ks_latch work_latch(0);
    work_latch.add(1);

    ks_error ret = ks_error().timeout_error();
    auto future= ks_future<std::string>::rejected(ret);

    future.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        ASSERT_TRUE(result.is_error());
        EXPECT_EQ(result.to_error().get_code(), 0xFF338002);
        work_latch.count_down();
     });

    work_latch.wait();
}

TEST(test_future_suite, test_wait) {
    ks_latch work_latch(0);
    work_latch.add(1);

    ks_future<void>::post(ks_apartment::background_sta(), {}, []() {
            ks_future<void>::post_delayed(ks_apartment::background_sta(), {}, []() {}, 100).__wait();
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<void>& result) -> void {
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_latch.count_down();
            });

        work_latch.wait();
}

TEST(test_future_suite, test_alive) {
    ks_latch work_latch(0);
    work_latch.add(1);

    bool work = false;

    struct OBJ {
        OBJ(int id) : m_id(id) { ++s_counter(); ++obj_create(); }
        ~OBJ() { --s_counter(); ++obj_free(); }
        _DISABLE_COPY_CONSTRUCTOR(OBJ);
        static std::atomic<int>& s_counter() { static std::atomic<int> s_n(0); return s_n; }
        static std::atomic<int>& obj_create() { static std::atomic<int> create(0); return create; }
        static std::atomic<int>& obj_free() { static std::atomic<int> fr(0); return fr; }
        const int m_id;
    };

    if (true) {
        auto obj1 = std::make_shared<OBJ>(1);

        ks_future<void>::post_delayed(
            ks_apartment::default_mta(), 
            make_async_context().bind_owner(std::move(obj1)).bind_controller(nullptr),
            [&work]() { work = true; },
            100
        ).on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
            ks_future<void>::post_delayed(
                ks_apartment::default_mta(), make_async_context(), [&work_latch]() {
                    work_latch.count_down(); 
                }, 100);
        });
    }

    work_latch.wait();
    EXPECT_TRUE(work);
    EXPECT_EQ(OBJ::s_counter(), 0);
    EXPECT_EQ(OBJ::obj_create(), 1);
    EXPECT_EQ(OBJ::obj_free(), 1);
}

TEST(test_future_suite, test_set_timeout) {
    ks_latch work_latch(0);
    work_latch.add(1);

    auto future_T = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() {
        return std::string("pass");
     }, 500);

    future_T.set_timeout(50);

    future_T.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) {
        ASSERT_TRUE(result.is_error());
        EXPECT_EQ(result.to_error().get_code(), 0xFF338002);
        work_latch.count_down();
     });

    work_latch.wait();
}

TEST(test_future_suite, test_then) {
    ks_latch work_latch(0);
    work_latch.add(1);

    ks_error ret = ks_error().timeout_error();
    std::atomic<int> sn = { 0 };
    ks_future<std::string>::rejected(ret)
        .then<std::string>(ks_apartment::default_mta(), make_async_context(), [p_sn = &sn](const std::string& value) {
                ++(*p_sn);
                return value + ".then";
         })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
                ASSERT_TRUE(result.is_error());
                EXPECT_EQ(result.to_error().get_code(), 0xFF338002);
                work_latch.count_down();
         });


    work_latch.wait();
    ASSERT_EQ(sn, 0);
    work_latch.add(1);

    ks_future<std::string>::resolved("a")
        .then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::string& value) {
        return value + ".then_R";
         })
        .then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::string& value) {
             return ks_result<std::string>(value + ".then_ks_result<R>");
         })
        .then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::string& value) {
             return ks_future<std::string>::resolved(value + ".then_ks_future<R>");
         })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
            EXPECT_EQ(_result_to_str(result), "a.then_R.then_ks_result<R>.then_ks_future<R>");
            work_latch.count_down();
        });

    work_latch.wait();
    work_latch.add(1);

    ks_future<void>::resolved()
        .then<void>(ks_apartment::default_mta(), make_async_context(), []() -> void {
        })
        .then<void>(ks_apartment::default_mta(), make_async_context(), []() -> ks_result<void> {
            return nothing;
        })
        .then<void>(ks_apartment::default_mta(), make_async_context(), []() -> ks_future<void> {
            return ks_future<void>::resolved();
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<void>& result) -> void {
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_latch.count_down();
        });

    work_latch.wait();
    work_latch.add(1);

    ks_future<void>::resolved()
        .then<void>(ks_apartment::default_mta(), make_async_context(), [](ks_cancel_inspector* inspector) -> ks_result<void> {
        if (inspector->check_cancel())
            return ks_error::cancelled_error();
        else
            return nothing;
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<void>& result) -> void {
        if(result.is_error())
            EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        else
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_latch.count_down();
        });

    work_latch.wait();
}

TEST(test_future_suite, test_transform) {
    ks_latch work_latch(0);
    work_latch.add(1);

    ks_error ret = ks_error().timeout_error();
    std::atomic<int> sn = { 0 };
    ks_future<std::string>::rejected(ret)
        .transform<std::string>(ks_apartment::default_mta(), make_async_context(), [p_sn = &sn](const ks_result<std::string>& result) {
                ++(*p_sn);
                return result.is_value() ? ks_result<std::string>(_result_to_str(result) + ".transform") : ks_result<std::string>(result.to_error());
         })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
                ASSERT_TRUE(result.is_error());
                EXPECT_EQ(result.to_error().get_code(), 0xFF338002);
                work_latch.count_down();
         });


    work_latch.wait();
    ASSERT_EQ(sn.load(), 1);
    work_latch.add(1);

    ks_future<std::string>::resolved("a")
        .transform<std::string>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<std::string>& result) {
        return (_result_to_str(result) + ".transform_R");
         })
        .transform<std::string>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<std::string>& result) {
        return result.is_value() ? ks_result<std::string>(_result_to_str(result) + ".transform_ks_result<R>") : ks_result<std::string>(result.to_error());
         })
        .transform<std::string>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<std::string>& result) {
        return result.is_value() ? ks_future<std::string>::resolved(_result_to_str(result) + ".transform_ks_future<R>") : ks_future<std::string>::rejected(result.to_error());
         })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
            EXPECT_EQ(_result_to_str(result), "a.transform_R.transform_ks_result<R>.transform_ks_future<R>");
            work_latch.count_down();
        });

    work_latch.wait();
    work_latch.add(1);

    ks_future<void>::resolved()
        .transform<void>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<void>&result) -> void {
        })
        .transform<void>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<void>&result) -> ks_result<void> {
            return nothing;
        })
        .transform<void>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<void>&result) -> ks_future<void> {
            return ks_future<void>::resolved();
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<void>& result) -> void {
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_latch.count_down();
        });

    work_latch.wait();
    work_latch.add(1);

    ks_future<void>::resolved()
        .transform<void>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<void>&result, ks_cancel_inspector* inspector) -> ks_result<void> {
        if (inspector->check_cancel())
            return ks_error::cancelled_error();
        else
            return nothing;
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<void>& result) -> void {
        if(result.is_error())
            EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        else
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_latch.count_down();
        });

    work_latch.wait();
}

TEST(test_future_suite, test_flat_then) {
    ks_latch work_latch(0);
    work_latch.add(1);

    ks_error ret = ks_error().timeout_error();
    std::atomic<int> sn = { 0 };
    ks_future<std::string>::rejected(ret)
        .flat_then<std::string>(ks_apartment::default_mta(), make_async_context(), [p_sn = &sn](const std::string& value) {
                ++(*p_sn);
                return ks_future<std::string>::resolved(value + ".then");
         })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
                ASSERT_TRUE(result.is_error());
                EXPECT_EQ(result.to_error().get_code(), 0xFF338002);
                work_latch.count_down();
         });


    work_latch.wait();
    ASSERT_EQ(sn.load(), 0);
    work_latch.add(1);

    ks_future<std::string>::resolved("a")
        .flat_then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::string& value) {
             return ks_future<std::string>::resolved(value + ".flat_then_ks_future<R>");
         })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
            EXPECT_EQ(_result_to_str(result), "a.flat_then_ks_future<R>");
            work_latch.count_down();
        });

    work_latch.wait();
    work_latch.add(1);

    ks_future<void>::resolved()
        .flat_then<void>(ks_apartment::default_mta(), make_async_context(), []() -> ks_future<void> {
            return ks_future<void>::resolved();
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<void>& result) -> void {
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_latch.count_down();
        });

    work_latch.wait();
    work_latch.add(1);

    ks_future<void>::resolved()
        .flat_then<void>(ks_apartment::default_mta(), make_async_context(), [](ks_cancel_inspector* inspector) -> ks_future<void> {
        if (inspector->check_cancel())
            return ks_future<void>::rejected(ks_error::cancelled_error());
        else
            return ks_future<void>::resolved();
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<void>& result) -> void {
        if(result.is_error())
            EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        else
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_latch.count_down();
        });

    work_latch.wait();
}

TEST(test_future_suite, test_flat_transform) {
    ks_latch work_latch(0);
    work_latch.add(1);

    ks_error ret = ks_error().timeout_error();
    std::atomic<int> sn = { 0 };
    ks_future<std::string>::rejected(ret)
        .flat_transform<std::string>(ks_apartment::default_mta(), make_async_context(), [p_sn = &sn](const ks_result<std::string>& result) {
                ++(*p_sn);
                return result.is_value() ? ks_result<std::string>(_result_to_str(result) + ".flat_transform") : ks_result<std::string>(result.to_error());
         })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
                ASSERT_TRUE(result.is_error());
                EXPECT_EQ(result.to_error().get_code(), 0xFF338002);
                work_latch.count_down();
         });


    work_latch.wait();
    ASSERT_EQ(sn.load(), 1);
    work_latch.add(1);

    ks_future<std::string>::resolved("a")
        .flat_transform<std::string>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<std::string>& result) {
        return (_result_to_str(result) + ".flat_transform_R");
         })
        .flat_transform<std::string>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<std::string>& result) {
        return result.is_value() ? ks_result<std::string>(_result_to_str(result) + ".flat_transform_ks_result<R>") : ks_result<std::string>(result.to_error());
         })
        .flat_transform<std::string>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<std::string>& result) {
        return result.is_value() ? ks_future<std::string>::resolved(_result_to_str(result) + ".flat_transform_ks_future<R>") : ks_future<std::string>::rejected(result.to_error());
         })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
            EXPECT_EQ(_result_to_str(result), "a.flat_transform_R.flat_transform_ks_result<R>.flat_transform_ks_future<R>");
            work_latch.count_down();
        });

    work_latch.wait();
    work_latch.add(1);

    ks_future<void>::resolved()
        .flat_transform<void>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<void>&result) -> ks_future<void> {
            return ks_future<void>::resolved();
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<void>& result) -> void {
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_latch.count_down();
        });

    work_latch.wait();
    work_latch.add(1);

    ks_future<void>::resolved()
        .flat_transform<void>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<void>&result, ks_cancel_inspector* inspector) -> ks_future<void> {
        if (inspector->check_cancel())
            return ks_future<void>::rejected(ks_error::cancelled_error());
        else
            return ks_future<void>::resolved();
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<void>& result) -> void {
        if(result.is_error())
            EXPECT_EQ(result.to_error().get_code(), 0xFF338003);
        else
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_latch.count_down();
        });

    work_latch.wait();
}

TEST(test_future_suite, test_succ_fail) {
    ks_latch work_latch(0);
    work_latch.add(1);

    std::atomic<int> sn = { 0 };
    std::atomic<int> cn = { 0 };
    ks_future<std::string>::resolved("a")
        .on_failure(ks_apartment::default_mta(), make_async_context(), [p_sn = &sn](const ks_error& error) -> void {
        ++(*p_sn);
        })
        .on_success(ks_apartment::default_mta(), make_async_context(), [p_cn=&cn](const auto& value) -> void {
            ++(*p_cn);
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
            EXPECT_EQ(_result_to_str(result), "a");
            work_latch.count_down();
        });

     work_latch.wait();
     ASSERT_EQ(sn, 0);
     ASSERT_EQ(cn, 1);
     work_latch.add(1);

     ks_future<std::string>::rejected(ks_error::eof_error())
        .on_failure(ks_apartment::default_mta(), make_async_context(), [p_sn = &sn](const ks_error& error) -> void {
        ++(*p_sn);
        })
        .on_success(ks_apartment::default_mta(), make_async_context(), [p_cn=&cn](const auto& value) -> void {
            ++(*p_cn);
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
            ASSERT_TRUE(result.is_error());
            EXPECT_EQ(result.to_error().get_code(), 0xFF339002);
            work_latch.count_down();
        });

    work_latch.wait();
    ASSERT_EQ(sn, 1);
    ASSERT_EQ(cn, 1);
}

TEST(test_future_suite, test_cast) {
    ks_latch work_latch(0);
    work_latch.add(1);

    ks_future<const char*>::resolved("a")
        .cast<std::string>()
        .then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::string& value) {
        return value + ".then_R";
         })
        .then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::string& value) {
             return ks_result<std::string>(value + ".then_ks_result<R>");
         })
        .then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::string& value) {
             return ks_future<std::string>::resolved(value + ".then_ks_future<R>");
         })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
            EXPECT_EQ(_result_to_str(result), "a.then_R.then_ks_result<R>.then_ks_future<R>");
            work_latch.count_down();
        });

    work_latch.wait();
    work_latch.add(1);

    ks_future<std::string>::resolved("a")
        .cast<void>()
        .then<void>(ks_apartment::default_mta(), make_async_context(), []() -> void {
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<void>& result) -> void {
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_latch.count_down();
        });

    work_latch.wait();
    work_latch.add(1);

    ks_future<std::string>::resolved("a")
        .cast<nothing_t>()
        .then<nothing_t>(ks_apartment::default_mta(), make_async_context(), [](nothing_t nothing) {
        return nothing;
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<nothing_t>& result) -> void {
            
            work_latch.count_down();
        });

    work_latch.wait();
}

TEST(test_future_suite, test_map) {
    ks_latch work_latch(0);
    work_latch.add(1);

    ks_future<std::string>::resolved("a")
        .map<std::string>( [](const std::string& value) {
             return value + ".map";
         })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
            EXPECT_EQ(_result_to_str(result), "a.map");
            work_latch.count_down();
        });

    work_latch.wait();
    work_latch.add(1);

    ks_future<std::string>::resolved("a")
        .map<int>( [](const std::string& value) {
        return 1;
         })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
            EXPECT_EQ(_result_to_str(result), "1");
            work_latch.count_down();
        });

    work_latch.wait();
}

TEST(test_future_suite, test_map_value) {
    ks_latch work_latch(0);
    work_latch.add(1);

    ks_future<std::string>::resolved("a")
        .map_value<int>(123)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
            EXPECT_EQ(_result_to_str(result), "123");
            work_latch.count_down();
        });

    work_latch.wait();
}

TEST(test_future_suite, test_methods_combination) {
    ks_latch work_latch(0);
    work_latch.add(1);

    std::atomic<int> success{ 0 };
    std::atomic<int> failure{ 0 };

      ks_future<std::string>::resolved("a")
        .cast<std::string>()
        .then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::string& value) {
            return value + ".then";
        })
        .transform<std::string>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<std::string>& result) -> ks_result<std::string> {
            return result.is_value() ? ks_result<std::string>(_result_to_str(result) + ".transform") : ks_result<std::string>(result.to_error());
        })
        .flat_then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::string& value) -> ks_future<std::string> {
            return ks_future<std::string>::resolved(value + ".flat_then");
        })
        .flat_transform<std::string>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<std::string>& result) -> ks_future<std::string> {
            return result.is_value() ? ks_future<std::string>::resolved(_result_to_str(result) + ".flat_transform") : ks_future<std::string>::rejected(result.to_error());
        })
        .on_success(ks_apartment::default_mta(), make_async_context(), [&success](const auto& value) -> void {
            success++;
        })
        .on_failure(ks_apartment::default_mta(), make_async_context(), [&failure](const ks_error& error) -> void {
            failure++;
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const auto& result) -> void {
            work_latch.count_down();
        });

    work_latch.wait();

    EXPECT_EQ(success, 1);
    EXPECT_EQ(failure, 0);

    work_latch.add(1);

    ks_future<void>::resolved().cast<nothing_t>()
        .cast<void>()
        .then<void>(ks_apartment::default_mta(), make_async_context(), []() -> void {
            
        })
        .transform<void>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<void>& result) -> void {
            
        })
        .on_failure(ks_apartment::default_mta(), make_async_context(), [&failure](const ks_error& error) -> void {
            failure++;
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [&work_latch](const ks_result<void>& result) -> void {
            EXPECT_EQ(_result_to_str(result), "VOID");
            work_latch.count_down();
        });

    work_latch.wait();
    EXPECT_EQ(failure, 0);
}
