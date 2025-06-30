// include/beman/task/detail/handle.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_HANDLE
#define INCLUDED_INCLUDE_BEMAN_TASK_DETAIL_HANDLE

#include <beman/execution/execution.hpp>
#include <coroutine>
#include <memory>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::task::detail {
template <typename P>
class handle {
  private:
    using deleter = decltype([](auto p) {
        if (p) {
            std::coroutine_handle<P>::from_promise(*p).destroy();
        }
    });
    std::unique_ptr<P, deleter> h;

  public:
    explicit handle(P* p) : h(p) {}
    void reset() { this->h.reset(); }
    template <typename... A>
    void start(A&&... a) noexcept {
        this->h->start(::std::forward<A>(a)...);
    }
    template <::beman::execution::receiver Receiver>
    void complete(Receiver&& receiver) {
        this->h->complete(::std::forward<Receiver>(receiver));
    }
};

} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
