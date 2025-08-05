# Examples

## Code used to prepare Dietmar's C++Now 2025 presentation

<details>
<summary>
<a href='https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-affinity.cpp'>c++now-affinity.cpp</a>
<a href='https://godbolt.org/z/8qEG5x7sz'><img src='https://raw.githubusercontent.com/bemanproject/task/refs/heads/main/docs/compiler-explorer.ico' width='15' height='15'/></a>:
demo scheduler affinity
</summary>

The example program
[`c++now-affinity.cpp`](https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-affinity.cpp)
uses [`demo::thread_loop`](../examples/demo-thread_loop.hpp) to demonstrate
the behavior of _scheduler affinity_: the idea is that scheduler
affinity causes the coroutine to resume on the same scheduler as
the one the coroutine was started on. The program implements three
coroutines which have most of their behavior in common:

1. Each coroutine is executed from `main` using <code>sync_wait(_fun_(loop.get_scheduler()))</code>.
2. Each coroutine prints the id of the thread it is executing on prior to any `co_await` and after all `co_await` expression.
3. Each coroutine `co_await`s the result of `scheduler(sched) | then([]{ ... })` where the function passed to `then` just prints the thread id.
4. `work2` additionally changes the coroutines scheduler to be an `inline_scheduler` and later restores the original scehduler using `change_coroutine_scheduler`.
5. While `work1` and `work2` use the default scheduler (`task_scheduler` a
type-erased scheduler which gets initialized from the receiver's environment's `get_scheduler`), `work3` sets the coroutine's scheduler up to be `inline_scheduler`, effectively causing the coroutine to resume wherever
the `co_await`'s expression resumed.

The output of the program is someting like the below:

```
before id=0x1fd635f00
then id  =0x16b64b000
after id =0x1fd635f00

before id=0x1fd635f00
then id  =0x16b64b000
after1 id=0x16b64b000
then id  =0x16b64b000
after2 id=0x1fd635f00

before id=0x1fd635f00
then id  =0x16b64b000
after id =0x16b64b000
```

It shows that:

1. The thread on which the `then`'s function is executed is always the
same and different from the thread each of the coroutines started on.
2. For `work1` the `co_await` resumes on the same thread as the one the
coroutine was started on.
3. For `work2` the first `co_await` after `schedule(sched)` resumes on
the thread used by `sched`. After restoring the original scheduler the
`co_await` resumes on the original thread.
4. For `work3` the `co_await` resumes on the thread used by `sched` as
the `inline_scheduler` doesn't do any actual scheduling.

</details>

<details>
<summary>
<a href='https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-allocator.cpp'><code>c++now-allocator.cpp</code></a>
<a href='https://godbolt.org/z/719v7en6a'><img src='https://raw.githubusercontent.com/bemanproject/task/refs/heads/main/docs/compiler-explorer.ico' width='15' height='15'/></a>:
demo allocator use
</summary>

This demo shows how to configure `task`'s environment argument to
use a different allocator than the default `std::allocator<std::byte>`.
To do so it defines an environment type `with_allocator` which
defines a nested type alias `allocator_type` to be
`std::pmr::polymorphic_allocator<std::byte>`.

The coroutine `coro` shows how to use `read_env` to extract the
used allocator object to potentially use it for any allocation
purposes within the coroutine.  There are two uses of `coro`, the
first one using the default which just uses
`std::pmr::polymorphic_allocator<std::byte>()` to allocate memory.
The second use explicitly specifies the memory resource
`std::pmr::new_delete_resource()` to initialized the use
`std::pmr::polymorphic_allocator<std::byte>`.

</details>

<details>
<summary>
<a href='https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-basic.cpp'><code>c++now-basic.cpp</code></a>
<a href='https://godbolt.org/z/7Pn5TEhfK'><img src='https://raw.githubusercontent.com/bemanproject/task/refs/heads/main/docs/compiler-explorer.ico' width='15' height='15'/></a>:
demo basic task use
</summary>

