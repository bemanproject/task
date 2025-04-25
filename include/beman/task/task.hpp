// include/beman/task/task.hpp                                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_LAZY_TASK
#define INCLUDED_INCLUDE_BEMAN_LAZY_TASK

#include <beman/task/detail/allocator_of.hpp>
#include <beman/task/detail/any_scheduler.hpp>
#include <beman/task/detail/inline_scheduler.hpp>
#include <beman/task/detail/into_optional.hpp>
#include <beman/task/detail/task.hpp>
#include <beman/task/detail/scheduler_of.hpp>
#include <beman/task/detail/stop_source.hpp>

// ----------------------------------------------------------------------------

namespace beman::task {
template <typename Context>
using allocator_of_t = ::beman::task::detail::allocator_of_t<Context>;
template <typename Context>
using scheduler_of_t = ::beman::task::detail::scheduler_of_t<Context>;
template <typename Context>
using stop_source_of_t = ::beman::task::detail::stop_source_of_t<Context>;

using any_scheduler    = ::beman::task::detail::any_scheduler;
using inline_scheduler = ::beman::task::detail::inline_scheduler;
using into_optional_t  = ::beman::task::detail::into_optional_t;
using ::beman::task::detail::into_optional;

using ::beman::task::detail::default_context;
using ::beman::task::detail::with_error;
} // namespace beman::task

namespace beman::execution {
template <typename Context>
using allocator_of_t = ::beman::task::detail::allocator_of_t<Context>;
template <typename Context>
using scheduler_of_t = ::beman::task::detail::scheduler_of_t<Context>;
template <typename Context>
using stop_source_of_t = ::beman::task::detail::stop_source_of_t<Context>;

using any_scheduler    = ::beman::task::detail::any_scheduler;
using inline_scheduler = ::beman::task::detail::inline_scheduler;
using into_optional_t  = ::beman::task::detail::into_optional_t;
using ::beman::task::detail::into_optional;

using ::beman::task::detail::default_context;
using ::beman::task::detail::with_error;
using ::beman::task::detail::change_coroutine_scheduler;
template <typename T = void, typename Context = ::beman::task::default_context>
using task = ::beman::task::detail::task<T, Context>;
} // namespace beman::execution

// ----------------------------------------------------------------------------

#endif
