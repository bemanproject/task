// include/beman/task/detail/affine_on.hpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_AFFINE_ON
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_AFFINE_ON

#include <beman/execution/execution.hpp>
#include <beman/execution/detail/meta_unique.hpp>
#include <beman/task/detail/inline_scheduler.hpp>
#include <concepts>
#include <utility>
#include <tuple>
#include <variant>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
struct affine_on_t {
    template <::beman::execution::sender Sender>
    struct sender;

    template <::beman::execution::sender Sender>
    auto operator()(Sender&& sndr) const {
        using result_t = sender<::std::remove_cvref_t<Sender>>;
        static_assert(::beman::execution::sender<result_t>);
        return result_t(::std::forward<Sender>(sndr));
    }
};

template <::beman::execution::sender Sender>
struct affine_on_t::sender : ::beman::execution::detail::product_type<::beman::task::detail::affine_on_t, Sender> {
    using sender_concept = ::beman::execution::sender_t;

    template <typename Scheduler>
    static constexpr bool elide_schedule =
        ::std::same_as<::beman::task::detail::inline_scheduler, ::std::remove_cvref_t<Scheduler>>;

    template <typename Env>
    auto get_completion_signatures(const Env& env) const& noexcept {
        return ::beman::execution::get_completion_signatures(::std::remove_cvref_t<Sender>(this->template get<1>()),
                                                             env);
    }
    template <typename Env>
    auto get_completion_signatures(const Env& env) && noexcept {
        return ::beman::execution::get_completion_signatures(
            ::std::remove_cvref_t<Sender>(::std::move(this->template get<1>())), env);
    }

    template <::beman::execution::receiver Receiver>
    struct state {
        template <typename>
        struct to_tuple_t;
        template <typename R, typename... A>
        struct to_tuple_t<R(A...)> {
            using type = ::std::tuple<R, std::remove_cvref_t<A>...>;
        };
        template <typename>
        struct to_variant_t;
        template <typename... Sig>
        struct to_variant_t<::beman::execution::completion_signatures<Sig...>> {
            using type = ::beman::execution::detail::meta::unique<::std::variant<typename to_tuple_t<Sig>::type...>>;
        };

        using operation_state_concept = ::beman::execution::operation_state_t;
        using completion_signatures   = decltype(::beman::execution::get_completion_signatures(
            ::std::declval<Sender>(), ::beman::execution::get_env(::std::declval<Receiver>())));
        using value_type              = typename to_variant_t<completion_signatures>::type;

        struct schedule_receiver {
            using receiver_concept = ::beman::execution::receiver_t;
            state* s;
            auto   set_value() noexcept -> void {
                static_assert(::beman::execution::receiver<schedule_receiver>);
                std::visit(
                    [this](auto&& v) -> void {
                        std::apply(
                            [this](auto tag, auto&&... a) { tag(std::move(this->s->receiver), ::std::move(a)...); },
                            v);
                    },
                    s->value);
            }
            auto get_env() const noexcept -> ::beman::execution::empty_env { /*-dk:TODO */ return {}; }
        };

        struct work_receiver {
            using receiver_concept = ::beman::execution::receiver_t;
            state* s;
            template <typename... T>
            auto set_value(T&&... args) noexcept -> void {
                static_assert(::beman::execution::receiver<work_receiver>);
                this->s->value
                    .template emplace<::std::tuple<::beman::execution::set_value_t, ::std::remove_cvref_t<T>...>>(
                        ::beman::execution::set_value, ::std::forward<T>(args)...);
                this->s->sched_op_.start();
            }
            template <typename E>
            auto set_error(E&& error) noexcept -> void {
                static_assert(::beman::execution::receiver<work_receiver>);
                this->s->value
                    .template emplace<::std::tuple<::beman::execution::set_error_t, ::std::remove_cvref_t<E>>>(
                        ::beman::execution::set_error, ::std::forward<E>(error));
                this->s->sched_op_.start();
            }
            auto set_stopped(auto&&...) noexcept -> void {
                static_assert(::beman::execution::receiver<work_receiver>);
                this->s->value.template emplace<::std::tuple<::beman::execution::set_stopped_t>>(
                    ::beman::execution::set_stopped);
                this->s->sched_op_.start();
            }
            auto get_env() const noexcept -> decltype(::beman::execution::get_env(::std::declval<Receiver>())) {
                return ::beman::execution::get_env(this->s->receiver);
            }
        };
        using scheduler_t =
            decltype(::beman::execution::get_scheduler(::beman::execution::get_env(::std::declval<Receiver>())));
        using schedule_op = decltype(::beman::execution::connect(
            ::beman::execution::schedule(::std::declval<scheduler_t>()), ::std::declval<schedule_receiver>()));
        using work_op =
            decltype(::beman::execution::connect(::std::declval<Sender>(), ::std::declval<work_receiver>()));

