// include/beman/task/detail/into_optional.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_TASK_DETAIL_TASK
#define INCLUDED_BEMAN_TASK_DETAIL_TASK

#include <beman/task/detail/state.hpp>
#include <beman/task/detail/awaiter.hpp>
#include <beman/execution/execution.hpp>
#include <beman/task/detail/allocator_of.hpp>
#include <beman/task/detail/allocator_support.hpp>
#include <beman/task/detail/task_scheduler.hpp>
#include <beman/task/detail/completion.hpp>
#include <beman/task/detail/scheduler_of.hpp>
#include <beman/task/detail/stop_source.hpp>
#include <beman/task/detail/inline_scheduler.hpp>
#include <beman/task/detail/final_awaiter.hpp>
#include <beman/task/detail/handle.hpp>
#include <beman/task/detail/sub_visit.hpp>
#include <beman/task/detail/with_error.hpp>
#include <beman/task/detail/error_types_of.hpp>
#include <beman/task/detail/promise_type.hpp>
#include <beman/execution/detail/meta_combine.hpp>
#include <concepts>
#include <coroutine>
#include <optional>
#include <type_traits>

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

// ----------------------------------------------------------------------------

namespace beman::task::detail {

struct default_environment {};

template <typename Value = void, typename Env = default_environment>
class task {
  private:
    using stop_source_type = ::beman::task::detail::stop_source_of_t<Env>;
    using stop_token_type  = decltype(std::declval<stop_source_type>().get_token());

  public:
    using sender_concept        = ::beman::execution::sender_t;
    using completion_signatures = ::beman::execution::detail::meta::combine<
        ::beman::execution::completion_signatures<beman::task::detail::completion_t<Value>,
                                                  ::beman::execution::set_stopped_t()>,
        ::beman::task::detail::error_types_of_t<Env> >;

    using promise_type = ::beman::task::detail::promise_type<task, Value, Env>;

  private:
    template <typename Receiver>
    using state = ::beman::task::detail::state<task, Value, Env, Receiver>;

    ::beman::task::detail::handle<promise_type> handle;

  private:
    friend promise_type;
    explicit task(::beman::task::detail::handle<promise_type> h) : handle(std::move(h)) {}

  public:
    task(const task&)                = delete;
    task(task&&) noexcept            = default;
    task& operator=(const task&)     = delete;
    task& operator=(task&&) noexcept = delete;
    ~task()                          = default;

    template <typename Receiver>
    state<Receiver> connect(Receiver receiver) {
        return state<Receiver>(std::forward<Receiver>(receiver), std::move(this->handle));
    }
    auto as_awaitable() -> ::beman::task::detail::awaiter<Value, Env, promise_type> {
        return ::beman::task::detail::awaiter<Value, Env, promise_type>(::std::move(this->handle));
    }
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
