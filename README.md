<!--
SPDX-FileCopyrightText: 2026 roolrz
SPDX-License-Identifier: Apache-2.0
-->

# HypeR Lib

HypeR Lib is the freestanding C foundation for applications compiled directly
for the HypeR Native ABI. It provides low-level syscall veneers and the small
set of C primitives required before a complete native runtime exists.

This component is deliberately not a Linux, FreeBSD, or POSIX compatibility
layer. Foreign OS personalities are outside the Native SDK and are not a
roadmap commitment. Any future compatibility runtime would keep its contracts
separate from HypeR Lib.

## Current scope

- AArch64 and RV64 Native syscall entry using the published machine conventions;
- Native startup-stack parsing, CRT entry, and bootstrap-handle discovery;
- capability-scoped console and filesystem I/O, object wait, byte and capability
  channels, VMO/VMAR operations, staged Process construction, and core
  lifecycle wrappers;
- a shared process heap with `malloc`, `calloc`, `realloc`, `free`, and `aligned_alloc`;
- freestanding memory and string primitives, including `memcpy`, `memset`,
  `memcmp`, `memchr`, `strlen`, `strcmp`, `strncmp`, and `strchr`;
- Clang-only cross compilation into `libhyper.a` and `libhyper.so`; and
- a public-interface-only Native application fixture for product integration.

Native userspace supports AArch64 and RV64GC/LP64D. Additional architecture
veneers are added only when the corresponding Kernel entry is
functional.

## Process heap

The dynamic loader enters the shared runtime startup handoff after relocation and before
any application or DSO constructors. Static CRT performs this handoff itself;
dynamic CRT uses the already initialized runtime before calling `hyper_main`.
Initialization uses the process's bootstrap ROOT_VMAR capability. The heap's
low-end address hint is `0xe0000000`, after the shared-library range. Its initial reservation is one eighth of the
HAL application address limit, rounded down to pages (16 TiB on the default
AArch64 profile, 16 GiB on RISC-V Sv39). A conflicting hint may be relocated;
the allocator always uses the returned base. This is SDK layout policy, not a
fixed-address ABI guarantee or a total allocation limit. The runtime retains
an independent MAP-only root handle for later overflow reservations.

Reservation allocates no backing pages. Allocations map read/write, non-executable
VMOs in regions of at least 64 KiB. If the first reservation cannot fit a region,
an independent VMAR is requested with a hint immediately above the first one.
No recursive malloc is needed: metadata lives in the mapped region itself.
First-fit blocks split and coalesce. Fully free regions release their backing;
overflow regions also destroy their VMAR. The initial reservation remains for
reuse. Allocation is subject to address-space and process resource limits.
A private mutex serializes metadata, blocking rather than spinning on contention.
This is a general-purpose initial allocator, not a constant-time or real-time
allocation contract. It is not reentrant from asynchronous handlers.

`<stdlib.h>` exposes the C allocation subset. Failure returns NULL without
an errno facility; `calloc` detects multiplication overflow; `malloc(0)` returns
a freeable minimum allocation when memory is available; `free(NULL)` is a no-op;
`realloc(p, 0)` frees and returns NULL. Failed nonzero realloc preserves the old
allocation. `aligned_alloc` requires a power-of-two alignment and a size divisible
by it. `<hyper/heap.h>` exposes the arbitrary-size aligned allocation interface
used by Rust. Each pointer must be released through the same process heap.
Dynamic applications and their DSOs share the allocator in `libhyper.so`;
static applications use the same implementation from `libhyper.a`.

## Guarded, growable stacks

`<hyper/stack.h>` provides one stack descriptor for the initial thread and
SDK-created threads. `hyper_stack_current()` borrows the current descriptor;
`hyper_stack_get_info()` reports its usable range, fixed top and reserved
capacity. `hyper_stack_grow()` extends downwards without moving existing frames.
Call it before a deeper workload while sufficient stack headroom remains.
Growth is explicit, not triggered by a page fault, and never shrinks a stack.

Every reservation has an unmapped page at each end. Uncommitted capacity below
the usable range also remains unmapped. Main and SDK-created worker stacks
reserve at least 256 MiB; larger initial sizes or requested worker capacities
are honored. These are virtual reservations, not eagerly allocated RAM.

**The reservation capacity is the per-stack hard limit for in-place growth.**
For an ordinary SDK worker, a zero initial-size request selects 64 KiB of
usable stack, while the default capacity is 256 MiB, excluding both guard
pages. `hyper_stack_grow()` can increase the usable size up to that capacity,
but cannot enlarge the reservation, relocate the stack, or grow beyond it even
if adjacent addresses are free. Request a larger capacity at creation with
`hyper_runtime_thread_spawn_with_stack()` or `hyper_stack_create()` when needed;
256 MiB is the default minimum reservation, not a global maximum stack size.
The final main stack follows the same fixed-capacity rule. Query its actual
limit with `hyper_stack_get_info()` rather than assuming the default.

