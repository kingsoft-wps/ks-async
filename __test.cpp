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

// ConsoleApplication3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "ks_future.h"
#include "ks_promise.h"
#include "ks_async_flow.h"
#include "ks_notification_center.h"
#include <iostream>
#include <sstream>
#include <string>

namespace {
    template <class T>
    std::string _result_to_str(const ks_result<T>& result) {
        if (result.is_value())
            return (std::stringstream() << result.to_value()).str();
        else if (result.is_error())
            return (std::stringstream() << "(Err:" << result.to_error().get_code() << ")").str();
        else
            return "(!Fin)";
    }
    template <>
    std::string _result_to_str(const ks_result<void>& result) {
        if (result.is_value())
            return "VOID";
        else if (result.is_error())
            return (std::stringstream() << "(Err:" << result.to_error().get_code() << ")").str();
        else
            return "(!Fin)";
    }

    template <class T>
    void _output_result(const char* title, const ks_result<T>& result) {
        if (result.is_value())
            std::cout << title << _result_to_str(result) << " (succ)\n";
        else if (result.is_error())
            std::cout << title << "{ code: " << result.to_error().get_code() << " } (error)\n";
        else
            std::cout << title << "-- (not-completed)\n";
    }

} //namespace


ks_latch g_exit_latch(0);


void test_promise() {
    g_exit_latch.add(1);
    std::cout << "test promise ... ";

    ks_promise<std::string> promise(std::create_inst);
    promise.get_future()
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
            _output_result("completion: ", result);
            g_exit_latch.count_down();
        });

    std::thread([promise]() {
        promise.resolve("pass");
    }).join();

    g_exit_latch.wait();
}

void test_post() {
    g_exit_latch.add(1);
    std::cout << "test post ... ";

    auto future = ks_future<std::string>::post(ks_apartment::default_mta(), make_async_context(), []() {
        return std::string("pass");
    });

    future.on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
        _output_result("completion: ", result);
        g_exit_latch.count_down();
    });

    g_exit_latch.wait();
}

void test_post_pending() {
    g_exit_latch.add(1);
    std::cout << "test post_pending ... ";

    ks_pending_trigger trigger;
    auto future = ks_future<std::string>::post_pending(ks_apartment::default_mta(), make_async_context(), []() {
        return std::string("pass");
        }, & trigger);

    future.on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
        _output_result("completion: ", result);
        g_exit_latch.count_down();
        });

    trigger.start();
    g_exit_latch.wait();
}

void test_post_delayed() {
    g_exit_latch.add(1);
    std::cout << "test post_delayed (400ms) ... ";

    const auto post_time = std::chrono::steady_clock::now();
    auto future = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), [post_time]() {
        auto duration = std::chrono::steady_clock::now() - post_time;
        int64_t real_delay = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        std::stringstream ss;
        ss << "pass (" << real_delay << "ms)";
        return ss.str();
    }, 400);

    future.on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
        _output_result("completion: ", result);
        g_exit_latch.count_down();
    });

    g_exit_latch.wait();
}

void test_all() {
    g_exit_latch.add(1);
    std::cout << "test all ... ";

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
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
            _output_result("completion: ", result);
            g_exit_latch.count_down();
        });

    g_exit_latch.wait();
}

void test_any() {
    g_exit_latch.add(1);
    std::cout << "test any ... ";

    auto f1 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "a";
    }, 100);
    auto f2 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "b";
    }, 80);
    auto f3 = ks_future<std::string>::post_delayed(ks_apartment::default_mta(), make_async_context(), []() -> std::string {
        return "c";
    }, 120);

    ks_future_util::any(f1, f2, f3)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
            _output_result("completion: ", result);
            g_exit_latch.count_down();
        });

    g_exit_latch.wait();
}

void test_parallel() {
    g_exit_latch.add(1);
    std::cout << "test parallel ... ";

    auto c = std::make_shared<std::atomic<int>>(0);

    ks_future_util
        ::parallel(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>{5, [c]() { ++(*c); }})
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
            _output_result("completion: ", result);
            g_exit_latch.count_down();
        });

    g_exit_latch.wait();
    ASSERT(*c == 5);
}

