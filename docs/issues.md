---
title: Coroutine Task Issues
document: D37960
date: 2025-07-13
audience:
    - Concurrency Working Group (SG1)
    - Library Evolution Working Group (LEWG)
    - Library Working Group (LWG)
author:
    - name: Dietmar Kühl (Bloomberg)
      email: <dkuhl@bloomberg.net>
source:
    - https://github.com/bemanproject/task/doc/issues.md
toc: true
---

After the [task proposal](https://wg21.link/p3552) was voted to be
forwarded to plenary for inclusion into C++26 a number of issues
were brought up. The issues were reported using various means. This
paper describes the issues and proposes potential changes to address
them.

# Change History

## R0 Initial Revision

# General

Non-trivial components can always be improved in some way. The same
is true for `std::execution::task`. If we wait until we have something
everybody is fully satisfied with, we will hardly ever get anything
at all.  The key question is whether the current specification works
or if there are fatal flaws making it a bad choice. For the `task`
proposal as it currently is, I see two questions:

1. Does the specified `task` work and can it reasonably be used for
    expected use cases?
2. Can the interface and specification of `task` be improved?

I haven't heard any concern which implies that `task` as specified
can't be used at all. There are some concerns which my imply otherwise
but these are really issues with the current wording. Some of the
concerns raised are about performance of `task`. Even if the semantic
operations of `task` are exactly right, bad performance may be a
reason why `task` can't be used in practice. Currently, I'm not in
a position to produce any data showing that `task` performance is
acceptable or that its performance is unacceptable.

One statement from the planary was that `task` is the obvious name
for a coroutine task and we should get it right. There are certainly
improvements which can be applied. Although I'm not aware of anything
concrete beyond the raised issues there is certainly potential for
improvements. If the primary concern is that there may be better
task interfaces in the future and a better task should get the
name `task`, it seems reasonable to rename the component. The
original name used for the component was `lazy` and feedback from
Hagenberg was to rename it `task`.

For a different name, the proposal would be something like `task26`
and to similarly name future revisions with a version number. This
way the name `task` would be reserved until the design stablizes.
This direction would allow making a coroutine task available now
without concerns about using the good name for a version which is
inferior to a future design. If the component should be renamed it
is up to LEWG to acutally pick a name.

The naming concern isn't really specific to `task`: if we believe
we can't change the API of a components in a future reversion of
the standard, it is prudent to use a name which may get superseded.
This argument is even stronger if we believe the ABI needs to be
stable: it is actually quite unlikely that the very first implementation
of a component in a standard library use the optimal interface and
implementation.

Specifically for `task` it is worth pointing out that multiple
coroutine task components can coexist. Due to the design of coroutines
and sender/receiver different coroutine tasks can interoperate.

# The Concerns

Some concerns were posted to the LWG chat on Mattermost (they were
originally raised on a Discord chat where multiple of the authors
of sender/receiver components coordinate the work). Other concerns
were raised separately (and the phrasing isn't exactly that of the
originaly, e.g., because it was stated in a different language). I
became aware of all but one of these concerns after the poll by LWG
to forward the proposal for inclusion into C++26.  The rest of this
section discusses these concerns:

## `affine_on` Concerns

This section covers different concerns around the specification,
or rather lack thereof, of `affine_on`. The `affine_on` is at the
core of the scheduler affinity implementation: this algorithm is
used to establish the invariant that a `task` executes on currently
installed scheduler. The idea is that `affine_on(@_sndr_@, @_sch_@)`
behaves like `continues_on(@_sndr_@, @_sch_@)` but it is customized
to avoid actual scheduling when it knows that `@_sndr_@` completes
on `@_sch_@`.

To some extend `affine_on`'s specification is deliberately vague
because currently the `execution` specification is lacking some
tools which would allow omission of scheduling, e.g., for `just`
which completes immediately with a result. While users can't
necessarily tap into the customizations, yet, implementation could
use tools like those proposed by [P3206](https://wg21.link/p3206)
"A sender query for completion behaviour" using a suitable hidden
name.

### `affine_on` Default Implementation Lacks a Specification

The wording of
[`affine_on`](https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#executionaffine_on-exec.affine.on)
doesn't have a specification for the default implementation. For
other algorithms the default implementation is specified. To resolve
this concern add a new paragraph in
[`affine_on`](https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#executionaffine_on-exec.affine.on)
to provide a specification for the default implementation:

::: add
> [5]{.pnum} Let `sndr` and `env` be subexpressions such that `Sndr`
> is `decltype((sndr))`. If `@_sender-for_@<Sndr, affine_on_t>` is
> `false`, then the expression `affine_on.transform_sender(sndr, env)` is
> ill-formed; otherwise, it is equivalent to:
>
> ```
> auto [_, sch, child] = sndr;
> return transform_sender(
>   @_query-with-default_@(get_doman, sch, default_domain()),
>   continues_on(std::move(child), std::move(sch)));
> ```
>
> except that `sch` is only evaluated once.
:::

The intention for `affine_on` was to all optimization/customization
in a way reducing the necessary scheduling: if the implementation
can determine if a sender completed already on the correct execution
agent it should be allowed to avoid scheduling. That may be achieved
by using `get_completion_scheduler<set_value_t>` of the sender,
using (for now implementatio specific) queries like those proposed
by
[P3206 "A sender query for completion behaviour"](https://wg21.link/P3206),
or some other means. Unfortunately, the specification proposed above
seems to disallow implementation specific techniques to avoid
scheduling. Future revisions of the standard could require some of
the techniques to avoid scheduling assuming the necessary infrastructure
gets standardized.

### `affine_on` Semantics Are Not Clear

The wording in
[`affine_on`](https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#executionaffine_on-exec.affine.on)
p5 says:

> ...  Calling `start(op)` will start `sndr` on the current execution
> agent and execution completion operations on `out_rcvr` on an
> execution agent of the execution resource associated with `sch`.
> If the current execution resource is the same as the execution
> resource associated with `sch`, the completion operation on
> `out_rcvr` may be called before `start(op)` completes. If scheduling
> onto `sch` fails, an error completion on `out_rcvr` shall be
> executed on an unspecified execution agent.

The sentence If the current execution resource is the same as the
execution resource associated with `sch` is not clear to which
execution resource is the "current execution resource". It could
be the "current execution agent" that was used to call `start(op)`,
or it could be the execution agent that the predecessor completes
on.

The intended meaning for "current execution resource" was to refer
to the execution resource of the execution agent that was used to
call `stop(op)`. The specification could be clarified by changing
the wording to become:

> ...  Calling `start(op)` will start `sndr` on the current execution
> agent and execution completion operations on `out_rcvr` on an
> execution agent of the execution resource associated with `sch`.
> If the [current]{.rm} execution resource [of the execution agent
> that was used to invoke `start(op)` ]{.add} is the same as the
> execution resource associated with `sch`, the completion operation
> on `out_rcvr` may be called before `start(op)` completes. If
> scheduling onto `sch` fails, an error completion on `out_rcvr`
> shall be executed on an unspecified execution agent.

It is also not clear what the actual difference in semantics
between `continues_on` and `affine_on` is. The `continues_on`
semantics already requires that the resulting sender completes on
the specified schedulers execution agent. It does not specify that
it must evaluate a `schedule()` (although that is what the default
impl does), and so in theory it already permits an
implementation/customization to skip the schedule (e.g. if the child
senders completion scheduler was equal to the target scheduler).

The key semantic that we want here is to specify one of two possible
semantics that differ from `continues_on`:

1. That the completion of an `affine_on` sender will occur on the
    same scheduler that the operation started on. This is a slightly
    stronger requirement than that of `continues_on`, in that it
    puts a requirement on the caller of `affine_on` to ensure that
    the operation is started on the scheduler passed to `affine_on`,
    but then also grants permission for the operation to complete
    inline if it completes synchronously.
2. That the completion of an `affine_on` sender will either complete
    inline on the execution agent that it was started on, or it
    will complete asynchronously on an execution agent associated
    with the provided scheduler. This is a slightly more permissive
    than option 1. in that it permits the caller to start on any
    context, but also is no longer able to definitively advertise
    a completion context, since it might now complete on one of two
    possible contexts (even if in many cases those two contexts
    might be the same). This weaker semantic can be used in conjunction
    with knowledge by the caller that they will start the operation
    on a context associated with the same scheduler passed to
    `affine_on` to ensure that the operation will complete on the
    given scheduler.

The description in the paper at the Hagenberg meeting assumed that
`task` uses `continues_on` directly to achieve scheduler affinity.
During the SG1 discussion it was requested that the approach to
scheduler affinity doesn't use `continues_on` directly but rather
uses a different algorithm which can be customized separately. This
is the algorithm now named `affine_on`. The intention was that
`affine_on` can avoid scheduling in more cases than `continues_on`.

It is worth noting that an implementation can't determine the
execution resource of the execution agent which invoked `start(op)`
directly. If `rcvr` is the receiver used to create `op` it can be
queried for `get_scheduler(get_env(rcvr))` which is intended to
yield this execution resource. However, there is no guarantee that
this is the case [yet?].

The intended direction here is to pursue the second possibility
mentioned above. Compared to `continues_on` that would effectively
relax the requirement that `affine_on` completes on `sch` if `sender`
doesn't actually change schedulers and completes inline: if `start(op)`
gets invoked on an execution agents which matches `sch`'s execution
resources the guarantee holds but `affine_on` would be allowed to
complete on the execution agent `start(op)` was invoked. It is upon
the user to invoke `start(op)` on the correct execution agent.

Another difference to `continues_on` is that `affine_on` can be
separately customized.

### `affine_on`'s Shape May Not Be Correct

The `affine_on` algorithm defined in
[`affine_on`](https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#executionaffine_on-exec.affine.on) takes two arguments; a
[`sender`](https://eel.is/c++draft/exec.snd.concepts) and a
[`scheduler`](https://eel.is/c++draft/exec.sched).

As mentioned above, the semantic that we really want for the purpose
in coroutines is that the operation completes on the same execution
context that it started on. This way, we can ensure, by induction,
that the coroutine which starts on the right context, and resumes
on the same context after each suspend-point, will itself complete
on the same context.

This then also begs the question: Could we just take the scheduler
that the operation will be started on from the
[`get_scheduler`](https://eel.is/c++draft/exec.get.scheduler) query
on the receivers environment and avoid having to explicitly pass
the scheduler as an argument?

To this end, we should consider potentially simplifying the `affine_on`
algorithm to just take an input sender and to pick up the scheduler
that it will be started on from the receivers environment and promise
to complete on that context.

For example, the
[`await_transform`](https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html?validation_key=7c6057b778f7a41d2ba63fd94676bf5e#class-taskpromise_type-task.promise)
function could potentially be changed to return:
`as_awaitable(affine_on(std::forward<Sndr>(sndr)))`.  Then we could
define the `affine_on.transform_sender(sndr, env)` expression (which
provides the default implementation) to be equivalent to
`continues_on(sndr, get_scheduler(env))`.

Such an approach would also a require change to the semantic
requirements of `get_scheduler(get_env(rcvr))` to require that
`start(op)` is called on that context.

This change isn't strictly necessary, though. The interface of
`affine_on` as is can be used. Making this change does improve the
design. Nothing outside of `task` is currently using `affine_on`
and as it is an algorithm tailored for `task`'s needs it seems
reasonable to make it fit this use case exactly. This change would
also align with the direction that the execution agent used to
invoke `start(op)` matches the execution resource of
`get_scheduler(get_env(rcvr))`.

### `affine_on` Shouldn't Forward Stop Requests To Scheduling Operations

The `affine_on` algorithm is used by the `task` coroutine to ensure
that the coroutine always resumes back on its associated scheduler
by applying the `affine_on` algorithm to each awaited value in a
`co_await` expression.

In cases where the awaited operation completes asynchronously,
resumption of the coroutine will be scheduled using the coroutines
associated scheduler via a
[`schedule`](https://eel.is/c++draft/exec.schedule) operation.

If that schedule operation completes with `set_value` then the
coroutine successfully resumes on its associated execution context.
However, if it completes with `set_error` or `set_stopped` then resuming
the coroutine on the execution context of the completion is not
going to preserve the invariant that the coroutine is always going
to resume on its associated context.

For some cases this may not be an issue, but for other cases,
resuming on the right execution context may be important for
correctness, even during exception unwind or due to cancellation.
For example, destructors may require running in a UI thread in order
to release UI resources. Or the associated scheduler may be a strand
(which runs all tasks scheduled to it sequentially) in order to
synchronise access to shared resources used by destructors.

Thus, if a stop-request has been sent to the coroutine, that
stop-request should be propagated to child operations so that the
child operation it is waiting on can be cancelled if necessary, but
should probably not be propagated to any schedule operation created
by the implicit `affine_on` algorithm as this is needed to complete
successfully in order to ensure the coroutine resumes on its
associated context.

One option to work around this with the status-quo would be to
define a scheduler adapter that adapted the underlying `schedule()`
operation to prevent passing through stop-requests from the parent
environment (e.g. applying the [`unstoppable`](https://wg21.link/p3284)
adapter). If failing to reschedule onto the associated context was
a fatal error, you could also apply a `terminate_on_error` adaptor
as well.

Then the user could apply this adapter to the scheduler before
passing it to the `task`.

For example:

```
template<std::execution::scheduler S>
struct infallible_scheduler {
  using scheduler_concept = std::execution::scheduler_t;
  S scheduler;
  auto schedule() {
    return unstoppable(terminate_on_error(std::execution::schedule(scheduler)));
  }
  bool operator==(const infallible_scheduler&) const noexcept = default;
};
template<std::execution::scheduler S>
infallible_scheduler(S) -> infallible_scheduler<S>;

std::execution::task<void, std::execution::env<>> example() {
  co_await some_cancellable_op();
}

std::execution::task<void, std::execution::env<>> caller() {
  std::execution::scheduler auto sched = co_await std::execution::read_env(get_scheduler);
  co_await std::execution::on(infallible_scheduler{sched}, example());
}
```

However, this approach has the downside that this scheduler behaviour
now also applies to all other uses of the scheduler - not just the
uses required to ensure the coroutines invariant of always resuming
on the associated context.

Other ways this could be tackled include:

- Making it the default behaviour of `affine_on` to suppress stop
    requests for the scheduling operations. That would mean that
    `affine_on` won't delegate to [`continues_on`]().
- Somehow making the behaviour a policy decision specified via the
    `Environment` template parameter of the `task`.
- Somehow using domain-based customisation to allow the coroutine
    to customise the behaviour of `affine_on`.
- Making the `task::promise_type::await_transform` apply this adapter
    to the scheduler passed to `affine_on`. i.e. it calls
    `affine_on(std::forward<Sndr>(sndr), infallible_scheduler{SCHED(*sched)})`.
    Taking this route would mean that the shape of `affine_on` should
    not be changed.

### `affine_on` Customization For Other Senders

Assuming the the `affine_on` algorithm semantics are changed to
just require that it completes either inline or on the context of
the receiver environments `get_scheduler` query, then there are
probably some other algorithms that could either make use of
this, or provide customisations for it that short-circuit the need
to schedule unnecessarily.

For example:

- `affine_on(just(args...))` could be simplified to `just(args...)`
- `affine_on(on(sch, sndr))` can be simplified to `on(sch, sndr)`
    as on already provides `affine_on`-like semantics
- The `counting_scope::join` sender currently already provides
    `affine_on`-like semantics. 
    - We could potentially simplify this sender to just complete
        inline unless the join-sender is wrapped in `affine_on`, in
        which case the resulting `affine_on(scope.join())` sender would
        have the semantics that `scope.join()` has today.
    - Alternatively, we could just customise `affine_on(scope.join())`
        to be equivalent to `scope.join()`.
- Other similar senders like those returned from
    `bounded_queue::async_push` and `bounded_queue::async_pop` which
    are defined to return a sender that will resume on the original
    scheduler.

The intended use of `affine_on` is to avoid scheduling where the
algorithm already resumes on a suitable execution agent. However,
as the proposal was late it didn't require potential optimizations.
The intend was to leave the specification of the default implementation
vague enough to let implementations avoid scheduling where they
know it isn't needed. Making these a requirement is intended for
future revisions of the standard.

## Task Operation

This section groups three concerns relating to the way `task` gets
started and stopped:

1. `task` shouldn't reschedule upon `start()`.
2. `task`s `co_await`ing `task`s shouldn't reschedule.
3. `task` doesn't support symmetric Transfer.

### Starting A `task` Should Not Unconditionally Reschedule

In
[[task.promise]]((https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#class-taskpromise_type-task.promise))
p6, the wording for `initial_suspend` says that it unconditionally
suspends and reschedules onto the associated scheduler. The intent
of this wording seems to be that the coroutine ensures execution
initially starts on the associated scheduler by executing a schedule
operation and then resuming the coroutine from the initial suspend
point inside the set_value completion handler of the schedule
operation. The effect of this would be that every call to a coroutine
would have to round-trip through the scheduler, which, depending
on the behaviour of the schedule operation, might requeue it to the
back of the schedulers queue, greatly increasing latency of the
call.

The specification of `initial_suspend()` (assuming it is rephrased
to not resume the coroutine immediately) doesn't really say it
unconditionally reschedules. It merely states that an awaiter is
returned which arranges for the coroutine to get resumed on the
correct scheduler. In general, i.e., when the coroutine gets resumed
via `start(os)` on an operation state `os` obtained from `connect(tsk,
rcvr)` it will need to reschedule.  However, an implementation can
pass additional context information, e.g., via implementation
specific properties on the receiver's environment: while the
`get_scheduler` query may not provide the necessary guarantee that
the returned scheduler is the scheduler the operation was started
on a different, non-forwardable query could provide this guarantee.

A possible alternative is to require that `start(op)`, where `op`
is the result of `connect(sndr, rcvr)`, is invoked on an execution
agent associated with `get_scheduler(get_env(rcvr))`, at least when
`sndr` is a `task<...>`. Such a requirement could be desirable more
general but it would also be part of the general sender/receiver
contract. The current specification in
[[exec.async.ops](https://eel.is/c++draft/exec.async.ops)] doesn't
have a corresponding constraint. `task<...>` is a sender and
where it can be used with standard library algorithms this constraint
would hold.

A different way to approach the issue is to use a `task`-specific
awaitable when a `task` is `co_await`ed by another `task`. This use
of an awaiter shouldn't be observable but it may be better to
explicitly spell out that `task` has a domain with custom transformation
of the `affine_on` algorithm and `as_awaitable` operation.

### Resuming After A `task` Should Not Reschedule

Similar to the previous issue, a `task` needs to resume on the
expected scheduler. The definition of `await_transform` which is used
for a `task` is defined in [[task.promise]]((https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#class-taskpromise_type-task.promise)) paragraph 10
(the `co_await`ed `task` would be the `sndr` parameter):

```
as_awaitable(affine_on(std::forward<Sender>(sndr), SCHED(*this)), *this)
```

As defined there is no additional information about the scheduler
on which the `sndr` completes. Thus, it is likely that a scheduling
operation is needed after a `co_await`ed `task` completes when the
outer `task` resumes, even if the `co_await`ed `task` completed on
the same execution agent.

As `task` is scheduler affine, it is likely that it completes on
the same scheduler it was started on, i.e., there shouldn't be a
need to reschedule (if the scheduler used by the `task` was changed
using `co_await change_coroutine_scheduler(sch)` it still needs to
be rescheduled). The implementation can provide a query on the state
used to execute the `task` which identifies the scheduler on which
it completed and the `affine_on` implementation can use this
information to decide whether it is necessary to reschedule after
the `task`'s completion. It would be preferable if a corresponding
query were specificied by the standard library to have user-defined
senders provide similar information but currently there is no such
query.

A different approach is to transform the `affine_on(task, sch)`
operation into a `task`-specific implementation which arranges to
always complete on `sch` using `task` specific information. It may
be necessary to explicit specify or, at least, allow that `task`
provides a domain with a custom transformation for `affine_on`.

### No Support For Symmetric Transfer

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

- Agreed: like other senders, `task` doesn't have an awaitable interface.
- To guarantee scheduler affinity, it may be necessary to reschedule, i.e., the whether symmetric transfer could be used is conditional.
- With scheduler affinity there isn't an issue (also see below).
- I don't think an implementation is prohibited from providing an awaiter interface and taking advantage of symmetric transfer: how does the user tell?

## Allocation

### Unusual Allocator Customisation

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

### Shadowing The Environment Allocator Is Questionable

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

## Stop Token Management

### A Stop Source Always Needs To Be Created

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

## Miscelleaneous Concerns

The remaining concerns aren't as coupled to other concerns and
discussed separately.

### Task Is Not Actually Lazily Started

The wording for `task<...>::promise_type::initial_suspend` in
[[task.promise]](https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#class-taskpromise_type-task.promise)
p6 may imply that a `task` is eagerly started:

> `auto initial_suspend() noexcept;`
>
> _Returns:_ An awaitable object of unspecified type ([expr.await])
> whose member functions arrange for
>
> - the calling coroutine to be suspended,
> - the coroutine to be resumed on an execution agent of the execution
>     resource associated with `SCHED(*this)`.

In particular the second bullet can be interpreted to mean that the
task gets resumed immediately. That wouldn't actually work because
`SCHED(*this)` only gets initialized when the `task` gets `connect`ed
to a suitable receiver. The intention of the current specification
is to establish the invariant that the coroutine is running on the
correct scheduler when the coroutine is resumed (see discussion of
`affine_on` below). The mechanisms used to achieve that are not
detailed to avoid requiring that it gets scheduled. The formulation
should, at least, be improved to clarify that the coroutine isn't
resumed immediates, possibly changing the text like this:

> - the coroutine [to be resumed]{.rm}[resuming]{.add} on an execution
>     agent of the execution resource associated with `SCHED(*this)`
>     [when it gets resumed]{.add}.

The proposed fix from the issue is to specify that `initial_suspend()`
always returns `suspend_always{}` and require that `start(...)`
calls `handle.resume()` to resume the coroutine on the appropriate
scheduler after `SCHED(*this)` has been initialized. The corresponding
change could be

Change [[task.promise]](https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#class-taskpromise_type-task.promise) paragraph 6:

> `auto initial_suspend() noexcept;`
>
::: rm
> _Returns:_ An awaitable object of unspecified type ([expr.await])
> whose member functions arrange for:
>
> - the calling coroutine to be suspended,
> - the coroutine to be resumed on an execution agent of the execution
>     resource associated with `SCHED(*this)`.
:::
::: add
> _Returns:_ `suspend_always{}`.
:::

The suggestion is to ensure that the task gets resumed on the
correct associated context via added requirements on the receiver's
`get_scheduler()` (see below).

In separate discussions it was suggested to relax the specification
of `initial_suspend()` to allow returning an awaiter which does
semantically what `suspend_always` does but isn't necessary
`suspend_always`. The proposal was to copy the wording of the
`suspend_always` specification
[[coroutine.trivial.awaitables]](https://eel.is/c++draft/coroutine.trivial.awaitables#lib:suspend_always).
This specification just shows the class completion definition.

### `task<T, E>` Has No Default Arguments

The current specification of `task<T, E>` doesn't have any default arguments
in its first declaration in [[execution.syn]](https://eel.is/c++draft/execution.syn). The intent was to default the
type `T` to `void` and the environment `E` to `env<>`. That is, this change
should be applied to [[execution.syn]](https://eel.is/c++draft/execution.syn):

> ```
>   ...
>   // [exec.task]
>   template <class T@[ = void]{.add}@, class Environment@[ = env<>]{.add}@>
>   class task;
>   ...
> ```

It isn't catastrophic if that change isn't made but it seems to
improve usability without any drawbacks.

### `unhandled_stopped()` Isn't `noexcept`

The `unhandled_stopped()` member function of `task::promise_type`
[[task.promise]](https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#class-taskpromise_type-task.promise)
is not currently marked as `noexcept`.

As this method is generally called from the `set_stopped`
completion-handler of a receiver (such as in [[exec.as.awaitable] p4.3](https://eel.is/c++draft/exec.as.awaitable#4.3))
and is invoked without handler from a `noexcept` function, we should
probably require that this function is marked `noexcept` as well.

The equivalent method defined as part of the with_awaitable_senders
base-class
([[exec.with.awaitable.senders]](https://eel.is/c++draft/exec.with.awaitable.senders#1)
p1) is also marked `noexcept`.

This change should be applied to [[task.promise]](https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#class-taskpromise_type-task.promise):

> ```
>     ...
>     void uncaught_exception();
>     coroutine_handle<> unhandled_stopped()@[ noexcept]{.add}@;
> 
>     void return_void(); // present only if is_void_v<T> is true;
>     ...
> ```
>
> ...
>
> ```
> coroutine_handle<> unhandled_stopped()@[ noexcept]{.add}@;
> ```
>
> [13]{.pnum} _Effects_: Completes the asynchronous operation associated with `STATE(*this)` by invoking `set_stopped(std::move(RCVR(*this)))`.

### The Environment Design Is Odd

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

### No Completion Scheduler

The concern raised is that `task` doesn't define a `get_env` that
returns an environent with a `get_completion_scheduler<Tag>` query.
As a result, it will be necessary to reschedule in situations although
the `task` already completes on the correct scheduler.

The query `get_completion_scheduler(env)` operates on the sender's
enviroment, i.e., it would operate on the `task`'s enironment.
However, when the `task` is created it has no information about the
scheduler, yet, it is going to use. The entire information the
`task` has is what is passed to the corouting [factory] function.
The information about any scheduling gets provided when the `task`
is `connect`ed to a receiver: the receiver provides the queries on
where the task is started (`get_scheduler`). The `task` may complete
on this scheduler assuming the scheduler isn't changed (using
`co_awaiit change_coroutine_scheduler(sch)`).

The only place where a `task` actually knows its completion scheduler
is once it has completed. However, there is no query or environment
defined for completed operation states. Thus, it seems there is no
way where a `get_completion_scheduler` would be useful. A future
evolution of the sender/receiver interface may change that in which
case `task` should be revisted.

### Awaitable non-`sender`s Are Not Supported

The overload of `await_transform` described in
[[task.promise]](https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#class-taskpromise_type-task.promise)
is constrained to require arguments to satisfy the
[`sender`](https://eel.is/c++draft/exec.snd.concepts) concept.
However, this precludes awaiting types that implement the
[`as_awaitable()`](https://eel.is/c++draft/exec.as.awaitable)
customization point but that do not satisfy the
[`sender`](https://eel.is/c++draft/exec.snd.concepts) concept from
being able to be awaited within a `task` coroutine.

This is inconsistent with the behaviour of the [`with_awaitable_senders`]()
base class defined in
[[exec.with.awaitable.senders]](https://eel.is/c++draft/exec.with.awaitable.senders),
which only requires that the awaited value supports the
[`as_awaitable()`](https://eel.is/c++draft/exec.as.awaitable)
operation.

The rationale for this is that the argument needs to be passed to
the
[`affine_on`](https://wiki.edg.com/pub/Wg21sofia2025/StrawPolls/P3552R3.html#executionaffine_on-exec.affine.on)
algorithm which currently requires its argument to model
[`sender`](https://eel.is/c++draft/exec.snd.concepts). This requirement in turn is
there because it is unclear how to guarantee scheduler affinity with an awaitable-only
interface without wrapping a coroutine around the awaitable.

Do we want to consider relaxing this constraint to be consistent with the constraints
on 
[[exec.with.awaitable.senders]](https://eel.is/c++draft/exec.with.awaitable.senders)?

This would require either:

- relaxing the [`sender`](https://eel.is/c++draft/exec.snd.concepts)
  constraint on the `affine_on` algorithm to also allow an argument
  that has only an
  [`as_awaitable()`](https://eel.is/c++draft/exec.as.awaitable) but
  that did not satisfy the
  [`sender`](https://eel.is/c++draft/exec.snd.concepts) concept.
- extending the [`sender`](https://eel.is/c++draft/exec.snd.concepts)
  concept to match types that provide the `.as_awaitable()`
  member-function similar to how it supports types with `operator
  co_await()` (see [[exec.connect]](https://eel.is/c++draft/exec.connect)).

### A Future Coroutine Feature Could Avoid `co_yield` For Errors

The mechanism to complete with an error without using an exception
uses `co_yield with_error(x)`. This use may surprise users as
`co_yield`ing is normally assumed not to be a final operation. A
future language change may provide a better way to achieve the
objective of reporting errors without using exceptions.

Using not, yet, proposed features which may become part of a future
revision of the C++ standard may always provide facilities which
result in a better design for pretty much anything. There is no
current `task` implementation and certainly none which promises ABI
stability. Assuming a corresponding language change gets proposed
reasonably early for the next cycle, implementation can take it into
account for possibly improving the interface without the need to
break ABI. The shape of the envisioned language change is unknown
but most likely it is an extension which hopefully be integrated
with the existing design. The functionality to use `co_yield`
would, however, continue to exist until it eventually gets
deprecated and removed (assuming a better alternative emerges).

### There Is No Hook To Capture/Restore TLS

This concern is the only one I was aware of for a few weeks prior
to the Sofia meeting. I believe the necessary functionality can be
implemented although it is possibly harder to do than with more
direct support. The concern raised is that xisting code accesses
thread local storage (TLS) to store context information. When a
`co_await` actually suspends it is possible that a different task
running on the same thread replaces used TLS data or the task gets
resumed on a different thread of a thread pool. In that case it is
necessary to capture the TLS variables prior to suspending and
restoring them prior to resuming the coroutine.  There is currently
no way to actually do that.

The sender/receiver approach to propagating context information
isn't using TLS but rather the environments accessed via the receiver.
However, existing software does use TLS and at least for migration
corresponding support may be necessary. One way to implement this
functionality is using a scheduler adapter with a customized
`affine_on` algorithm:

- When the `affine_on` algorithm is started, it captures the relevant
    TLS data into the operation state and then starts `affine_on`
    for the adapted scheduler with a receiver observing the
    completions.
- When the adapted scheduler invokes any of the completion signals
    the TLS data is restored before invoke the algorithms completion
    signal.

Support for optionally capturing some state before starting a child
operation and restoring the captured started before resuming could
be added to the `task`, avoiding the need to use custom scheduling.
Doing so would yield a nicer and easier to use interface. In
environment where TLS is currently used and developers move to an
asychronous model, failure to capture and restore data in TLS is a
likely source of errors. However, it will be necessary to specify
what data needs to be stored, i.e., the problems can't be automatically
avoided.

# Potential Polls

1. Should `task` be renamed to something else?
2. Is the naming scheme `task@_year_@` an approach to be used?

# Acknowledgement

The issue descriptions are largely based on a draft written by
Lewis Baker. Lewis Baker and Tomasz Kamiński contributed to the
discussions towards addressing the issues.
