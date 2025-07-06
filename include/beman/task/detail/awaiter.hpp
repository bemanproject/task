// include/beman/task/detail/awaiter.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_AWAITER
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_AWAITER

#include <beman/task/detail/handle.hpp>
#include <beman/task/detail/state_base.hpp>
#include <coroutine>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
template <typename Value, typename Env, typename Promise>
class awaiter : public ::beman::task::detail::state_base<Value, Env> {
  public:
    using stop_token_type = typename ::beman::task::detail::state_base<Value, Env>::stop_token_type;
    using scheduler_type  = typename ::beman::task::detail::state_base<Value, Env>::scheduler_type;

    explicit awaiter(::beman::task::detail::handle<Promise> h) : handle(::std::move(h)) {}
    constexpr auto await_ready() const noexcept -> bool { return false; }
    template <typename PP>
    auto await_suspend(::std::coroutine_handle<PP> parent) noexcept {
        this->scheduler.emplace(
            this->template from_env<scheduler_type>(::beman::execution::get_env(parent.promise())));
        this->parent = ::std::move(parent);
        assert(this->parent);
        return this->handle.start(this);
    }
    auto await_resume() { return this->result_resume(); }
    auto parent_handle() -> ::std::coroutine_handle<> { return ::std::move(this->parent); }

  private:
    auto do_complete() -> std::coroutine_handle<> override {
        assert(this->parent);
        return ::std::move(this->parent);
    }
    auto do_get_scheduler() -> scheduler_type override { return *this->scheduler; }
    auto do_set_scheduler(scheduler_type other) -> scheduler_type override {
        return ::std::exchange(*this->scheduler, other);
    }
    auto do_get_stop_token() -> stop_token_type override { return {}; }
    auto do_get_environment() -> Env& override { return this->env; }

    Env                                    env;
    ::std::optional<scheduler_type>        scheduler;
    ::beman::task::detail::handle<Promise> handle;
    ::std::coroutine_handle<>              parent{};
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
