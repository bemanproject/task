// examples/hello.cpp                                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/task.hpp>
#include <beman/execution/execution.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <new>
#include <utility>

namespace ex = beman::execution;

template <typename Task>
struct defer_frame
{
    Task task;
    template <typename... Arg>
    auto operator()(Arg&&... arg) const {
        return ex::let_value(ex::read_env(ex::get_allocator),
            [&task=this->task, ...a=std::forward<Arg>(arg)](auto alloc) {
            return std::invoke(task, std::allocator_arg, alloc, std::move(a)...);
        });
    }
};

struct env {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
};

auto lambda{[](int i, auto&&...)->ex::task<void, env> {
    auto alloc = co_await ex::read_env(ex::get_allocator);
    alloc.deallocate(alloc.allocate(1), 1);
    std::cout << "lambda(" << i << ")\n"; co_return;
}};

struct resource
    : std::pmr::memory_resource
{
    void* do_allocate(std::size_t n, std::size_t) override {
        std::cout << "    resource::allocate(" << n << ")\n";
        return operator new(n);
    }
    void do_deallocate(void* p, std::size_t n, std::size_t) override {
        std::cout << "    resource::deallocate(" << n << ")\n";
        operator delete(p, n);
    }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override{
        return this == &other;
    }
   
};

int main() {
    resource res{};
    std::pmr::polymorphic_allocator<std::byte> alloc(&res);
    std::cout << "direct allocator use:\n";
    alloc.deallocate(alloc.allocate(1), 1);
    std::cout << "write_env/just/then use:\n";
    ex::sync_wait(
        ex::write_env(
            ex::just(alloc)
          | ex::then([](auto a) {
              a.deallocate(a.allocate(1), 1);
            }),
          ex::env{ex::prop{ex::get_allocator, alloc}})
    );
    std::cout << "write_env/read_env/then use:\n";
    ex::sync_wait(
        ex::write_env(
           ex::read_env(ex::get_allocator)
        | ex::then([](auto a) {
            a.deallocate(a.allocate(1), 1);
          }),
        ex::env{ex::prop{ex::get_allocator, alloc}})
    );
    std::cout << "write_env/let_value/then use:\n";
    ex::sync_wait(
        ex::write_env(
           ex::just()
        | ex::let_value([]{
            return ex::read_env(ex::get_allocator)
              | ex::then([](auto a) { a.deallocate(a.allocate(1), 1); })
              ;
        }),
        ex::env{ex::prop{ex::get_allocator, alloc}})
    );
    std::cout << "write_env/task<>:\n";
    ex::sync_wait(
        ex::write_env(
        []()->ex::task<>{
            auto a = co_await ex::read_env(ex::get_allocator);
            a.deallocate(a.allocate(1), 1);
        }(),
        ex::env{ex::prop{ex::get_allocator, alloc}})
    );
    std::cout << "write_env/task<void, env>:\n";
    ex::sync_wait(
        ex::write_env(
        [](auto&&...)->ex::task<void, env>{
            auto a = co_await ex::read_env(ex::get_allocator);
            a.deallocate(a.allocate(1), 1);
        }(std::allocator_arg, alloc),
        ex::env{ex::prop{ex::get_allocator, alloc}})
    );
    std::cout << "write_env/defer_frame<task<void, env>>:\n";
    static constexpr defer_frame t0(
        [](auto, auto, int i)->ex::task<void, env>{
            std::cout << "  i=" << i << "\n";
            auto a = co_await ex::read_env(ex::get_allocator);
            a.deallocate(a.allocate(1), 1);
        });
    ex::sync_wait(
        ex::write_env(
        t0(17),
        ex::env{ex::prop{ex::get_allocator, alloc}})
        );
    std::cout << "write_env/temporary defer_frame<task<void, env>>:\n";
    ex::sync_wait(
        ex::write_env(
        defer_frame([](auto, auto, auto i)->ex::task<void, env>{
            std::cout << "  i=" << i << "\n";
            co_await std::suspend_never{};
            auto a = co_await ex::read_env(ex::get_allocator);
            a.deallocate(a.allocate(1), 1);
        })(42),
        ex::env{ex::prop{ex::get_allocator, alloc}})
    );
    std::cout << "done\n";
}
