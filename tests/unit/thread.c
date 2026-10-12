/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/thread.h>
#include <hyper/syscall.h>
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <time.h>

static uintptr_t key;
static atomic_uint destructors;
static atomic_uint round;
static uint32_t state;
static atomic_uint entered;

hyper_native_status_t hyper_thread_yield(void)
{
	sched_yield();
	return 0;
}

hyper_call_result_t hyper_clock_get_monotonic(void)
{
	struct timespec now;
	assert(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
	return (hyper_call_result_t){0, (uint64_t)now.tv_sec * 1000000000 + now.tv_nsec, 0};
}

_Noreturn void hyper_process_exit(int64_t code)
{
	(void)code;
	__builtin_trap();
}

static void destroy(void *value)
{
	assert(hyper_runtime_tls_get(key) == NULL);
	unsigned *count = value;
	if (++*count == 1) {
		hyper_runtime_tls_set(key, value);
	} else {
		free(count);
		atomic_fetch_add(&destructors, 1);
	}
}

static void *worker(void *argument)
{
	(void)argument;
	assert(hyper_runtime_thread_attach() == 0);
	assert(hyper_runtime_thread_attach() == 0);
	assert(hyper_runtime_tls_get(key) == NULL);
	unsigned *count = calloc(1, sizeof(*count));
	assert(count);
	hyper_runtime_tls_set(key, count);
	atomic_fetch_add(&entered, 1);
	assert(hyper_runtime_wait_u32(&state, 0, UINT64_MAX) == 1);
	assert(hyper_runtime_tls_get(key) == count);
	for (unsigned i = 0; i < 1000; ++i) {
		while (atomic_exchange_explicit(&round, 1, memory_order_acquire))
			sched_yield();
		assert(hyper_runtime_tls_get(key) == count);
		atomic_store_explicit(&round, 0, memory_order_release);
	}
	hyper_runtime_thread_detach();
	hyper_runtime_thread_detach();
	return NULL;
}

int main(void)
{
	key = hyper_runtime_tls_create(destroy);
	pthread_t threads[4];
	for (unsigned i = 0; i < 4; ++i)
		assert(pthread_create(&threads[i], NULL, worker, NULL) == 0);
	while (atomic_load(&entered) != 4)
		sched_yield();
	__atomic_store_n(&state, 1, __ATOMIC_RELEASE);
	hyper_runtime_wake_u32(&state, UINT32_MAX);
	for (unsigned i = 0; i < 4; ++i)
		assert(pthread_join(threads[i], NULL) == 0);
	assert(atomic_load(&destructors) == 4);
	uint64_t start = hyper_clock_get_monotonic().value0;
	assert(hyper_runtime_wait_u32(&state, 1, start + 1000000) == 0);
	assert(hyper_clock_get_monotonic().value0 >= start + 1000000);
	assert(hyper_runtime_wait_u32(&state, 0, 0) == 1);
	hyper_runtime_tls_destroy(key);
	assert(hyper_runtime_thread_attach() == 0);
	hyper_runtime_tls_set(key, (void *)1);
	hyper_runtime_thread_detach(); /* Deleted key's destructor must not run. */
	return 0;
}
