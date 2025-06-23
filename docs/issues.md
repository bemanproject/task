---
title: Coroutine Task Issues
document: D0000R0
date: 2025-06-22
audience:
    - Concurrency Working Group (SG1)
    - Library Evolution Working Group (LEWG)
    - Library Working Group (LWG)
author:
    - name: Dietmar Kühl (Bloomberg)
      email: <dkuhl@bloomberg.net>
source:
    - https://github.com/bemanproject/task/doc/issues.md
toc: false
---

After the [task proposal](https://wg21.link/p3552) was voted to be
forwarded to plenary for inclusion into C++26 a number of issues
were brought. The reported issues came mostly as an immediate
reaction and both the description may be incomplete as well as the
list. This paper tries to address these known issues.

# Change History

## R0 Initial Revision

# General

Non-trivial components can always improved in some way. The same
is true for `std::execution::task`. If we wait until we have something
everybody is satisfied with, we will hardly ever get anything at
all.  The key question is whether the current specification works
or if there are fatal flaws making it a bad choice. For the `task`
proposal as it currently is, I see two questions:

1. Does the specified `task` work and can it reasonably be used for
    expected use cases?
2. Can the interface and specification of `task` be improved?

I haven't heard any concern which implies that `task` as specified
can't be used at all. Some of the concerns raised are about performance
of `task`. Even if the semantic operations of `task` are exactly
right, bad performance may be a reason why `task` can't be used in
practice. Currently, I'm not in a position to produce any data
showing that `task` performance is acceptable or that its performance
is unacceptable.

One statement from the planary was that `task` is the obvious name
for a coroutine task and we should get it right. There are certainly
improvements which can be applied. Although I'm not aware of anything
concrete beyond the raised issues there is certainly potential
improvements. If the primary concern is that there may be better
task interfaces in the future and a better `task` should get the
name `task`, it seems reasonable to rename the component. The
original name used for the component was `lazy` and feedback from
Hagenberg was to rename it `task`.

For a different name, my proposal would be something like `task26`
and to similarly name future revisions with a version number. This
way the name `task` would be reserved until the design stablizes.
This direction would allow making a coroutine task available now
without concerns about using the good name for a version which is
inferior to a future design.

It is also worth pointing out that it is possible to have more than
one coroutine task component. Due to the design of coroutines and
sender/receiver they can interoperate.

# The Concerns

Some concerns were posted to the LWG chat on Mattermost (they were
originally raised on a Discord chat where multiple of the authors
of sender/receiver components coordinate the work). Other concerns
were raised separately (and the phrasing isn't exactly that of the
originaly, e.g., because it was stated in a different language). I
became aware of all but one of these concerns after the poll by LWG
to forward the proposal for inclusion into C++26.  The rest of this
section discusses these concerns:

## No Support For Symmetric Transfer

Concern raised:

> it doesn't customise `as_awaitable()` and so can't make use of
> symmetric transfer when another coroutine awaits a `task` - this
> will lead to more stack-overflow situations.

Example (see `issue-symmetric-transfer.cpp`):

```c++
template <typename Env>
task<void, Env> test() {
    for (std::size_t i{}; i < 1000000; ++i) {
        co_await std::invoke([]()->std::execution::task<> {
            co_await std::execution::just();
        });
    }
}
```

TODO - turn into text

Depending on the choice of `Env` this code may stack overflow:

- Using the default environment with a scheduler enqueuing tasks
    will not cause an overflow but does schedule each task (see
    below).
- Using an environment a scheduler immediately starting work like
    `inline_scheduler` or the default environment with such a
    scheduler may cause a stack overflow.

- Agreed: like other senders, `task` doesn't have an awaitable
    interface.
- To guarantee scheduler affinity, it may be necessary to reschedule,
    i.e., the whether symmetric transfer could be used is conditional.
- With scheduler affinity there isn't an issue (also see below).
- I don't think an implementation is prohibited from providing
    an awaiter interface and taking advantage of symmetric transfer:
    how does the user tell?

## Unusual Allocator Customisation

Concern raised:

> allocator customisation mechanism is inconsistent with the design
> of `generator` allocator customisation (with `generator`, when you
> don't specify an allocator in the template arg, then you can use
> any `allocator_arg` type, `allocator_arg` is only allowed in the
> first or second position), A bit vage - I'm not sure what's the
> suggested design here, but I suspect this can be fixed.

TODO turn into text

Example (see `issue-frame-allocator.cpp`):

```c++
template <typename Env>
ex::task<int, Env> test(int i, auto&&...) {
    co_return co_await ex::just(i);
}

struct default_env {};
struct allocator_env {
    using allocator_type = std::pmr::polymorphic_allocator<>;
};
```

Depending on how the coroutine is invoked this may fail to compile:

- `test<default_env>(17)`: OK - use default allocator
- `test<default_env>(17, std::allocator_arg, std::allocator<int>())`: OK - allocator is convertible
- `test<default_env>(17, std::allocator_arg, std::pmr::polymorphic_allocator<>())`: compile-time error
- `test<alloctor_env>(17, std::allocator_arg, std::pmr::polymorphic_allocator<>())`: OK

### Issue: Flexible Allocator Position

- `generator` only allows the `std::allocator_arg`/allocator pair as
    leading parameters
- `task` allows them to go anywhere
- allowing arbitrary positions is intentional
    - otherwise the implementer of a coroutine decides allocator use
    - alternative 1: the coroutine could be implemented twice
    - alternative 2: the coroutine could determine presence of allocator
    - a trailing `auto&&...` is preferable
    - arguably `generator` is "broken" in that sense => needs paper to improve
- the use of `allocator_arg` in other than the first parameter is inconsistent
    with all other uses, though

### Issue: Unconfigured Allocator Leads To Compiler-Time Error

- `generator` allows to use an arbitrary allocator without configuration
- omission: task could always use allocator argument for the coroutine
    frame but not expose it downstream when no allocator is configured
- doing so could be done later (things which currently don't compile start
    compiling) or as an NB comment

## `affine_on` Underspecified

Concern raised:

> the semantics of the `affine_on` customisation point seems poorly
> defined, or not exactly what we need it to be (how does the
> operatiom map onto a scheduler and how does it determine whether
> or not it needs to reschedule?),

TODO turn into text

Example (see `issue-affine_on.cpp`):

```c++
template <ex::sender Sender>
ex::task<> test(Sender&& sender) {
    co_await std::move(sender);
}
```

The `co_await` awaits the result of `task`'s
`promise_type::await_transform` which returns the result of
`affine_on(sender, sched)` for the parameter `sender` and the
scheduler `sched` associated with the `task`. `affine_on`
in turn conditionally returns `sender` if it can determine that
`sender` completes on the execution agent associated with `sched`
or `continues_on(sender, sched)`:

- Does `ex::sync_wait(test(ex::just()))` result the an operation
    being scheduled? It can be statically determined that (the default)
    `just()` doesn't change the execution agent
- Does this use result in an operation being scheduled?

    ```c++
    ex::sync_wait(test(
          ex::read_env(ex::get_scheduler)
        | ex::let_value([](auto sched){ return ex::starts_on(sched, ex::just()); })
    ));
    ```

    It can be dynamically determined that the completion scheduler is
    identical to the expected scheduler and no scheduling is needed.
- The specification is deliberately kept vague to allow future changes
    to the behavior when implementation can detect more cases where no
    scheduling is needed.
- That direction was discussed in Hagenberg.
- There [P3206](https://wg21.link/P3206) to define a corresponding interface.
- Implementation can do something like that for standard library senders.
- Implementations can be required to do some of these optimisations in future
    revisions of the standard.
- Note that avoiding scheduling work increases the chances of stack overflow.

## Starting A `task` Reschedules

Concern raised:

> calling a task coroutine unconditionally schedules onto
> its scheduler (a large performance bottleneck),

Example (see `issue-start-reschedules.cpp`):

```cpp
ex::task<int> test(auto sched) {
    std::cout << "init =" << std::this_thread::get_id() << "\n";
    co_await ex::starts_on(sched, scheduler(), ex::just());
    std::cout << "final=" << std::this_thread::get_id() << "\n";
}
```

TODO turn into text

- assuming an execution agent with only one thread is used, the
    two thread ids need to be identical for scheduler affinity:
    this is the affinity invariant of `task`
- `ex::sync_wait(test(pool2.get_scheduler()))`: no scheduling needed
    as the `task` is started on the same thread as the one used by
    the `run_loop` within `sync_wait`.
- `ex::sync_wait(ex::starts_on(pool1.get_scheduler(), test(pool2.get_scheduler())))`:
    also no scheduling needed as `starts_on` provides a receiver with the same
    scheduler it starts the child on.
- the specification doesn't explicitly say that the `initial_suspend()`
    awaiter reschedules but it specifies the that it resumes on the
    scheduler's execution agent.
- it seems to contradict the idea of scheduler affinity if the
    thread ids printed in the example could be different, i.e., if it
    is allowed to resume on a different execution agent than that
    corresponding to `get_scheduler(get_env(rcvr))` for receiver
    `rcvr` the task is connected to
- if it is never possible that the execution agent on which the `task`
    gets started mismatches the execution agent(s) from the receiver,
    no scheduling for the initial resumption is needed but I don't
    if that is guaranteed
- my current implementation reschedules when the awaiter returned from
    `initial_suspend()` resumes: doing so _may_ be unnecessary
- I don't see how the coroutine could see what scheduler it is
    started on, i.e., I don't know how the rescheduling can be avoided
- if there is a way, however, the specification doesn't enforce
    rescheduling as long as the coroutine resumes on the correct
    scheduler

## The Environment Design Is Odd

Concern raised:

> the design of the `Environment` template parameter seems weird -
> it's trying to be a trait-like class but also a `queryable`,

TODO add example? turn into text

- the `Enviroment` (i.e, second) template parameter has sort of
    dual use:
    1. It provides configurations for various types (`allocator_type`,
        `scheduler_type`, `stop_source_type`, `error_type`)
    2. An object of the type is created which is either initialized
        with the receiver's environment or, if that isn't possibly,
        default initialized. This object is then used to satisfy
        queries if available. The actual logic is a bit more complicated,
        actually, to allow capturing state depending on the receiver's
        enironment type.
- the functionality could be separated by naming the parameter `Configuration`
    which may optionally provide an `environement_type` in addition to the
    other type configurations
- this change would slightly change the use but I think it would retain the
    spirit of the current design
- both approaches work

## Shadowing The Environment Allocator Is Questionable

Concern raised:

> I'm unsure whether the shadowing of the `get_allocator` query
> on the parent environment with the coroutine promise's allocator
> is the behavior we really wnat here.,

TODO add example? turn into text

- creating a coroutine (calling the coroutine function) chooses the
    allocotor for the coroutine frame, either explicitly specified or
    implicitly determined. Either way the coroutine frame is allocated
    when the function is called.
- the fundamental idea of the allocator model is that children of an
    entity use the same allocator as their parent unless an allocator
    is explicitly specified for a child.
- the `co_await`ed entities are children of the coroutine and should
    share the coroutine's allocator
- further, to forward the allocator from the receiver's environment in
    an environment, its static type needs to be convertible to the
    coroutine's allocator type: the coroutine's environment types are
    determined when the coroutine is created at which point the receiver
    isn't known, yet
- on the flip side, the receiver's environment can contain the configuration
    from the user of a work-graph which is likely better informed about
    the allocator
- the allocator from the receiver's environment could be forwarded if the
    `task`'s allocator can be initialized with it, e.g., because the `task`
    use `std::pmr::polymorphic_allocator<>`.
- it isn't a priori clear what should happen if the receiver's environment
    has an allocator which can't be converted to the `task`'s environment:
    at least ignoring a mismatching allocator or producing a compile time
    error are options.

## No Completion Scheduler

Concern raised:

> task doesn't define `get_env` that returns an env with a
> completion-scheduler, this means awaiting a task from within a
> coroutine will always reschedule back onto the parent coroutine's
> context - so you schedule when entering a coroutine and schedule
> when resuming - very inefficient,

TODO add example? turn into text

- `get_completion_scheduler(env)` is a query operating on the sender's
    enviroment, i.e., it would operate on the `task`'s enironment.
- a `task` is created without any information of scheduling at all:
    the only state it gets are the arguments passed to the coroutine
    [factory] function and what the coroutine uses for scheduling is
    unknown!
- Once the `task` is `connect`ed to a receiver, the resulting operation
    state could be inspected for its complection scheduler - if queries
    were defined on operation states
- I believe a similar issue exists in other contexts where a sender can't
    tell on which scheduler it may complete until it is actually `connect`ed
    to a receiver.
- If there is a meaningful way to determine the completion scheduler for a
    `task` I'd consider the omission of a `get_completion_scheduler` query
    an error which should be corrected. As of know I don't know what a
    reasonable result may be.

## A Stop Source Always Needs To Be Created

Concern raised:

> the way the promise is worded seems to specify that a stop-source
> is always constructed as a member of the coroutine promise - you
> shoud really only need a new stop-source when adapting between
> incompatable stop-token types, if everything is using
> `inplace_stop_token` then it should just be passed through and have
> no space overhead in the promise.

TODO add example? turn into text

- the stop source is an exposition-only member of the `promise_type`
    and I don't think the existance of such a member has any implication
    on whether such an object needs to exist or where it is created
- an implementation can (and probably should) create the stop source in
    the operation state object when `connect`ed to a receiver with an
    an environment with a mismatching stop token.
- if there is a concern with that the specification is unnecessarily
    restrictive, it should be fixed to use a specifiation macro
    similar to other specification macros used which refer to
    entities in the operation state
- there is certainly no design intend to create unnecessary stop sources
    and I don't think the specification implies that it is required

## A Future Coroutine Feature Could Avoid `co_yield` For Errors

Concern raised:

> I would prefer a language change to avoid the need for the hack using
> `co_yield with_error(x)`.

TODO turn into text

- yes - a future revision of the C++ standard may provide facilities
    which result in a better design for pretty much anything.
- assuming such a language change gets proposed reasonably early (there
    are no existing `task` implementations and there is certainly none
    which promises ABI stability for the next few years) a corresponding
    change can be taken into account such that a future revision can
    actually change the interface
- I don't know what the shape of the envisioned language change is but
    it seems to be a pure extension which can hopefully be integrated
    with the existing design

## There Is No Hook To Capture/Restore TLS

This concern is the only one I was aware of for a few weeks prior
to the Sofia meeting. I believe the necessary functionality can be
implemented although it is possibly harder to do than with more
direct support.

Concern raised:

> Existing code accesses TLS to store context information. When
> a `co_await` actually suspends it is possible that a different
> task running on the same thread replaces used TLS data or the
> task gets resumed on a different thread of a thread pool. In
> that case it is necessary to capture the TLS variables prior
> to suspending and restoring them prior to resuming the coroutine.
> There is currently no way to actually do that.

TODO add example and turn into text

- using a custom scheduler with a custom implementation of
    `affine_on` allows this functionality (via the domain)
- it would still be nice to have an easier interface
