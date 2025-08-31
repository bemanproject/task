// examples/issue-co_await-task.cpp                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
    struct coro {
        struct promise_type {
            coro get_return_object() { return {}; }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception() {}
        };

        struct awaiter {
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<>) noexcept {}
            void await_resume() noexcept {}
        };
        //awaiter operator co_await() { return {}; }
        bool await_ready() noexcept { return false; }
        auto await_suspend(std::coroutine_handle<> h) noexcept { return h; }
        void await_resume() noexcept {}
    };
}

int main() {
    []()->coro { co_await std::suspend_never{}; }();
    []()->coro { co_await []->coro { co_return; }(); }();
    ex::sync_wait([]()->ex::task<> { co_return; }());
    //[]()->coro { co_await []()->ex::task<> { co_return; }(); }();
}