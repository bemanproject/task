// include/beman/task/detail/promise_type.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_LAZY_DETAIL_PROMISE_TYPE
#define INCLUDED_INCLUDE_BEMAN_LAZY_DETAIL_PROMISE_TYPE

#include <beman/task/detail/affine_on.hpp>
#include <beman/task/detail/allocator_of.hpp>
#include <beman/task/detail/allocator_support.hpp>
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
#include <coroutine>
#include <optional>
#include <type_traits>

// ----------------------------------------------------------------------------

struct opt_rcvr {
    using receiver_concept = ::beman::execution::receiver_t;
    void set_value(auto&&...) && noexcept {}
    void set_error(auto&&) && noexcept {}
    void set_stopped() && noexcept {}
};

namespace beman::task::detail {

template <typename Scheduler>
struct optional_ref_scheduler {
    using scheduler_concept = ::beman::execution::scheduler_t;
    using ptr_type          = ::std::optional<Scheduler>*;

    template <typename Receiver>
    struct state {
        using operation_state_concept = ::beman::execution::operation_state_t;
        ptr_type sched;
        Receiver receiver;
        using inner_t =
            decltype(::beman::execution::connect(::beman::execution::schedule(**sched), ::std::declval<Receiver>()));
        struct connector {
            inner_t inner;
            template <typename S, typename R>
            connector(S&& s, R&& r) : inner(::beman::execution::connect(::std::forward<S>(s), ::std::forward<R>(r))) {}
            void start() { ::beman::execution::start(this->inner); }
        };
        std::optional<connector> inner;
        void                     start() & noexcept {
            inner.emplace(::beman::execution::schedule(**this->sched), ::std::forward<Receiver>(receiver));
            (*this->inner).start();
        }
    };
    struct env {
        ptr_type sched;

        template <typename Tag>
        optional_ref_scheduler query(::beman::execution::get_completion_scheduler_t<Tag>) const noexcept {
            return {this->sched};
        }
    };
    struct sender {
        using sender_concept = ::beman::execution::sender_t;
        using completion_signatures =
            ::beman::execution::completion_signatures<::beman::execution::set_value_t(),
                                                      ::beman::execution::set_error_t(::std::exception_ptr),
                                                      ::beman::execution::set_error_t(::std::system_error),
                                                      ::beman::execution::set_stopped_t()>;
        ptr_type sched;
        template <::beman::execution::receiver Receiver>
        auto connect(Receiver&& receiver) {
            return state<::std::remove_cvref_t<Receiver>>{this->sched, ::std::forward<Receiver>(receiver), {}};
        }
        env get_env() const noexcept { return {this->sched}; }
    };
    static_assert(::beman::execution::sender<sender>);

