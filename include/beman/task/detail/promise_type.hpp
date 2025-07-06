// include/beman/task/detail/promise_type.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_PROMISE_TYPE
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_PROMISE_TYPE

#include <beman/task/detail/awaiter.hpp>
#include <beman/task/detail/affine_on.hpp>
#include <beman/task/detail/allocator_of.hpp>
#include <beman/task/detail/allocator_support.hpp>
#include <beman/task/detail/change_coroutine_scheduler.hpp>
#include <beman/task/detail/error_types_of.hpp>
#include <beman/task/detail/final_awaiter.hpp>
#include <beman/task/detail/find_allocator.hpp>
#include <beman/task/detail/handle.hpp>
#include <beman/task/detail/inline_scheduler.hpp>
#include <beman/task/detail/promise_base.hpp>
#include <beman/task/detail/result_type.hpp>
#include <beman/task/detail/scheduler_of.hpp>
#include <beman/task/detail/state_base.hpp>
#include <beman/task/detail/with_error.hpp>
#include <beman/execution/execution.hpp>
#include <beman/execution/detail/meta_contains.hpp>
#include <beman/task/detail/promise_env.hpp>
#include <cassert>
#include <coroutine>
#include <optional>
#include <type_traits>

// ----------------------------------------------------------------------------

namespace beman::task::detail {

template <typename Coroutine, typename Value, typename Environment>
class promise_type
    : public ::beman::task::detail::
          promise_base<::beman::task::detail::stoppable::yes, ::std::remove_cvref_t<Value>, Environment>,
      public ::beman::task::detail::allocator_support<::beman::task::detail::allocator_of_t<Environment>> {
  public:
    using allocator_type   = ::beman::task::detail::allocator_of_t<Environment>;
    using scheduler_type   = ::beman::task::detail::scheduler_of_t<Environment>;
    using stop_source_type = ::beman::task::detail::stop_source_of_t<Environment>;
    using stop_token_type  = decltype(std::declval<stop_source_type>().get_token());

    template <typename... A>
    promise_type(const A&... a) : allocator(::beman::task::detail::find_allocator<allocator_type>(a...)) {}

    constexpr auto initial_suspend() noexcept -> ::std::suspend_always { return {}; }
    constexpr auto final_suspend() noexcept -> ::beman::task::detail::final_awaiter { return {}; }

    auto                    unhandled_exception() noexcept { /*-dk:TODO*/ }
    std::coroutine_handle<> unhandled_stopped() {
        this->get_state()->complete();
        return std::noop_coroutine();
    }

    auto get_return_object() noexcept { return Coroutine(::beman::task::detail::handle<promise_type>(this)); }

    template <::beman::execution::sender Sender>
    auto await_transform(Sender&& sender) noexcept {
        if constexpr (requires { ::std::forward<Sender>(sender).as_awaitable(); }) {
            return ::std::forward<Sender>(sender).as_awaitable();
        } else if constexpr (requires { ::beman::execution::as_awaitable(::std::forward<Sender>(sender), *this); }) {
            return ::beman::execution::as_awaitable(::std::forward<Sender>(sender), *this);
        } else {
            return ::beman::execution::as_awaitable(
                ::beman::task::affine_on(::std::forward<Sender>(sender), this->get_scheduler()), *this);
        }
    }
    auto await_transform(::beman::task::detail::change_coroutine_scheduler<scheduler_type> c) {
        return ::std::move(c);
    }

    template <typename E>
    auto yield_value(with_error<E> with) noexcept -> ::beman::task::detail::final_awaiter {
        this->get_state()->set_error(::std::move(with.error));
        return {};
    }

    auto get_env() const noexcept -> ::beman::task::detail::promise_env<promise_type> { return {this}; }

    auto start(::beman::task::detail::state_base<Value, Environment>* state) -> ::std::coroutine_handle<> {
        this->set_state(state);
        return ::std::coroutine_handle<promise_type>::from_promise(*this);
    }
    auto           notify_complete() -> ::std::coroutine_handle<> { return this->get_state()->complete(); }
    scheduler_type change_scheduler(scheduler_type other) {
        return this->get_state()->set_scheduler(::std::move(other));
    }

    auto get_scheduler() const noexcept -> scheduler_type { return this->get_state()->get_scheduler(); }
    auto get_allocator() const noexcept -> allocator_type { return this->allocator; }
    auto get_stop_token() const noexcept -> stop_token_type { return this->get_state()->get_stop_token(); }
    auto get_environment() const noexcept -> const Environment& { return this->get_state()->get_environment(); }

  private:
    using env_t = ::beman::task::detail::promise_env<promise_type>;

    allocator_type                  allocator{};
    ::std::optional<scheduler_type> scheduler{};
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
