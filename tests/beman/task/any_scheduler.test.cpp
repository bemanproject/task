// tests/beman/task/any_scheduler.test.cpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/any_scheduler.hpp>
#include <beman/task/detail/inline_scheduler.hpp>
#include <beman/execution/execution.hpp>
#include <beman/execution/stop_token.hpp>
#include <atomic>
#include <latch>
#include <exception>
#include <system_error>
#include <thread>
#include <condition_variable>
#include <mutex>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

namespace ex = beman::execution;
namespace ly = beman::task;

// ----------------------------------------------------------------------------

namespace {
struct thread_context {
    enum class complete { success, failure, exception, never };
    struct base {
        base* next{};
        virtual ~base()         = default;
        virtual void complete() = 0;
    };

    std::mutex              mutex;
    std::condition_variable condition;
    bool                    done{false};
    base*                   work{};
    std::thread             thread;

    base* get_work() {
        std::unique_lock cerberus(this->mutex);
        condition.wait(cerberus, [this] { return this->done || this->work; });
        base* rc{this->work};
        if (rc) {
            this->work = rc->next;
        }
        return rc;
    }
    void enqueue(base* w) {
        {
            std::lock_guard cerberus(this->mutex);
            w->next = std::exchange(this->work, w);
        }
        this->condition.notify_one();
    }

    thread_context()
        : thread([this] {
              while (auto w{this->get_work()}) {
                  w->complete();
              }
          }) {}
    ~thread_context() {
        this->stop();
        this->thread.join();
    }

    struct scheduler {
        using scheduler_concept = ex::scheduler_t;
        thread_context* context;
        complete        cmpl{complete::success};
        bool            operator==(const scheduler&) const = default;

        template <ex::receiver Receiver>
        struct state : base {
            struct stopper {
                state* st;
                void   operator()() noexcept {
                    auto self{this->st};
                    self->callback.reset();
                    ex::set_stopped(std::move(self->receiver));
                }
            };
            using operation_state_concept = ex::operation_state_t;
            using token_t                 = decltype(ex::get_stop_token(ex::get_env(std::declval<Receiver>())));
            using callback_t              = ex::stop_callback_for_t<token_t, stopper>;

            thread_context*               ctxt;
            std::remove_cvref_t<Receiver> receiver;
            thread_context::complete      cmpl;
            std::optional<callback_t>     callback;

            template <typename R>
            state(auto c, R&& r, thread_context::complete cm) : ctxt(c), receiver(std::forward<R>(r)), cmpl(cm) {}
            void start() & noexcept {
                callback.emplace(ex::get_stop_token(ex::get_env(this->receiver)), stopper{this});
                if (cmpl != thread_context::complete::never)
                    this->ctxt->enqueue(this);
            }
            void complete() override {
                this->callback.reset();
                if (this->cmpl == thread_context::complete::success)
                    ex::set_value(std::move(this->receiver));
                else if (this->cmpl == thread_context::complete::failure)
                    ex::set_error(std::move(this->receiver), std::make_error_code(std::errc::address_in_use));
                else
                    ex::set_error(
                        std::move(this->receiver),
                        std::make_exception_ptr(std::system_error(std::make_error_code(std::errc::address_in_use))));
            }
        };
        struct env {
            thread_context* ctxt;
            scheduler       query(const ex::get_completion_scheduler_t<ex::set_value_t>&) const noexcept {
                return scheduler{ctxt};
            }
        };
        struct sender {
            using sender_concept = ex::sender_t;
            using completion_signatures =
                ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::error_code)>;

            thread_context*          ctxt;
            thread_context::complete cmpl;

            template <ex::receiver Receiver>
            auto connect(Receiver&& receiver) {
                static_assert(ex::operation_state<state<Receiver>>);
                return state<Receiver>(this->ctxt, std::forward<Receiver>(receiver), this->cmpl);
            }
            env get_env() const noexcept { return {this->ctxt}; }
        };
        static_assert(ex::sender<sender>);

        sender schedule() noexcept { return sender{this->context, this->cmpl}; }
    };
    static_assert(ex::scheduler<scheduler>);

    scheduler get_scheduler(complete cmpl = complete::success) { return scheduler{this, cmpl}; }
    void      stop() {
        {
            std::lock_guard cerberus(this->mutex);
            this->done = true;
        }
        this->condition.notify_one();
    }
};

enum class stop_result { none, success, failure, stopped };
template <typename Token>
struct stop_env {
    Token token;
    auto  query(ex::get_stop_token_t) const noexcept { return this->token; }
};
template <typename Token>
stop_env(Token&&) -> stop_env<std::remove_cvref_t<Token>>;

template <typename Token>
struct stop_receiver {
    using receiver_concept = ex::receiver_t;
    Token        token;
    stop_result& result;
    std::latch*  completed{};
    auto         get_env() const noexcept { return stop_env{this->token}; }

