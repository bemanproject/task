// examples/hello.cpp                                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>

namespace ex = beman::execution;

int main() {
    struct affine_env {};
    struct inline_env {
        using scheduler_type = ex::inline_scheduler;
    };
    return std::get<0>(ex::sync_wait([]() -> ex::task<int, inline_env> {
            for (std::size_t i{}; i < 1000000; ++i) {
                co_await std::invoke([]()->ex::task<> {
                    co_await ex::just();
                });
            }
            co_return 0;
        }()).value_or(std::tuple(-1)));
}
