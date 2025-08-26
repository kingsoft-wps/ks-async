/* Copyright 2024 The Kingsoft's ks-async/ktl Authors. All Rights Reserved.

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
#include "ks_cxxbase.h"

#ifndef __KS_SOURCE_LOCATION
#define __KS_SOURCE_LOCATION

class ks_source_location {
public:
    constexpr explicit ks_source_location(nullptr_t)
        : m_file_name(nullptr), m_line(0), m_function_name(nullptr), m_custom_desc(nullptr) {
    }

    constexpr explicit ks_source_location(
        const char* file_name, unsigned int line, const char* function_name, const char* custom_desc = nullptr)
        : m_file_name(file_name), m_line(line), m_function_name(function_name), m_custom_desc(custom_desc) {
    }

    constexpr ks_source_location(const ks_source_location&) = default;
    constexpr ks_source_location(ks_source_location&&) noexcept = default;

    constexpr ks_source_location& operator=(const ks_source_location&) = default;
    constexpr ks_source_location& operator=(ks_source_location&&) noexcept = default;

public:
    constexpr bool is_empty() const { return this->m_file_name == nullptr; }

    constexpr const char*  file_name() const { return m_file_name; }
    constexpr unsigned int line() const { return m_line; }
    constexpr const char*  function_name() const { return m_function_name; }
    constexpr const char* custom_desc() const { return m_custom_desc; }

public:
    static constexpr const char* __custom_desc_of() { return nullptr; }
    static constexpr const char* __custom_desc_of(const char* custom_desc) { return custom_desc; }

private:
    const char*  m_file_name;
    unsigned int m_line;
    const char*  m_function_name;
    const char*  m_custom_desc;
};


#define current_source_location(...)  (ks_source_location(__FILE__, __LINE__, __FUNCTION__, ks_source_location::__custom_desc_of(__VA_ARGS__)))


#endif //__KS_SOURCE_LOCATION
