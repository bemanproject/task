# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(ALL_EXAMPLES co_await-task)
set(ALL_EXAMPLES
    affinity
    alloc
    c++now-affinity
    c++now-allocator
    c++now-basic
    c++now-cancel
    c++now-errors
    c++now-query
    c++now-result-types
    c++now-return
    c++now-stop_token
    c++now-with_error
    co_await-result
    co_await-task
    container
    environment
    error
    escaped-exception
    friendly
    hello
    into_optional
    loop
    query
    result_example
    stop
    task_scheduler
)

message("Examples to be built: ${ALL_EXAMPLES}")

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.task.examples.${example})
    target_sources(beman.task.examples.${example} PRIVATE ${example}.cpp)
    if(WIN32)
        target_link_libraries(beman.task.examples.${example} beman::task)
    else()
        target_link_libraries(
            beman.task.examples.${example}
            beman::task
            beman::net
        )
    endif()
    add_test(
        NAME beman.task.examples.${example}
        COMMAND $<TARGET_FILE:beman.task.examples.${example}>
    )
endforeach()
