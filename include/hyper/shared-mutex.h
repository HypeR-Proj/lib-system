/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_SHARED_MUTEX_H
#define HYPER_SHARED_MUTEX_H
#include <hyper/native.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
#include <atomic>
typedef std::atomic<uint64_t> hyper_shared_mutex_word_t;
static_assert(sizeof(hyper_shared_mutex_word_t) == 8 && alignof(hyper_shared_mutex_word_t) >= 8 &&
		  hyper_shared_mutex_word_t::is_always_lock_free,
	      "shared mutex requires an aligned lock-free u64");
extern "C" {
#else
#include <stdatomic.h>
typedef _Atomic uint64_t hyper_shared_mutex_word_t;
_Static_assert(sizeof(hyper_shared_mutex_word_t) == 8 && _Alignof(hyper_shared_mutex_word_t) >= 8 &&
		   ATOMIC_LLONG_LOCK_FREE == 2,
	       "shared mutex requires an aligned lock-free u64");
#endif

/* Participants and guards are local, thread-affine values: never copy an active
 * guard, use it on another Thread, reuse another Thread's participant, or pass
 * an active guard as acquisition output. All
 * handles and mappings are borrowed and must remain live throughout use.
 * Only the aligned atomic control word is interpreted; payload bytes are never
 * converted into typed values by this API. No operation resets a live mutex.
 * Initialize the word to zero only before publication or after externally
 * established quiescence of every possible owner, waiter and future user. */
typedef struct {
	hyper_native_handle_t domain;
	uint64_t domain_koid;
	uint64_t token;
} hyper_sync_participant_t;

typedef struct {
	hyper_shared_mutex_word_t *word;
	hyper_native_handle_t domain;
	uint64_t domain_koid;
	hyper_native_handle_t vmo;
	uint64_t offset;
} hyper_shared_mutex_t;

typedef struct {
	const hyper_shared_mutex_t *mutex;
	uint64_t token;
	bool recovering;
	bool active;
} hyper_shared_mutex_guard_t;

typedef enum {
	HYPER_SHARED_MUTEX_ACQUIRED,
	HYPER_SHARED_MUTEX_OWNER_DIED,
	HYPER_SHARED_MUTEX_NOT_RECOVERABLE,
	HYPER_SHARED_MUTEX_ERROR,
} hyper_shared_mutex_outcome_t;

typedef struct {
	hyper_shared_mutex_outcome_t outcome;
	hyper_native_status_t status;
} hyper_shared_mutex_result_t;

hyper_native_status_t hyper_sync_participant_join(hyper_native_handle_t domain,
						  hyper_sync_participant_t *participant);
/* The caller proves word aliases exactly vmo[offset..offset+8], uses only u64
 * atomics for control, and keeps all borrowed resources alive. Init validates
 * binding and SYNC_WAIT/SYNC_WAKE authority without changing the control word. */
hyper_native_status_t hyper_shared_mutex_view(hyper_shared_mutex_t *mutex,
					      hyper_shared_mutex_word_t *word,
					      hyper_native_handle_t domain,
					      hyper_native_handle_t vmo, uint64_t offset);
hyper_shared_mutex_result_t hyper_shared_mutex_try_lock(const hyper_shared_mutex_t *mutex,
							const hyper_sync_participant_t *participant,
							hyper_shared_mutex_guard_t *guard);
/* Infinite deadline selects blocking lock. Cancellation is zero or an Event
 * with WAIT. Treat cancellation as one-shot: keep the Event and every borrowed
 * handle live throughout the call, and never clear SIGNALED while any acquisition
 * can observe that Event. Reuse requires quiescence of all previous users.
 * One initial nonblocking acquisition attempt precedes checking the
 * deadline/cancellation. Every later retry checks both; successful CAS always
 * commits ownership. A timeout/error result never holds the mutex. */
hyper_shared_mutex_result_t hyper_shared_mutex_lock(const hyper_shared_mutex_t *mutex,
						    const hyper_sync_participant_t *participant,
						    uint64_t deadline,
						    hyper_native_handle_t cancellation,
						    hyper_shared_mutex_guard_t *guard);
/* OWNER_DIED holds exclusive recovery authority, but payload may contain invalid
 * representations. Repair raw/crash-valid bytes before declaring consistency.
 * Unlocking an unconfirmed recovery guard permanently poisons the mutex. */
hyper_native_status_t hyper_shared_mutex_mark_consistent(hyper_shared_mutex_guard_t *guard);
void hyper_shared_mutex_unlock(hyper_shared_mutex_guard_t *guard);
/* A corrupted guard or unexpected wake failure is fatal: continuing could lose
 * the only notification obligation. Process termination supplies domain wake. */
#ifdef __cplusplus
}
#endif
#endif
