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
#include "../ks_error.h"

TEST(test_async_flow_suite, test_is_completed) {
    ks_waitgroup work_wg(0);

    ks_async_flow flow;
    work_wg.add(1);
    bool b;
    uint64_t id;

    b = flow.add_task<std::string>("a1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return "a1-tasktask";
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("a2", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return ks_result<std::string>("a2-tasktask");
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("b1: a1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        EXPECT_TRUE(this_flow.is_task_completed("a1"));
        return ks_future<std::string>::resolved("b1-tasktask");
        });
    ASSERT_TRUE(b);

    id = flow.add_flow_completed_observer(ks_apartment::background_sta(), [&work_wg](const ks_async_flow& this_flow, const ks_error& error) {
        EXPECT_EQ(error.get_code(), 0);
        work_wg.done();
    }, ks_async_context());
    ASSERT_TRUE(id != 0);

    flow.start();

    work_wg.wait();
    ASSERT_TRUE(flow.is_flow_completed());
    ASSERT_TRUE(flow.is_task_completed("a1"));
    ASSERT_TRUE(flow.is_task_completed("a2"));
    ASSERT_TRUE(flow.is_task_completed("b1"));
}

TEST(test_async_flow_suite, test_task_observer) {
    ks_waitgroup work_wg(0);

    ks_async_flow flow;
    bool b;
    uint64_t id;
    std::atomic<int> task_running_count{ 0 };
    std::atomic<int> task_completed_count{ 0 };
    bool a1_completed;
    bool b1_running;

    flow.put_custom_value<int>("v1", 1);
    ASSERT_TRUE(flow.get_value<int>("v1") == 1);

    b = flow.add_task<std::string>("a1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return "a1-tasktask";
        });
    ASSERT_TRUE(b);
    work_wg.add(1);

    b = flow.add_task<std::string>("a2", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return ks_result<std::string>("a2-tasktask");
        });
    ASSERT_TRUE(b);
    work_wg.add(1);

    b = flow.add_task<std::string>("b1: a1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        EXPECT_TRUE(this_flow.is_task_completed("a1"));
        return ks_future<std::string>::resolved("b1-tasktask");
        });
    ASSERT_TRUE(b);
    work_wg.add(1);

    id = flow.add_task_running_observer("*", ks_apartment::background_sta(), [&task_running_count](const ks_async_flow& this_flow, const char* task_name) {
        task_running_count++;
    }, ks_async_context());
    ASSERT_TRUE(id != 0);

    id = flow.add_task_running_observer("b1", ks_apartment::background_sta(), [&b1_running](const ks_async_flow& this_flow, const char* task_name) {
        b1_running=true;
    }, ks_async_context());
    ASSERT_TRUE(id != 0);

    id = flow.add_task_completed_observer("*", ks_apartment::background_sta(), [&task_completed_count,&work_wg](const ks_async_flow& this_flow, const char* task_name, const ks_error& error) {
        if (!error.has_code())
            task_completed_count++;
        work_wg.done();
        }, ks_async_context());
    ASSERT_TRUE(id != 0);

     id = flow.add_task_completed_observer("a1", ks_apartment::background_sta(), [&a1_completed](const ks_async_flow& this_flow, const char* task_name, const ks_error& error) {
        if (!error.has_code())
            a1_completed=true;
        }, ks_async_context());
    ASSERT_TRUE(id != 0);

    flow.start();

    work_wg.wait();

    EXPECT_EQ(task_running_count, 3);
    EXPECT_EQ(task_completed_count, 3);
    EXPECT_TRUE(a1_completed);
    EXPECT_TRUE(b1_running);
}

