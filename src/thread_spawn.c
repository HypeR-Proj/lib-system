/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/thread.h>
#include "stack-internal.h"
#include <hyper/syscall.h>
#include "mutex-internal.h"
#include <stdlib.h>

/* Tokens have one caller owner until join or release. The worker borrows its
 * token until Native termination. A single process-lifetime cleanup worker
 * observes kernel TERMINATED events, including raw thread_exit/stop paths
 * that never return through the language trampoline. Registrations use kernel
 * IDs, never token pointers, so a queued stale event cannot dereference freed
 * storage after join removes a subscription.
 *
 * The WaitSet admits at most 1024 outstanding subscriptions. Failure is
 * reported before worker start. Consuming termination releases that slot even
 * while a completed joinable token remains caller-owned. */
typedef struct thread_token {
	hyper_stack_t *stack;
	hyper_native_handle_t thread;
	hyper_runtime_thread_entry_t entry;
	void *argument;
	uint64_t registration;
	int detached;
	int terminated;
	struct thread_token *next;
} thread_token_t;

static hyper_mutex_t queue_lock;
static thread_token_t *registered;
static hyper_mutex_t init_lock;
static int reaper_started;
static hyper_native_handle_t termination_set;

static int64_t wait_terminated(hyper_native_handle_t thread)
{
	return hyper_object_wait_one(thread, HYPER_NATIVE_SIGNAL_THREAD_TERMINATED, UINT64_MAX)
		.status;
}