        ::std::remove_cvref_t<Receiver> receiver;
        value_type                      value;
        schedule_op                     sched_op_;
        work_op                         work_op_;

        template <typename S, typename R>
        explicit state(S&& s, R&& r)
            : receiver(::std::forward<R>(r)),
              sched_op_(::beman::execution::connect(::beman::execution::schedule(::beman::execution::get_scheduler(
                                                        ::beman::execution::get_env(this->receiver))),
                                                    schedule_receiver{this})),
              work_op_(::beman::execution::connect(::std::forward<S>(s), work_receiver{this})) {
            static_assert(::beman::execution::operation_state<state>);
            if constexpr (not ::std::same_as<
                              ::beman::execution::completion_signatures<::beman::execution::set_value_t()>,
                              decltype(::beman::execution::get_completion_signatures(
                                  ::beman::execution::schedule(
                                      ::beman::execution::get_scheduler(::beman::execution::get_env(this->receiver))),
                                  ::beman::execution::get_env(this->receiver)))>) {
                static_assert(std::same_as<void,
                                           decltype(::beman::execution::get_scheduler(
                                               ::beman::execution::get_env(this->receiver)))>);
            }
            static_assert(::std::same_as<::beman::execution::completion_signatures<::beman::execution::set_value_t()>,
                                         decltype(::beman::execution::get_completion_signatures(
                                             ::beman::execution::schedule(::beman::execution::get_scheduler(
                                                 ::beman::execution::get_env(this->receiver))),
                                             ::beman::execution::get_env(this->receiver)))>);
        }
        auto start() & noexcept -> void { ::beman::execution::start(this->work_op_); }
    };

    template <typename S>
    sender(S&& s)
        : ::beman::execution::detail::product_type<::beman::task::detail::affine_on_t, Sender>{
              {{::beman::task::detail::affine_on_t{}}, {Sender(::std::forward<S>(s))}}} {}

    template <::beman::execution::receiver Receiver>
    auto connect(Receiver&& receiver) && {
        static_assert(::std::same_as<::beman::execution::completion_signatures<::beman::execution::set_value_t()>,
                                     decltype(::beman::execution::get_completion_signatures(
                                         ::beman::execution::schedule(
                                             ::beman::execution::get_scheduler(::beman::execution::get_env(receiver))),
                                         ::beman::execution::get_env(receiver)))>,
                      "affine_on requires that the receiver's scheduler is infallible");
        if constexpr (elide_schedule<decltype(::beman::execution::get_scheduler(
                          ::beman::execution::get_env(receiver)))>) {
            return ::beman::execution::connect(::std::move(this->template get<1>()),
                                               ::std::forward<Receiver>(receiver));
        } else {
            return state<Receiver>(::std::move(this->template get<1>()), ::std::forward<Receiver>(receiver));
        }
    }
};
} // namespace beman::task::detail

namespace beman::task {
using affine_on_t = ::beman::task::detail::affine_on_t;
inline constexpr ::beman::task::detail::affine_on_t affine_on{};
} // namespace beman::task

// ----------------------------------------------------------------------------

#endif
