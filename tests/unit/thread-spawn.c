/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
/* Drive real subscription/ownership code with controlled kernel event order. */
#include "../../src/thread_spawn.c"

struct hyper_stack {
	int owned;
};

struct fake_thread {
	int live, started, terminated;
	uint64_t registration;
};
static struct fake_thread threads[128];
static uint64_t next_thread = 2, next_registration = 100;
static unsigned allocated_stacks, live_registrations, registration_limit = 1024;
static int fail_start, fail_add;
static size_t smallest_requested_stack = SIZE_MAX;

hyper_native_status_t hyper_stack_create(size_t size, size_t capacity, hyper_stack_t **out)
{
	if (size < smallest_requested_stack)
		smallest_requested_stack = size;
	(void)capacity;
	*out = calloc(1, sizeof(**out));
	assert(*out);
	++allocated_stacks;
	return 0;
}

hyper_native_status_t hyper_stack_get_info(hyper_stack_t *stack, hyper_stack_info_t *out)
{
	assert(stack);
	*out = (hyper_stack_info_t){0x1000, 0x11000, 0x10000, 0x100000};
	return 0;
}

void hyper_stack_claim_runtime(hyper_stack_t *stack)
{
	assert(!stack->owned);
	stack->owned = 1;
}

void hyper_stack_release_runtime(hyper_stack_t *stack)
{
	assert(stack->owned);
	--allocated_stacks;
	free(stack);
}

hyper_native_status_t hyper_runtime_thread_attach_stack(hyper_stack_t *stack)
{
	assert(stack);
	return 0;
}

void hyper_runtime_thread_detach(void)
{
}

int hyper_runtime_wait_u32(const uint32_t *word, uint32_t expected, uint64_t deadline)
{
	(void)word;
	(void)expected;
	(void)deadline;
	__builtin_trap();
}

uint32_t hyper_runtime_wake_u32(const uint32_t *word, uint32_t count)
{
	(void)word;
	(void)count;
	return 0;
}

_Noreturn void hyper_process_exit(int64_t status)
{
	(void)status;
	__builtin_trap();
}

_Noreturn void hyper_thread_exit(int64_t status)
{
	(void)status;
	__builtin_trap();
}

hyper_call_result_t hyper_wait_set_create(size_t capacity)
{
	assert(capacity == 1024);
	return (hyper_call_result_t){0, 1, 0};
}

hyper_call_result_t hyper_wait_set_add(uint64_t set, uint64_t thread, uint64_t signals)
{
	assert(set == 1 && threads[thread].live && !threads[thread].started);
	assert(signals == HYPER_NATIVE_SIGNAL_THREAD_TERMINATED);
	if (fail_add || live_registrations == registration_limit)
		return (hyper_call_result_t){HYPER_NATIVE_STATUS_RESOURCE_LIMIT, 0, 0};
	threads[thread].registration = ++next_registration;
	++live_registrations;
	return (hyper_call_result_t){0, next_registration, 0};
}

hyper_call_result_t hyper_wait_set_remove(uint64_t set, uint64_t registration)
{
	assert(set == 1);
	for (size_t i = 2; i < next_thread; ++i) {
		if (threads[i].registration == registration) {
			threads[i].registration = 0;
			--live_registrations;
			return (hyper_call_result_t){0, 0, 0};
		}
	}
	__builtin_trap(); /* Never remove an already removed subscription. */
}

hyper_call_result_t hyper_wait_set_wait(uint64_t set, uint64_t deadline, void *out, size_t size)
{
	(void)set;
	(void)deadline;
	(void)out;
	(void)size;
	__builtin_trap();
}

hyper_call_result_t hyper_thread_create(uint64_t entry, uint64_t stack, uint64_t tls,
					uint64_t argument, const uint64_t *affinity, size_t words)
{
	(void)entry;
	(void)stack;
	(void)tls;
	(void)argument;
	(void)affinity;
	(void)words;
	assert(next_thread < 128);
	threads[next_thread].live = 1;
	return (hyper_call_result_t){0, next_thread++, 0};
}