static void reclaim(thread_token_t *token)
{
	if (hyper_handle_close(token->thread) != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
	hyper_stack_release_runtime(token->stack);
	free(token);
}

static _Noreturn void worker(void *argument)
{
	thread_token_t *token = argument;

	if (hyper_runtime_thread_attach_stack(token->stack) != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(HYPER_NATIVE_STATUS_NO_MEMORY);
	token->entry(token->argument);
	hyper_runtime_thread_detach();
	hyper_thread_exit(0);
}

/* queue_lock serializes registration ownership with event consumption and
 * join. Removing a one-shot subscription remains necessary after its event
 * is consumed. Any failure retains all owners by terminating the process. */
static void unregister_locked(thread_token_t *token)
{
	if (!token->registration)
		return;
	if (hyper_wait_set_remove(termination_set, token->registration).status !=
	    HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);

	thread_token_t **link = &registered;

	while (*link && *link != token)
		link = &(*link)->next;
	if (!*link)
		hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
	*link = token->next;
	token->registration = 0;
	token->next = NULL;
}

static void completed_registration(uint64_t registration)
{
	hyper_mutex_lock(&queue_lock);
	thread_token_t *token = registered;

	while (token && token->registration != registration)
		token = token->next;
	if (!token) {
		/* Join may remove a registration after wait dequeues its event but
		 * before this lock is acquired. IDs are never reused. */
		hyper_mutex_unlock(&queue_lock);
		return;
	}
	unregister_locked(token);
	token->terminated = 1;
	int detached = token->detached;

	hyper_mutex_unlock(&queue_lock);
	if (detached)
		reclaim(token);
}

static _Noreturn void reaper(void *argument)
{
	if (hyper_runtime_thread_attach_stack(argument) != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(HYPER_NATIVE_STATUS_NO_MEMORY);
	for (;;) {
		hyper_native_wait_set_event_t event;
		hyper_call_result_t result =
			hyper_wait_set_wait(termination_set, UINT64_MAX, &event, sizeof(event));
		if (result.status != HYPER_NATIVE_STATUS_OK ||
		    !(event.signals & HYPER_NATIVE_SIGNAL_THREAD_TERMINATED))
			hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
		completed_registration(event.registration);
	}
}

static int64_t ensure_reaper(void)
{
	hyper_mutex_lock(&init_lock);
	int64_t status = HYPER_NATIVE_STATUS_OK;

	if (!reaper_started) {
		hyper_call_result_t set = hyper_wait_set_create(1024);
		if (set.status != HYPER_NATIVE_STATUS_OK) {
			hyper_mutex_unlock(&init_lock);
			return set.status;
		}
		termination_set = set.value0;
		hyper_stack_t *stack;

		status = hyper_stack_create(64 * 1024, 0, &stack);
		if (status != HYPER_NATIVE_STATUS_OK) {
			if (hyper_handle_close(termination_set) != HYPER_NATIVE_STATUS_OK)
				hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
			termination_set = 0;
			hyper_mutex_unlock(&init_lock);
			return status;
		}

		hyper_stack_info_t info;

		status = hyper_stack_get_info(stack, &info);
		if (status != HYPER_NATIVE_STATUS_OK)
			hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
		hyper_stack_claim_runtime(stack);
		hyper_call_result_t result = hyper_thread_create((uintptr_t)reaper, info.top, 0,
								 (uintptr_t)stack, NULL, 0);
		status = result.status;
		if (status == HYPER_NATIVE_STATUS_OK) {
			status = hyper_thread_start(result.value0);
			if (hyper_handle_close(result.value0) != HYPER_NATIVE_STATUS_OK)
				hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
		}
		if (status == HYPER_NATIVE_STATUS_OK)
			reaper_started = 1;
		else {
			hyper_stack_release_runtime(stack);
			if (hyper_handle_close(termination_set) != HYPER_NATIVE_STATUS_OK)
				hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
			termination_set = 0;
		}
	}
	hyper_mutex_unlock(&init_lock);
	return status;
}

hyper_native_status_t hyper_runtime_thread_spawn_with_stack(size_t size, size_t capacity,
							    hyper_runtime_thread_entry_t entry,
							    void *argument, uintptr_t *result)
{
	if (!entry || !result)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	if (!size)
		size = 64 * 1024;

	thread_token_t *token = calloc(1, sizeof(*token));

	if (!token)
		return HYPER_NATIVE_STATUS_NO_MEMORY;
	int64_t status = hyper_stack_create(size, capacity, &token->stack);
	if (status != HYPER_NATIVE_STATUS_OK) {
		free(token);
		return status;
	}
	hyper_stack_claim_runtime(token->stack);
	status = ensure_reaper();
	if (status != HYPER_NATIVE_STATUS_OK) {
		hyper_stack_release_runtime(token->stack);
		free(token);
		return status;
	}
	hyper_stack_info_t info;

	if (hyper_stack_get_info(token->stack, &info) != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
	token->entry = entry;
	token->argument = argument;
	hyper_call_result_t created =
		hyper_thread_create((uintptr_t)worker, info.top, 0, (uintptr_t)token, NULL, 0);
	if (created.status != HYPER_NATIVE_STATUS_OK) {
		hyper_stack_release_runtime(token->stack);
		free(token);
		return created.status;
	}

	token->thread = created.value0;
	hyper_mutex_lock(&queue_lock);
	hyper_call_result_t subscription = hyper_wait_set_add(
		termination_set, token->thread, HYPER_NATIVE_SIGNAL_THREAD_TERMINATED);
	if (subscription.status == HYPER_NATIVE_STATUS_OK) {
		token->registration = subscription.value0;
		token->next = registered;
		registered = token;
	}
	hyper_mutex_unlock(&queue_lock);
	if (subscription.status != HYPER_NATIVE_STATUS_OK) {
		reclaim(token);
		return subscription.status;
	}

	status = hyper_thread_start(token->thread);
	if (status != HYPER_NATIVE_STATUS_OK) {
		hyper_mutex_lock(&queue_lock);
		unregister_locked(token);
		hyper_mutex_unlock(&queue_lock);
		reclaim(token);
		return status;
	}

	*result = (uintptr_t)token;
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_runtime_thread_spawn(size_t size, hyper_runtime_thread_entry_t entry,
						 void *argument, uintptr_t *result)
{
	return hyper_runtime_thread_spawn_with_stack(size, 0, entry, argument, result);
}

hyper_native_status_t hyper_runtime_thread_join(uintptr_t raw)
{
	thread_token_t *token = (thread_token_t *)raw;

	int64_t status = wait_terminated(token->thread);
	if (status == HYPER_NATIVE_STATUS_OK) {
		hyper_mutex_lock(&queue_lock);
		unregister_locked(token);
		hyper_mutex_unlock(&queue_lock);
		reclaim(token);
	}
	return status;
}

void hyper_runtime_thread_release(uintptr_t raw)
{
	thread_token_t *token = (thread_token_t *)raw;

	hyper_mutex_lock(&queue_lock);
	token->detached = 1;
	int terminated = token->terminated;
	hyper_mutex_unlock(&queue_lock);
	if (terminated)
		reclaim(token);
}
