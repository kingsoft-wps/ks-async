cmake_minimum_required(VERSION 3.15)

project(ks-async)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MY_GENERAL_COMPILE_OPTIONS
		$<$<CXX_COMPILER_ID:MSVC>:
			/permissive- /MP
			/W3>
			#/WX
		$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
			-fvisibility=hidden
			-fvisibility-inlines-hidden
			-fPIC
			-Wall -Wextra
			-Wshadow -Wpointer-arith
			-Wno-deprecated-declarations
			-Wno-unused-parameter>
			#-Werror
)


if (NOT DEFINED KS_ASYNC_LIB_TYPE)
	set(KS_ASYNC_LIB_TYPE "SHARED")  #default is SHARED
endif()

if (NOT(${KS_ASYNC_LIB_TYPE} STREQUAL "SHARED" OR ${KS_ASYNC_LIB_TYPE} STREQUAL "EMBEDDED"))
	message(FATAL_ERROR "KS_ASYNC_LIB_TYPE=${KS_ASYNC_LIB_TYPE} is invalid")
endif()


set(MY_LIB_NAME ks-async)
set(MY_LIB_TEST_NAME ks-async-test)
set(MY_LIB_BENCH_NAME ks-async-bench)


set(MY_SOURCE_FILES
	#about apartment
	ks_apartment.h
	ks_apartment.cpp
	ks_single_thread_apartment_imp.h
	ks_single_thread_apartment_imp.cpp
	ks_thread_pool_apartment_imp.h
	ks_thread_pool_apartment_imp.cpp

	#about future
	ks_future.h
	ks_future_void.inl
	ks_future_util.h
	ks_future_util.inl
	ks_future_util.cpp
	#about promise
	ks_promise.h
	ks_promise_void.inl
	#about result
	ks_result.h
	ks_result_void.inl

	#about context and controller
	ks_async_context.h
	ks_async_context.cpp
	ks_async_controller.h
	ks_async_controller.cpp

	#about async-flow
	ks_async_flow.h

	#about notificiation-center
	ks_notification_center.h
	ks_notification_center.cpp
	ks_notification.h
	ks_notification.cpp

	#others
	ks_pending_trigger.h
	ks_pending_trigger.cpp
	ks_cancel_inspector.h
	ks_cancel_inspector.cpp
	ks_async_base.h
	ks_error.h
	ks_error.cpp
)

set(MY_ASYNC_RAW_SOURCE_FILES
	#ks-async-raw
	ks-async-raw/ks_raw_future.h
	ks-async-raw/ks_raw_future.cpp
	ks-async-raw/ks_raw_promise.h
	ks-async-raw/ks_raw_promise.cpp
	ks-async-raw/ks_raw_result.h
	ks-async-raw/ks_raw_result.cpp
	ks-async-raw/ks_raw_value.h
	ks-async-raw/ks_raw_value.cpp
	ks-async-raw/ks_raw_async_flow.h
	ks-async-raw/ks_raw_async_flow.cpp
	ks-async-raw/ks_raw_internal_helper.hpp
)

