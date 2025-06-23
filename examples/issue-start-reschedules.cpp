// examples/issue-start-reschedules.cpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include "demo-thread_pool.hpp"
#include <iostream>
#include <thread>

namespace ex = beman::execution;

ex::task<> test(auto sched) {
    std::cout << "init =" << std::this_thread::get_id() << "\n";
    co_await ex::starts_on(sched, ex::just());
    std::cout << "final=" << std::this_thread::get_id() << "\n";
}

int main() {
    demo::thread_pool pool1;
    demo::thread_pool pool2;
    std::cout << "main =" << std::this_thread::get_id() << "\n";
    ex::sync_wait(ex::schedule(pool1.get_scheduler()) | ex::then([]{
        std::cout << "pool1=" << std::this_thread::get_id() << "\n";
    }));
    ex::sync_wait(ex::schedule(pool2.get_scheduler()) | ex::then([]{
        std::cout << "pool2=" << std::this_thread::get_id() << "\n";
    }));
    std::cout << "--- use 1 ---\n";
    ex::sync_wait(test(pool2.get_scheduler()));
    std::cout << "--- use 2 ---\n";
    ex::sync_wait(ex::starts_on(pool1.get_scheduler(), test(pool2.get_scheduler())));
}
