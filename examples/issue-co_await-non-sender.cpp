// examples/issue-co_await-non-sender.cpp                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
    struct awaitable {
        bool await_ready() const noexcept { return false; }
        auto await_suspend(std::coroutine_handle<> h) const noexcept { return h; }
        int await_resume() const noexcept { return 42; }
    };
}

int main()
{
    static_assert(ex::sender<awaitable>);
    ex::sync_wait(awaitable{} | ex::then([](int v) { return v + 1; }));
    auto t = []() -> ex::task<int> {
        co_return co_await awaitable{};
    }();
    ex::sync_wait(std::move(t));
}