set(MY_KTL_SOURCE_FILES 
	#ktl
	ktl/ks_cxxbase.h
	ktl/ks_functional.h
	ktl/ks_type_traits.h
	ktl/ks_any.h
	ktl/ks_defer.h
	ktl/ks_concurrency.h
	ktl/ks_source_location.h

	#ktl/ks_concurrency/* (internal)
	ktl/ks_concurrency/ks_atomic.h
	ktl/ks_concurrency/ks_refcount.h
	ktl/ks_concurrency/ks_spinlock.h
	ktl/ks_concurrency/ks_mutex.h
	ktl/ks_concurrency/ks_condition_variable.h
	ktl/ks_concurrency/ks_semaphore.h
	ktl/ks_concurrency/ks_waitgroup.h
	ktl/ks_concurrency/ks_event.h
	#ktl/ks_concurrency/_implement/* (internal)
	ktl/ks_concurrency/_implement/ks_atomic_storage.h
	ktl/ks_concurrency/_implement/ks_atomic_integral_common.h
	ktl/ks_concurrency/_implement/ks_atomic_integral_waitable.h
	ktl/ks_concurrency/_implement/ks_atomic_enum_common.h
	ktl/ks_concurrency/_implement/ks_atomic_enum_waitable.h
	ktl/ks_concurrency/_implement/ks_atomic_bool.h
	ktl/ks_concurrency/_implement/ks_atomic_floating.h
	ktl/ks_concurrency/_implement/ks_atomic_pointer.h
	ktl/ks_concurrency/_implement/ks_spinlock_std.h
	ktl/ks_concurrency/_implement/ks_spinlock_win.h
	ktl/ks_concurrency/_implement/ks_spinlock_mac.h
	ktl/ks_concurrency/_implement/ks_spinlock_linux.h
	ktl/ks_concurrency/_implement/ks_semaphore_std.h
	ktl/ks_concurrency/_implement/ks_semaphore_win.h
	ktl/ks_concurrency/_implement/ks_semaphore_mac.h
	ktl/ks_concurrency/_implement/ks_semaphore_linux.h
	ktl/ks_concurrency/_implement/ks_waitgroup_std.h
	ktl/ks_concurrency/_implement/ks_waitgroup_win.h
	ktl/ks_concurrency/_implement/ks_waitgroup_mac.h
	ktl/ks_concurrency/_implement/ks_waitgroup_linux.h
	ktl/ks_concurrency/_implement/ks_event_std.h
	ktl/ks_concurrency/_implement/ks_event_win.h
	ktl/ks_concurrency/_implement/ks_event_mac.h
	ktl/ks_concurrency/_implement/ks_event_linux.h
	ktl/ks_concurrency/_implement/ks_concurrency_helper.h
)


set(PUBLIC_HEADER_FILES
	#about apartment
	ks_apartment.h
	ks_single_thread_apartment_imp.h
	ks_thread_pool_apartment_imp.h

	#about future
	ks_future.h
	ks_future_void.inl
	ks_future_util.h
	ks_future_util.inl
	#about promise
	ks_promise.h
	ks_promise_void.inl
	#about result
	ks_result.h
	ks_result_void.inl

	#about context and controller
	ks_async_context.h
	ks_async_controller.h

	#about async-flow
	ks_async_flow.h

	#about notificiation-center
	ks_notification_center.h
	ks_notification.h

	#others
	ks_cancel_inspector.h
	ks_pending_trigger.h
	ks_async_base.h
	ks_error.h
)

set(PUBLIC_ASYNC_RAW_HEADER_FILES
	#ks-async-raw
	ks-async-raw/ks_raw_future.h
	ks-async-raw/ks_raw_promise.h
	ks-async-raw/ks_raw_result.h
	ks-async-raw/ks_raw_value.h
	ks-async-raw/ks_raw_async_flow.h
)


set(PUBLIC_KTL_HEADER_FILES
	#ktl
	ktl/ks_cxxbase.h
	ktl/ks_functional.h
	ktl/ks_type_traits.h
	ktl/ks_any.h
	ktl/ks_defer.h
	ktl/ks_concurrency.h
	ktl/ks_source_location.h
)

set(PUBLIC_KTL_CONCURRENCY_HEADER_FILES
	#ktl/ks_concurrency/* (internal)
	ktl/ks_concurrency/ks_atomic.h
	ktl/ks_concurrency/ks_refcount.h
	ktl/ks_concurrency/ks_spinlock.h
	ktl/ks_concurrency/ks_mutex.h
	ktl/ks_concurrency/ks_condition_variable.h
	ktl/ks_concurrency/ks_semaphore.h
	ktl/ks_concurrency/ks_waitgroup.h
	ktl/ks_concurrency/ks_event.h
)