TEST(test_async_flow_suite, test_flow_observer) {
    ks_waitgroup work_wg(0);

    ks_async_flow flow1, flow2;
    work_wg.add(2);
    bool b;
    uint64_t id;

    std::atomic<int> flow_running_count{ 0 };
    std::atomic<int> flow_completed_count{ 0 };

    b = flow1.add_task<std::string>("a1", ks_apartment::default_mta(), [](const ks_async_flow& flow) {
        return "a1-tasktask";
        });
    ASSERT_TRUE(b);

    b = flow1.add_task<std::string>("a2", ks_apartment::default_mta(), [](const ks_async_flow& flow) {
        return ks_result<std::string>("a2-tasktask");
        });
    ASSERT_TRUE(b);

    b = flow1.add_task<std::string>("b1: a1", ks_apartment::default_mta(), [](const ks_async_flow& flow) {
        EXPECT_TRUE(flow.is_task_completed("a1"));
        return ks_future<std::string>::resolved("b1-tasktask");
        });
    ASSERT_TRUE(b);

    id = flow1.add_flow_running_observer(ks_apartment::background_sta(), [&flow_running_count](const ks_async_flow& flow) {
        flow_running_count++;
    }, ks_async_context());
    ASSERT_TRUE(id != 0);
    
    id = flow2.add_flow_running_observer(ks_apartment::background_sta(), [&flow_running_count](const ks_async_flow& flow) {
        flow_running_count++;
    }, ks_async_context());
    ASSERT_TRUE(id != 0);

    id = flow1.add_flow_completed_observer(ks_apartment::background_sta(), [&flow_completed_count,&work_wg](const ks_async_flow& flow, const ks_error& error) {
        flow_completed_count++;
        EXPECT_EQ(error.get_code(), 0);
        work_wg.done();
    }, ks_async_context());
    ASSERT_TRUE(id != 0);

    id = flow2.add_flow_completed_observer(ks_apartment::background_sta(), [&flow_completed_count,&work_wg](const ks_async_flow& flow, const ks_error& error) {
        flow_completed_count++;
        EXPECT_EQ(error.get_code(), 0);
        work_wg.done();
    }, ks_async_context());
    ASSERT_TRUE(id != 0);

    flow1.start();
    flow2.start();

    work_wg.wait();
    EXPECT_EQ(flow_running_count, 2);
    EXPECT_EQ(flow_completed_count, 2);
}

TEST(test_async_flow_suite, test_get_future) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    ks_async_flow flow;
    bool b;
    uint64_t id;
    std::atomic<int> park{ 0 };
    std::atomic<int> task_future_count{ 0 };

    b = flow.add_task<std::string>("a1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return "a1-tasktask";
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("a2", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return ks_result<std::string>("a2-tasktask");
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("b1: a1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return ks_future<std::string>::resolved("b1-tasktask");
        });
    ASSERT_TRUE(b);

    id = flow.add_flow_completed_observer(ks_apartment::background_sta(), [&work_wg](const ks_async_flow& this_flow, const ks_error& error) {
        EXPECT_EQ(error.get_code(), 0);
        work_wg.done();
    }, ks_async_context());
    ASSERT_TRUE(id != 0);

    ks_future<ks_async_flow> flow_future=flow.get_flow_future();
    ks_future<std::string> future_a1 = flow.get_task_future<std::string>("a1");
    ks_future<std::string> future_a2 = flow.get_task_future<std::string>("a2");
    ks_future<std::string> future_b1 = flow.get_task_future<std::string>("b1");
    flow.start();

    work_wg.wait();
    EXPECT_EQ(flow.peek_task_result<std::string>("a1").to_value(), "a1-tasktask");
    EXPECT_EQ(flow.peek_task_result<std::string>("a2").to_value(), "a2-tasktask");
    EXPECT_EQ(flow.peek_task_result<std::string>("b1").to_value(), "b1-tasktask");

    work_wg.add(4);

    flow_future.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg,&park](const auto& result) {
        ASSERT_TRUE(result.is_value());
        park++;
        work_wg.done();
     });


    future_a1.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg,&task_future_count](const auto& result) {
        ASSERT_TRUE(result.is_value());
        task_future_count++;
        work_wg.done();
     });

    future_a2.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg,&task_future_count](const auto& result) {
        ASSERT_TRUE(result.is_value());
        task_future_count++;
        work_wg.done();
     });

    future_b1.on_completion(ks_apartment::default_mta(), make_async_context(), [&work_wg,&task_future_count](const auto& result) {
        ASSERT_TRUE(result.is_value());
        task_future_count++;
        work_wg.done();
     });

    work_wg.wait();
    EXPECT_EQ(park, 1);
    EXPECT_EQ(task_future_count, 3);
}

