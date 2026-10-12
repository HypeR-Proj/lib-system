/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_THREAD_H
#define HYPER_THREAD_H
#include <hyper/native.h>
#include <stddef.h>
#include <stdint.h>

/* SDK-private thread runtime, not a syscall ABI. Each SDK-created thread starts
 * with a zero thread pointer (TPIDR_EL0 on AArch64, tp on RV64) and must
 * attach before entering language runtimes.
 * Detach runs key destructors on that thread before releasing its storage.
 * The runtime trampoline performs attach -> entry -> detach -> thread_exit.
 * Rust uses key TLS; ELF PT_TLS / compiler-native TLS is not supported yet. */
hyper_native_status_t hyper_runtime_thread_attach(void);
void hyper_runtime_thread_detach(void);
typedef void (*hyper_tls_destructor_t)(void *);
uintptr_t hyper_runtime_tls_create(hyper_tls_destructor_t destructor);
void hyper_runtime_tls_destroy(uintptr_t key);
void *hyper_runtime_tls_get(uintptr_t key);
void hyper_runtime_tls_set(uintptr_t key, void *value);

/* Atomic u32 address, aligned and live for the call. Spurious wakes allowed.
 * Absolute monotonic deadline; UINT64_MAX means infinite.
 * Wait returns 0 only on timeout, or 1 on a value mismatch or wake (including
 * a spurious wake). A return of 1 does not guarantee a changed value: callers
 * must recheck their predicate. A value mismatch returns 1 without parking.
 * Wake returns the actual number notified, bounded by count.
 * Wake does not publish memory: the caller must release-store before waking. */
int hyper_runtime_wait_u32(const uint32_t *address, uint32_t expected, uint64_t deadline);
uint32_t hyper_runtime_wake_u32(const uint32_t *address, uint32_t count);
typedef void (*hyper_runtime_thread_entry_t)(void *);
/* Usable stack size excludes one no-access page at each end; rounded up to
 * runtime Native pages with a one-page minimum. Zero selects 64 KiB. Join/detached cleanup reclaims the stack
 * only after the kernel publishes termination. See hyper/stack.h for the same
 * grow/query interface used by the main thread. Raw thread_create callers
 * remain responsible for their own stack allocation and protection. At most
 * 1024 outstanding termination subscriptions are admitted; exhaustion fails
 * spawn before start. Entry should return to run TLS cleanup; direct Native
 * exit skips destructors but its terminated stack is still reclaimed. */
hyper_native_status_t hyper_runtime_thread_spawn(size_t stack_size,
						 hyper_runtime_thread_entry_t entry, void *argument,
						 uintptr_t *token);
/* Explicit reservation capacity permits in-place growth by the worker.
 * Reserves at least 256 MiB, excluding guards, without mapping that entire
 * capacity. The resulting capacity is fixed and limits later stack growth;
 * request more here if the worker needs a larger limit. */
hyper_native_status_t hyper_runtime_thread_spawn_with_stack(size_t stack_size, size_t capacity,
							    hyper_runtime_thread_entry_t entry,
							    void *argument, uintptr_t *token);
hyper_native_status_t hyper_runtime_thread_join(uintptr_t token);
void hyper_runtime_thread_release(uintptr_t token);
#endif
