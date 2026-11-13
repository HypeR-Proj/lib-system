/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <hyper/shared-mutex.h>
#include <hyper/syscall.h>
#include <stdlib.h>
extern _Noreturn void abort(void);

static uint64_t owner_state, now;
static unsigned queries, wakes, waits, clocks, cancellations, control_loads;
static uint32_t last_wake;
static int wait_action;
static bool cancelled;
static uint64_t domain_rights;
static _Atomic uint64_t word;
#define W HYPER_NATIVE_SYNC_MUTEX_WAITERS
#define R HYPER_NATIVE_SYNC_MUTEX_RECOVERING
#define P HYPER_NATIVE_SYNC_MUTEX_NOT_RECOVERABLE

hyper_call_result_t hyper_object_get_basic_info(uint64_t handle,
						hyper_native_object_basic_info_t *info, size_t size)
{
	assert(handle == 11 && size == sizeof(*info));
	info->object_kind = HYPER_NATIVE_OBJECT_SYNC_DOMAIN;
	info->koid = 47;
	return (hyper_call_result_t){.value0 = sizeof(*info)};
}
hyper_call_result_t hyper_handle_get_info(uint64_t handle, hyper_native_handle_info_t *info,
					  size_t size)
{
	assert((handle == 11 || handle == 12) && size == sizeof(*info));
	info->rights = handle == 11 ? domain_rights : HYPER_NATIVE_RIGHTS_MASK;
	return (hyper_call_result_t){.value0 = sizeof(*info)};
}
hyper_call_result_t hyper_sync_domain_join(uint64_t domain, uint64_t options)
{
	assert(domain == 11 && options == 0);
	return (hyper_call_result_t){.value0 = 2};
}
hyper_call_result_t hyper_sync_domain_wake_u64(uint64_t domain, uint64_t vmo, uint64_t offset,
					       uint32_t count, uint64_t options)
{
	assert(domain == 11 && vmo == 12 && offset == 8 && options == 0);
	++wakes;
	last_wake = count;
	return (hyper_call_result_t){0};
}
hyper_call_result_t hyper_sync_domain_owner_query(uint64_t domain, uint64_t owner, uint64_t options)
{
	assert(domain == 11 && owner && options == 0);
	++queries;
	if (wait_action == 4) {
		/* The observed dead owner was replaced before recovery's exact CAS. */
		atomic_store(&word, 3 | W);
	}
	return (hyper_call_result_t){.value0 = owner_state};
}
hyper_call_result_t hyper_sync_domain_wait_u64(uint64_t domain,
					       const hyper_native_sync_domain_wait_t *request,
					       size_t size)
{
	assert(domain == 11 && size == sizeof(*request));
	uint64_t expected_owner = wait_action == 5 ? 3 : 1;
	assert(request->vmo == 12 && request->offset == 8 && request->owner == expected_owner);
	assert(request->expected == (expected_owner | W));
	assert(atomic_load(&word) == request->expected);
	++waits;
	if (wait_action == 1 || wait_action == 5) {
		atomic_store(&word, 0);
		return (hyper_call_result_t){0};
	}
	if (wait_action == 2) {
		owner_state = HYPER_NATIVE_SYNC_OWNER_DEAD;
		return (hyper_call_result_t){.value0 = HYPER_NATIVE_SYNC_WAIT_OWNER_EXITED,
					     .value1 = 1};
	}
	return (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_CANCELLED};
}
hyper_call_result_t hyper_object_wait_one(uint64_t handle, uint64_t signals, uint64_t deadline)
{
	assert(handle == 13 && signals == HYPER_NATIVE_SIGNAL_EVENT_SIGNALED && deadline == 0);
	++cancellations;
	return (hyper_call_result_t){.status = cancelled ? 0 : HYPER_NATIVE_STATUS_TIMED_OUT,
				     .value0 = cancelled ? HYPER_NATIVE_SIGNAL_EVENT_SIGNALED : 0};
}
hyper_call_result_t hyper_clock_get_monotonic(void)
{
	++clocks;
	return (hyper_call_result_t){.value0 = now};
}
_Noreturn void hyper_process_exit(int64_t status)
{
	(void)status;
	abort();
}
static void reset(uint64_t value)
{
	atomic_store(&word, value);
	owner_state = HYPER_NATIVE_SYNC_OWNER_LIVE;
	queries = wakes = waits = clocks = cancellations = control_loads = 0;
	last_wake = 0;
	now = 10;
	wait_action = 0;
	cancelled = false;
}
int main(void)
{
	hyper_sync_participant_t participant;
	hyper_shared_mutex_t mutex;
	hyper_shared_mutex_guard_t guard = {0};
	domain_rights =
	    HYPER_NATIVE_RIGHT_INSPECT | HYPER_NATIVE_RIGHT_WAIT | HYPER_NATIVE_RIGHT_SIGNAL;
	assert(hyper_shared_mutex_view(&mutex, &word, 11, 12, 8) ==
	       HYPER_NATIVE_STATUS_ACCESS_DENIED);
	domain_rights = HYPER_NATIVE_RIGHT_INSPECT | HYPER_NATIVE_RIGHT_SYNC_WAIT |
			HYPER_NATIVE_RIGHT_SYNC_WAKE;
	assert(!hyper_sync_participant_join(11, &participant));
	assert(!hyper_shared_mutex_view(&mutex, &word, 11, 12, 8));
	reset(0);
	for (unsigned i = 0; i != 1000; ++i) {
		assert(hyper_shared_mutex_try_lock(&mutex, &participant, &guard).outcome ==
		       HYPER_SHARED_MUTEX_ACQUIRED);
		hyper_shared_mutex_unlock(&guard);
	}
	assert(!queries && !wakes && !waits && !clocks);
	/* Initial attempt wins even an expired deadline or pre-signaled cancel. */
	cancelled = true;
	assert(hyper_shared_mutex_lock(&mutex, &participant, 0, 13, &guard).outcome ==
	       HYPER_SHARED_MUTEX_ACQUIRED);
	assert(!clocks && !cancellations);
	assert(hyper_shared_mutex_try_lock(&mutex, &participant, &(hyper_shared_mutex_guard_t){0})
		   .status == HYPER_NATIVE_STATUS_BAD_STATE);
	hyper_shared_mutex_unlock(&guard);

	reset(1);
	assert(hyper_shared_mutex_try_lock(&mutex, &participant, &guard).status ==
	       HYPER_NATIVE_STATUS_WOULD_BLOCK);
	assert(queries == 1 && !waits);
	owner_state = HYPER_NATIVE_SYNC_OWNER_INVALID;
	assert(hyper_shared_mutex_try_lock(&mutex, &participant, &guard).status ==
	       HYPER_NATIVE_STATUS_BAD_STATE);
	owner_state = HYPER_NATIVE_SYNC_OWNER_DEAD;
	assert(hyper_shared_mutex_try_lock(&mutex, &participant, &guard).outcome ==
	       HYPER_SHARED_MUTEX_OWNER_DIED);
	assert(atomic_load(&word) == (2 | R));
	assert(!hyper_shared_mutex_mark_consistent(&guard));
	assert(hyper_shared_mutex_mark_consistent(&guard) == HYPER_NATIVE_STATUS_BAD_STATE);
	hyper_shared_mutex_unlock(&guard);
	assert(!atomic_load(&word));

	reset(1 | R | W);
	owner_state = HYPER_NATIVE_SYNC_OWNER_DEAD;
	assert(hyper_shared_mutex_try_lock(&mutex, &participant, &guard).outcome ==
	       HYPER_SHARED_MUTEX_OWNER_DIED);
	hyper_shared_mutex_unlock(&guard);
	assert(atomic_load(&word) == P && last_wake == UINT32_MAX);
	assert(hyper_shared_mutex_try_lock(&mutex, &participant, &guard).outcome ==
	       HYPER_SHARED_MUTEX_NOT_RECOVERABLE);

	reset(1);
	wait_action = 1;
	assert(hyper_shared_mutex_lock(&mutex, &participant, UINT64_MAX, 13, &guard).outcome ==
	       HYPER_SHARED_MUTEX_ACQUIRED);
	assert(waits == 1 && (atomic_load(&word) & W));
	hyper_shared_mutex_unlock(&guard);
	assert(wakes == 1 && last_wake == 1);

	reset(1);
	wait_action = 2;
	assert(hyper_shared_mutex_lock(&mutex, &participant, UINT64_MAX, 0, &guard).outcome ==
	       HYPER_SHARED_MUTEX_OWNER_DIED);
	assert(waits == 1);
	assert(!hyper_shared_mutex_mark_consistent(&guard));
	hyper_shared_mutex_unlock(&guard);
	assert(last_wake == 1);

	reset(1);
	assert(hyper_shared_mutex_lock(&mutex, &participant, UINT64_MAX, 13, &guard).status ==
	       HYPER_NATIVE_STATUS_CANCELLED);
	assert(waits == 1 && wakes == 1 && last_wake == 1);
	reset(1);
	assert(hyper_shared_mutex_lock(&mutex, &participant, 5, 0, &guard).status ==
	       HYPER_NATIVE_STATUS_TIMED_OUT);
	assert(!waits && wakes == 1);
	reset(1);
	cancelled = true;
	assert(hyper_shared_mutex_lock(&mutex, &participant, UINT64_MAX, 13, &guard).status ==
	       HYPER_NATIVE_STATUS_CANCELLED);
	assert(!waits && wakes == 1);

	reset(1);
	owner_state = HYPER_NATIVE_SYNC_OWNER_DEAD;
	wait_action = 4;
	assert(hyper_shared_mutex_try_lock(&mutex, &participant, &guard).status ==
	       HYPER_NATIVE_STATUS_WOULD_BLOCK);
	assert(atomic_load(&word) == (3 | W));

	/* B releases after this waiter decided to block; D then acquires without
	 * WAITERS before the slow attempt observes D. The eventual wait must arm
	 * D's exact word, not rely on a flag previously set on B. */
	reset(1);
	wait_action = 5;
	assert(hyper_shared_mutex_lock(&mutex, &participant, UINT64_MAX, 0, &guard).outcome ==
	       HYPER_SHARED_MUTEX_ACQUIRED);
	assert(waits == 1 && (atomic_load(&word) & W));
	hyper_shared_mutex_unlock(&guard);
	assert(wakes == 1 && last_wake == 1);
	reset(UINT64_C(1) << 63);
	assert(hyper_shared_mutex_try_lock(&mutex, &participant, &guard).status ==
	       HYPER_NATIVE_STATUS_BAD_STATE);
	reset(R);
	assert(hyper_shared_mutex_try_lock(&mutex, &participant, &guard).status ==
	       HYPER_NATIVE_STATUS_BAD_STATE);
	reset(P | 1);
	assert(hyper_shared_mutex_try_lock(&mutex, &participant, &guard).status ==
	       HYPER_NATIVE_STATUS_BAD_STATE);
	return 0;
}

/* Compile the production algorithm with one instrumented load seam. This
 * deterministically inserts the external B-release/D-acquire interleaving at
 * the second attempt's observation, without a production test hook or a model
 * that duplicates the mutex algorithm. The wrapper itself uses the original
 * C atomic macro before the override below is defined. */
static uint64_t controlled_load(const volatile hyper_shared_mutex_word_t *address,
				memory_order order)
{
	++control_loads;
	if (wait_action == 5 && control_loads == 2) {
		atomic_store_explicit(&word, 0, memory_order_release);
		uint64_t unlocked = 0;
		assert(atomic_compare_exchange_strong_explicit(
		    &word, &unlocked, 3, memory_order_acquire, memory_order_relaxed));
	}
	return atomic_load_explicit(address, order);
}
#undef atomic_load_explicit
#define atomic_load_explicit(address, order) controlled_load((address), (order))
#include "../../src/shared-mutex.c"
#undef atomic_load_explicit
