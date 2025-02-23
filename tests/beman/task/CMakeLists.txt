# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

list(
    APPEND
    task_tests
    single_thread_context
    affine_on
    allocator_of
    allocator_support
    any_scheduler
    completion
    error_types_of
    final_awaiter
    find_allocator
    handle
    inline_scheduler
    lazy
    poly
    promise_base
    promise_type
    result_type
    scheduler_of
    state_base
    sub_visit
    task
    with_error
)

foreach(test ${task_tests})
    add_executable(beman.task.tests.${test})
    target_sources(beman.task.tests.${test} PRIVATE ${test}.test.cpp)
    target_link_libraries(beman.task.tests.${test} PRIVATE beman::task)
    add_test(
        NAME beman.task.tests.${test}
        COMMAND $<TARGET_FILE:beman.task.tests.${test}>
    )
endforeach()
