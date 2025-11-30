# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(TODO tls-scheduler)

set(ALL_EXAMPLES
    odd-return
    bulk
    dangling-references
    rvalue-task
    aggregate-return
    customize
    issue-symmetric-transfer
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
    error
    escaped-exception
    friendly
    hello
    into_optional
    issue-affine_on
    issue-frame-allocator
    issue-start-reschedules
    loop
    query
    result_example
    stop
    task_scheduler
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