void test_parallel_n() {
    g_exit_latch.add(1);
    std::cout << "test parallel_n ... ";

    auto c = std::make_shared<std::atomic<int>>(0);
    ks_future_util
        ::parallel_n(
            ks_apartment::default_mta(), 
            [c]() { ++(*c); },
            5)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
            _output_result("completion: ", result);
            g_exit_latch.count_down();
        });

    g_exit_latch.wait();
    ASSERT(*c == 5);
}

void test_sequential() {
    g_exit_latch.add(1);
    std::cout << "test sequential ... ";

    auto c = std::make_shared<std::atomic<int>>(0);
    ks_future_util
        ::sequential(
            ks_apartment::default_mta(),
            std::vector<std::function<void()>>{5, [c]() { ++(*c); }})
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
            _output_result("completion: ", result);
            g_exit_latch.count_down();
        });

    g_exit_latch.wait();
    ASSERT(*c == 5);
}

void test_sequential_n() {
    g_exit_latch.add(1);
    std::cout << "test sequential_n ... ";

    auto c = std::make_shared<std::atomic<int>>(0);
    ks_future_util
        ::sequential_n(
            ks_apartment::default_mta(),
            [c]() { ++(*c); },
            5)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
        _output_result("completion: ", result);
        g_exit_latch.count_down();
            });

    g_exit_latch.wait();
    ASSERT(*c == 5);
}

void test_repeat() {
    g_exit_latch.add(1);
    std::cout << "test repeat ... ";

    std::atomic<int> sn = { 0 };

    auto fn = [&sn]() -> ks_result<void> {
        if (++sn <= 5) {
            std::cout << sn << " ";
            return nothing;
        }
        else {
            return ks_error::eof_error();
        }
    };

    ks_future_util
        ::repeat(ks_apartment::default_mta(), fn)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
            _output_result("completion: ", result);
            g_exit_latch.count_down();
        });

    g_exit_latch.wait();
}

void test_repeat_periodic() {
    g_exit_latch.add(1);
    std::cout << "test repeat_periodic ... ";

    std::atomic<int> sn = { 0 };

    auto fn = [&sn]() -> ks_result<void> {
        if (++sn <= 5) {
            std::cout << sn << " ";
            return nothing;
        }
        else {
            return ks_error::eof_error();
        }
    };

    auto f = ks_future_util
        ::repeat_periodic(ks_apartment::default_mta(), fn, 0, 100)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
            _output_result("completion: ", result);
            g_exit_latch.count_down();
        });

    g_exit_latch.wait();
}

void test_repeat_productive() {
    g_exit_latch.add(1);
    std::cout << "test repeat_productive ... ";

    std::atomic<int> sn = { 0 };

    auto produce_fn = [&sn]() -> ks_result<int> {
        int i = ++sn;
        if (i <= 5) {
            std::cout << sn << " ";
            return i;
        }
        else {
            return ks_error::eof_error();
        }
    };

    auto consume_fn = [](const int& i) -> void {
        std::cout << "# ";
    };

    ks_future_util
        ::repeat_productive<int>(
            ks_apartment::default_mta(), produce_fn, 
            ks_apartment::default_mta(), consume_fn)
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
            _output_result("completion: ", result);
            g_exit_latch.count_down();
        });

    g_exit_latch.wait();
}

void test_future_wait() {
    g_exit_latch.add(1);
    std::cout << "test wait ... ";

    ks_future<void>::post(ks_apartment::background_sta(), {}, []() {
            ks_future<void>::post_delayed(ks_apartment::background_sta(), {}, []() {}, 100).__wait();
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const ks_result<void>& result) -> void {
            std::cout << "->on_completion(void) ";
            _output_result("complete(void): ", result);
            g_exit_latch.count_down();
            });

        g_exit_latch.wait();
}

