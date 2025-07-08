// examples/issue-symmetric-transfer.cpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <functional>
#define NODEBUG
#include <cassert>

namespace ex = beman::execution;

template <typename Env>
ex::task<void, Env> test() {
    for (std::size_t i{}; i < 1000000; ++i) {
        // co_await std::invoke([]() -> ex::task<> { co_await ex::just(); });
        co_await std::invoke([]() -> ex::task<> { co_return; });
    }
}

int main() {
    struct affine_env {};
    struct inline_env {
        using scheduler_type = ex::inline_scheduler;
    };
    [[maybe_unused]] auto env = ex::inline_scheduler{};

    ex::sync_wait(test<affine_env>()); // OK
    // ex::sync_wait(test<inline_env>()); // error: stack overflow without symmetric transfer
}
