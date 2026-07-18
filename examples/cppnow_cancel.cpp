// examples/cppnow_cancel.cpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// ----------------------------------------------------------------------------

#include <beman/task.hpp>
#include <beman/execution.hpp>

namespace ex = beman::execution;

// ----------------------------------------------------------------------------
namespace {
ex::task<> cancel() { co_await ex::just_stopped(); }
} // namespace

int main() { ex::sync_wait(cancel()); }
