// examples/demo-thread_pool.hpp                                      -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_EXAMPLES_DEMO_THREAD_POOL
#define INCLUDED_EXAMPLES_DEMO_THREAD_POOL

// ----------------------------------------------------------------------------

#include <beman/execution/execution.hpp>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>

namespace demo {
struct thread_pool {

    struct node {
        node*        next;
        virtual void run() = 0;

      protected:
        ~node() = default;
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

    thread_pool() = default;
    ~thread_pool() {
        this->stop();
        this->driver.join();
    }
    void stop() {
        {
            std::lock_guard cerberus(this->mutex);
            stopped = true;
        }
        this->condition.notify_one();
    }

    struct scheduler {
        using scheduler_concept = beman::execution::scheduler_t;
        struct env {
            thread_pool* pool;

            template <typename T>
            scheduler query(const beman::execution::get_completion_scheduler_t<T>&) const noexcept {
                return {this->pool};
            }
        };
        template <typename Receiver>
        struct state final : thread_pool::node {
            using operation_state_concept = beman::execution::operation_state_t;
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
            void run() override { beman::execution::set_value(std::move(this->receiver)); }
        };
        struct sender {
            using sender_concept        = beman::execution::sender_t;
            using completion_signatures = beman::execution::completion_signatures<beman::execution::set_value_t()>;
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

static_assert(beman::execution::scheduler<thread_pool::scheduler>);

} // namespace demo

// ----------------------------------------------------------------------------

#endif
