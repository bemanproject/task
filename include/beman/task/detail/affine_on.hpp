// include/beman/task/detail/affine_on.hpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_AFFINE_ON
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_AFFINE_ON

#include <beman/execution/execution.hpp>
#include <beman/task/detail/inline_scheduler.hpp>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
struct affine_on_t {
    template <::beman::execution::sender Sender>
    struct sender;

    template <::beman::execution::sender Sender>
    auto operator()(Sender&& sndr, auto&&) const {
        using result_t = sender<::std::remove_cvref_t<Sender>>;
        static_assert(::beman::execution::sender<result_t>);
        return result_t(::std::forward<Sender>(sndr));
    }
};

template <::beman::execution::sender Sender>
struct affine_on_t::sender
    : ::beman::execution::detail::product_type<::beman::task::detail::affine_on_t, Sender> {
    using sender_concept                 = ::beman::execution::sender_t;

    template <typename Scheduler>
    static constexpr bool elide_schedule = ::std::same_as<::beman::task::detail::inline_scheduler, ::std::remove_cvref_t<Scheduler>>;

    template <typename Env>
    auto get_completion_signatures(const Env& env) const& noexcept {
        if constexpr (elide_schedule<decltype(::beman::execution::get_scheduler(::std::declval<Env>()))>) {
            return ::beman::execution::get_completion_signatures(
                ::std::remove_cvref_t<Sender>(::std::move(this->template get<2>())), env);
        } else {
            return ::beman::execution::get_completion_signatures(
                ::beman::execution::continues_on(this->template get<1>(), ::beman::execution::get_scheduler(env)), env);
        }
    }
    template <typename Env>
    auto get_completion_signatures(const Env& env) && noexcept {
        if constexpr (elide_schedule<decltype(::beman::execution::get_scheduler(::std::declval<Env>()))>) {
            return ::beman::execution::get_completion_signatures(
                ::std::remove_cvref_t<Sender>(::std::move(this->template get<1>())), env);
        } else {
            return ::beman::execution::get_completion_signatures(
                ::beman::execution::continues_on(::std::move(this->template get<1>()),
                                                 ::beman::execution::get_scheduler(env)),
                env);
        }
    }

    template <typename S>
    sender(S&& s)
        : ::beman::execution::detail::product_type<::beman::task::detail::affine_on_t, Sender>{
              {{::beman::task::detail::affine_on_t{}},
               {Sender(::std::forward<S>(s))}}} {}

    template <::beman::execution::receiver Receiver>
    auto connect(Receiver&& receiver) const& {
        if constexpr (elide_schedule<decltype(::beman::execution::get_scheduler(::beman::execution::get_env(receiver)))>) {
            return ::beman::execution::connect(this->template get<1>(), ::std::forward<Receiver>(receiver));
        } else {
            return ::beman::execution::connect(
                    ::beman::execution::continues_on(this->template get<1>(),
                    ::beman::execution::get_scheduler(::beman::execution::get_env(receiver))
                ),
                ::std::forward<Receiver>(receiver));
        }
    }
    template <::beman::execution::receiver Receiver>
    auto connect(Receiver&& receiver) && {
        if constexpr (elide_schedule<decltype(::beman::execution::get_scheduler(::beman::execution::get_env(receiver)))>) {
            return ::beman::execution::connect(::std::move(this->template get<1>()),
                                               ::std::forward<Receiver>(receiver));
        } else {
            return ::beman::execution::connect(::beman::execution::continues_on(
                                                    ::std::move(this->template get<1>()),
                                               ::beman::execution::get_scheduler(::beman::execution::get_env(receiver))),
                                               ::std::forward<Receiver>(receiver));
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