Stacks independently reserve root children, using a low-end hint that prefers
the top of the application range. The kernel selects the
nearest feasible range; there is no fixed SDK stack arena or slot stride.
The runtime retains a separate MAP-only root handle, so closing the application's
startup handle cannot break later thread creation. Address-space exhaustion is
reported and destroyed reservations are reusable. `hyper_stack_create()` and
`hyper_runtime_thread_spawn_with_stack()` accept an explicit capacity.

The main stack initially maps 256 KiB unless the application's `PT_GNU_STACK`
requests a different size. Requests are rounded up to pages and enlarged when
needed for the entry vector and handoff frames. This differs from the 64 KiB
default for ordinary SDK workers.

The kernel supplies a temporary stack and its VMAR through the bootstrap
channel. The caller-selected loader maps the application and encodes
INITIAL_STACK_VMAR and stack geometry in the SDK-private startup vector.
The stage-1 loader accepts only static applications; the formal dynamic loader
accepts static and dynamic applications and resolves dynamic dependencies itself.
There is no second interpreter mapping or loader handoff. Runtime initialization
creates the final main stack through the same allocator as worker stacks,
copies startup strings and records, and performs a non-returning assembly SP
switch. It releases the bootstrap stack mapping and VMAR from the new stack before
constructors and app entry. For a static application it also releases the selected
loader mapping; a dynamic application retains the formal loader for constructors
and runtime linking. It then acknowledges readiness to the parent. Readiness
precedes constructors and `main`; it is not an application-health acknowledgement.
Init has no waiting parent.
The consumed handle is omitted from the app startup view. The dynamic loader
uses the same shared-runtime handoff as static CRT;
it never restores the abandoned bootstrap SP. Normal
main return terminates the process, whose address space reclaims its reservation.
Worker reclamation waits for Native TERMINATED, including detached
workers, even when an entry exits directly instead of returning through the
runtime trampoline. One blocking WaitSet watches at most 1024 outstanding
termination subscriptions; exhausted registration capacity fails spawn before
start. Consuming termination releases a subscription even before join. A completed
joinable worker retains its token, Thread handle and stack until join or release;
a detached worker is reclaimed after termination is observed. Direct
Native exit still bypasses language/TLS destructors; callers must perform their
own cleanup. Runtime-owned stacks cannot be destroyed through the public stack API.
If destruction fails, ownership and the reserved address range remain until a
successful retry; an already unmapped payload cannot be queried or grown.
Failed internal cleanup is retained and retried on subsequent creation/release.

Rust applications can use `hyper_os::thread::current_stack()` and
`grow_current_stack()` on main or `std::thread` workers. Ordinary Rust lifetime,
TLS destruction and join semantics remain unchanged. Guards detect accesses to
unmapped pages; they do not make arbitrary jumps beyond a reservation safe.

For example, the same Rust code can run on the main thread or a worker:

```rust
let stack = hyper_os::thread::current_stack()?;
let next = stack.size.saturating_add(64 * 1024).min(stack.capacity);
hyper_os::thread::grow_current_stack(next)?;
```

`size` is the total desired usable extent, not an increment. Reaching `capacity`
requires choosing a larger reservation at creation; live frames cannot be moved
by growing the stack. No main-thread check is needed at the call site.


## Build

HypeR Lib consumes the generated C header from `sdk/abi/`. The include
directory is injected explicitly so the runtime does not carry a second ABI
definition.

```sh
cmake -S stdlib/hyper -B target/sdk-lib/aarch64 \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_ASM_COMPILER=clang \
  -DCMAKE_AR=llvm-ar \
  -DCMAKE_RANLIB=llvm-ranlib \
  -DCMAKE_SYSTEM_NAME=Generic \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER_TARGET=aarch64-none-elf \
  -DCMAKE_ASM_COMPILER_TARGET=aarch64-none-elf \
  -DHYPER_LD=ld.lld \
  -DHYPER_ABI_INCLUDE_DIR="$PWD/sdk/abi/include"
cmake --build target/sdk-lib/aarch64
```

`sdk/toolchain/` owns SDK assembly, compiler-driver defaults, and the final
executable link contract. This component owns library semantics only.

## Testing

Library tests under `tests/unit` are host-executed unit tests for implementation
semantics. They can be run independently of the target archive:

```sh
cmake -S stdlib/hyper/tests/unit -B target/sdk-lib/unit \
  -DCMAKE_C_COMPILER=clang \
  -DHYPER_ABI_INCLUDE_DIR="$PWD/sdk/abi/include"
cmake --build target/sdk-lib/unit
ctest --test-dir target/sdk-lib/unit --output-on-failure
```

`syscall-veneer` and `syscall-rust-transport` intercept the production C and
raw Rust SDK calls at `hyper_native_call6`. Their independent slot expectations
cover argument arities, reserved zeros, high-bit scalar transport, pointers,
implicit record sizes, and all three result fields on success and failure.
The Rust test needs the repository's host Rust compiler and Python, but no SDK
sysroot. It compiles the real crates and links the C veneers rather than copying
FFI declarations into a mock. These tests cover transport shapes; they do not
prove architecture assembly, kernel validation, or capability ownership policy.
Target integration tests retain those responsibilities.