The example
<a href='https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-basic.cpp'><code>c++now-basic.cpp</code></a>
shows some basic use of a `task`:

1. The coroutine `basic` just `co_await`s the awaiter `std::suspend_never{}` which immediately completes.
    This use demonstrates that any awaiter can be `co_await`ed by a `task<...>`.
2. The coroutine `await_sender` demonstrates the results of `co_await`ing various senders. It uses variations of
    `just*` to show the different results:
    - `co_await`ing a sender completing with `set_value_t()`, e.g., `just()`, produces an expression with type `void`.
    - `co_await`ing a sender completing with `set_value_t(T)`, e.g., `just(1)`, produces an expression with type `T`.
    - `co_await`ing a sender completing with <code>set_value_t(T<sub>0</sub>, ..., T<sub>n</sub>)</code>, e.g., `just(1, true)`, produces an expression with type <code>tuple&lt;T<sub>0</sub>, ..., T<sub>n</sub>&gt;</code>`.
    - `co_await`ing a sender completing with `set_error_t(E)`, e.g., `just_error(1)`, results in an exception of type `E` being thrown.
    - `co_await`ing a sender completing with `set_stopped_t()`, e.g., `just_stopped()`, results in the corouting never getting resumed although all local objects are properly destroyed.
</details>

<details>
<summary>
<a href='https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-cancel.cpp'><code>c++now-cancel.cpp</code></a>
<a href='https://godbolt.org/z/vx4PqYvE6'><img src='https://raw.githubusercontent.com/bemanproject/task/refs/heads/main/docs/compiler-explorer.ico' width='15' height='15'/></a>:
demo how a `task` can actively cancel the work
</summary>

The example
<a href='https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-basic.cpp'><code>c++now-basic.cpp</code></a>
shows a coroutine `co_await`ing `just_stopped()` which results in the coroutine getting cancelled. The coroutine will
complete with `set_stopped()`.
</details>

- [`c++now-errors.cpp`](https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-errors.cpp) [![Compiler Explorer](compiler-explorer.ico)](https://godbolt.org/z/95Mhr5MGn)
- [`c++now-query.cpp`](https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-query.cpp) [![Compiler Explorer](compiler-explorer.ico)](https://godbolt.org/z/dPboEeqfv)
- [`c++now-result-types.cpp`](https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-result-types.cpp) [![Compiler Explorer](compiler-explorer.ico)](https://godbolt.org/z/aWfc8T8he)
- [`c++now-return.cpp`](https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-return.cpp) [![Compiler Explorer](compiler-explorer.ico)](https://godbolt.org/z/f5YE5W4Ta)
- [`c++now-stop_token.cpp`](https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-stop_token.cpp) [![Compiler Explorer](compiler-explorer.ico)](https://godbolt.org/z/TxYe3jEs7)
- [`c++now-with_error.cpp`](https://github.com/bemanproject/task/blob/main/examples/c%2B%2Bnow-with_error.cpp) [![Compiler Explorer](compiler-explorer.ico)](https://godbolt.org/z/6oqox6zf8)

## Tools Used By The Examples

<details>
<summary>
[`demo::thread_loop`](../examples/demo-thread_loop.hpp) is a `run_loop` whose `run()` is called from a `std::thread`.
</summary>

Technically [`demo::thread_loop`](../examples/demo-thread_loop.hpp)
is a class `public`ly derived from `execution::run_loop` which is
also owning a `std::thread`. The `std::thread` is constructed with
a function object calling `run()` on the
[`demo::thread_loop`](../examples/demo-thread_loop.hpp) object.
Destroying the object calls `finish()` and then `join()`s the
`std::thread`: the destructor will block until the `execution::run_loop`'s
`run()` returns.

The important bit is that work executed on the
[`demo::thread_loop`](../examples/demo-thread_loop.hpp)'s `scheduler`
will be executed on a corresponding `std::thread`.
</details>
