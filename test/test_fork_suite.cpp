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

#if !defined(_WIN32) && __KS_APARTMENT_ATFORK_ENABLED

#include <unistd.h>
#include <sys/wait.h>


TEST(test_fork_suite, test_fork) {
    ks_latch work_latch(0);
    work_latch.add(1);

    ks_apartment::default_mta()->atfork_prepare();
    ks_apartment::background_sta()->atfork_prepare();

    pid_t pid = fork();
    ASSERT_TRUE(pid != -1);

    if (pid == 0) {
        ks_apartment::default_mta()->atfork_child();
        ks_apartment::background_sta()->atfork_child();
    }
    else {
        ks_apartment::default_mta()->atfork_parent();
        ks_apartment::background_sta()->atfork_parent();
    }

    if (pid == 0) {
        // 子进程
        ks_apartment::default_mta()->async_stop();
        ks_apartment::background_sta()->async_stop();
        ks_apartment::default_mta()->wait();
        ks_apartment::background_sta()->wait();
        exit(0);
    }

    // if (pid != 0 && pid != -1) {
    //     // 父进程
    //     int status = 0;
    //     pid_t waited_pid = waitpid(pid, &status, 0);
    //     ASSERT_TRUE(waited_pid == pid);
    // }

    work_latch.count_down();
    work_latch.wait();
}


#endif
