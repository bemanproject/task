 // tests/beman/task/affine_on.test.cpp                                -*-C++-*-
 // SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 
 #include <beman/task/detail/affine_on.hpp>
 #include <beman/task/detail/single_thread_context.hpp>
 #include <beman/task/detail/inline_scheduler.hpp>
 #include <beman/execution/execution.hpp>
 #ifdef NDEBUG
 #undef NDEBUG
 #endif
 #include <cassert>
 
 namespace ex = beman::execution;
 
 // ----------------------------------------------------------------------------
 
 namespace {
 template <ex::scheduler Scheduler>
 struct receiver {
     using receiver_concept = ex::receiver_t;
 
     std::remove_cvref_t<Scheduler> scheduler;
     auto get_env() const noexcept { return ex::detail::make_env(ex::get_scheduler, scheduler); }
     auto                           set_value(auto&&...) && noexcept -> void {}
 };
 template <ex::scheduler Scheduler>
 receiver(Scheduler&& sched) -> receiver<Scheduler>;
 
 template <typename... Sigs>
 struct test_scheduler {
     using scheduler_concept = ex::scheduler_t;
     struct sender {
         using sender_concept = ex::sender_t;
         template <typename Env>
         auto get_completion_signatures(Env&& env) const noexcept {
             if constexpr (ex::unstoppable_token<decltype(ex::get_stop_token(env))>)
                 return ex::completion_signatures<ex::set_value_t(), Sigs...>{};
             else
                 return ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t(), Sigs...>{};
         }
         struct env {
             template <typename Tag>
             auto query(ex::get_completion_scheduler_t<Tag>) const noexcept -> test_scheduler {
                 return {};
             }
         };
         auto get_env() const noexcept { return env{}; }
 
         template <ex::receiver Rcvr>
         struct op_state {
             using operation_state_concept = ex::operation_state_t;
             std::remove_cvref_t<Rcvr> rcvr;
             void                      start() & noexcept {
                 static_assert(ex::operation_state<op_state>);
                 ex::set_value(std::move(this->rcvr));
             }
         };
         template <ex::receiver Rcvr>
         auto connect(Rcvr&& rcvr) noexcept {
             static_assert(ex::sender<sender>);
             return op_state<Rcvr>{std::forward<Rcvr>(rcvr)};
         }
     };
 
     auto schedule() noexcept -> sender {
         static_assert(ex::scheduler<test_scheduler<>>);
         return {};
     }
     auto operator==(const test_scheduler&) const -> bool = default;
 };
 static_assert(ex::scheduler<test_scheduler<>>);
 
 static_assert(ex::receiver<receiver<beman::task::detail::inline_scheduler>>);
 static_assert(ex::receiver<receiver<test_scheduler<>>>);
 
 template <ex::receiver Receiver>
 auto test_affine_on_accepted_scheduler(Receiver rcvr) {
     if constexpr (requires { ex::connect(beman::task::affine_on(ex::just()), std::move(rcvr)); }) {
         auto state(ex::connect(beman::task::affine_on(ex::just()), std::move(rcvr)));
         ex::start(state);
     }
 }
 
 auto test_affine_on_accepted_scheduler() {
     test_affine_on_accepted_scheduler(receiver{beman::task::detail::inline_scheduler{}});
     test_affine_on_accepted_scheduler(receiver{test_scheduler<>{}});
     // test_affine_on_accepted_scheduler(receiver{test_scheduler<ex::set_error_t(int)>{}});
 }
 
 } // namespace
 
 int main() {
     beman::task::detail::single_thread_context context;
 
     auto main_id{std::this_thread::get_id()};
     auto [thread_id]{
         ex::sync_wait(ex::schedule(context.get_scheduler()) | ex::then([] { return std::this_thread::get_id(); }))
             .value_or(std::tuple{std::thread::id{}})};
 
     assert(main_id != thread_id);
 
     [[maybe_unused]] auto s(beman::task::affine_on(ex::just(42)));
     static_assert(ex::sender<decltype(s)>);
     [[maybe_unused]] auto st(ex::connect(std::move(s), receiver{context.get_scheduler()}));
     ex::sync_wait(
         beman::task::affine_on(ex::starts_on(context.get_scheduler(), ex::just(42)) | ex::then([thread_id](int value) {
                                    assert(thread_id == std::this_thread::get_id());
                                    assert(value == 42);
                                })) |
         ex::then([thread_id]() { assert(thread_id != std::this_thread::get_id()); }));
 }
