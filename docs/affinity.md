---
title: Scheduler Affinity
document: D????
date: 2025-11-23
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
guarantee it can its objective at run-time.
</p>

# Change History

## R0 Initial Revision

# Discussion

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
a better design than was previously documented:
</p>
<ol>
  <li>
    To implement scheduler affinity the algorithm needs to know the
    scheduler on which it was started itself. The correct receiver
    may actually be hard to determine while build the work graph.
    However, this scheduler is communicated using
    <code>get_scheduler(get_env(<i>rcvr</i>))</code> when an algorithm
    is `start`ed. This requirement is more general than just
    `affine_on` and is introduced by
    [P3826R2](https://isocpp.org/files/papers/P3826R2.html) *TODO*
    verify the reference: with this guarantee in place, `affine_on`
    only needs one parameter, i.e., the sender for the work to be
    executed.
  </li>
  <li>
    The scheduler <code><i>sched</i></code> on which the work needs
    to resume has to guarantee that it is possible to resume in the
    correct context. The implication is that scheduling work needs
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
</ol>
