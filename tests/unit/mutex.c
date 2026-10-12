/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include "../../src/mutex-internal.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>

extern atomic_uint test_mutex_wait_calls, test_mutex_wake_calls, test_mutex_sleepers;
static hyper_mutex_t mutex;
static unsigned counter;

static void *worker(void *unused)
{
	(void)unused;
	for (unsigned i = 0; i < 10000; ++i) {
		hyper_mutex_lock(&mutex);
		++counter;
		hyper_mutex_unlock(&mutex);
	}
	return NULL;
}

int main(void)
{
	hyper_mutex_lock(&mutex);
	hyper_mutex_unlock(&mutex);
	assert(!atomic_load(&test_mutex_wait_calls));
	assert(!atomic_load(&test_mutex_wake_calls));

	hyper_mutex_lock(&mutex);
	pthread_t threads[4];
	for (unsigned i = 0; i < 4; ++i)
		assert(!pthread_create(&threads[i], NULL, worker, NULL));
	while (atomic_load(&test_mutex_sleepers) != 4)
		sched_yield();
	/* A wake does not grant ownership. All four callers must check again and
	 * sleep while we retain the mutex, even after a spurious notification. */
	unsigned before = atomic_load(&test_mutex_wait_calls);
	assert(hyper_runtime_wake_u32((const uint32_t *)&mutex.state, 1) == 1);
	while (atomic_load(&test_mutex_wait_calls) == before ||
	       atomic_load(&test_mutex_sleepers) != 4)
		sched_yield();
	hyper_mutex_unlock(&mutex);
	/* Wakes are deliberately wake-one: losing the slow-path contended mark
	 * would strand remaining sleepers and trigger the test timeout. */
	for (unsigned i = 0; i < 4; ++i)
		assert(!pthread_join(threads[i], NULL));
	assert(counter == 40000);
	assert(!atomic_load(&test_mutex_sleepers));
	return 0;
}
