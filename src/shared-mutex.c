/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/shared-mutex.h>
#include <hyper/syscall.h>
#include <stddef.h>

#define OWNER HYPER_NATIVE_SYNC_OWNER_TOKEN_MAX
#define WAITERS HYPER_NATIVE_SYNC_MUTEX_WAITERS
#define RECOVERING HYPER_NATIVE_SYNC_MUTEX_RECOVERING
#define POISON HYPER_NATIVE_SYNC_MUTEX_NOT_RECOVERABLE

static hyper_shared_mutex_result_t failure(hyper_native_status_t status)
{
	return (hyper_shared_mutex_result_t){HYPER_SHARED_MUTEX_ERROR, status};
}

static void notify(const hyper_shared_mutex_t *mutex, uint32_t count)
{
	hyper_call_result_t result =
	    hyper_sync_domain_wake_u64(mutex->domain, mutex->vmo, mutex->offset, count, 0);
	if (result.status || result.value0 > count || result.value1)
		hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
}

static hyper_native_status_t domain_identity(hyper_native_handle_t domain, uint64_t *koid)
{
	hyper_native_object_basic_info_t info = {0};
	hyper_call_result_t result = hyper_object_get_basic_info(domain, &info, sizeof(info));
	if (result.status)
		return result.status;
	if (!info.koid || info.object_kind != HYPER_NATIVE_OBJECT_SYNC_DOMAIN || info.reserved)
		return HYPER_NATIVE_STATUS_BAD_HANDLE;
	*koid = info.koid;
	return 0;
}

hyper_native_status_t hyper_sync_participant_join(hyper_native_handle_t domain,
						  hyper_sync_participant_t *participant)
{
	if (!participant)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	uint64_t koid;
	hyper_native_status_t status = domain_identity(domain, &koid);
	if (status)
		return status;
	hyper_call_result_t result = hyper_sync_domain_join(domain, 0);
	if (result.status)
		return result.status;
	if (!result.value0 || result.value0 > OWNER || result.value1)
		return HYPER_NATIVE_STATUS_INTERNAL;
	*participant = (hyper_sync_participant_t){domain, koid, result.value0};
	return 0;
}

hyper_native_status_t hyper_shared_mutex_view(hyper_shared_mutex_t *mutex,
					      hyper_shared_mutex_word_t *word,
					      hyper_native_handle_t domain,
					      hyper_native_handle_t vmo, uint64_t offset)
{
	if (!mutex || !word || ((uintptr_t)word & 7) || (offset & 7))
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	hyper_native_handle_info_t info = {0};
	hyper_call_result_t result = hyper_handle_get_info(domain, &info, sizeof(info));
	if (result.status)
		return result.status;
	const uint64_t required = HYPER_NATIVE_RIGHT_SYNC_WAIT | HYPER_NATIVE_RIGHT_SYNC_WAKE |
				  HYPER_NATIVE_RIGHT_INSPECT;
	if ((info.rights & required) != required)
		return HYPER_NATIVE_STATUS_ACCESS_DENIED;
	result = hyper_handle_get_info(vmo, &info, sizeof(info));
	if (result.status)
		return result.status;
	if ((info.rights & (HYPER_NATIVE_RIGHT_READ | HYPER_NATIVE_RIGHT_WRITE)) !=
	    (HYPER_NATIVE_RIGHT_READ | HYPER_NATIVE_RIGHT_WRITE))
		return HYPER_NATIVE_STATUS_ACCESS_DENIED;
	result = hyper_sync_domain_wake_u64(domain, vmo, offset, 0, 0);
	if (result.status)
		return result.status;
	if (result.value0 || result.value1)
		return HYPER_NATIVE_STATUS_INTERNAL;
	uint64_t koid;
	hyper_native_status_t status = domain_identity(domain, &koid);
	if (status)
		return status;
	*mutex = (hyper_shared_mutex_t){word, domain, koid, vmo, offset};
	return 0;
}

static hyper_native_status_t validate(const hyper_shared_mutex_t *mutex,
				      const hyper_sync_participant_t *participant,
				      const hyper_shared_mutex_guard_t *guard)
{
	if (!mutex || !participant || !guard || !mutex->word ||
	    participant->domain_koid != mutex->domain_koid || !participant->token ||
	    participant->token > OWNER)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	return 0;
}