set(PUBLIC_KTL_CONCURRENCY_IMPLEMENT_HEADER_FILES
	#ktl/ks_concurrency/_implement/* (internal)
	ktl/ks_concurrency/_implement/ks_atomic_storage.h
	ktl/ks_concurrency/_implement/ks_atomic_integral_common.h
	ktl/ks_concurrency/_implement/ks_atomic_integral_waitable.h
	ktl/ks_concurrency/_implement/ks_atomic_enum_common.h
	ktl/ks_concurrency/_implement/ks_atomic_enum_waitable.h
	ktl/ks_concurrency/_implement/ks_atomic_bool.h
	ktl/ks_concurrency/_implement/ks_atomic_floating.h
	ktl/ks_concurrency/_implement/ks_atomic_pointer.h
	ktl/ks_concurrency/_implement/ks_spinlock_std.h
	ktl/ks_concurrency/_implement/ks_spinlock_win.h
	ktl/ks_concurrency/_implement/ks_spinlock_mac.h
	ktl/ks_concurrency/_implement/ks_spinlock_linux.h
	ktl/ks_concurrency/_implement/ks_semaphore_std.h
	ktl/ks_concurrency/_implement/ks_semaphore_win.h
	ktl/ks_concurrency/_implement/ks_semaphore_mac.h
	ktl/ks_concurrency/_implement/ks_semaphore_linux.h
	ktl/ks_concurrency/_implement/ks_waitgroup_std.h
	ktl/ks_concurrency/_implement/ks_waitgroup_win.h
	ktl/ks_concurrency/_implement/ks_waitgroup_mac.h
	ktl/ks_concurrency/_implement/ks_waitgroup_linux.h
	ktl/ks_concurrency/_implement/ks_event_std.h
	ktl/ks_concurrency/_implement/ks_event_win.h
	ktl/ks_concurrency/_implement/ks_event_mac.h
	ktl/ks_concurrency/_implement/ks_event_linux.h
	ktl/ks_concurrency/_implement/ks_concurrency_helper.h
)


source_group(
	TREE  ${CMAKE_CURRENT_SOURCE_DIR}
	FILES ${MY_SOURCE_FILES} ${MY_ASYNC_RAW_SOURCE_FILES} ${MY_KTL_SOURCE_FILES} 
		  __forcelinks.cpp __dll_forcelinker.cpp
)

if (KS_ASYNC_TEST_ENABLED)
    file(GLOB_RECURSE MY_TEST_SOURCE_FILES "test/*.h" "test/*.cpp")
	source_group(
		TREE  ${CMAKE_CURRENT_SOURCE_DIR}
		FILES ${MY_TEST_SOURCE_FILES}
	)

	file(GLOB_RECURSE MY_BENCH_SOURCE_FILES "bench/*.h" "bench/*.cpp")
	source_group(
		TREE  ${CMAKE_CURRENT_SOURCE_DIR}
		FILES ${MY_BENCH_SOURCE_FILES}
	)
endif()

#fetch extern gtest and gbench
if (KS_ASYNC_TEST_ENABLED)
	include(FetchContent)

	### gtest ...
	if (KS_ASYNC_EXTERNAL_GTEST_PROJECT_URL)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY ${KS_ASYNC_EXTERNAL_GTEST_GIT_URL}
            GIT_TAG  master
        )
    else()
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
        )
    endif()

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

	### gbench ...
    if (KS_ASYNC_EXTERNAL_GBENCH_PROJECT_URL)
        FetchContent_Declare(
            benchmark
            GIT_REPOSITORY ${KS_ASYNC_EXTERNAL_GBENCH_PROJECT_URL}
            GIT_TAG master
        )
    else()
        FetchContent_Declare(
            benchmark
            URL https://github.com/google/benchmark/archive/v1.9.4.zip
        )
    endif()
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)
endif()


