// include/beman/task/detail/inline_scheduler.hpp                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_TASK_DETAIL_INLINE_SCHEDULER
#define INCLUDED_BEMAN_TASK_DETAIL_INLINE_SCHEDULER

#include <beman/execution/execution.hpp>
#include <utility>
#include <type_traits>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
/*!
 * \brief Scheduler completing immediately when started on the same thread
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 *
 * The class `inline_scheduler` is used to prevent any actual schedulering.
 * It does have a scheduler interface but it completes synchronously on
 * the thread on which it gets `start`ed before returning from `start`.
 * The implication is that any blocking working gets executed on the
 * calling thread. Also, if there is lot of synchronous work repeatedly
 * getting scheduled using `inline_scheduler` it is possible to get a
 * stack overflow.
 *
 * In general, any use of `inline_scheduler` should receive a lot of
 * attention as it is fairly easy to create subtle bugs using this scheduler.
 */
struct inline_scheduler {
    struct env {
        inline_scheduler
        query(const ::beman::execution::get_completion_scheduler_t<::beman::execution::set_value_t>&) const noexcept {
            return {};
        }
    };
    template <::beman::execution::receiver Receiver>
    struct state {
        using operation_state_concept = ::beman::execution::operation_state_t;
        std::remove_cvref_t<Receiver> receiver;
        void                          start() & noexcept { ::beman::execution::set_value(std::move(receiver)); }
    };
    struct sender {
        using sender_concept        = ::beman::execution::sender_t;
        using completion_signatures = ::beman::execution::completion_signatures<::beman::execution::set_value_t()>;

        env get_env() const noexcept { return {}; }
        template <::beman::execution::receiver Receiver>
        state<Receiver> connect(Receiver&& receiver) {
            return {std::forward<Receiver>(receiver)};
        }
    };
    static_assert(::beman::execution::sender<sender>);

    using scheduler_concept = ::beman::execution::scheduler_t;
    constexpr sender schedule() noexcept { return {}; }
    bool             operator==(const inline_scheduler&) const = default;
};
static_assert(::beman::execution::scheduler<inline_scheduler>);
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