void test_future_methods() {
    g_exit_latch.add(1);
    std::cout << "test future ... ";

    ks_future<std::string>::resolved("a")
        .cast<std::string>()
        .then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::string& value) {
            std::cout << "->then() ";
            return value + ".then";
        })
        .transform<std::string>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<std::string>& result) -> ks_result<std::string> {
            std::cout << "->transform() ";
            return result.is_value() ? ks_result<std::string>(_result_to_str(result) + ".transform") : ks_result<std::string>(result.to_error());
        })
        .flat_then<std::string>(ks_apartment::default_mta(), make_async_context(), [](const std::string& value) -> ks_future<std::string> {
            return ks_future<std::string>::resolved(value + ".flat_then");
        })
        .flat_transform<std::string>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<std::string>& result) -> ks_future<std::string> {
            return result.is_value() ? ks_future<std::string>::resolved(_result_to_str(result) + ".flat_transform") : ks_future<std::string>::rejected(result.to_error());
        })
        .on_success(ks_apartment::default_mta(), make_async_context(), [](const auto& value) -> void {
            std::cout << "->on_success() ";
        })
        .on_failure(ks_apartment::default_mta(), make_async_context(), [](const ks_error& error) -> void {
            std::cout << "->on_failure() ";
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) -> void {
            std::cout << "->on_completion() ";
            _output_result("completion: ", result);
            g_exit_latch.count_down();
        });

    g_exit_latch.wait();
    g_exit_latch.add(1);

    ks_future<void>::resolved().cast<nothing_t>()
        .cast<void>()
        .then<void>(ks_apartment::default_mta(), make_async_context(), []() -> void {
            std::cout << "->then(void) ";
        })
        .transform<void>(ks_apartment::default_mta(), make_async_context(), [](const ks_result<void>& result) -> void {
            std::cout << "->transform(void) ";
        })
        .on_failure(ks_apartment::default_mta(), make_async_context(), [](const ks_error& error) -> void {
            std::cout << "->on_failure(void) ";
        })
        .on_completion(ks_apartment::default_mta(), make_async_context(), [](const ks_result<void>& result) -> void {
            std::cout << "->on_completion(void) ";
            _output_result("complete(void): ", result);
            g_exit_latch.count_down();
        });

    g_exit_latch.wait();
}

void test_async_flow() {
    g_exit_latch.add(1);
    std::cout << "test flow ... \n";

    ks_async_flow flow;
    bool b;
    uint64_t id;

    flow.put_custom_value<int>("v1", 1);
    ASSERT(flow.get_value<int>("v1") == 1);

    b = flow.add_task<std::string>("a1", ks_apartment::default_mta(), [](const ks_async_flow& flow) {
        return "a1-tasktask";
        });
    ASSERT(b);

    b = flow.add_task<std::string>("a2", ks_apartment::default_mta(), [](const ks_async_flow& flow) {
        return "a2-tasktask";
        });
    ASSERT(b);

    b = flow.add_task<std::string>("b1: a1", ks_apartment::default_mta(), [](const ks_async_flow& flow) {
        return "b1-tasktask";
        });
    ASSERT(b);

    id = flow.add_task_running_observer("*", ks_apartment::background_sta(), [](const ks_async_flow& flow, const char* task_name) {
        std::cout << (std::stringstream() << "flow: notify: task/" << task_name << ": running" << "\n").str();
    }, ks_async_context());
    ASSERT(id != 0);

    id = flow.add_task_completed_observer("*", ks_apartment::background_sta(), [](const ks_async_flow& flow, const char* task_name, const ks_error& error) {
        std::cout << (std::stringstream() << "flow: notify: task/" << task_name << (error.get_code() == 0 ? ": succeeded" : ": failed") << "\n").str();
        }, ks_async_context());
    ASSERT(id != 0);

    id = flow.add_flow_running_observer(ks_apartment::background_sta(), [](const ks_async_flow& flow) {
        std::cout << (std::stringstream() << "flow: notify: flow" << ": running" << "\n").str();
    }, ks_async_context());
    ASSERT(id != 0);

    id = flow.add_flow_completed_observer(ks_apartment::background_sta(), [](const ks_async_flow& flow, const ks_error& error) {
        std::cout << (std::stringstream() << "flow: notify: flow" << (error.get_code() == 0 ? ": succeeded" : ": failed") << "\n").str();

        std::cout << "flow ";
        _output_result("complete(void): ", error.get_code() == 0 ? ks_result<void>(nothing) : ks_result<void>(error));
        g_exit_latch.count_down();
    }, ks_async_context());
    ASSERT(id != 0);

    flow.start();

    g_exit_latch.wait();
}

