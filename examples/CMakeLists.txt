# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(ALL_EXAMPLES
    when-all
    task_scheduler
    environment
    c++now-basic
    c++now-result-types
    c++now-errors
    c++now-cancel
    c++now-return
    c++now-with_error
    c++now-allocator
    c++now-stop_token
    c++now-query
    c++now-affinity
    affinity
    escaped-exception
    co_await-result
    friendly
    loop
    stop
    alloc
    container
    hello
    query
    async-lock
    error
    into_optional
    result_example
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
