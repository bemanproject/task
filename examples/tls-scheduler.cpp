// examples/hello.cpp                                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>

namespace ex = beman::execution;

struct tls_domain {
    template <::beman::execution::sender Sndr, ::beman::execution::scheduler Sch, ::beman::execution::receiver Rcvr>
    struct affine_state {
        struct receiver {
            using receiver_concept = ::beman::execution::receiver_t;
            affine_state* st;
            auto          set_value() && noexcept -> void { this->st->complete(); }
            template <typename E>
            auto set_error(E&& e) && noexcept -> void {
                ::beman::execution::set_error(::std::move(this->st->rcvr), ::std::forward<E>(e));
            }
            auto set_stopped() && noexcept -> void { ::beman::execution::set_stopped(::std::move(this->st->rcvr)); }
        };
        using operation_state_concept = ::beman::execution::operation_state_t;
        using state_t                 = decltype(::beman::execution::connect(
            ::beman::execution::affine_on(::std::declval<Sndr>(), ::std::declval<Sch>()), ::std::declval<receiver>()));

        std::remove_cvref_t<Rcvr> rcvr;
        state_t                   state;

        template <::beman::execution::sender S, ::beman::execution::scheduler SC, ::beman::execution::receiver R>
        affine_state(S&& s, SC&& sc, R&& r)
            : rcvr(::std::forward<R>(r)),
              state(::beman::execution::connect(
                  ::beman::execution::affine_on(::std::forward<S>(s), ::std::forward<SC>(sc)), receiver{this})) {}
        auto start() & noexcept {
            std::cout << "affine_state::start\n";
            ::beman::execution::start(this->state);
        }
        auto complete() {
            std::cout << "affine_state::complete\n";
            ::beman::execution::set_value(::std::move(this->rcvr));
        }
    };
    template <::beman::execution::sender Sndr, ::beman::execution::scheduler Sch>
    struct affine_sender {
        using sender_concept = ::beman::execution::sender_t;

        std::remove_cvref_t<Sndr> sndr;
        std::remove_cvref_t<Sch>  sch;

        template <typename Env>
        auto get_completion_signatures(const Env& env) const noexcept {
            return ::beman::execution::get_completion_signatures(this->sndr, env);
        }
        template <::beman::execution::receiver Rcvr>
        auto connect(Rcvr&& rcvr) && {
            return affine_state<Sndr, Sch, Rcvr>(std::move(sndr), std::move(sch), std::forward<Rcvr>(rcvr));
        }
    };
    template <::beman::execution::sender Sndr, typename... Env>
        requires std::same_as<::beman::execution::tag_of_t<::std::remove_cvref_t<Sndr>>,
                              ::beman::execution::affine_on_t>
    auto transform_sender(Sndr&& s, const Env&... env) const noexcept {
        auto [tag, sch, sndr] = s;
        return affine_sender<decltype(sndr), decltype(sch)>{::std::forward_like<Sndr>(sndr),
                                                            ::std::forward_like<Sndr>(sch)};
    }
};

template <::beman::execution::scheduler Scheduler>
struct tls_scheduler {
    using scheduler_concept = ::beman::execution::scheduler_t;
    struct env {
        Scheduler sched;
        template <typename Tag>
        auto query(const ::beman::execution::get_completion_scheduler_t<Tag>&) const noexcept -> tls_scheduler {
            return {this->sched};
        }
    };
    template <::beman::execution::receiver Receiver>
    struct state {
        using operation_state_concept = ::beman::execution::operation_state_t;
        using upstream_state_t        = decltype(::beman::execution::connect(
            ::beman::execution::schedule(std::declval<Scheduler>()), std::declval<std::remove_cvref_t<Receiver>>()));

        upstream_state_t upstream;
        template <::beman::execution::sender Sndr, ::beman::execution::receiver Rcvr>
        state(Sndr&& sndr, Rcvr&& rcvr)
            : upstream(::beman::execution::connect(std::forward<Sndr>(sndr), std::forward<Rcvr>(rcvr))) {}
        auto start() & noexcept -> void { ::beman::execution::start(this->upstream); }
    };
    struct tls_sender {
        using sender_concept = ::beman::execution::sender_t;
        using sender_type    = decltype(::beman::execution::schedule(std::declval<Scheduler>()));

        Scheduler sched;

        template <typename... Env>
        auto get_completion_signatures(const Env&... env) const noexcept {
            return ::beman::execution::get_completion_signatures(::beman::execution::schedule(Scheduler(this->sched)),
                                                                 env...);
        }
        auto get_env() const noexcept -> env { return {this->sched}; }

        template <::beman::execution::receiver Receiver>
        auto connect(Receiver&& rcvr) && -> state<Receiver> {
            return state<Receiver>(::beman::execution::schedule(this->sched), std::forward<Receiver>(rcvr));
        }
    };

    Scheduler sched;
    template <typename Sch>
        requires(!std::same_as<tls_scheduler, std::remove_cvref_t<Sch>>)
    tls_scheduler(Sch&& sch) : sched(sch) {}

    auto schedule() -> tls_sender { return {this->sched}; }

    auto query(const ::beman::execution::get_domain_t&) const noexcept -> tls_domain { return {}; }

    auto operator==(const tls_scheduler&) const -> bool = default;
};
static_assert(::beman::execution::sender<tls_scheduler<::beman::execution::task_scheduler>::tls_sender>);
static_assert(::beman::execution::sender_in<tls_scheduler<::beman::execution::task_scheduler>::tls_sender,
                                            ::beman::execution::empty_env>);
static_assert(::beman::execution::scheduler<tls_scheduler<::beman::execution::task_scheduler>>);

// ----------------------------------------------------------------------------

struct tls_env {
    using scheduler_type = tls_scheduler<::beman::execution::task_scheduler>;
};

int main() {
    ex::sync_wait([]() -> ex::task<void, tls_env> {
        std::cout << "before co_await\n";
        co_await (ex::just() | ex::then([] { std::cout << "co_await'ed then\n"; }));
        std::cout << "after co_await\n";
    }());
}