hyper_native_status_t hyper_thread_start(uint64_t thread)
{
	assert(threads[thread].live && !threads[thread].started);
	if (fail_start)
		return HYPER_NATIVE_STATUS_NO_MEMORY;
	if (thread != 2)
		assert(threads[thread].registration); /* Register before start. */
	threads[thread].started = 1;
	return 0;
}

hyper_native_status_t hyper_handle_close(uint64_t handle)
{
	if (handle == 1)
		return 0;
	assert(threads[handle].live);
	/* Only the process-lifetime reaper may close its handle while executing. */
	assert(handle == 2 || !threads[handle].started || threads[handle].terminated);
	assert(!threads[handle].registration);
	threads[handle].live = 0;
	return 0;
}

hyper_call_result_t hyper_object_wait_one(uint64_t thread, uint64_t signals, uint64_t deadline)
{
	assert(signals == HYPER_NATIVE_SIGNAL_THREAD_TERMINATED && deadline == UINT64_MAX);
	assert(threads[thread].live && threads[thread].terminated);
	return (hyper_call_result_t){0, signals, 0};
}

static void entry(void *argument)
{
	(void)argument;
}

static thread_token_t *spawn(void)
{
	uintptr_t token = 0;
	assert(hyper_runtime_thread_spawn(1, entry, NULL, &token) == 0);
	assert(token);
	assert(smallest_requested_stack == 1);
	return (thread_token_t *)token;
}

static uint64_t terminate(thread_token_t *token)
{
	assert(threads[token->thread].started);
	threads[token->thread].terminated = 1;
	return token->registration;
}

static void clean(void)
{
	assert(!registered && !live_registrations && allocated_stacks == 1);
}

int main(void)
{
	/* A direct kernel exit never executes the worker's return epilogue. */
	thread_token_t *a = spawn();
	uint64_t id = terminate(a);
	hyper_runtime_thread_release((uintptr_t)a);
	completed_registration(id);
	clean();

	/* Termination before detach releases its slot, retaining caller ownership. */
	a = spawn();
	id = terminate(a);
	completed_registration(id);
	assert(!live_registrations && allocated_stacks == 2);
	hyper_runtime_thread_release((uintptr_t)a);
	clean();

	/* A dequeued event can outlive join and token storage; a new token never
	 * inherits the old registration even if malloc reuses its address. */
	a = spawn();
	id = terminate(a);
	assert(hyper_runtime_thread_join((uintptr_t)a) == 0);
	thread_token_t *b = spawn();
	assert(b->registration != id);
	completed_registration(id);
	assert(live_registrations == 1 && !b->terminated);
	id = terminate(b);
	completed_registration(id);
	assert(hyper_runtime_thread_join((uintptr_t)b) == 0);
	completed_registration(id);
	clean();

	/* Both failures happen while the kernel thread is still dormant. */
	uintptr_t raw = 0;
	fail_add = 1;
	assert(hyper_runtime_thread_spawn(65536, entry, NULL, &raw) ==
	       HYPER_NATIVE_STATUS_RESOURCE_LIMIT);
	fail_add = 0;
	clean();
	fail_start = 1;
	assert(hyper_runtime_thread_spawn(65536, entry, NULL, &raw) ==
	       HYPER_NATIVE_STATUS_NO_MEMORY);
	fail_start = 0;
	clean();

	/* A terminated but unjoined token no longer occupies a WaitSet slot. */
	registration_limit = 1;
	a = spawn();
	assert(hyper_runtime_thread_spawn(65536, entry, NULL, &raw) ==
	       HYPER_NATIVE_STATUS_RESOURCE_LIMIT);
	id = terminate(a);
	completed_registration(id);
	b = spawn();
	hyper_runtime_thread_release((uintptr_t)b);
	id = terminate(b);
	completed_registration(id);
	hyper_runtime_thread_release((uintptr_t)a);
	clean();
	return 0;
}
