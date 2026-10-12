/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/launch.h>
#include <hyper/heap.h>
#include <hyper/stack.h>
#include <hyper/system.h>
#include <hyper/syscall.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "../../src/stack-internal.h"
#include <assert.h>
#include <string.h>

struct hyper_stack {
	unsigned cookie;
};

static _Alignas(16) unsigned char final_stack[256 * 1024];
static jmp_buf returned;
static unsigned switched, unmapped, destroyed, acknowledged, closed;
static const uintptr_t *final_vector;
static int64_t expected_failure;
static struct hyper_stack initial = {123};
static unsigned heap_calls, stack_calls, attach_calls, capability_calls;
static const hyper_native_startup_handle_t *original_handles;

static void check_original(const hyper_startup_t *startup)
{
	assert(startup->handles == original_handles && startup->handle_count == 3);
	hyper_native_handle_t handle = 0;
	assert(hyper_startup_find_handle(startup,
					 HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_INITIAL_STACK_VMAR,
					 &handle) == HYPER_NATIVE_STATUS_OK);
	assert(handle == 22);
}

hyper_native_status_t hyper_heap_initialize(const hyper_startup_t *startup)
{
	check_original(startup);
	assert(!heap_calls++ && !stack_calls && !attach_calls && !capability_calls);
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_stack_initialize(const hyper_startup_t *startup, hyper_stack_t **output)
{
	check_original(startup);
	assert(heap_calls == 1 && !stack_calls++ && !attach_calls && !capability_calls);
	*output = &initial;
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_runtime_thread_attach_stack(hyper_stack_t *stack)
{
	assert(stack == &initial && stack->cookie == 123);
	assert(heap_calls == 1 && stack_calls == 1 && !attach_calls++ && !capability_calls);
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_runtime_capabilities_initialize(const hyper_startup_t *startup)
{
	check_original(startup);
	assert(heap_calls == 1 && stack_calls == 1 && attach_calls == 1 && !capability_calls++);
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_page_size(size_t *page)
{
	*page = 4096;
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_stack_get_info(hyper_stack_t *stack, hyper_stack_info_t *info)
{
	assert(stack == &initial);
	*info = (hyper_stack_info_t){.top = (uintptr_t)final_stack + sizeof(final_stack),
				     .capacity = 256 * 1024 * 1024,
				     .size = sizeof(final_stack)};
	return HYPER_NATIVE_STATUS_OK;
}

hyper_stack_t *hyper_stack_current(void)
{
	return &initial;
}

hyper_native_status_t hyper_stack_grow(hyper_stack_t *stack, size_t size)
{
	(void)stack;
	(void)size;
	__builtin_trap();
}

_Noreturn void hyper_process_exit(int64_t status)
{
	if (expected_failure)
		_exit(status == expected_failure ? 0 : 1);
	__builtin_trap();
}

hyper_native_status_t hyper_vmar_unmap(uint64_t handle, uintptr_t base, size_t size)
{
	assert(switched && !acknowledged);
	if (unmapped++ == 0) {
		assert(!destroyed && handle == 22 && base == 0xf17c1000 && size == 0x40000);
	} else {
		assert(destroyed && unmapped == 2 && handle == 11 && base == 0x100000 &&
		       size == 0x5000);
	}
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_vmar_destroy(uint64_t handle)
{
	assert(handle == 22 && unmapped && !destroyed++);
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_byte_channel_write(hyper_native_handle_t channel, const void *data,
					       size_t size)
{
	assert(channel == 44 && size == sizeof(hyper_launch_result_t));
	assert(switched && unmapped == 2 && destroyed && !acknowledged++ && !closed);
	assert(((const hyper_launch_result_t *)data)->status == 0);
	return 0;
}

hyper_native_status_t hyper_handle_close(hyper_native_handle_t handle)
{
	assert(handle == 44 && acknowledged && !closed++);
	return 0;
}

_Noreturn void __hyper_runtime_switch_stack(uintptr_t sp, void (*entry)(const uintptr_t *))
{
	assert(!switched++ && !unmapped && !destroyed);
	assert(sp % 16 == 0 && sp >= (uintptr_t)final_stack &&
	       sp < (uintptr_t)final_stack + sizeof(final_stack));
	entry((const uintptr_t *)sp);
	__builtin_trap();
}

static void entered(const uintptr_t *stack)
{
	assert(switched && unmapped == 2 && destroyed && acknowledged && closed);
	final_vector = stack;
	longjmp(returned, 1);
}

int main(void)
{
	assert(hyper_runtime_startup() == NULL);
	assert(hyper_runtime_initialize(NULL) == HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
	assert(hyper_runtime_startup() == NULL && !heap_calls);
	char argument[] = "/bin/example";
	char option[] = "--help";
	char environment[] = "TERM=hyper";
	hyper_native_startup_handle_t handles[] = {
		{HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_ROOT_VMAR, 0, 11},
		{HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_INITIAL_STACK_VMAR, 0, 22},
		{HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_CONSOLE, 0, 33},
	};
	original_handles = handles;
	uintptr_t words[] = {
		2,
		(uintptr_t)argument,
		(uintptr_t)option,
		0,
		(uintptr_t)environment,
		0,
		HYPER_AUXV_STARTUP_HANDLES,
		(uintptr_t)handles,
		HYPER_AUXV_STARTUP_HANDLE_COUNT,
		3,
		HYPER_AUXV_INITIAL_STACK_BASE,
		0xf1000000,
		HYPER_AUXV_INITIAL_STACK_CAPACITY,
		0x800000,
		HYPER_AUXV_INITIAL_STACK_SIZE,
		0x40000,
		HYPER_AUXV_LOADER_BASE,
		0x100000,
		HYPER_AUXV_LOADER_SIZE,
		0x5000,
		HYPER_AUXV_STARTUP_CHANNEL,
		44,
		6,
		HYPER_NATIVE_PAGE_SIZE,
		0,
		0,
	};
	/* Reject malformed bootstrap ownership before any allocation or SP change. */
	for (unsigned mode = 0; mode < 3; ++mode) {
		pid_t child = fork();
		assert(child >= 0);
		if (child == 0) {
			expected_failure = HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
			if (mode == 0)
				words[14] = 123; /* Missing mapped-size tag. */
			else if (mode == 1)
				words[13] = UINTPTR_MAX; /* Overflow/misalignment. */
			else
				words[14] = HYPER_AUXV_INITIAL_STACK_BASE;
			hyper_runtime_start(words, entered);
		}
		int status;
		assert(waitpid(child, &status, 0) == child);
		assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
	}
	if (!setjmp(returned))
		hyper_runtime_start(words, entered);
	const hyper_startup_t *view = hyper_runtime_startup();
	assert(view && view->handle_count == 2 && view->handles != handles);
	assert(view->handles[0].purpose == handles[0].purpose && view->handles[0].handle == 11);
	assert(view->handles[1].purpose == handles[2].purpose && view->handles[1].handle == 33);
	assert(view->argument_count == 2 && view->arguments[0] != argument &&
	       memcmp(view->arguments[0], argument, sizeof(argument)) == 0 &&
	       memcmp(view->arguments[1], option, sizeof(option)) == 0);
	assert(view->environment_count == 1 && view->environment[0] != environment &&
	       memcmp(view->environment[0], environment, sizeof(environment)) == 0);
	assert(view->auxiliary_count == 9);
	assert(view->auxiliary[view->auxiliary_count].key == 0);
	assert(view->auxiliary[view->auxiliary_count].value == 0);
	const hyper_auxiliary_entry_t *original_aux = (const hyper_auxiliary_entry_t *)&words[6];
	for (size_t i = 0; i < view->auxiliary_count; ++i) {
		assert(view->auxiliary[i].key == original_aux[i].key);
		if (view->auxiliary[i].key == HYPER_AUXV_STARTUP_HANDLES)
			assert(view->auxiliary[i].value == (uintptr_t)view->handles);
		else if (view->auxiliary[i].key == HYPER_AUXV_STARTUP_HANDLE_COUNT)
			assert(view->auxiliary[i].value == view->handle_count);
		else if (view->auxiliary[i].key == HYPER_AUXV_INITIAL_STACK_BASE)
			assert(view->auxiliary[i].value == (uintptr_t)final_stack +
								   sizeof(final_stack) -
								   256 * 1024 * 1024 - 4096);
		else if (view->auxiliary[i].key == HYPER_AUXV_INITIAL_STACK_CAPACITY)
			assert(view->auxiliary[i].value == 256 * 1024 * 1024);
		else
			assert(view->auxiliary[i].value == original_aux[i].value);
	}
	hyper_startup_t moved;
	assert(hyper_startup_parse(final_vector, &moved) == HYPER_NATIVE_STATUS_OK);
	assert(moved.handle_count == 2 && moved.argument_count == 2);
	memset(argument, 0xcc, sizeof(argument));
	memset(option, 0xcc, sizeof(option));
	memset(environment, 0xcc, sizeof(environment));
	assert(memcmp(moved.arguments[0], "/bin/example", sizeof(argument)) == 0);
	assert(memcmp(moved.arguments[1], "--help", sizeof(option)) == 0);
	assert(memcmp(moved.environment[0], "TERM=hyper", sizeof(environment)) == 0);
	assert(handles[1].handle == 22 && original_aux[1].value == 3);
	hyper_native_handle_t output = 0;
	assert(hyper_startup_find_handle(view,
					 HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_INITIAL_STACK_VMAR,
					 &output) != HYPER_NATIVE_STATUS_OK);
	assert(hyper_runtime_initialize(words) == HYPER_NATIVE_STATUS_OK);
	assert(hyper_runtime_startup() == view);
	assert(heap_calls == 1 && stack_calls == 1 && attach_calls == 1 && capability_calls == 1);
	return 0;
}
