/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_MUTEX_INTERNAL_H
#define HYPER_MUTEX_INTERNAL_H
#include <hyper/thread.h>
#include <stdatomic.h>

/* Process-private, nonrecursive mutex. Zero initialization is sufficient.
 * No heap/TLS dependency. The word must remain mapped and alive until all
 * users, including the unlock wake syscall, have completed.
 * 0 = unlocked, 1 = locked, 2 = locked with possible waiters.
 * No fairness/owner-death guarantee. Native wait errors follow the runtime's
 * fail-stop policy; cancellation/spurious wakes retry the condition. */
typedef struct hyper_mutex {
	_Atomic(uint32_t) state;
} hyper_mutex_t;

_Static_assert(sizeof(_Atomic(uint32_t)) == sizeof(uint32_t), "Native wait word size");
_Static_assert(_Alignof(hyper_mutex_t) >= 4, "Native wait word alignment");

static inline void hyper_mutex_lock(hyper_mutex_t *mutex)
{
	uint32_t expected = 0;
	if (atomic_compare_exchange_strong_explicit(&mutex->state, &expected, 1,
						    memory_order_acquire, memory_order_relaxed))
		return;
	/* Keep the contended marker when acquiring on the slow path: another
	 * sleeper may still need the next owner's unlock to wake it. */
	while (atomic_exchange_explicit(&mutex->state, 2, memory_order_acquire) != 0)
		hyper_runtime_wait_u32((const uint32_t *)&mutex->state, 2, UINT64_MAX);
}

static inline void hyper_mutex_unlock(hyper_mutex_t *mutex)
{
	if (atomic_exchange_explicit(&mutex->state, 0, memory_order_release) == 2)
		hyper_runtime_wake_u32((const uint32_t *)&mutex->state, 1);
}
#endif