    void set_value(auto&&...) && noexcept {
        this->result = stop_result::success;
        if (this->completed)
            this->completed->count_down();
    }
    void set_error(auto&&) && noexcept {
        this->result = stop_result::failure;
        if (this->completed)
            this->completed->count_down();
    }
    void set_stopped() && noexcept {
        this->result = stop_result::stopped;
        if (this->completed)
            this->completed->count_down();
    }
};
template <typename Token>
stop_receiver(Token&&, stop_result&, std::latch* = nullptr) -> stop_receiver<std::remove_cvref_t<Token>>;
static_assert(ex::receiver<stop_receiver<ex::inplace_stop_token>>);

} // namespace

// ----------------------------------------------------------------------------

int main() {
    static_assert(ex::scheduler<ly::detail::any_scheduler>);

    thread_context ctxt1;
    thread_context ctxt2;

    assert(ctxt1.get_scheduler() == ctxt1.get_scheduler());
    assert(ctxt2.get_scheduler() == ctxt2.get_scheduler());
    assert(ctxt1.get_scheduler() != ctxt2.get_scheduler());

    ly::detail::any_scheduler sched1(ctxt1.get_scheduler());
    ly::detail::any_scheduler sched2(ctxt2.get_scheduler());
    assert(sched1 == sched1);
    assert(sched2 == sched2);
    assert(sched1 != sched2);

    ly::detail::any_scheduler copy(sched1);
    assert(copy == sched1);
    assert(copy != sched2);
    ly::detail::any_scheduler move(std::move(copy));
    assert(move == sched1);
    assert(move != sched2);

    copy = sched2;
    assert(copy == sched2);
    assert(copy != sched1);

    move = std::move(copy);
    assert(move == sched2);
    assert(move != sched1);

    std::atomic<std::thread::id> id1{};
    std::atomic<std::thread::id> id2{};
    ex::sync_wait(ex::schedule(sched1) | ex::then([&id1]() { id1 = std::this_thread::get_id(); }));
    ex::sync_wait(ex::schedule(sched2) | ex::then([&id2]() { id2 = std::this_thread::get_id(); }));
    assert(id1 != id2);
    ex::sync_wait(ex::schedule(ly::detail::any_scheduler(sched1)) |
                  ex::then([&id1]() { assert(id1 == std::this_thread::get_id()); }));
    ex::sync_wait(ex::schedule(ly::detail::any_scheduler(sched2)) |
                  ex::then([&id2]() { assert(id2 == std::this_thread::get_id()); }));

    {
        bool success{false};
        bool failed{false};
        bool exception{false};
        ex::sync_wait(ex::schedule(ctxt1.get_scheduler(thread_context::complete::failure)) |
                      ex::then([&success] { success = true; }) | ex::upon_error([&failed, &exception](auto err) {
                          if constexpr (std::same_as<decltype(err), std::error_code>)
                              failed = true;
                          else if constexpr (std::same_as<decltype(err), std::exception_ptr>)
                              exception = true;
                      }));
        assert(not success);
        assert(failed);
        assert(not exception);
    }
    {
        bool success{false};
        bool failed{false};
        bool exception{false};
        ex::sync_wait(ex::schedule(ctxt1.get_scheduler(thread_context::complete::exception)) |
                      ex::then([&success] { success = true; }) | ex::upon_error([&failed, &exception](auto err) {
                          if constexpr (std::same_as<decltype(err), std::error_code>)
                              failed = true;
                          else if constexpr (std::same_as<decltype(err), std::exception_ptr>)
                              exception = true;
                      }));
        assert(not success);
        assert(not failed);
        assert(exception);
    }
    {
        ex::inplace_stop_source source;
        stop_result             result{stop_result::none};
        auto                    state{ex::connect(ex::schedule(ctxt1.get_scheduler(thread_context::complete::never)),
                               stop_receiver{source.get_token(), result})};
        assert(result == stop_result::none);
        ex::start(state);
        assert(result == stop_result::none);
        source.request_stop();
        assert(result == stop_result::stopped);
    }
    {
        ex::inplace_stop_source source;
        stop_result             result{stop_result::none};
        auto                    state{
            ex::connect(ex::schedule(ly::detail::any_scheduler(ctxt1.get_scheduler(thread_context::complete::never))),
                        stop_receiver{source.get_token(), result})};
        assert(result == stop_result::none);
        ex::start(state);
        assert(result == stop_result::none);
        source.request_stop();
        assert(result == stop_result::stopped);
    }
    {
        ex::stop_source source;
        stop_result     result{stop_result::none};
        auto            state{
            ex::connect(ex::schedule(ly::detail::any_scheduler(ctxt1.get_scheduler(thread_context::complete::never))),
                        stop_receiver{source.get_token(), result})};
        assert(result == stop_result::none);
        ex::start(state);
        assert(result == stop_result::none);
        source.request_stop();
        assert(result == stop_result::stopped);
    }
    {
        std::latch  completed{1};
        stop_result result{stop_result::none};
        auto        state{ex::connect(
            ex::schedule(ly::detail::any_scheduler(ctxt1.get_scheduler(thread_context::complete::success))),
            stop_receiver{ex::never_stop_token(), result, &completed})};
        assert(result == stop_result::none);
        ex::start(state);
        completed.wait();
        assert(result == stop_result::success);
    }
}
