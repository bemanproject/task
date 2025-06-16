// examples/when-all.cpp                                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <concepts>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

int main() {
    ex::sync_wait([]()->ex::task<>{
        [[maybe_unused]] auto s = ex::when_all(
            ex::just(true),
            ex::just(10)
            );
        //static_assert(std::same_as<void, decltype(ex::get_completion_signatures(s, ex::empty_env{}))>);
        co_await std::move(s);
        co_return;
        }());
}
