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
        return result_t{*this, ::std::forward<Sender>(sndr), ::std::forward<Scheduler>(scheduler)};
    }
};

template <::beman::execution::sender Sender, ::beman::execution::scheduler Scheduler>
struct affine_on_t::sender {
    using sender_concept                 = ::beman::execution::sender_t;
    static constexpr bool elide_schedule = ::std::same_as<::beman::task::detail::inline_scheduler, Scheduler>;

    template <typename Env>
    auto get_completion_signatures(const Env& env) const& noexcept {
        if constexpr (elide_schedule) {
            return ::beman::execution::get_completion_signatures(
                ::std::remove_cvref_t<Sender>(::std::move(this->upstream)), env);
        } else {
            return ::beman::execution::get_completion_signatures(
                ::beman::execution::continues_on(this->upstream, this->scheduler), env);
        }
    }
    template <typename Env>
    auto get_completion_signatures(const Env& env) && noexcept {
        if constexpr (elide_schedule) {
            return ::beman::execution::get_completion_signatures(
                ::std::remove_cvref_t<Sender>(::std::move(this->upstream)), env);
        } else {
            return ::beman::execution::get_completion_signatures(
                ::beman::execution::continues_on(::std::move(this->upstream), ::std::move(this->scheduler)), env);
        }
    }

    affine_on_t tag{};
    Sender      upstream;
    Scheduler   scheduler;

    template <::beman::execution::receiver Receiver>
    auto connect(Receiver&& receiver) const& {
        if constexpr (elide_schedule) {
            return ::beman::execution::connect(this->upstream, ::std::forward<Receiver>(receiver));
        } else {
            return ::beman::execution::connect(::beman::execution::continues_on(this->upstream, this->scheduler),
                                               ::std::forward<Receiver>(receiver));
        }
    }
    template <::beman::execution::receiver Receiver>
    auto connect(Receiver&& receiver) && {
        if constexpr (elide_schedule) {
            return ::beman::execution::connect(::std::move(this->upstream), ::std::forward<Receiver>(receiver));
        } else {
            return ::beman::execution::connect(
                ::beman::execution::continues_on(::std::move(this->upstream), ::std::move(this->scheduler)),
                ::std::forward<Receiver>(receiver));
        }
    }
};
} // namespace beman::task::detail

namespace beman::task {
using affine_on_t = ::beman::task::detail::affine_on_t;
inline constexpr ::beman::task::detail::affine_on_t affine_on{};
}

// ----------------------------------------------------------------------------

#endif
