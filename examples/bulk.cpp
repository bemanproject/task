// examples/bulk.cpp                                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/execution/task.hpp>
#include <execution>

namespace ex = beman::execution;
namespace beman::execution {
    inline constexpr struct par_t {} par{};
    using parallel_scheduler = inline_scheduler;
}

// ----------------------------------------------------------------------------

int main() {
    struct env {
        auto query(ex::get_scheduler_t) const noexcept { return ex::parallel_scheduler(); }
    };
    struct work {
        auto operator()(std::size_t s){ /*...*/ };
    };

    ex::sync_wait(
        ex::write_env(ex::bulk(ex::just(), 16u, work{}),
        env{}
    ));

    ex::sync_wait(ex::write_env(
        []()->ex::task<void, ex::empty_env> { co_await ex::bulk(ex::just(), 16u, work{}); }(),
        env{}
    ));
}