/* At most one ordinary acquisition CAS, one owner observation and one recovery
 * CAS. A failed CAS never grants ownership and never permits stale recovery. */
static hyper_shared_mutex_result_t attempt(const hyper_shared_mutex_t *mutex,
					   const hyper_sync_participant_t *participant, bool slow,
					   hyper_shared_mutex_guard_t *guard, uint64_t *observed)
{
	uint64_t word = atomic_load_explicit(mutex->word, memory_order_relaxed);
	*observed = word;
	uint64_t owner = word & OWNER;
	if ((word & ~(OWNER | WAITERS | RECOVERING | POISON)) ||
	    ((word & POISON) && (owner || (word & RECOVERING))) || (!owner && (word & RECOVERING)))
		return failure(HYPER_NATIVE_STATUS_BAD_STATE);
	if (word & POISON)
		return (hyper_shared_mutex_result_t){HYPER_SHARED_MUTEX_NOT_RECOVERABLE, 0};
	if (owner == participant->token)
		return failure(HYPER_NATIVE_STATUS_BAD_STATE);
	bool recovering = false;
	if (owner) {
		hyper_call_result_t query = hyper_sync_domain_owner_query(mutex->domain, owner, 0);
		if (query.status)
			return failure(query.status);
		if (query.value1)
			return failure(HYPER_NATIVE_STATUS_INTERNAL);
		if (query.value0 == HYPER_NATIVE_SYNC_OWNER_LIVE)
			return failure(HYPER_NATIVE_STATUS_WOULD_BLOCK);
		if (query.value0 != HYPER_NATIVE_SYNC_OWNER_DEAD)
			return failure(HYPER_NATIVE_STATUS_BAD_STATE);
		/* The query Acquire observes terminal publication of completed writes;
		 * the following CAS protects takeover from another recovery candidate. */
		recovering = true;
	}
	uint64_t desired = participant->token | (word & WAITERS) | (slow ? WAITERS : 0) |
			   (recovering ? RECOVERING : 0);
	if (!atomic_compare_exchange_strong_explicit(mutex->word, &word, desired,
						     memory_order_acquire, memory_order_relaxed))
		return failure(HYPER_NATIVE_STATUS_WOULD_BLOCK);
	*guard = (hyper_shared_mutex_guard_t){mutex, participant->token, recovering, true};
	return (hyper_shared_mutex_result_t){
	    recovering ? HYPER_SHARED_MUTEX_OWNER_DIED : HYPER_SHARED_MUTEX_ACQUIRED, 0};
}

hyper_shared_mutex_result_t hyper_shared_mutex_try_lock(const hyper_shared_mutex_t *mutex,
							const hyper_sync_participant_t *participant,
							hyper_shared_mutex_guard_t *guard)
{
	hyper_native_status_t status = validate(mutex, participant, guard);
	if (status)
		return failure(status);
	uint64_t observed;
	return attempt(mutex, participant, false, guard, &observed);
}

static hyper_native_status_t budget(uint64_t deadline, hyper_native_handle_t cancellation)
{
	if (cancellation) {
		hyper_call_result_t result =
		    hyper_object_wait_one(cancellation, HYPER_NATIVE_SIGNAL_EVENT_SIGNALED, 0);
		if (!result.status)
			return (result.value0 == HYPER_NATIVE_SIGNAL_EVENT_SIGNALED &&
				!result.value1)
				   ? HYPER_NATIVE_STATUS_CANCELLED
				   : HYPER_NATIVE_STATUS_INTERNAL;
		if (result.status != HYPER_NATIVE_STATUS_TIMED_OUT)
			return result.status;
	}
	if (deadline != UINT64_MAX) {
		hyper_call_result_t now = hyper_clock_get_monotonic();
		if (now.status)
			return now.status;
		if (now.value0 >= deadline)
			return HYPER_NATIVE_STATUS_TIMED_OUT;
	}
	return 0;
}

