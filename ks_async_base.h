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

#include "ktl/ks_functional.h"
#include "ktl/ks_type_traits.h"
#include <string>
#include <memory>
#include <stdexcept>

#if !defined(KS_ASYNC_API)
#   ifdef KS_ASYNC_EXPORTS
#       define KS_ASYNC_API _DECL_EXPORT
#       define KS_ASYNC_INLINE_API
#   else
#       define KS_ASYNC_API _DECL_IMPORT
#       define KS_ASYNC_INLINE_API
#   endif
#endif


#define __KS_ASYNC_RAW_BEGIN  namespace __ks_async_raw {
#define __KS_ASYNC_RAW_END    }


#define __KS_ASYNC_RAW_FUTURE_GLOBAL_MUTEX_ENABLED  1
#define __KS_ASYNC_DX_RAW_FUTURE_GLOBAL_MUTEX_ENABLED  1
#define __KS_ASYNC_DX_RAW_FUTURE_PSEUDO_MUTEX_ENABLED  1

#define __KS_ASYNC_CONTEXT_FROM_SOURCE_LOCATION_ENABLED  0

#ifdef _WIN32
#   define __KS_APARTMENT_ATFORK_ENABLED  0  //WIN下开启也可通过编译，只是没有被使用需求
#else
#   define __KS_APARTMENT_ATFORK_ENABLED  1
#endif
