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
#include <iostream> //-dk:TODO remove

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

// ----------------------------------------------------------------------------

namespace beman::task::detail {

struct default_environment {};

template <typename Value = void, typename Env = default_environment>
class task {
  private:
    template <typename Receiver>
    using state            = ::beman::task::detail::state<task, Value, Env, Receiver>;
    using stop_source_type = ::beman::task::detail::stop_source_of_t<Env>;
    using stop_token_type  = decltype(std::declval<stop_source_type>().get_token());

  public:
    using task_concept           = int;
    using sender_concept         = ::beman::execution::sender_t;
    using xcompletion_signatures = ::beman::execution::detail::meta::combine<
        ::beman::execution::completion_signatures<beman::task::detail::completion_t<Value>,
                                                  ::beman::execution::set_stopped_t()>,
        ::beman::task::detail::error_types_of_t<Env> >;
    template <typename Ev>
    auto get_completion_signatures(const Ev&) const& noexcept {
        return xcompletion_signatures{};
    }

    using promise_type = ::beman::task::detail::promise_type<task, Value, Env>;
    friend promise_type;

    task(const task&)                = delete;
    task(task&&) noexcept            = default;
    task& operator=(const task&)     = delete;
    task& operator=(task&&) noexcept = default;
    ~task()                          = default;

    template <typename Receiver>
    auto connect(Receiver receiver) -> state<Receiver> {
        return state<Receiver>(std::forward<Receiver>(receiver), std::move(this->handle));
    }
    template <typename ParentPromise>
    auto as_awaitable(ParentPromise&) -> ::beman::task::detail::awaiter<Value, Env, promise_type, ParentPromise> {
        return ::beman::task::detail::awaiter<Value, Env, promise_type, ParentPromise>(::std::move(this->handle));
    }

    struct dom_sender {
        using sender_concept        = ::beman::execution::sender_t;
        using completion_signatures = ::beman::execution::completion_signatures< ::beman::execution::set_value_t()>;
        struct state {
            using operation_state_concept = ::beman::execution::operation_state_t;
            auto start() & noexcept {}
        };

        auto connect(auto&&) noexcept -> state { return state{}; }
    };
    struct domain {
        template <typename DS>
        auto transform_sender(DS&&, auto&&...) const noexcept {
            std::cout << "task::domain::transform_sender\n";
            return dom_sender{};
        }
    };
    struct env {
        auto query(const ::beman::execution::get_domain_t&) const noexcept -> domain {
            std::cout << "task::env::get_domain\n";
            return domain{};
        }
    };
    auto xget_env() const noexcept {
        std::cout << "task::get_env\n";
        return env{};
    }

  private:
    ::beman::task::detail::handle<promise_type> handle;

    explicit task(::beman::task::detail::handle<promise_type> h) : handle(std::move(h)) {}
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
