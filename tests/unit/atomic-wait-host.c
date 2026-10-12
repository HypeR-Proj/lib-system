/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
/* Host substitution for the Native syscall seam. Production never uses this
 * backend; Native integration separately exercises the kernel wait queues. */
#include <hyper/syscall.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
static pthread_mutex_t wait_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

hyper_native_status_t hyper_atomic_wait(const uint32_t *address, uint32_t expected,
					uint64_t deadline)
{
	pthread_mutex_lock(&wait_lock);
	int status = 0;
	if (__atomic_load_n(address, __ATOMIC_RELAXED) == expected) {
		uint64_t now = hyper_clock_get_monotonic().value0;
		if (deadline != UINT64_MAX && now >= deadline)
			status = ETIMEDOUT;
		else if (deadline == UINT64_MAX)
			status = pthread_cond_wait(&condition, &wait_lock);
		else {
			struct timespec absolute;
			clock_gettime(CLOCK_REALTIME, &absolute);
			uint64_t delta = deadline - now;
			uint64_t nanos = (uint64_t)absolute.tv_nsec + delta % 1000000000;
			absolute.tv_sec += (time_t)(delta / 1000000000 + nanos / 1000000000);
			absolute.tv_nsec = (long)(nanos % 1000000000);
			status = pthread_cond_timedwait(&condition, &wait_lock, &absolute);
		}
	}
	pthread_mutex_unlock(&wait_lock);
	return status == ETIMEDOUT ? HYPER_NATIVE_STATUS_TIMED_OUT : HYPER_NATIVE_STATUS_OK;
}

hyper_call_result_t hyper_atomic_wake(const uint32_t *address, uint32_t count)
{
	(void)address;
	pthread_mutex_lock(&wait_lock);
	/* Spurious wakeups are permitted; do not strand another address's waiter
	 * by signalling only one member of this deliberately shared host queue. */
	if (count)
		pthread_cond_broadcast(&condition);
	pthread_mutex_unlock(&wait_lock);
	return (hyper_call_result_t){0, 0, 0};
}
