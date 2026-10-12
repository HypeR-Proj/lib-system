/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/thread.h>
#include "stack-internal.h"
#include <hyper/heap.h>
#include <hyper/syscall.h>
#include <stdatomic.h>
#include <stdlib.h>

/* Keys are never reused: a stale per-thread value cannot become associated
 * with a newly allocated key. Nodes live until process exit, but values and
 * per-thread tables are reclaimed. No locks are held across destructors. */
typedef struct tls_key {
	hyper_tls_destructor_t destructor;
	atomic_bool active;
} tls_key_t;

typedef struct tls_value {
	tls_key_t *key;
	void *value;
	struct tls_value *next;
} tls_value_t;

typedef struct thread_state {
	tls_value_t *values;
	hyper_stack_t *stack;
} thread_state_t;

#ifdef HYPER_THREAD_HOST_TEST
static _Thread_local thread_state_t *host_thread;

static thread_state_t *current(void)
{
	return host_thread;
}

static void install(thread_state_t *state)
{
	host_thread = state;
}
#elif defined(__aarch64__)
static thread_state_t *current(void)
{
	thread_state_t *state;
	__asm__ volatile("mrs %0, tpidr_el0" : "=r"(state));
	return state;
}

static void install(thread_state_t *state)
{
	__asm__ volatile("msr tpidr_el0, %0" : : "r"(state) : "memory");
}
#elif defined(__riscv) && __riscv_xlen == 64
static thread_state_t *current(void)
{
	thread_state_t *state;
	__asm__ volatile("mv %0, tp" : "=r"(state));
	return state;
}

static void install(thread_state_t *state)
{
	__asm__ volatile("mv tp, %0" : : "r"(state) : "memory");
}
#else
#error Unsupported Native thread architecture
#endif

hyper_native_status_t hyper_runtime_thread_attach(void)
{
	if (current() != NULL)
		return HYPER_NATIVE_STATUS_OK;
	thread_state_t *state = calloc(1, sizeof(*state));
	if (state == NULL)
		return HYPER_NATIVE_STATUS_NO_MEMORY;
	install(state);
	return HYPER_NATIVE_STATUS_OK;
}

hyper_stack_t *hyper_stack_current(void)
{
	thread_state_t *state = current();
	return state ? state->stack : NULL;
}

hyper_native_status_t hyper_runtime_thread_attach_stack(hyper_stack_t *stack)
{
	if (!stack)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	hyper_native_status_t status = hyper_runtime_thread_attach();
	if (status != HYPER_NATIVE_STATUS_OK)
		return status;
	thread_state_t *state = current();
	if (state->stack && state->stack != stack)
		return HYPER_NATIVE_STATUS_BAD_STATE;
	state->stack = stack;
	return HYPER_NATIVE_STATUS_OK;
}

static thread_state_t *require_current(void)
{
	if (hyper_runtime_thread_attach() != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(HYPER_NATIVE_STATUS_NO_MEMORY);
	return current();
}

uintptr_t hyper_runtime_tls_create(hyper_tls_destructor_t destructor)
{
	tls_key_t *key = malloc(sizeof(*key));
	if (key == NULL)
		hyper_process_exit(HYPER_NATIVE_STATUS_NO_MEMORY);
	key->destructor = destructor;
	atomic_init(&key->active, 1);
	return (uintptr_t)key;
}

void hyper_runtime_tls_destroy(uintptr_t key)
{
	atomic_store_explicit(&((tls_key_t *)key)->active, 0, memory_order_release);
}

void *hyper_runtime_tls_get(uintptr_t key)
{
	for (tls_value_t *slot = require_current()->values; slot; slot = slot->next)
		if (slot->key == (tls_key_t *)key)
			return slot->value;
	return NULL;
}

void hyper_runtime_tls_set(uintptr_t key, void *value)
{
	thread_state_t *state = require_current();
	for (tls_value_t *slot = state->values; slot; slot = slot->next) {
		if (slot->key == (tls_key_t *)key) {
			slot->value = value;
			return;
		}
	}
	tls_value_t *slot = malloc(sizeof(*slot));
	if (slot == NULL)
		hyper_process_exit(HYPER_NATIVE_STATUS_NO_MEMORY);
	*slot = (tls_value_t){(tls_key_t *)key, value, state->values};
	state->values = slot;
}

void hyper_runtime_thread_detach(void)
{
	thread_state_t *state = current();
	if (state == NULL)
		return;
	for (unsigned pass = 0; pass < 5; ++pass) {
		int ran = 0;
		for (tls_value_t *slot = state->values; slot; slot = slot->next) {
			if (slot->value && slot->key->destructor &&
			    atomic_load_explicit(&slot->key->active, memory_order_acquire)) {
				void *value = slot->value;
				slot->value = NULL;
				slot->key->destructor(value);
				ran = 1;
			}
		}
		if (!ran)
			break;
	}
	tls_value_t *slot = state->values;
	while (slot) {
		tls_value_t *next = slot->next;
		free(slot);
		slot = next;
	}
	install(NULL);
	free(state);
}

int hyper_runtime_wait_u32(const uint32_t *address, uint32_t expected, uint64_t deadline)
{
	hyper_native_status_t status = hyper_atomic_wait(address, expected, deadline);
	if (status == HYPER_NATIVE_STATUS_TIMED_OUT)
		return 0;
	if (status != HYPER_NATIVE_STATUS_OK && status != HYPER_NATIVE_STATUS_CANCELLED)
		hyper_process_exit(status);
	return 1;
}

uint32_t hyper_runtime_wake_u32(const uint32_t *address, uint32_t count)
{
	hyper_call_result_t result = hyper_atomic_wake(address, count);
	if (result.status != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(result.status);
	if (result.value0 > count)
		hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
	return (uint32_t)result.value0;
}