void test_alive() {
    g_exit_latch.add(1);
    std::cout << "test alive ... ";

    struct OBJ {
        OBJ(int id) : m_id(id) { ++s_counter(); std::cout << "->::OBJ(id:" << m_id << ") "; }
        ~OBJ() { --s_counter(); std::cout << "->::~OBJ(id:" << m_id << ") "; }
        _DISABLE_COPY_CONSTRUCTOR(OBJ);
        static std::atomic<int>& s_counter() { static std::atomic<int> s_n(0); return s_n; }
        const int m_id;
    };

    if (true) {
        auto obj1 = std::make_shared<OBJ>(1);

        ks_future<void>::post_delayed(
            ks_apartment::default_mta(), 
            make_async_context().bind_owner(std::move(obj1)).bind_controller(nullptr),
            []() { std::cout << "->fn() "; }, 
            100
        ).on_completion(ks_apartment::default_mta(), make_async_context(), [](const auto& result) {
            ks_future<void>::post_delayed(
                ks_apartment::default_mta(), make_async_context(), []() {
                    g_exit_latch.count_down(); 
                }, 100);
        });
    }

    g_exit_latch.wait();
    if (OBJ::s_counter() == 0)
        std::cout << "completion: no leak (succ)\n";
    else
        std::cout << "completion: " << OBJ::s_counter() << " objs leak (error)\n";
}

void test_notification_center() {
    g_exit_latch.add(2);
    std::cout << "test notification-center ... ";

    struct {} sender, observer;
    ks_notification_center::default_center()->add_observer(&observer, "a.b.c.d", ks_apartment::default_mta(), [](const ks_notification& notification) {
        ASSERT(false);
        std::cout << "a.b.c.d notification: name=" << notification.get_name() << "; ";
        });
    ks_notification_center::default_center()->add_observer(&observer, "a.b.*", ks_apartment::default_mta(), [](const ks_notification& notification) {
        std::cout << "a.b.* notification: name=" << notification.get_name() << "; ";
        g_exit_latch.count_down();
        });
    ks_notification_center::default_center()->add_observer(&observer, "*", ks_apartment::default_mta(), [](const ks_notification& notification) {
        std::cout << "* notification: name=" << notification.get_name() << "; ";
        g_exit_latch.count_down();
        });

    ks_notification_center::default_center()->post_notification(&sender, "a.x.y.z");
    ks_notification_center::default_center()->post_notification_with_payload<int>(&sender, "a.x.y.z", 1);
    ks_notification_center::default_center()->post_notification_with_payload<std::string>(&sender, "a.b.c.d.e", "xxx");

    g_exit_latch.wait();
    ks_notification_center::default_center()->remove_observer(&sender);

    std::cout << "completion (succ)\n";
}



#ifdef _WIN32
#   define __TEST_FORK_ENABLED  0
#else
#   define __TEST_FORK_ENABLED  1
#   include <unistd.h>
#   include <sys/wait.h>
#endif

bool g_is_child_process = false;

void test_fork() {
#if __TEST_FORK_ENABLED
    g_exit_latch.add(1);
    std::cout << "test fork ... \n";

    ks_apartment::default_mta()->atfork_prepare();
    ks_apartment::background_sta()->atfork_prepare();

    pid_t pid = fork();
    ASSERT(pid != -1);

    if (pid == 0) {
        g_is_child_process = true;
        ks_apartment::default_mta()->atfork_child();
        ks_apartment::background_sta()->atfork_child();
    }
    else {
        ks_apartment::default_mta()->atfork_parent();
        ks_apartment::background_sta()->atfork_parent();
    }

    if (pid == 0) {
        ks_future<void>::post(ks_apartment::default_mta(), {}, []() {
            std::cout << "test fork child-post\n";
        });
    }

    if (pid != 0 && pid != -1) {
        int status = 0;
        pid_t wait_res = wait(&status);
        ASSERT(wait_res != -1);
    }

    g_exit_latch.count_down();
    g_exit_latch.wait();
    std::cout << "test fork completion (succ)\n";
#else
    std::cout << "test fork is not enabled (skipped)\n";
#endif
}


int main() {
    std::cout << "start ...\n";

    test_promise();
    test_post();
    test_post_pending();
    test_post_delayed();
    test_future_wait();
    test_future_methods();

    test_all();
    test_any();

    test_parallel();
    test_parallel_n();

    test_sequential();
    test_sequential_n();

    test_repeat();
    test_repeat_periodic();
    test_repeat_productive();

    test_async_flow();
    test_alive();

    test_notification_center();

    test_fork();

    ks_apartment::default_mta()->async_stop();
    ks_apartment::background_sta()->async_stop();
    ks_apartment::default_mta()->wait();
    ks_apartment::background_sta()->wait();
    std::cout << (g_is_child_process ? "child end." : "end.") << "\n";
    return 0;
}
