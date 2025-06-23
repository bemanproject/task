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

    for (std::size_t i{}; i < 1000000; ++i) {
        co_await std::invoke([]()->std::execution::task<> {
            co_await std::execution::just();
        });
    }

TODO - turn into text

- Agreed: like other senders, `task` doesn't have an awaitable
    interface.
- To guarantee scheduler affinity, it may be necessary to reschedule.
- With scheduler affinity there isn't an issue (also see below).
- I don't think an implementation is prohibited from providing a
    an awaiter interface and taking advantage of symmetric transfer.

## Unusual Allocator Customisation

Concern raised:

> allocator customisation mechanism is inconsistent with the design
> of `generator` allocator customisation (with `generator`, when you
> don't specify an allocator in the template arg, then you can use
> any `allocator_arg` type, `allocator_arg` is only allowed in the
> first or second position), A bit vage - I'm not sure what's the
> suggested design here, but I suspect this can be fixed.

TODO add example and response

## `affine_on` Underspecified

Concern raised:

> the semantics of the `affine_on` customisation point seems poorly
> defined, or not exactly what we need it to be (how does the
> operatiom map onto a scheduler and how does it determine whether
> or not it needs to reschedule?),

TODO add example and response

## Starting A `task` Reschedules

Concern raised:

> calling a task coroutine unconditionally schedules onto
> its scheduler (a large performance bottleneck),

TODO add example and response

## The Environment Design Is Odd

Concern raised:

> the design of the `Environment` template parameter seems weird -
> it's trying to be a trait-like class but also a `queryable`,

TODO add example and response

## Shadowing The Environment Allocator Is Questionable

Concern raised:

> I'm unsure wehther the shadowing of the `get_allocator` query
> on the parent environment with the coroutine promise's allocator
> is the behavior we really wnat here.,

TODO add example and response

## No Completion Scheduler

Concern raised:

> task doesn't define `get_env` that returns an env with a
> completion-scheduler, this means awaiting a task from within a
> coroutine will always reschedule back onto the parent coroutine's
> context - so you schedule when entering a coroutine and schedule
> when resuming - very inefficient,

TODO add example and response

## A Stop Source Always Needs To Be Created

Concern raised:

> the way the promise is worded seems to specify that a stop-source
> is always constructed as a member of the coroutine promise - you
> shoud really only need a new stop-source when adapting between
> incompatable stop-token types, if everything is using
> `inplace_stop_token` then it should just be passed through and have
> no space overhead in the promise.

TODO add example and response

## A Future Coroutine Feature Could Avoid `co_yield` For Errors

Concern raised:

> I would prefer a language change to avoid the hack using
> `co_yield with_error(x)`.

TODO add example and response

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

TODO add example and response

- using a custom scheduler with a custom implementation of
    `affine_on` allows this functionality (via the domain)
- it would still be nice to have an easier interface