#lib
if (${KS_ASYNC_LIB_TYPE} STREQUAL "SHARED")
	add_library(${MY_LIB_NAME} SHARED ${MY_SOURCE_FILES} ${MY_ASYNC_RAW_SOURCE_FILES} ${MY_KTL_SOURCE_FILES})
	target_compile_definitions(${MY_LIB_NAME} PRIVATE KS_ASYNC_EXPORTS)
	target_compile_options(${MY_LIB_NAME} PRIVATE ${MY_GENERAL_COMPILE_OPTIONS})
elseif (${KS_ASYNC_LIB_TYPE} STREQUAL "EMBEDDED")
	add_library(${MY_LIB_NAME} STATIC ${MY_SOURCE_FILES} ${MY_ASYNC_RAW_SOURCE_FILES} ${MY_KTL_SOURCE_FILES} __forcelinks.cpp)
	target_compile_definitions(${MY_LIB_NAME} PRIVATE KS_ASYNC_EXPORTS)
	target_compile_options(${MY_LIB_NAME} PRIVATE ${MY_GENERAL_COMPILE_OPTIONS})
	set_property(TARGET ${MY_LIB_NAME} PROPERTY __EMBEDDED TRUE)
endif()


#test & bench
if (KS_ASYNC_TEST_ENABLED)
	#shared embedding library (for test)
	set(MY_TEST_LINKING_LIB_NAME ${MY_LIB_NAME})
    if (${KS_ASYNC_LIB_TYPE} STREQUAL "EMBEDDED")
		set(MY_TEST_LINKING_LIB_NAME ${MY_LIB_NAME}-test-embedding)
        add_library(${MY_TEST_LINKING_LIB_NAME} SHARED __dll_forcelinker.cpp)
        target_link_libraries(${MY_TEST_LINKING_LIB_NAME} PRIVATE ${MY_LIB_NAME})
        target_compile_definitions(${MY_TEST_LINKING_LIB_NAME} PRIVATE KS_ASYNC_EXPORTS)
	endif()

	# test
    add_executable(${MY_LIB_TEST_NAME} ${MY_TEST_SOURCE_FILES})
	target_compile_options(${MY_LIB_TEST_NAME} PRIVATE ${MY_GENERAL_COMPILE_OPTIONS})
    target_include_directories(${MY_LIB_TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test)
	target_link_libraries(${MY_LIB_TEST_NAME} PRIVATE ${MY_TEST_LINKING_LIB_NAME})
	target_link_libraries(${MY_LIB_TEST_NAME} PRIVATE gtest)

	# bench
	add_executable(${MY_LIB_BENCH_NAME} ${MY_BENCH_SOURCE_FILES})
	target_compile_options(${MY_LIB_TEST_NAME} PRIVATE ${MY_GENERAL_COMPILE_OPTIONS})
	target_include_directories(${MY_LIB_BENCH_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bench)	
	target_link_libraries(${MY_LIB_BENCH_NAME} PRIVATE ${MY_TEST_LINKING_LIB_NAME})
	target_link_libraries(${MY_LIB_BENCH_NAME} PRIVATE benchmark::benchmark)
	target_link_libraries(${MY_LIB_BENCH_NAME} PRIVATE benchmark::benchmark_main)

endif()


#install
install(TARGETS ${MY_LIB_NAME})
install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include/ks-async/)
install(FILES ${PUBLIC_ASYNC_RAW_HEADER_FILES} DESTINATION include/ks-async/ks-async-raw/)
install(FILES ${PUBLIC_KTL_HEADER_FILES} DESTINATION include/ks-async/ktl/)
install(FILES ${PUBLIC_KTL_CONCURRENCY_HEADER_FILES} DESTINATION include/ks-async/ktl/ks_concurrency/)
install(FILES ${PUBLIC_KTL_CONCURRENCY_IMPLEMENT_HEADER_FILES} DESTINATION include/ks-async/ktl/ks_concurrency/_implement/)
install(FILES "README.md" DESTINATION include/ks-async/)
install(FILES "LICENSE" DESTINATION include/ks-async/)
install(DIRECTORY "doc" DESTINATION include/ks-async/)
