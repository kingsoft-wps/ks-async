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
#include <type_traits>
#include <memory>


#ifndef __STD_TYPE_TRAITS_FIX14
#define __STD_TYPE_TRAITS_FIX14

#if __cplusplus < 201703L && !defined(_MSVC_LANG)
namespace std {
	template<bool __v>
	using bool_constant = integral_constant<bool, __v>;

	template <typename _Tp>
	constexpr bool is_void_v = is_void<_Tp>::value;
	template <typename _Tp>
	constexpr bool is_null_pointer_v = is_null_pointer<_Tp>::value;

	template <typename _Tp, typename _Up>
	constexpr bool is_same_v = is_same<_Tp, _Up>::value;
	template <typename _Tp>
	constexpr bool is_copy_assignable_v = is_copy_assignable<_Tp>::value;
	template <typename _From, typename _To>
	constexpr bool is_convertible_v = is_convertible<_From, _To>::value;
	template <typename _Base, typename _Derived>
	constexpr bool is_base_of_v = is_base_of<_Base, _Derived>::value;

	template <typename _Tp>
	constexpr bool is_reference_v = is_reference<_Tp>::value;
	template <typename _Tp>
	constexpr bool is_lvalue_reference_v = is_lvalue_reference<_Tp>::value;
	template <typename _Tp>
	constexpr bool is_rvalue_reference_v = is_rvalue_reference<_Tp>::value;

	template <typename _Tp>
	constexpr bool is_trivial_v = is_trivial<_Tp>::value;
	template <typename _Tp>
	constexpr bool is_standard_layout_v = is_standard_layout<_Tp>::value;

	template <typename _Tp>
	constexpr bool is_const_v = is_const<_Tp>::value;
	template <typename _Tp>
	constexpr bool is_volatile_v = is_volatile<_Tp>::value;

	template <typename _Tp>
	constexpr bool is_signed_v = is_signed<_Tp>::value;
	template <typename _Tp>
	constexpr bool is_integral_v = is_integral<_Tp>::value;
	template <typename _Tp>
	constexpr bool is_floating_point_v = is_floating_point<_Tp>::value;

	template <typename...>
	struct conjunction : true_type {};
	template <typename _Bn>
	struct conjunction<_Bn> : _Bn {};
	template <typename _Arg, class... _Args>
	struct conjunction<_Arg, _Args...> : conditional_t<!bool(_Arg::value), _Arg, conjunction<_Args...>> {};
	template <typename... _Bn>
	constexpr bool conjunction_v = conjunction<_Bn...>::value;

	template <typename...>
	struct disjunction : false_type {};
	template <typename _Bn>
	struct disjunction<_Bn> : _Bn {};
	template <typename _Arg, class... _Args>
	struct disjunction<_Arg, _Args...> : conditional_t<bool(_Arg::value), _Arg, disjunction<_Args...>> {};
	template <typename... _Bn>
	constexpr bool disjunction_v = disjunction<_Bn...>::value;
}
#endif

#if __cplusplus < 202002L
namespace std {
	template <class T>
	struct remove_cvref { using type = remove_cv_t<remove_reference_t<T>>; };
	template <class T>
	using remove_cvref_t = typename remove_cvref<T>::type;

	template <class T>
	struct type_identity { using type = T; };
	template <class T>
	using type_identity_t = typename type_identity<T>::type;
}
#endif

#endif //__STD_TYPE_TRAITS_FIX14


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#ifndef __KS_TYPE_TRAITS_DEF
#define __KS_TYPE_TRAITS_DEF

namespace std {
	//is_nothing
	template <class T>
	struct is_nothing : std::is_same<std::remove_cvref_t<T>, nothing_t> {};
	template <class T>
	constexpr bool is_nothing_v = is_nothing<T>::value;

	//is_mutable
	template <class T>
	struct is_mutable : std::bool_constant<!std::is_reference_v<T> && !std::is_const_v<T>> {};
	template <class T>
	constexpr bool is_mutable_v = is_mutable<T>::value;

	//is_const_lvalue_reference
	template <class T>
	struct is_const_lvalue_reference : std::bool_constant<std::is_lvalue_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>> {};
	template <class T>
	constexpr bool is_const_lvalue_reference_v = is_const_lvalue_reference<T>::value;

	//is_mutable_lvalue_reference
	template <class T>
	struct is_mutable_lvalue_reference : std::bool_constant<std::is_lvalue_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>> {};
	template <class T>
	constexpr bool is_mutable_lvalue_reference_v = is_mutable_lvalue_reference<T>::value;

