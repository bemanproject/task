// include/beman/task/detail/task_scheduler.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_TASK_DETAIL_task_scheduler
#define INCLUDED_BEMAN_TASK_DETAIL_task_scheduler

#include <beman/execution/execution.hpp>
#include <beman/task/detail/poly.hpp>
#include <new>
#include <optional>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::task::detail {

/*!
 * \brief Type-erasing scheduler
 * \headerfile beman/task/task.hpp <beman/task/task.hpp>
 *
 * The class `task_scheduler` is used to type-erase any scheduler class.
 * Any error produced by the underlying scheduler except `std::error_code` is turned into
 * an `std::exception_ptr`. `std::error_code` is forwarded as is. The `task_scheduler`
 * forwards stop requests reported by the stop token obtained from the `connect`ed
 * receiver to the sender used by the underlying scheduler.
 *
 * Completion signatures:
 *
 * - `ex::set_value_t()`
 * - `ex::set_error_t(std::error_code)`
 * - `ex::set_error_t(std::exception_ptr)`
 * - `ex::set_stopped()`
 *
 * Usage:
 *
 *     task_scheduler sched(other_scheduler);
 *     auto sender{ex::schedule(sched) | some_sender};
 */
class task_scheduler {
    struct state_base {
        virtual ~state_base()                                                               = default;
        virtual void                                   complete_value()                     = 0;
        virtual void                                   complete_error(::std::error_code)    = 0;
        virtual void                                   complete_error(::std::exception_ptr) = 0;
        virtual void                                   complete_stopped()                   = 0;
        virtual ::beman::execution::inplace_stop_token get_stop_token()                     = 0;
    };

    struct inner_state {
        struct receiver;
        struct env {
            state_base* state;
            auto query(::beman::execution::get_stop_token_t) const noexcept { return this->state->get_stop_token(); }
        };
        struct receiver {
            using receiver_concept = ::beman::execution::receiver_t;
            state_base* state;
            void        set_value() && noexcept { this->state->complete_value(); }
            void        set_error(std::error_code err) && noexcept { this->state->complete_error(err); }
            void        set_error(std::exception_ptr ptr) && noexcept { this->state->complete_error(std::move(ptr)); }
            template <typename E>
            void set_error(E e) && noexcept {
                this->state->complete_error(std::make_exception_ptr(std::move(e)));
            }
            void set_stopped() && noexcept { this->state->complete_stopped(); }
            env  get_env() const noexcept { return {this->state}; }
        };
        static_assert(::beman::execution::receiver<receiver>);

        struct base {
            virtual ~base()      = default;
            virtual void start() = 0;
        };
        template <::beman::execution::sender Sender>
        struct concrete : base {
            using state_t = decltype(::beman::execution::connect(std::declval<Sender>(), std::declval<receiver>()));
            state_t state;
            template <::beman::execution::sender S>
            concrete(S&& s, state_base* b) : state(::beman::execution::connect(std::forward<S>(s), receiver{b})) {}
            void start() override { ::beman::execution::start(state); }
        };
        ::beman::task::detail::poly<base, 16u * sizeof(void*)> state;
        template <::beman::execution::sender S>
        inner_state(S&& s, state_base* b) : state(static_cast<concrete<S>*>(nullptr), std::forward<S>(s), b) {}
        void start() { this->state->start(); }
    };

    template <::beman::execution::receiver Receiver>
    struct state : state_base {
        using operation_state_concept = ::beman::execution::operation_state_t;
        struct stopper {
            state* st;
            void   operator()() noexcept {
                state* self = this->st;
                self->callback.reset();
                self->source.request_stop();
            }
        };
        using token_t =
            decltype(::beman::execution::get_stop_token(::beman::execution::get_env(std::declval<Receiver>())));
        using callback_t = ::beman::execution::stop_callback_for_t<token_t, stopper>;

        std::remove_cvref_t<Receiver>           receiver;
        inner_state                             s;
        ::beman::execution::inplace_stop_source source;
        ::std::optional<callback_t>             callback;

        template <::beman::execution::receiver R, typename PS>
        state(R&& r, PS& ps) : receiver(std::forward<R>(r)), s(ps->connect(this)) {}
        void start() & noexcept { this->s.start(); }
        void complete_value() override { ::beman::execution::set_value(std::move(this->receiver)); }
        void complete_error(std::error_code err) override { ::beman::execution::set_error(std::move(receiver), err); }
        void complete_error(std::exception_ptr ptr) override {
            ::beman::execution::set_error(std::move(receiver), std::move(ptr));
        }
        void complete_stopped() override { ::beman::execution::set_stopped(std::move(this->receiver)); }
        ::beman::execution::inplace_stop_token get_stop_token() override {
            if constexpr (::std::same_as<token_t, ::beman::execution::inplace_stop_token>) {
                return ::beman::execution::get_stop_token(::beman::execution::get_env(this->receiver));
            } else {
                if constexpr (not ::std::same_as<token_t, ::beman::execution::never_stop_token>) {
                    if (not this->callback) {
                        this->callback.emplace(
                            ::beman::execution::get_stop_token(::beman::execution::get_env(this->receiver)),
                            stopper{this});
                    }
                }
                return this->source.get_token();
            }
        }
    };

