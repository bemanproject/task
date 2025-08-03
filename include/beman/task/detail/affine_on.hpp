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
    template <::beman::execution::sender Sender, ::beman::execution::scheduler Scheduler>
    struct sender;

    template <::beman::execution::sender Sender, ::beman::execution::scheduler Scheduler>
    auto operator()(Sender&& sndr, Scheduler&& scheduler) const {
        using result_t = sender<::std::remove_cvref_t<Sender>, ::std::remove_cvref_t<Scheduler>>;
        static_assert(::beman::execution::sender<result_t>);
        return result_t(::std::forward<Scheduler>(scheduler), ::std::forward<Sender>(sndr));
    }
};

template <::beman::execution::sender Sender, ::beman::execution::scheduler Scheduler>
struct affine_on_t::sender
    : ::beman::execution::detail::product_type<::beman::task::detail::affine_on_t, Scheduler, Sender> {
    using sender_concept                 = ::beman::execution::sender_t;
    static constexpr bool elide_schedule = ::std::same_as<::beman::task::detail::inline_scheduler, Scheduler>;

    template <typename Env>
    auto get_completion_signatures(const Env& env) const& noexcept {
        if constexpr (elide_schedule) {
            return ::beman::execution::get_completion_signatures(
                ::std::remove_cvref_t<Sender>(::std::move(this->template get<2>())), env);
        } else {
            return ::beman::execution::get_completion_signatures(
                ::beman::execution::continues_on(this->template get<2>(), this->template get<1>()), env);
        }
    }
    template <typename Env>
    auto get_completion_signatures(const Env& env) && noexcept {
        if constexpr (elide_schedule) {
            return ::beman::execution::get_completion_signatures(
                ::std::remove_cvref_t<Sender>(::std::move(this->template get<2>())), env);
        } else {
            return ::beman::execution::get_completion_signatures(
                ::beman::execution::continues_on(::std::move(this->template get<2>()),
                                                 ::std::move(this->template get<1>())),
                env);
        }
    }

    template <typename S, typename Sch>
    sender(Sch&& sch, S&& s)
        : ::beman::execution::detail::product_type<::beman::task::detail::affine_on_t, Scheduler, Sender>{
              {{::beman::task::detail::affine_on_t{}},
               {Scheduler(::std::forward<Sch>(sch))},
               {Sender(::std::forward<S>(s))}}} {}

    template <::beman::execution::receiver Receiver>
    auto connect(Receiver&& receiver) const& {
        if constexpr (elide_schedule) {
            return ::beman::execution::connect(this->template get<2>(), ::std::forward<Receiver>(receiver));
        } else {
            return ::beman::execution::connect(
                ::beman::execution::continues_on(this->template get<2>(), this->template get<1>()),
                ::std::forward<Receiver>(receiver));
        }
    }
    template <::beman::execution::receiver Receiver>
    auto connect(Receiver&& receiver) && {
        if constexpr (elide_schedule) {
            return ::beman::execution::connect(::std::move(this->template get<2>()),
                                               ::std::forward<Receiver>(receiver));
        } else {
            return ::beman::execution::connect(::beman::execution::continues_on(::std::move(this->template get<2>()),
                                                                                ::std::move(this->template get<1>())),
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
