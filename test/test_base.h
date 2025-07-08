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

#pragma once

#include <gtest/gtest.h>

#include "../ks_future.h"
#include "../ks_promise.h"
#include "../ks_future_util.h"
#include "../ks_async_flow.h"
#include "../ks_notification_center.h"

#include <iostream>
#include <sstream>
#include <string>


//test helper
template <class T>
inline std::string _result_to_str(const ks_result<T>& result) {
    if (result.is_value())
        return (std::stringstream() << result.to_value()).str();
    else if (result.is_error())
        return (std::stringstream() << "(Err:" << result.to_error().get_code() << ")").str();
    else
        return "(!Fin)";
}

template <>
inline std::string _result_to_str(const ks_result<void>& result) {
    if (result.is_value())
        return "VOID";
    else if (result.is_error())
        return (std::stringstream() << "(Err:" << result.to_error().get_code() << ")").str();
    else
        return "(!Fin)";
}


template <class T>
inline void _output_result(const char* title, const ks_result<T>& result) {
    if (result.is_value())
        std::cout << title << _result_to_str(result) << " (succ)\n";
    else if (result.is_error())
        std::cout << title << "{ code: " << result.to_error().get_code() << " } (error)\n";
    else
        std::cout << title << "-- (not-completed)\n";
}
