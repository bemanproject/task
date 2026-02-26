## [P2583: Symmetric Transfer and Sender Composition](https://isocpp.org/files/papers/D2583R1.pdf)

### Summary

[`std::execution`](https://wg21.link/exec) doesn't support symmetric transfer => stack overflows are inevitable.

### Details

- Section 2: History of symmetric transfer and why it was added: zero cost.
- Section 3: Survey of coroutine libraries: these use symmetric
  transfer
  ([Unifex](https://github.com/facebookexperimental/libunifex/tree/main/source)
  and [stdexec](https://github.com/NVIDIA/stdexec) are not on the
  list).
- Section 4: How coroutine integration works with
  [`std::execution`](https://wg21.link/https://eel.is/c++draft/exec):
  [<code><i>connect-awaitable</i></code>](https://eel.is/c++draft/exec#connect-3)
  and
  [<code><i>https://eel.is/c++draft/exec#as.awaitable-2</i></code>](https://eel.is/c++draft/exec#as.awaitable-2),
  neither using symmtric transfer
- Section 5: the sender protocol doesn't/can't (in its current form)
  return `std::coroutine_handle`, even when there _could_ be a `std::coroutine_handle`.
- Section 6: [`std::execution::task`](https://eel.is/c++draft/exec#task)
  uses [`affine_on`](https://eel.is/c++draft/exec#affine.on) and,
  thus, doesn't use symmetric transfer.  As a result, iterative-looking
  code may stack overflow (it is really recursive), as demonstrated
  by an example (which does not stack overflow using
  [beman.task](https://github.com/bemanproject/task), even when trying:
  using `inline_scheduler` and masking the `task`).
- Section 7: `task`-specific mitigation: `task` to `task` work uses
  symmetric transfer but is incomplete. The other mitigations (always
  actually schedule, trampoline scheduler) are not zero cost.
- Section 8: `task` can, unsurprisingly, only be launched via sender
  interface (unless it is `co_await`ed).
  [`Boost.Capy`](https://github.com/cppalliance/capy) is awesome
  and can do better.
- Section 9: the absence of symmetric transfer cannot be fixed in
  current model at all with zero cost!


## [P4003: Coroutines for I/O](https://isocpp.org/files/papers/P4003R0.pdf)
## [P4007: Senders and Coroutines)](https://isocpp.org/files/papers/P4007R0.pdf)
