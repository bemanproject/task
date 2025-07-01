# Task Issues Overview

A brief overview of the issues based on two lists (there is some
overlap between the two lists; the initials are used to reference
the respective issue):

- Lewis's list [LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org)
- Dietmar's list [DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md) (build nearly complete from Mattermost feedback)

## Task is eager

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#task-is-not-actually-lazily-started)
[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#task-is-not-actually-lazily-started)

- the current spec implies eager start
- the intent is to establish task's invariant (scheduler affinity)
- fix: `initial_suspend() -> suspend_always` and constraint start
- commment: rephrase to clarify it is resumed later

## Initial resume shouldn't reschedule

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#task-should-not-unconditionally-reschedule-when-control-enters-the-coroutine)
[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#task-should-not-unconditionally-reschedule-when-control-enters-the-coroutine)
[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#starting-a-task-reschedules)

- `initial_suspend()`'s spec implies reschedule
- fix: constrain `start(rcvr)` to be called on `get_scheduler(get_env(rcvr))` instead
- comment: the description from `initial_suspend()` merely lays out what needs to hold => that's OK

## Shouldn't reschedule after `co_await`ing task

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#task-awaiting-another-task-should-not-reschedule-on-resumption)

- `await_transform` uses `affine_on` without enough context information => reschedule
- fix: specify a custom `affine_on` for `task` and arrange for various things
- comment: how can a user tell what exactly happened? the `co_await`ing `task` and the `co_await`ed task are from the implementation

## No symmetric transfer

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#task-coroutine-awaiting-another-task-does-not-use-symmetric-transfer)
[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#no-support-for-symmetric-transfer)

- `co_await tsk;` doesn't use symmetric transfer
- fix: require symmetric transfer
- comment: recommended practice? the `task`s can communicate appropriately

## no support for non-senders

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#minor-task-does-not-accept-awaiting-types-that-provide-as_awaitable-but-that-do-not-satisfy-sender-concept)

- `as_awaitable` isn't used
- fix: use `as_awaitable`
- comment: how to guarantee affinity? seems to require an additional coroutine
- don't do?

## `affine_on` has no default spec

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#affine_on-is-missing-a-specification-for-a-default-implementation)
[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#affine_on-underspecified)

- there is no default specification of what `affine_on` does.
- fix: say something specific
- comment: we don't have the tools to say what implementations can do for known senders

## `affine_on` semantics unclear

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#affine_on-semantics-are-not-clear)

- the wording is unclear
- fix: two versions
- comment: I think I had the 2nd in mind 

## `affine_on` shape

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#affine_on-might-not-have-the-right-shape)

- `affine_on` should just depend on the receiver's `get_scheduler()`.
- comment: agreed - that is design, though

## `affine_on` schedule vs. stop token

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#affine_on-should-probably-not-forward-stop-requests-to-reschedule-operation)

- non-success completions don't guarantee affinity invariant
- fix: suppress stop token
- comment: in case of `set_stopped()` the coroutine doesn't resume; for `set_error(e)` other reasons may exist
- comment: however, I don't object to this direction 

## `affine_on` vs. other algorithms

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#we-should-probably-define-customsiations-for-affine_on-for-some-other-senders)

- `affine_on(sch, sndr)` should behave special for some `sndr`
- fix: specify some of the interactions
- comment: I think an implementation can do these; once we have better tools we may want to require some

## promise_type vs with_awaitable_sender

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#minor-taskpromise_type-doesnt-use-with_awaitable_senders---should-it)

- should it use `with_awaitable_sender`?
- potential fix: have affinity aware `with_awaitable_sender` variant?
- comment: there are multiple tools which may be useful

## unhandled_stopped should be noexcept

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#taskpromise_typeunhandled_stopped-should-be-marked-noexcept)

- fix: do it!
- comment: for consistency it should be `noexcept`

## allocator type not specified

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#behaviour-when-the-tasks-environment-type-does-not-specify-an-allocator_type)
[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#unusual-allocator-customisation)

- generator uses the allocator anyway
- fix: use type-erased allocator in that case like generator
- comment: that's an oversight; I didn't realize generator does that

## allocator_arg more permissive

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#handling-of-allocator_arg-is-more-permissive-than-for-stdgenerator)
[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#issue-flexible-allocator-position)

- generator requires allocator in first position
- fix: should generator be generalized?
- comment: yes but not for C++26

## hiding parent's allocator

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#minor-task-environments-allocator_type-overrides-the-parent-environments-get_allocator)
[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#shadowing-the-environment-allocator-is-questionable)

- task's allocator (from ctor) hides receiver's allocator
- fix: allow receiver's allocator through
- comment: that misaligns with my understanding of the allocator model

## no stop_source in promise

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#taskpromise_type-should-not-contain-a-stop-source)
[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#a-stop-source-always-needs-to-be-created)

- the stop_source is only need for misaligned stop tokens
- fix: have the stop source in promise and only when needed
- comment: I think that is already allowed; maybe should be clarified, recommended practice?

## stop_token default constructible

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#taskpromise_type-wording-assumes-that-stop-token-is-default-constructible)

- the exposition-only member for stop_token require default constructible
- fix: don't have it: get it from env or stop source
- comment: I think that is alrady allowed

## coro frame destroyed too late

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#task-coroutine-state-is-not-destroyed-early-enough-after-completing)

- frame should be destroyed before completing
- fix: result in state, destroy before completing
- comment: that is what I had hoped to say - may need wording fix

## inefficient env

[LB](https://github.com/lewissbaker/papers/blob/master/isocpp/task-issues.org#taskpromise_typeget_env-seems-to-require-an-inefficient-implementation)
[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#the-environment-design-is-odd)

- own_env must copy lots of state
- comment: own_env just allows a potential type-erase entity to be workable, primarily it is env

## no completion scheduler

[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#no-completion-scheduler)

- `task` doesn't expose a completion scheduler
- comment: it can't have one
- fix: redesign get_completion_scheduler(sender, env) or get_completion_scheduler(os)

## future coro may not need `co_yield error`

[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#a-future-coroutine-feature-could-avoid-co_yield-for-errors)

- sure

## no hook to capture/restore TLS

[DK](https://github.com/bemanproject/task/blob/issues/docs/issues.md#there-is-no-hook-to-capturerestore-tls)

- legacy APIs may need TLS to be set appropriately
- objective: capture TLS before async, restore after
- fix: use custom `affine_on` with wrapped scheduler, delegating
