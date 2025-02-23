// tests/beman/task/affine_on.test.cpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/affine_on.hpp>
#include <beman/task/detail/single_thread_context.hpp>
#include <beman/execution/execution.hpp>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------

namespace {
struct receiver {
    using receiver_concept = ex::receiver_t;

    void set_value(auto&&...) && noexcept {}
    void set_error(auto&&) && noexcept {}
    void set_stopped() && noexcept {}
};
static_assert(ex::receiver<receiver>);
} // namespace

int main() {
    beman::task::detail::single_thread_context context;

    auto main_id{std::this_thread::get_id()};
    auto [thread_id]{
        ex::sync_wait(ex::schedule(context.get_scheduler()) | ex::then([] { return std::this_thread::get_id(); }))
            .value_or(std::tuple{std::thread::id{}})};

    assert(main_id != thread_id);

    auto s(beman::task::affine_on(ex::just(42), context.get_scheduler()));
    (void)s;
    static_assert(ex::sender<decltype(s)>);
    auto st(ex::connect(std::move(s), receiver{}));
    (void)st;
#if 0
    ex::sync_wait(beman::task::affine_on(ex::just(42), context.get_scheduler())
#else
    ex::sync_wait(beman::execution::continues_on(ex::just(42), context.get_scheduler())
#endif
                  | ex::then([thread_id](int value) {
                        assert(thread_id == std::this_thread::get_id());
                        assert(value == 42);
                    }));
}
