cmake_minimum_required(VERSION 3.15)

project(ks-async)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -fPIC")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG -fPIC")
endif()
if (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -fPIC")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG -fPIC")
endif()


if (NOT DEFINED KS_ASYNC_LIB_TYPE)
	set(KS_ASYNC_LIB_TYPE "SHARED")  #default is SHARED
endif()

if (${KS_ASYNC_LIB_TYPE} STREQUAL "SHARED")
	set(MY_LIB_NAME ks-async)
	set(MY_LIB_TEST_NAME ks-async-test)
elseif (${KS_ASYNC_LIB_TYPE} STREQUAL "EMBEDDED")
	set(MY_LIB_NAME ks-async-embedded) #特殊命名
	set(MY_LIB_TEST_NAME ks-async-embedded-test)
	set(MY_LIB_TEST_EMBEDDED_HOST_NAME ks-async-embedded-host-for-test)
else()
	message(FATAL_ERROR "KS_ASYNC_LIB_TYPE=${KS_ASYNC_LIB_TYPE} is invalid")
endif()


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
	ks_notification.h
	ks_notification_center.h
	ks_notification_center.cpp

	#others
	ks_pending_trigger.h
	ks_pending_trigger.cpp
	ks_cancel_inspector.h
	ks_cancel_inspector.cpp
	ks_async_base.h
	ks_error.h
)

set(MY_ASYNC_RAW_SOURCE_FILES
	#ks-async-raw
	ks-async-raw/ks_raw_future.h
	ks-async-raw/ks_raw_future.cpp
	ks-async-raw/ks_raw_promise.h
	ks-async-raw/ks_raw_promise.cpp
	ks-async-raw/ks_raw_value.h
	ks-async-raw/ks_raw_result.h
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
	ks_notification.h
	ks_notification_center.h

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
	ks-async-raw/ks_raw_value.h
	ks-async-raw/ks_raw_result.h
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
endif()


#fetch extern gtest and gflags
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

	### gflags ...
    if (KS_ASYNC_EXTERNAL_GFLAGS_PROJECT_URL)
        FetchContent_Declare(
            gflags
            URL ${KS_ASYNC_EXTERNAL_GFLAGS_URL}
        )
    else()
        FetchContent_Declare(
            gflags
            URL https://github.com/gflags/gflags/archive/v2.2.2.tar.gz
        )
    endif()

    set(BUILD_gflags_LIB ON CACHE BOOL "" FORCE)
    set(BUILD_gflags_nothreads_LIB OFF CACHE BOOL "" FORCE)
    mark_as_advanced(FORCE BUILD_gflags_LIB BUILD_gflags_nothreads_LIB)

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gflags_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(gflags)
endif()


#lib
if (${KS_ASYNC_LIB_TYPE} STREQUAL "EMBEDDED")
	add_library(${MY_LIB_NAME} STATIC ${MY_SOURCE_FILES} ${MY_ASYNC_RAW_SOURCE_FILES} ${MY_KTL_SOURCE_FILES} __forcelinks.cpp)
	target_compile_definitions(${MY_LIB_NAME} PRIVATE KS_ASYNC_EXPORTS)
	set_property(TARGET ${MY_LIB_NAME} PROPERTY __EMBEDDED TRUE)
else()
	add_library(${MY_LIB_NAME} SHARED ${MY_SOURCE_FILES} ${MY_ASYNC_RAW_SOURCE_FILES} ${MY_KTL_SOURCE_FILES})
	target_compile_definitions(${MY_LIB_NAME} PRIVATE KS_ASYNC_EXPORTS)
endif()

#test
if (KS_ASYNC_TEST_ENABLED)
    add_executable(${MY_LIB_TEST_NAME} ${MY_TEST_SOURCE_FILES})
    target_include_directories(${MY_LIB_TEST_NAME} 
        PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/test  
    )
	
    if (${KS_ASYNC_LIB_TYPE} STREQUAL "EMBEDDED")
        add_library(${MY_LIB_NAME}-embedding SHARED __dll_forcelinker.cpp)
        target_link_libraries(${MY_LIB_NAME}-embedding PRIVATE ${MY_LIB_NAME})
        target_compile_definitions(${MY_LIB_NAME}-embedding PRIVATE KS_ASYNC_EXPORTS)
        target_link_libraries(${MY_LIB_TEST_NAME} PRIVATE ${MY_LIB_NAME}-embedding)
    else()
		target_link_libraries(${MY_LIB_TEST_NAME} PRIVATE ${MY_LIB_NAME})
    endif()

	target_link_libraries(${MY_LIB_TEST_NAME} PRIVATE gtest)
	target_link_libraries(${MY_LIB_TEST_NAME} PRIVATE gflags)
endif()


#install
install(TARGETS ${MY_LIB_NAME})
install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include/ks-async)
install(FILES ${PUBLIC_ASYNC_RAW_HEADER_FILES} DESTINATION include/ks-async/ks-async-raw)
install(FILES ${PUBLIC_KTL_HEADER_FILES} DESTINATION include/ks-async/ktl)
install(FILES "README.md" DESTINATION include/ks-async)
install(FILES "LICENSE" DESTINATION include/ks-async)
install(DIRECTORY "doc" DESTINATION include/ks-async)