`test-app` is not part of the Lib unit-test suite. The top-level SDK check
compiles it against the assembled Native SDK to validate the public headers,
runtime, and compiler-driver integration.

## License

Licensed under the Apache License, Version 2.0. See
[the project license](../../LICENSE).

## Rust standard library bridge

The SDK also installs `libhyper-std.a` and `crt-std.o` for ordinary Rust
applications. Shared startup, stream buffering, and key-based thread-local
storage live in `libhyper`; the std archive carries stateless adapters.
See the [std runtime contract](../../sdk/toolchain/rust-std/README.md) and the
[`hyper/std.h`](include/hyper/std.h) / [`hyper/thread.h`](include/hyper/thread.h)
interfaces. These are SDK interfaces, not additions to the kernel syscall ABI.

Interactive standard streams inherit typed capability-channel connectors
(`hyper/stream.h`), then open one private byte-channel request connection per
process and stream. Plain pipes retain their existing byte-channel byte
semantics and buffering. `std::process::Command` inherits connectors, never a
parent's response channel; redirected streams remain pipes. Inheritance preserves
its terminal lease and does not grant a new foreground lease. A caller handing
input to a child must stop its own reads and collect any pending cancellation,
or explicitly delegate foreground ownership through the terminal API. Distinct
reply channels prevent response stealing, but do not decide which process
should consume keyboard input.

`READ` requests specify the exact maximum byte count. A successful reply commits
only that prefix, so a one-byte shell read cannot prefetch the following pasted
command into a disposable child. The Rust `Stdin` adapter bypasses its 8 KiB
prefetch buffer for these request streams while preserving ordinary pipe
buffering. Explicit `BufRead` is still a buffering opt-in: callers own and must
account for any bytes they prefetch before handing input to another process.
`hyper_os::stream::ProcessInput` exclusively borrows the same runtime connection
for asynchronous reads and Native waits. Ordinary std reads return `BUSY` during
that borrow. Cancellation resolves the original read exactly once: either its
already committed data or a cancelled result. Dropping an unfinished borrow
closes its private connection; a later std read reconnects without stale replies.

`WRITE` replies report an accepted prefix, and an ordered `FENCE` follows earlier
writes on that same connection. `std::io::Write::flush` sends this fence after
flushing its local buffer. The physical console fence means bytes reached the
kernel's output queue; it does not claim the UART has transmitted them. Per-stream
metadata drives `IsTerminal`; pipes and raw bootstrap streams report false.

The runtime performs no character conversion, line editing, echo, flow control
or Ctrl-D interpretation. `console-raw` supplies byte-transparent bootstrap
streams and independently bounded service-log ingress. The formal terminal
service supplies terminal policy and scoped client controls from `lib/terminal`.
Consequently bootstrap and fallback input do not depend on the formal terminal.

### Runtime system configuration

`hyper_system_config(key)` queries public scalar kernel properties without an
inspector capability. `HYPER_NATIVE_SYSTEM_CONFIG_PAGE_SIZE` returns the Native
mapping granule, and `HYPER_NATIVE_SYSTEM_CONFIG_APPLICATION_ADDRESS_LIMIT`
returns the exclusive application address limit; unknown keys return
`NOT_SUPPORTED`. `<hyper/system.h>` exposes
`hyper_page_size(&size)`, an allocation-free, thread-safe cached query used by
the heap and unified stack manager. Failed or malformed replies are not cached.
Rust callers can use `hyper_os::system::{config, page_size}`.

An explicit nonzero SDK thread-stack request rounds up to runtime pages, with a
one-page minimum. Zero selects the 64 KiB SDK default; the internal reaper also
requests 64 KiB. Guard pages are additional to usable size. A one-page request
is permitted, not a guarantee that arbitrary C/Rust code fits. Main and worker
stack creation/growth use the same runtime page geometry. The kernel still
independently checks VMAR alignment, range overflow, ownership and permissions;
raw `thread_create` takes an aligned SP, not a stack descriptor, so callers must
supply their own stack reservation and guards.

`hyper_thread_set_current_name(name, length)` changes the caller's diagnostic
label without an additional capability. Names are 1–64 UTF-8 bytes, exclude
the NUL terminator and cannot contain control characters; rejected input leaves
the previous label intact. It does not rename the Process or another Thread.
Rust callers use `hyper_os::thread::set_current_name`; `std::thread::Builder::name`
also publishes the worker's name before its closure runs.

This does not change the current kernel/ELF 4 KiB configuration or fixed device
protocol layouts; making those configurable is a separate change.

### Internal synchronization

Heap, stack topology, standard input, cwd and thread-cleanup bookkeeping share
one private, zero-initialized mutex implementation. Uncontended lock/unlock use
only acquire/release atomics. Contention uses Native atomic wait, with a possible
waiter marker retained across handoff and wake-one on contended unlock. There
is no allocation or TLS initialization in the userspace lock path and no yield
polling. Locks are nonrecursive and provide no fairness or owner-death recovery.
Their storage must outlive all lock, wait and wake calls. Unexpected Native wait
errors retain the runtime's existing process-termination policy; the kernel's
current waiter allocation policy is unchanged.
