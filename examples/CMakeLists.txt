# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.30...4.3)

# include(../cmake/prelude.cmake)

project(beman_execution.example LANGUAGES CXX)

# TODO(CK): cmake -S . -G Ninja -B build -D CMAKE_BUILD_TYPE=RelWithDebInfo --fresh
if(PROJECT_IS_TOP_LEVEL)
    # include(../cmake/cxx-modules-rules.cmake)

    find_package(beman.task 0.1.0 EXACT REQUIRED)

    enable_testing()
endif()

set(TODO tls-scheduler into_optional issue-start-reschedules loop)

set(ALL_EXAMPLES
    task_scheduler
    bulk
    c++now-allocator
    c++now-cancel
    c++now-errors
    alloc
    affinity
    odd-return
    dangling-references
    rvalue-task
    aggregate-return
    customize
    issue-affine_on
    issue-symmetric-transfer
    c++now-affinity
    c++now-basic
    # FIXME: c++now-query
    c++now-result-types
    c++now-return
    # FIXME: c++now-stop_token
    c++now-with_error
    co_await-result
    co_await-task
    container
    error
    escaped-exception
    friendly
    hello
    issue-frame-allocator
    # FIXME: query
    result_example
    # FIXME: stop
)

set(xALL_EXAMPLES issue-symmetric-transfer)
set(xALL_EXAMPLES customize)

message("Examples to be built: ${ALL_EXAMPLES}")

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.task.examples.${example})
    target_sources(beman.task.examples.${example} PRIVATE ${example}.cpp)
    target_link_libraries(beman.task.examples.${example} beman::task)
    add_test(
        NAME beman.task.examples.${example}
        COMMAND $<TARGET_FILE:beman.task.examples.${example}>
    )
endforeach()