TEST(test_async_flow_suite, test_error) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    ks_async_flow flow, flow_error;
    bool b;
    uint64_t id;
    std::atomic<int> a3{ 0 };

    b = flow.add_task<std::string>("a1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return "a1-tasktask";
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("a2", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return ks_error().unexpected_error();
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("a3: a2", ks_apartment::default_mta(), [&a3](const ks_async_flow& this_flow) {
        a3++;
        return "a3-tasktask";
        });
    ASSERT_TRUE(b);

    id = flow.add_flow_completed_observer(ks_apartment::background_sta(), [&work_wg](const ks_async_flow& this_flow, const ks_error& error) {
        EXPECT_EQ(error.get_code(), 0xFF338001);
        work_wg.done();
    }, ks_async_context());
    ASSERT_TRUE(id != 0);

    flow.start();

    work_wg.wait();

    EXPECT_EQ(a3, 0);
    ASSERT_TRUE(flow.is_flow_completed());
    EXPECT_EQ(flow.get_last_error().get_code(), ks_error::unexpected_error().get_code());
    EXPECT_EQ(flow.peek_task_error("a2").get_code(), ks_error::unexpected_error().get_code());
    EXPECT_EQ(flow.peek_task_error("a3").get_code(), ks_error::unexpected_error().get_code());
    EXPECT_TRUE(flow.is_task_completed("a1"));
    EXPECT_TRUE(flow.is_task_completed("a2"));
    EXPECT_TRUE(flow.is_task_completed("a3"));
    EXPECT_EQ(flow.get_last_failed_task_name(),"a2");

    work_wg.add(1);

    b = flow_error.add_task<std::string>("c1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return ks_error().unexpected_error();
        });
    ASSERT_TRUE(b);

     id = flow_error.add_flow_completed_observer(ks_apartment::background_sta(), [&work_wg](const ks_async_flow& this_flow, const ks_error& error) {
        EXPECT_EQ(error.get_code(), 0xFF338001);
        work_wg.done();
    }, ks_async_context());
    ASSERT_TRUE(id != 0);

    flow_error.start();

    work_wg.wait();
    ASSERT_TRUE(flow_error.is_flow_completed());
    EXPECT_EQ(flow_error.get_last_error().get_code(), ks_error::unexpected_error().get_code());
    EXPECT_TRUE(flow_error.is_task_completed("c1"));
    EXPECT_EQ(flow_error.get_last_failed_task_name(),"c1");
}



TEST(test_async_flow_suite, test_try_cancel) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    ks_async_flow flow;
    bool b;
    uint64_t id;

    b = flow.add_task<std::string>("a1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return "a1-tasktask";
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("a2: a1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return ks_result<std::string>("a2-tasktask");
        });
    ASSERT_TRUE(b);

    id = flow.add_flow_completed_observer(ks_apartment::background_sta(), [&work_wg](const ks_async_flow& this_flow, const ks_error& error) {
        EXPECT_EQ(error.get_code(), 0xFF338003);
        work_wg.done();
    }, ks_async_context());
    ASSERT_TRUE(id != 0);

    flow.__try_cancel();
    
    work_wg.wait();
    EXPECT_TRUE(flow.is_flow_completed());
    EXPECT_EQ(flow.get_last_error().get_code(), ks_error::cancelled_error().get_code());
    EXPECT_TRUE(flow.is_task_completed("a1"));
    EXPECT_TRUE(flow.is_task_completed("a2"));
    EXPECT_EQ(flow.get_last_failed_task_name(), "a1");
}

TEST(test_async_flow_suite, test_complex_dependent) {
    ks_waitgroup work_wg(0);
    work_wg.add(1);

    ks_async_flow flow;
    bool b;
    uint64_t id;

    flow.put_custom_value<int>("v1", 1);
    ASSERT_TRUE(flow.get_value<int>("v1") == 1);

    b = flow.add_task<std::string>("a1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return "a1-tasktask";
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("a2", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return ks_result<std::string>("a2-tasktask");
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("b1: a1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        EXPECT_TRUE(this_flow.is_task_completed("a1"));
        return ks_future<std::string>::resolved("b1-tasktask");
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("b2: a2,b1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        EXPECT_TRUE(this_flow.is_task_completed("a2"));
        EXPECT_TRUE(this_flow.is_task_completed("b1"));
        return ks_future<std::string>::resolved("b2-tasktask");
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("b4: b1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        EXPECT_TRUE(this_flow.is_task_completed("b1"));
        return ks_future<std::string>::resolved("b4-tasktask");
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("b3: b1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        EXPECT_TRUE(this_flow.is_task_completed("b1"));
        return ks_future<std::string>::resolved("b3-tasktask");
        });
    ASSERT_TRUE(b);


    b = flow.add_task<std::string>("c1", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        return "c1-tasktask";
        });
    ASSERT_TRUE(b);

    b = flow.add_task<std::string>("d1:c1,b2 b3;b4", ks_apartment::default_mta(), [](const ks_async_flow& this_flow) {
        EXPECT_TRUE(this_flow.is_task_completed("c1"));
        EXPECT_TRUE(this_flow.is_task_completed("b2"));
        EXPECT_TRUE(this_flow.is_task_completed("b3"));
        EXPECT_TRUE(this_flow.is_task_completed("b4"));
        return "a1-tasktask";
        });
    ASSERT_TRUE(b);

    id = flow.add_flow_completed_observer(ks_apartment::background_sta(), [&work_wg](const ks_async_flow& this_flow, const ks_error& error) {
        EXPECT_EQ(error.get_code(), 0);
        work_wg.done();
    }, ks_async_context());
    ASSERT_TRUE(id != 0);

    flow.start();

    work_wg.wait();
    ASSERT_TRUE(flow.is_flow_completed());
    EXPECT_TRUE(flow.is_task_completed("d1"));
}
