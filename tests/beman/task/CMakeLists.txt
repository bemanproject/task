# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(task_tests
    allocator_of
    allocator_support
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
    single_thread_context
    state_base
    sub_visit
    task_scheduler
    with_error
)

if(NOT MSVC)
    list(APPEND task_tests msvc-asan-issue task)
endif()
set(task_tests)

foreach(test ${task_tests})
    add_executable(beman.task.tests.${test})
    target_sources(beman.task.tests.${test} PRIVATE ${test}.test.cpp)
    target_link_libraries(beman.task.tests.${test} PRIVATE beman::task_headers)
    add_test(
        NAME beman.task.tests.${test}
        COMMAND $<TARGET_FILE:beman.task.tests.${test}>
    )
endforeach()
