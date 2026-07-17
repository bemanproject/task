# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.30...4.3)

# include(../cmake/prelude.cmake)

project(beman_task.example LANGUAGES CXX)

# TODO(CK): cmake -S . -G Ninja -B build -D CMAKE_BUILD_TYPE=RelWithDebInfo --fresh
if(PROJECT_IS_TOP_LEVEL)
    # include(../cmake/cxx-modules-rules.cmake)
    set(BEMAN_USE_MODULES OFF)
    set(BEMAN_USE_STD_MODULE OFF)
    set(CMAKE_CXX_STANDARD 23)

    find_package(beman.task 0.2.0 EXACT REQUIRED)

    enable_testing()
endif()

set(TODO tls_scheduler into_optional issue_start_reschedules loop)

set(ALL_EXAMPLES
    task_scheduler
    rvalue_task
    customize
    issue_symmetric_transfer
    container
)

if(NOT MSVC)
    list(
        APPEND ALL_EXAMPLES
        task_sender
        alloc_1
        alloc_2
        c++now_allocator
        c++now_cancel
        c++now_errors
        alloc
        affinity
        odd_return
        dangling_references
        aggregate_return
        issue_affine
        c++now_affinity
        c++now_basic
        # FIXME: c++now_query
        c++now_result_types
        c++now_return
        # FIXME: c++now_stop_token
        c++now_with_error
        co_await_result
        co_await_task
        error
        escaped_exception
        friendly
        hello
        issue_frame_allocator
        # FIXME: query
        result_example
        # FIXME: stop
    )
endif()

set(xALL_EXAMPLES issue_symmetric_transfer)
set(xALL_EXAMPLES customize)

message(STATUS "Examples to be built: ${ALL_EXAMPLES}")

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.task.examples.${example})
    target_sources(beman.task.examples.${example} PRIVATE ${example}.cpp)
    target_link_libraries(beman.task.examples.${example} beman::task_headers)
    add_test(
        NAME beman.task.examples.${example}
        COMMAND $<TARGET_FILE:beman.task.examples.${example}>
    )
endforeach()