	//is_mutable_rvalue_reference
	template <class T>
	struct is_mutable_rvalue_reference : std::bool_constant<std::is_rvalue_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>> {};
	template <class T>
	constexpr bool is_mutable_rvalue_reference_v = is_mutable_rvalue_reference<T>::value;

	//variadic_size
	template <class... Ts>
	struct variadic_size : std::tuple_size<std::tuple<Ts...>> {};
	template <class... Ts>
	constexpr size_t variadic_size_v = variadic_size<Ts...>::value;

	//variadic_element
	template <size_t IDX, class... Ts>
	struct variadic_element : std::tuple_element<IDX, std::tuple<Ts...>> {};
	template <size_t IDX, class... Ts>
	using variadic_element_t = typename variadic_element<IDX, Ts...>::type;

	//is_all_void
	template <class... Ts>
	struct is_all_void;
	template <class T, class... Ts>
	struct is_all_void<T, Ts...> : std::bool_constant<std::is_void_v<T>&& is_all_void<Ts...>::value> {};
	template <>
	struct is_all_void<> : std::true_type {};  //特化
	template <class... Ts>
	constexpr bool is_all_void_v = is_all_void<Ts...>::value;

	//has_some_void
	template <class... Ts>
	struct has_some_void;
	template <class T, class... Ts>
	struct has_some_void<T, Ts...> : std::bool_constant<std::is_void_v<T> || has_some_void<Ts...>::value> {};
	template <>
	struct has_some_void<> : std::false_type {};  //特化
	template <class... Ts>
	constexpr bool has_some_void_v = has_some_void<Ts...>::value;

	//is_all_same
	template <class... Ts>
	struct is_all_same;
	template <class T, class U, class... Ts>
	struct is_all_same<T, U, Ts...> : std::bool_constant<std::is_same_v<T, U>&& is_all_same<U, Ts...>::value> {};
	template <class T>
	struct is_all_same<T> : std::true_type {}; //特化
	template <>
	struct is_all_same<> : std::true_type {};  //特化
	template <class... Ts>
	constexpr bool is_all_same_v = is_all_same<Ts...>::value;

	//shared_pointer_traits
	template <class T>
	struct shared_pointer_traits {
		static constexpr bool is_shared_pointer_v = false;
	};
	template <class X>
	struct shared_pointer_traits<std::shared_ptr<X>> {
		static constexpr bool is_shared_pointer_v = true;
	};

	//weak_pointer_traits
	template <class T>
	struct weak_pointer_traits {
		static constexpr bool is_weak_pointer_v = false;
	};
	template <class X>
	struct weak_pointer_traits<std::weak_ptr<X>> {
		static constexpr bool is_weak_pointer_v = true;
		using locker_type = std::shared_ptr<X>;
		static std::shared_ptr<X> try_lock_weak_pointer(const std::weak_ptr<X>& pointer) { return pointer.lock(); }
		static void unlock_weak_pointer(const std::weak_ptr<X>& pointer, std::shared_ptr<X>& locker) { locker.reset(); }
		static bool check_weak_pointer_expired(const std::weak_ptr<X>& pointer) { return pointer.expired(); }
	};

	//is_shared_pointer
	template <class T>
	struct is_shared_pointer : std::bool_constant<shared_pointer_traits<T>::is_shared_pointer_v> {};
	template <class T>
	constexpr bool is_shared_pointer_v = is_shared_pointer<T>::value;

	//is_weak_pointer
	template <class T>
	struct is_weak_pointer : std::bool_constant<weak_pointer_traits<T>::is_weak_pointer_v> {};
	template <class T>
	constexpr bool is_weak_pointer_v = is_weak_pointer<T>::value;

}


namespace std { //helper funcs
	template <class T, class _ = std::enable_if_t<!std::is_const_v<std::remove_reference_t<T>>>>
	constexpr inline std::remove_reference_t<T>&& __move_mutable(T&& arg) noexcept {
		static_assert(!std::is_const_v<std::remove_reference_t<T>>, "T must be mutable");
		return std::move(arg);
	}

	template <class T, class _ = std::enable_if_t<!std::is_const_v<std::remove_reference_t<T>>>>
	constexpr inline T __forward_mutable(std::remove_reference_t<T>& arg) noexcept {
		static_assert(!std::is_const_v<std::remove_reference_t<T>>, "T must be mutable");
		return std::forward<T>(arg);
	}

	template <class T>
	constexpr inline void __try_prune_if_mutable_rvalue_reference(std::remove_reference_t<T>& arg) noexcept {
		if (std::is_mutable_rvalue_reference<T>::value && std::is_nothrow_move_constructible<std::remove_cvref_t<T>>::value)
			(void)std::remove_cvref_t<T>(std::move(arg));
	}
}


#endif //__KS_TYPE_TRAITS_DEF
