// include/beman/task/detail/affine_on.hpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_AFFINE_ON
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_AFFINE_ON

#include <beman/execution/execution.hpp>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
struct affine_on_t {
    template <::beman::execution::receiver Receiver>
    struct state {
        using operation_state_concept = ::beman::execution::operation_state_t;

        void start() & noexcept {
            //-dk:TODO
        }
    };
    template <::beman::execution::sender Sender, ::beman::execution::scheduler Scheduler>
    struct sender {
        using sender_concept = ::beman::execution::sender_t;
        template <typename Env>
        auto get_completion_signatures(const Env& env) const {
            return ::beman::execution::get_completion_signatures(this->upstream, env);
        }

        Sender    upstream;
        Scheduler scheduler;

        template <::beman::execution::receiver Receiver>
        auto connect(Receiver&&) const {
            using result_t = state<::std::remove_cvref_t<Receiver>>;
            static_assert(::beman::execution::operation_state<result_t>);
            return result_t{};
        }
    };

    template <::beman::execution::sender Sender, ::beman::execution::scheduler Scheduler>
    auto operator()(Sender&& sndr, Scheduler&& scheduler) const {
        using result_t = sender<::std::remove_cvref_t<Sender>, ::std::remove_cvref_t<Scheduler>>;
        static_assert(::beman::execution::sender<result_t>);
        return result_t{::std::forward<Sender>(sndr), ::std::forward<Scheduler>(scheduler)};
    }
};
} // namespace beman::task::detail

namespace beman::task {
inline constexpr ::beman::task::detail::affine_on_t affine_on{};
}

// ----------------------------------------------------------------------------

#endif