hyper_shared_mutex_result_t hyper_shared_mutex_lock(const hyper_shared_mutex_t *mutex,
						    const hyper_sync_participant_t *participant,
						    uint64_t deadline,
						    hyper_native_handle_t cancellation,
						    hyper_shared_mutex_guard_t *guard)
{
	hyper_native_status_t status = validate(mutex, participant, guard);
	if (status)
		return failure(status);
	uint64_t observed;
	hyper_shared_mutex_result_t result = attempt(mutex, participant, false, guard, &observed);
	if (result.status != HYPER_NATIVE_STATUS_WOULD_BLOCK)
		return result;
	for (;;) {
		status = budget(deadline, cancellation);
		if (status)
			break;
		result = attempt(mutex, participant, true, guard, &observed);
		if (result.status != HYPER_NATIVE_STATUS_WOULD_BLOCK) {
			if (result.outcome == HYPER_SHARED_MUTEX_ERROR ||
			    result.outcome == HYPER_SHARED_MUTEX_NOT_RECOVERABLE)
				notify(mutex, 1);
			return result;
		}
		/* Changed/free words require a retry, not a sleep with invalid owner 0.
		 * Admission compares the full word, closing release-before-park races. */
		if (!(observed & OWNER))
			continue;
		/* Bind the notification obligation to the exact owner we will wait
		 * for. A previous owner's release may have cleared WAITERS before a
		 * barging owner acquired the word; a preliminary fetch_or is not enough. */
		uint64_t armed = observed | WAITERS;
		if (!atomic_compare_exchange_strong_explicit(
			mutex->word, &observed, armed, memory_order_relaxed, memory_order_relaxed))
			continue;
		hyper_native_sync_domain_wait_t request = {
		    .vmo = mutex->vmo,
		    .offset = mutex->offset,
		    .expected = armed,
		    .owner = armed & OWNER,
		    .deadline = deadline,
		    .cancellation = cancellation,
		};
		hyper_call_result_t waited =
		    hyper_sync_domain_wait_u64(mutex->domain, &request, sizeof(request));
		if (waited.status) {
			status = waited.status;
			break;
		}
		if (!((waited.value0 == HYPER_NATIVE_SYNC_WAIT_RECHECK && waited.value1 == 0) ||
		      (waited.value0 == HYPER_NATIVE_SYNC_WAIT_OWNER_EXITED &&
		       waited.value1 == request.owner))) {
			status = HYPER_NATIVE_STATUS_INTERNAL;
			break;
		}
	}
	/* Relay even if this waiter was selected just as it observed cancellation.
	 * A Thread killed before relay causes a domain exit notification instead. */
	notify(mutex, 1);
	return failure(status);
}

hyper_native_status_t hyper_shared_mutex_mark_consistent(hyper_shared_mutex_guard_t *guard)
{
	if (!guard || !guard->active || !guard->recovering)
		return HYPER_NATIVE_STATUS_BAD_STATE;
	uint64_t word = atomic_load_explicit(guard->mutex->word, memory_order_relaxed);
	for (;;) {
		if ((word & ~(OWNER | WAITERS)) != RECOVERING || (word & OWNER) != guard->token)
			return HYPER_NATIVE_STATUS_BAD_STATE;
		if (atomic_compare_exchange_weak_explicit(guard->mutex->word, &word,
							  word & ~RECOVERING, memory_order_relaxed,
							  memory_order_relaxed)) {
			guard->recovering = false;
			return 0;
		}
	}
}

void hyper_shared_mutex_unlock(hyper_shared_mutex_guard_t *guard)
{
	if (!guard || !guard->active)
		hyper_process_exit(HYPER_NATIVE_STATUS_BAD_STATE);
	const hyper_shared_mutex_t *mutex = guard->mutex;
	uint64_t old = atomic_load_explicit(mutex->word, memory_order_relaxed);
	for (;;) {
		if ((old & OWNER) != guard->token ||
		    ((old & RECOVERING) != 0) != guard->recovering ||
		    (old & ~(OWNER | WAITERS | RECOVERING)))
			hyper_process_exit(HYPER_NATIVE_STATUS_BAD_STATE);
		if (atomic_compare_exchange_weak_explicit(
			mutex->word, &old, guard->recovering ? POISON : 0, memory_order_release,
			memory_order_relaxed))
			break;
	}
	guard->active = false;
	if (guard->recovering)
		notify(mutex, UINT32_MAX);
	else if (old & WAITERS)
		notify(mutex, 1);
}
