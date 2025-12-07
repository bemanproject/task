---
title: Scheduler Affinity
document: D0000R0
date: 2025-12-07
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

<p>
One important design of `std::execution::task` is that a coroutine
resumes after a `co_await` on the same scheduler as the one it was
executing on prior to the `co_await`. To achieve this, `task`
transforms the awaited object <code><i>obj</i></code> using
<code>affine_on(<i>obj</i>, <i>sched</i>)</code> where
<code><i>sched</i></code> is the corresponding scheduler. There
were multiple concerns raised against the specification of `affine_on`
and discussed as part of [P3796R1](https://wg21.link/P3796R1).  This
proposal is intended to specifically address the concerns raised
relating to `task`'s scheduler affinity and in particular `affine_on`.
The gist of this proposal is impose constraints on `affine_on` to
guarantee it can meet its objective at run-time.
</p>

# Change History

## R0 Initial Revision

# Overview of Changes

<p>
There are a few NB comments raised about the way `affine_on` works:
</p>
<ul>
   <li>[US 232-366](https://github.com/cplusplus/nbballot/issues/941): specify customization of `affine_on` when the scheduler doesn't change.</li>
   <li>[US 233-365](https://github.com/cplusplus/nbballot/issues/940): clarify `affine_on` vs. `continues_on`.</li>
   <li>[US 234-364](https://github.com/cplusplus/nbballot/issues/939): remove scheduler parameter from `affine_on`.</li>
   <li>[US 235-363](https://github.com/cplusplus/nbballot/issues/938): `affine_on` should not forward the stop token to the scheduling operation.</li>
   <li>[US 236-362](https://github.com/cplusplus/nbballot/issues/937): specify default implementation of `affine_on`.</li>
</ul>
<p>
The discussion on `affine_on` revealed some aspects which were not
quite clear previously and taking these into account points towards
a better design than was previously specified:
</p>
<ol>
  <li>
    To implement scheduler affinity the algorithm needs to know the
    scheduler on which it was started itself. The correct receiver
    may actually be hard to determine while building the work graph.
    However, this scheduler can be communicated using
    <code>get_scheduler(get_env(<i>rcvr</i>))</code> when an algorithm
    is `start`ed. This requirement is more general than just
    `affine_on` and is introduced by
    [P3718R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3718r0.html):
    with this guarantee in place, `affine_on` only needs one
    parameter, i.e., the sender for the work to be executed.
  </li>
  <li>
    The scheduler <code><i>sched</i></code> on which the work needs
    to resume has to guarantee that it is possible to resume in the
    correct execution agent. The implication is that scheduling work needs
    to be infallible, i.e., the completion signatures of
    <code>scheduler(<i>sched</i>)</code> cannot contain a
    `set_error_t(E)` completion signature. This requirement should
    be checked statically.
  </li>
  <li>
    The work needs to be resumed on the correct scheduler even when
    the work is stopped, i.e., the scheduling operation shall be
    `connect`ed to a receiver whose environment's `get_stop_token`
    query yields an `unstoppable_token`. In addition, the
    schedule operation shall not have a `set_stopped_t()` completion
    signature if the environment's `get_stop_token` query yields
    an `unstoppable_token`. This requirement should also be checked
    statically.
  </li>
  <li>
    When a sender knows that it will complete on the scheduler it
    was start on, it should be possible to customize the `affine_on`
    algorithm to avoid rescheduling. This customization can be
    achieved by `connect`ing to the result of an `affine_on` member
    function called on the child sender, if such a member function
    is present, when `connect`ing an `affine_on` sendering
  </li>
</ol>

<p>
None of these changes really contradict any earlier design: the
shape and behavior of the `affine_on` algorithm wasn't fully fleshed
out. Tightening the behavior scheduler affinity and the `affine_on`
algorithm has some implications on some other components:
</p>
<ol>
  <li>
    If `affine_on` requires an infallible scheluder at least
    `inline_scheduler`, `task_scheduler`, and `run_loop::scheduler`
    should be infallible (i.e., they always complete successfully
    with `set_value()`). `parallel_scheduler` can probably not be
    made infallible.
  </li>
  <li>
    The scheduling semantics when changing a `task`'s scheduler
    using <code>co_await change_coroutine_scheduler(<i>sch</i>)</code>
    become somewhat unclear and this functionality should be removed.
    Similar semantics are better modeled using <code>co_await
    on(<i>sch</i>, <i>nested-task</i>)</code>.
  </li>
  <li>
    The name `affine_on` isn't particular good and wasn't designed.
    It may be worth renaming the algorithms to something different.
  </li>
</ol>

# Discussion of Changes

## `affine_on` Shape

<p>
The original proposal for `task` used `continues_on` to schedule
the work back on the original scheduler. This algorithm takes the
work to be executed and the scheduler on which to continue as
arguments. When SG1 requested that a similar but different algorithms
is to be used to implement scheduler affinity, `continues_on` was
just replaced by `affine_on` with the same shape but the potential
to get customized differently.
</p>
<p>
For scheduler affinity the scheduler to resume on can, however,
also be communicated via the `get_scheduler` query on the receiver's
environment. The result from `get_scheduler` is also the scheduler
any use of `affine_on` would use when invoking the algorihtm. In
the context of the `task` coroutine this scheduler can be obtained
via the promise type but in general it is actually not straight
forward to get hold of this scheduler because it is only provided
by `connect`. It is much more reasonable to have `affine_on` only
take the work, i.e., a sender, as argument and determine the scheduler
to resume on from the receiver's environment in `connect`.
</p>
<p>
Thus, instead of using
```c++
affine_on(@_sndr_@, @_sch_@)
```
the algorithm is used just with the sender:
```c++
affine_on(@_sndr_@)
```
</p>
<p>
Note that this change implies that an operation state resulting
from `connect`ing `affine_on` to a receiver <code><i>rcvr</i></code>
is `start`ed on the execution agent associated with the scheduler obtained
from <code>get_scheduler(get_env(<i>rcvr</i>))</code>. The same
requirement is also assumed to be meet when `start`ing the operation
state resulting from `connect`ing a `task`. While it is possible to
statically detect whether the query is valid and provides a scheduler
it cannot be detected if the scheduler matches the execution agent on which
`start` was called.
[P3718r0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3718r0.html)
proposes to add this exact requirement to
[[exec.get.scheduler]](https://wg21.link/exec.get.scheduler).

</p>

## Infallible Schedulers
<p>
The objective of `affine_on(@_sndr_@)` is to execute `@_sndr_@` and
to complete on the execution agent on which the operation was
`start`ed. Let `sch` be the scheduler obtained from
`get_scheduler(get_env(@_rcvr_@))` where `@_rcvr_@` is the receiver
used when `connect`ing `affine_on(@_sndr_@)`. If `connect`ing and
`start`ing the result of `schedule(@_sch_@)` is successful, `affine_on`
can achieve its objective. However, if this scheduling operation
fails, i.e., it completes with `set_error(@_e_@)`, or if it gets
cancelled, i.e., it completes with `set_stopped()`, the execution
agent on which the scheduling operation resumes is unclear and
`affine_on` cannot guarantee its promise. Thus, it seems reasonable
to require that a scheduler used with `affine_on` is infallible, at
least when used appropriately.
</p>
<p>
The current working draft specifies 4 schedulers:
</p>
<ol>
  <li>
  [`inline_scheduler`](https://wg21.link/exec.inline.scheduler) which
  just completes with `set_value()` when `start()`ed, i.e., this
  scheduler is already infallibe.
  </li>
  <li>
  [`task_scheduler`](https://wg21.link/exec.task.scheduler) is a
  type-erased scheduler delegating to another scheduler. If the
  underlying scheduler is infallible, the only error case for
  `task_scheduler` is potential memory allocation during `connect`
  of its `@_ts-sender_@`. If `affine_on` creates an operation state
  for the scheduling operation during `connect`, it can guarantee
  that any necessary scheduling operation succeeds. Thus, this
  scheduler can be made infallible.
  </li>
  <li>
  The [`run_loop::@_run-loop-scheduler_@`](https://wg21.link/exec.run.loop)
  is used by [`run_loop`](https://wg21.link/exec.run.loop). The
  current specification allows the scheduling operation to fail
  with `set_error_t(std::exception_ptr)`. This permission allows
  an implementation to use [`std::mutex`](https://wg21.link/thread.mutex)
  and [`std::condition_variable`](https://wg21.link/thread.condition)
  whose operations may throw. It is possible to implement the logic
  using atomic operations which can't throw. The `set_stopped()`
  completion is only used when the receiver's stop token, i.e. the
  result of `get_stop_token(get_env(@_rcvr_@))`, was stopped. This
  receiver is controlled by `affine_on`, i.e., it can provide a
  [`never_stoptoken`](https://wg21.link/stoptoken.never) and this
  scheduler won't complete with `set_stopped()`.  If the
  [`get_completion_signatures`](https://wg21.link/exec.getcomplsigs) for
  the corresponding sender takes the environment into account, this
  scheduler can also be made infallible.
  </li>
  <li>
  The [`parallel_scheduler`](https://wg21.link/exec.par.scheduler)
  provides an interface to a replacable implementation of a thread
  pool. The current interface allows
  [`parallel_scheduler`](https://wg21.link/exec.par.scheduler) to
  complete with `set_error_t(std::exception_ptr)` as well as with
  `set_stopped_t()`. It seems unlikely that this interface can be
  constrained to make it infallible.
  </li>
</ol>
<p>
In general it seems unlikely that all schedulers can be constrained
to be infallible. As a result `affine_on` and, by extension, `task`
won't be usable with all schedulers if `affine_on` insists on using
only infallible schedulers. Note that `affine_on` can fail and get
cancelled but in all cases its promise is that it resumes on the
original scheduler. Thus, a `set_error(@_e_@)` completion can't be
used to indicate scheduling failure, either.
</p>
<p>
If a users wants to use a fallible scheduler with `affine_on` or
`task` the scheduler will need to be adapted. The adapted scheduler
can define what it means when the underlying scheduler fails. For
example, the user can cause this failure to terminate the program
or consider the execution agent on which the underlying scheduler
completed to be suitable to continue running.
</p>

## `affine_on` Customization

TODO

## Removing `change_coroutine_scheduler`

TODO

# Wording Changes