// tests/beman/task/promise_type.test.cpp                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/task/detail/promise_type.hpp>
#include <beman/task/detail/task_scheduler.hpp>
#include <beman/task/detail/inline_scheduler.hpp>
#include <beman/execution/execution.hpp>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <stdexcept>
#include <latch>
#include <cassert>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

namespace ex = beman::execution;
namespace bt = beman::task::detail;

// ----------------------------------------------------------------------------

namespace {

void unreachable(const char* msg) { assert(nullptr == msg); }

struct thread_pool {

    struct node {
        node*        next{};
        virtual void run() = 0;

        node()                       = default;
        node(const node&)            = delete;
        node(node&&)                 = delete;
        virtual ~node()              = default;
        node& operator=(const node&) = delete;
        node& operator=(node&&)      = delete;
    };

    std::mutex              mutex;
    std::condition_variable condition;
    node*                   stack{};
    bool                    stopped{false};
    std::thread             driver{[this] {
        while (std::optional<node*> n = [this] {
            std::unique_lock cerberus(mutex);
            condition.wait(cerberus, [this] { return stopped || stack; });
            return this->stack ? std::optional<node*>(std::exchange(this->stack, this->stack->next))
                                           : std::optional<node*>();
        }()) {
            (*n)->run();
        }
    }};

    thread_pool()                   = default;
    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&)      = delete;
    ~thread_pool() {
        this->stop();
        this->driver.join();
    }
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&)      = delete;
    void         stop() {
        {
            std::lock_guard cerberus(this->mutex);
            stopped = true;
        }
        this->condition.notify_one();
    }

    struct scheduler {
        using scheduler_concept = ex::scheduler_t;
        struct env {
            thread_pool* pool;

            template <typename T>
            scheduler query(const ex::get_completion_scheduler_t<T>&) const noexcept {
                return {this->pool};
            }
        };
        template <typename Receiver>
        struct state final : thread_pool::node {
            using operation_state_concept = ex::operation_state_t;
            std::remove_cvref_t<Receiver> receiver;
            thread_pool*                  pool;

            template <typename R>
            state(R&& r, thread_pool* p) : node{}, receiver(std::forward<R>(r)), pool(p) {}
            void start() & noexcept {
                {
                    std::lock_guard cerberus(this->pool->mutex);
                    this->next = std::exchange(this->pool->stack, this);
                }
                this->pool->condition.notify_one();
            }
            void run() override { ex::set_value(std::move(this->receiver)); }
        };
        struct sender {
            using sender_concept        = ex::sender_t;
            using completion_signatures = ex::completion_signatures<ex::set_value_t()>;
            thread_pool* pool;
            template <typename Receiver>
            state<Receiver> connect(Receiver&& receiver) {
                return state<Receiver>(std::forward<Receiver>(receiver), pool);
            }

            env get_env() const noexcept { return {this->pool}; }
        };
        thread_pool* pool;
        sender       schedule() { return {this->pool}; }
        bool         operator==(const scheduler&) const = default;
    };
    scheduler get_scheduler() { return {this}; }
};

static_assert(ex::scheduler<thread_pool::scheduler>);

struct environment {};

struct test_error : std::exception {
    int value;
    explicit test_error(int v) : value(v) {}
};

struct test_task : beman::task::detail::state_base<int, environment> {

    using promise_type = beman::task::detail::promise_type<test_task, int, environment>;

    beman::task::detail::handle<promise_type> handle;
    explicit test_task(beman::task::detail::handle<promise_type> h) : handle(std::move(h)) {}

    void run() {
        this->handle.start(this);
        this->latch.wait();
    }
    template <beman::execution::receiver Receiver>
    void complete(Receiver&& receiver) {
        this->complete(receiver);
    }

    using stop_source_type = beman::task::detail::stop_source_of_t<environment>;
    using stop_token_type  = decltype(std::declval<stop_source_type>().get_token());

    std::latch       latch{1u};
    environment      env;
    stop_source_type source;

    std::coroutine_handle<> do_complete() override {
        this->latch.count_down();
        return std::noop_coroutine();
    }
    stop_token_type do_get_stop_token() override { return this->source.get_token(); }
    environment&    do_get_environment() override { return this->env; }
    auto            do_get_scheduler() -> scheduler_type override { return scheduler_type(bt::inline_scheduler()); }
    auto            do_set_scheduler(scheduler_type other) -> scheduler_type override {
        return scheduler_type(bt::inline_scheduler());
    }

    beman::task::detail::task_scheduler scheduler{beman::task::detail::inline_scheduler{}};
    beman::task::detail::task_scheduler query(beman::execution::get_scheduler_t) const noexcept {
        return this->scheduler;
    }
};

struct exception_receiver {
    using receiver_concept = beman::execution::receiver_t;
    bool& flag;

    auto set_value(int) && noexcept { unreachable("unexcepted set_value"); }
    auto set_stopped() && noexcept { unreachable("unexcepted set_stopped"); }
    auto set_error(const std::exception_ptr& ex) && noexcept {
        flag = true;
        try {
            std::rethrow_exception(ex);
        } catch (const test_error& error) {
            assert(error.value == 17);
        } catch (...) {
            unreachable("unexpected exception");
        }
    }
};

void test_exception() {
    auto coro{[]() -> test_task {
        throw test_error(17);
        co_return 0;
    }()};
    coro.run();
    bool flag{};
    coro.complete(exception_receiver{flag});
    assert(flag == true);
}

void test_initial_scheduler() {
    static_assert(ex::sender<std::suspend_always>);
    thread_pool     pool;
    std::thread::id id{};
    ex::sync_wait(ex::schedule(pool.get_scheduler()) | ex::then([&id] { id = std::this_thread::get_id(); }));

    auto coro{[](std::thread::id expect) -> test_task {
        assert(expect == std::this_thread::get_id());
        co_await ex::just();
        assert(expect == std::this_thread::get_id());
        co_return 0;
    }(id)};
    coro.scheduler = beman::task::detail::task_scheduler(pool.get_scheduler());
    coro.run();
}
} // namespace

int main() {
    try {
        //-dk:TODO test_exception();
        //-dk:TODO test_initial_scheduler();
    } catch (...) {
        unreachable("no exception should escape to main");
    }
}
