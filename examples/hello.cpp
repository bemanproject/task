// examples/hello.cpp                                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>

namespace ex = beman::execution;
namespace ly = beman::task;

int main() {
    return std::get<0>(*ex::sync_wait([]() -> ex::task<int> {
        std::cout << "Hello, world!\n";
        co_return co_await ex::just(0);
    }()));
}
