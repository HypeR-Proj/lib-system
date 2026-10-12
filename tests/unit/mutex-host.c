/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
/* Test seam only: emulate address-keyed Native waits without polling. */
#include <hyper/thread.h>
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;

static struct waiter {
	const uint32_t *address;
	pthread_cond_t condition;
	int notified;
	struct waiter *next;
} *waiters;

atomic_uint test_mutex_wait_calls, test_mutex_wake_calls, test_mutex_sleepers;

int hyper_runtime_wait_u32(const uint32_t *address, uint32_t expected, uint64_t deadline)
{
	assert(deadline == UINT64_MAX);
	atomic_fetch_add(&test_mutex_wait_calls, 1);
	pthread_mutex_lock(&gate);
	if (__atomic_load_n(address, __ATOMIC_RELAXED) == expected) {
		struct waiter waiter = {address, PTHREAD_COND_INITIALIZER, 0, waiters};
		waiters = &waiter;
		atomic_fetch_add(&test_mutex_sleepers, 1);
		while (!waiter.notified)
			pthread_cond_wait(&waiter.condition, &gate);
		struct waiter **link = &waiters;
		while (*link != &waiter)
			link = &(*link)->next;
		*link = waiter.next;
		atomic_fetch_sub(&test_mutex_sleepers, 1);
		pthread_cond_destroy(&waiter.condition);
	}
	pthread_mutex_unlock(&gate);
	return 1;
}

uint32_t hyper_runtime_wake_u32(const uint32_t *address, uint32_t count)
{
	atomic_fetch_add(&test_mutex_wake_calls, 1);
	pthread_mutex_lock(&gate);
	uint32_t woke = 0;
	for (struct waiter *p = waiters; p && woke < count; p = p->next) {
		if (p->address == address && !p->notified) {
			p->notified = 1;
			++woke;
			pthread_cond_signal(&p->condition);
		}
	}
	pthread_mutex_unlock(&gate);
	return woke;
}
