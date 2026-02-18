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
struct ct
{
    Task& task;
    template <typename... Arg>
    auto operator()(Arg&&... arg) const {
        return ex::let_value(ex::read_env(ex::get_stop_token),
            [&task=this->task, ...a=std::forward<Arg>(arg)](auto) {
            return std::invoke(task, std::move(a)...);
        });
    }
};
template <typename Task>
ct(Task&) -> ct<Task>;

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
        std::cout << "resource::allocate(" << n << ")\n";
        return operator new(n);
    }
    void do_deallocate(void* p, std::size_t n, std::size_t) override {
        std::cout << "resource::deallocate(" << n << ")\n";
        operator delete(p, n);
    }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override{
        return this == &other;
    }
   
};

int main() {
    resource res{};
    std::pmr::polymorphic_allocator<std::byte> alloc(&res);
    ex::sync_wait(ex::write_env(ct(lambda)(17),
                  ex::env{ex::prop{ex::get_allocator, alloc}}));
}
