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


DEFINE_int32(parallel_num, 5, "numbers of parallel");
DEFINE_int32(future_num, 5, "numbers of future");


int main(int argc, char** argv) {
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    testing::InitGoogleTest(&argc, argv);
    int exit_code = RUN_ALL_TESTS();

    ks_apartment::default_mta()->async_stop();
    ks_apartment::background_sta()->async_stop();
    ks_apartment::default_mta()->wait();
    ks_apartment::background_sta()->wait();
    return exit_code;
}