    class sender;
    class env {
        friend class sender;

      private:
        const sender* sndr;
        env(const sender* s) : sndr(s) {}

      public:
        task_scheduler
        query(const ::beman::execution::get_completion_scheduler_t<::beman::execution::set_value_t>&) const noexcept {
            return this->sndr->inner_sender->get_completion_scheduler();
        }
    };

    // sender implementation
    class sender {
        friend class env;

      private:
        struct base {
            virtual ~base()                                         = default;
            virtual base*          move(void*)                      = 0;
            virtual base*          clone(void*) const               = 0;
            virtual inner_state    connect(state_base*)             = 0;
            virtual task_scheduler get_completion_scheduler() const = 0;
        };
        template <::beman::execution::scheduler Scheduler>
        struct concrete : base {
            using sender_t = decltype(::beman::execution::schedule(std::declval<Scheduler>()));
            sender_t sender;

            template <::beman::execution::scheduler S>
            concrete(S&& s) : sender(::beman::execution::schedule(std::forward<S>(s))) {}
            base*          move(void* buffer) override { return new (buffer) concrete(std::move(*this)); }
            base*          clone(void* buffer) const override { return new (buffer) concrete(*this); }
            inner_state    connect(state_base* b) override { return inner_state(::std::move(sender), b); }
            task_scheduler get_completion_scheduler() const override {
                return task_scheduler(::beman::execution::get_completion_scheduler<::beman::execution::set_value_t>(
                    ::beman::execution::get_env(this->sender)));
            }
        };
        poly<base, 4 * sizeof(void*)> inner_sender;

      public:
        using sender_concept = ::beman::execution::sender_t;
        using completion_signatures =
            ::beman::execution::completion_signatures<::beman::execution::set_value_t(),
                                                      ::beman::execution::set_error_t(std::error_code),
                                                      ::beman::execution::set_error_t(std::exception_ptr),
                                                      ::beman::execution::set_stopped_t()>;

        template <::beman::execution::scheduler S>
        explicit sender(S&& s) : inner_sender(static_cast<concrete<S>*>(nullptr), std::forward<S>(s)) {}
        sender(sender&&)      = default;
        sender(const sender&) = default;

        template <::beman::execution::receiver R>
        state<R> connect(R&& r) {
            return state<R>(std::forward<R>(r), this->inner_sender);
        }

        env get_env() const noexcept { return env(this); }
    };

    // scheduler implementation
    struct base {
        virtual ~base()                          = default;
        virtual sender schedule()                = 0;
        virtual base*  move(void* buffer)        = 0;
        virtual base*  clone(void*) const        = 0;
        virtual bool   equals(const base*) const = 0;
    };
    template <::beman::execution::scheduler Scheduler>
    struct concrete : base {
        Scheduler scheduler;
        template <typename S>
            requires ::beman::execution::scheduler<::std::remove_cvref_t<S>>
        explicit concrete(S&& s) : scheduler(std::forward<S>(s)) {}
        sender schedule() override { return sender(this->scheduler); }
        base*  move(void* buffer) override { return new (buffer) concrete(std::move(*this)); }
        base*  clone(void* buffer) const override { return new (buffer) concrete(*this); }
        bool   equals(const base* o) const override {
            auto other{dynamic_cast<const concrete*>(o)};
            return other ? this->scheduler == other->scheduler : false;
        }
    };

    poly<base, 4 * sizeof(void*)> scheduler;

  public:
    using scheduler_concept = ::beman::execution::scheduler_t;

    template <typename S, typename Allocator = ::std::allocator<void>>
        requires(not std::same_as<task_scheduler, std::remove_cvref_t<S>>) &&
                ::beman::execution::scheduler<::std::remove_cvref_t<S>>
    explicit task_scheduler(S&& s, Allocator = {})
        : scheduler(static_cast<concrete<std::decay_t<S>>*>(nullptr), std::forward<S>(s)) {}
    task_scheduler(task_scheduler&&)      = default;
    task_scheduler(const task_scheduler&) = default;
    template <typename Allocator>
    task_scheduler(const task_scheduler& other, Allocator) : scheduler(other.scheduler) {}
    task_scheduler& operator=(const task_scheduler&) = default;
    ~task_scheduler()                                = default;

    sender schedule() { return this->scheduler->schedule(); }
    bool   operator==(const task_scheduler&) const = default;
    template <typename Sched>
        requires(not ::std::same_as<task_scheduler, Sched>) && ::beman::execution::scheduler<Sched>
    bool operator==(const Sched& other [[maybe_unused]]) const {
        return *this == task_scheduler(other);
    }
};
static_assert(::beman::execution::scheduler<task_scheduler>);

} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