    ptr_type sched;
    sender   schedule() const noexcept { return {this->sched}; }
    bool     operator==(const optional_ref_scheduler&) const = default;
};
static_assert(::beman::execution::scheduler<::beman::task::detail::any_scheduler>);
static_assert(::beman::execution::scheduler<::beman::task::detail::inline_scheduler>);
static_assert(::beman::execution::scheduler<optional_ref_scheduler<::beman::task::detail::any_scheduler>>);
static_assert(::beman::execution::scheduler<optional_ref_scheduler<::beman::task::detail::inline_scheduler>>);

template <typename Coroutine, typename T, typename C>
struct promise_type : ::beman::task::detail::promise_base<::beman::task::detail::stoppable::yes,
                                                          ::std::remove_cvref_t<T>,
                                                          ::beman::task::detail::error_types_of_t<C>>,
                      ::beman::task::detail::allocator_support<::beman::task::detail::allocator_of_t<C>> {
    using allocator_type   = ::beman::task::detail::allocator_of_t<C>;
    using scheduler_type   = ::beman::task::detail::scheduler_of_t<C>;
    using stop_source_type = ::beman::task::detail::stop_source_of_t<C>;
    using stop_token_type  = decltype(std::declval<stop_source_type>().get_token());

    struct receiver {
        using receiver_concept = ::beman::execution::receiver_t;
        promise_type* self{};
        void set_value() && noexcept { std::coroutine_handle<promise_type>::from_promise(*this->self).resume(); }
        void set_error(auto&&) && noexcept {
            //-dk:TODO
        }
        void set_stopped() && noexcept {
            //-dk:TODO
        }
    };
    struct connector {
        decltype(::beman::execution::connect(
            ::beman::execution::schedule(::std::declval<::beman::task::detail::any_scheduler>()),
            ::std::declval<receiver>())) state;
        connector(::beman::task::detail::any_scheduler scheduler, receiver rcvr)
            : state(::beman::execution::connect(::beman::execution::schedule(::std::move(scheduler)),
                                                ::std::move(rcvr))) {}
    };

    void notify_complete() { this->state->complete(); }
    void start(auto&& e, ::beman::task::detail::state_base<C>* s) {
        this->state = s;
        if constexpr (std::same_as<::beman::task::detail::inline_scheduler, scheduler_type>) {
            this->scheduler.emplace();
        } else {
            this->scheduler.emplace(::beman::execution::get_scheduler(e));
        }
        this->initial->run();
    }

    template <typename... A>
    promise_type(const A&... a) : allocator(::beman::task::detail::find_allocator<allocator_type>(a...)) {}

    struct initial_base {
        virtual ~initial_base() = default;
        virtual void run()      = 0;
    };
    struct initial_sender {
        using sender_concept        = ::beman::execution::sender_t;
        using completion_signatures = ::beman::execution::completion_signatures<::beman::execution::set_value_t()>;

        template <::beman::execution::receiver Receiver>
        struct state : initial_base {
            using operation_state_concept = ::beman::execution::operation_state_t;
            promise_type* promise;
            Receiver      receiver;
            template <typename R>
            state(promise_type* p, R&& r) : promise(p), receiver(::std::forward<R>(r)) {}
            void start() & noexcept { this->promise->initial = this; }
            void run() override { ::beman::execution::set_value(::std::move(receiver)); }
        };

        promise_type* promise{};
        template <::beman::execution::receiver Receiver>
        auto connect(Receiver&& receiver) {
            return state<::std::remove_cvref_t<Receiver>>(this->promise, ::std::forward<Receiver>(receiver));
        }
    };

    auto initial_suspend() noexcept {
        return this->internal_await_transform(initial_sender{this},
                                              optional_ref_scheduler<scheduler_type>{&this->scheduler});
    }
    final_awaiter final_suspend() noexcept { return {}; }
    void          unhandled_exception() { this->set_error(std::current_exception()); }
    auto          get_return_object() { return Coroutine(::beman::task::detail::handle<promise_type>(this)); }

    template <typename E>
    auto await_transform(::beman::task::detail::with_error<E> with) noexcept {
        // This overload is only used if error completions use `co_await with_error(e)`.
        return std::move(with);
    }
    template <::beman::execution::sender Sender, typename Scheduler>
    auto internal_await_transform(Sender&& sender, Scheduler&& sched) noexcept {
        if constexpr (std::same_as<::beman::task::detail::inline_scheduler, scheduler_type>)
            return ::beman::execution::as_awaitable(std::forward<Sender>(sender), *this);
        else
            return ::beman::execution::as_awaitable(
#if 0
                ::beman::task::affine_on(::std::forward<Sender>(sender), ::std::forward<Scheduler>(sched)),
#else
                ::beman::execution::continues_on(::std::forward<Sender>(sender), ::std::forward<Scheduler>(sched)),
#endif
                *this);
    }
    template <::beman::execution::sender Sender>
    auto await_transform(Sender&& sender) noexcept {
        return this->internal_await_transform(::std::forward<Sender>(sender), *this->scheduler);
    }

    template <typename E>
    final_awaiter yield_value(with_error<E> with) noexcept {
        this->result.template emplace<E>(with.error);
        return {};
    }

    [[no_unique_address]] allocator_type  allocator;
    std::optional<scheduler_type>         scheduler{};
    ::beman::task::detail::state_base<C>* state{};
    initial_base*                         initial{};

    std::coroutine_handle<> unhandled_stopped() {
        this->state->complete();
        return std::noop_coroutine();
    }

    struct env {
        const promise_type* promise;

        scheduler_type  query(::beman::execution::get_scheduler_t) const noexcept { return *promise->scheduler; }
        allocator_type  query(::beman::execution::get_allocator_t) const noexcept { return promise->allocator; }
        stop_token_type query(::beman::execution::get_stop_token_t) const noexcept {
            return promise->state->get_stop_token();
        }
        template <typename Q, typename... A>
            requires requires(const C& c, Q q, A&&... a) { q(c, std::forward<A>(a)...); }
        auto query(Q q, A&&... a) const noexcept {
            return q(promise->state->get_context(), std::forward<A>(a)...);
        }
    };

    env get_env() const noexcept { return {this}; }
};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
