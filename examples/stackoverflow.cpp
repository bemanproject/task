#include <beman/execution/execution.hpp>
#include <beman/execution/task.hpp>
#include <iostream>
#include <string>

namespace ex = beman::execution;

namespace {

struct inline_env {
    ex::inline_scheduler scheduler;
};

template <typename Env = ex::env<>>
ex::task<void, Env> f(int i) {
    co_return;
}

template <typename Env = ex::env<>>
ex::task<void, Env> g(int total) {
    for (auto i = 0; i < total; ++i) {
        co_await (f(i) | ex::then([]{}));
        //co_await f(i);
    }
}

template <typename Env = ex::env<>>
ex::task<void> recursive(int total) {
    if (total > 0) {
        co_await (recursive(total - 1) | ex::then([]{}));
    }
}

}

int main(int ac, char* av[]) {
    std::size_t total{ac < 2? 1000: std::stoul(av[1])};
    std::cout << "total=" << total << "\n";
    ex::sync_wait(recursive(total));
    std::cout << "recursive done\n" << std::flush;
    ex::sync_wait(ex::on(ex::inline_scheduler{}, recursive(total)));
    std::cout << "recursive/inline_scheduler 1 done\n" << std::flush;
    ex::sync_wait(recursive<inline_env>(total));
    std::cout << "recursive/inline_scheduler 2 done\n" << std::flush;
    ex::sync_wait(g(total));
    std::cout << "run_loop::scheduler done\n" << std::flush;
    ex::sync_wait(ex::on(ex::inline_scheduler{}, g(total)));
    std::cout << "inline_scheduler 1 done\n" << std::flush;
    ex::sync_wait(g<inline_env>(total));
    std::cout << "inline_scheduler 2 done\n" << std::flush;
}
