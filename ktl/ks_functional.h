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
#include <functional>


//注：应把function也看作一种特殊的基本数据类型，故使其全局直接可用。
//但由于名字空间早已被污染，故取消提供全局function定义。
//template <class FN>
//using function = std::function<FN>;


//functional type-traits ...
#ifndef __KS_FUNCTIONAL_TYPE_TRAITS_DEF
#define __KS_FUNCTIONAL_TYPE_TRAITS_DEF

#if defined(_MSVC_LANG) ? _MSVC_LANG < 201703L : __cplusplus < 201703L
namespace std {
	//is_invocable (like c++17)
	template <class FN, class... ARGs>
	struct is_invocable : std::is_convertible<FN, std::function<void(ARGs...)>> {};
	template <class FN, class... ARGs>
	constexpr bool is_invocable_v = is_invocable<FN, ARGs...>::value;

	//is_invocable_r (like c++17)
	template <class R, class FN, class... ARGs>
	struct is_invocable_r : std::is_convertible<FN, std::function<R(ARGs...)>> {};
	template <class R, class FN, class... ARGs>
	constexpr bool is_invocable_r_v = is_invocable_r<R, FN, ARGs...>::value;

	//invoke_result (like c++17)
	template <bool IsInvocable, class FN, class... ARGs>
	struct __invoke_result_imp {};
	template <class FN, class... ARGs>
	struct __invoke_result_imp<true, FN, ARGs...> { using type = decltype((*(std::remove_reference_t<FN>*)(nullptr))(std::forward<ARGs>(*(std::remove_reference_t<ARGs>*)(nullptr))...)); };
	template <class FN, class... ARGs>
	struct invoke_result : __invoke_result_imp<std::is_invocable_v<FN, ARGs...>, FN, ARGs...> {};
	template <class FN, class... ARGs>
	using invoke_result_t = typename invoke_result<FN, ARGs...>::type;

}
#endif

#endif //__KS_FUNCTIONAL_TYPE_TRAITS_DEF
