/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/launch.h>
#include <hyper/stack.h>
#include <hyper/syscall.h>
#include "../../src/stack-internal.h"
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

hyper_call_result_t hyper_system_config(uint64_t key)
{
	if (key == HYPER_NATIVE_SYSTEM_CONFIG_APPLICATION_ADDRESS_LIMIT) {
		extern uintptr_t hyper_stack_test_base;
		return (hyper_call_result_t){.value0 = hyper_stack_test_base + (UINT64_C(4) << 30)};
	}
	assert(key == HYPER_NATIVE_SYSTEM_CONFIG_PAGE_SIZE);
	return (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_OK,
				     .value0 = (uint64_t)sysconf(_SC_PAGESIZE)};
}

uintptr_t hyper_stack_test_base;
static hyper_stack_t *current_stack;
static int fail_vmo, fail_map, fail_destroy, fail_unmap, fail_allocate, fail_duplicate;
static size_t backing_size;
static unsigned backing_live, root_live;

static struct region {
	uintptr_t base;
	size_t size, mapped;
	uint64_t parent;
} regions[128];

static struct mapping {
	uintptr_t base;
	size_t size;
	uint64_t vmar;
} mappings[256];

static const uint64_t backing_handle = 1024;

hyper_stack_t *hyper_stack_current(void)
{
	return current_stack;
}

hyper_native_status_t hyper_thread_yield(void)
{
	return HYPER_NATIVE_STATUS_OK;
}

_Noreturn void hyper_process_exit(int64_t status)
{
	(void)status;
	__builtin_trap();
}

hyper_call_result_t hyper_handle_duplicate(uint64_t source, uint64_t rights)
{
	assert(source == 1 && rights == HYPER_NATIVE_RIGHT_MAP);
	if (fail_duplicate)
		return (hyper_call_result_t){HYPER_NATIVE_STATUS_NO_MEMORY, 0, 0};
	assert(!root_live);
	root_live = 1;
	return (hyper_call_result_t){HYPER_NATIVE_STATUS_OK, 2, 0};
}

hyper_call_result_t hyper_vmar_allocate(uint64_t parent, uintptr_t base, size_t size,
					uint64_t options)
{
	assert(options == 0);
	/* The runtime must use its own handle, never the application's handle. */
	assert(parent == 2);
	parent = 1;
	assert(parent < 128 && regions[parent].size);
	if (fail_allocate)
		return (hyper_call_result_t){HYPER_NATIVE_STATUS_NO_MEMORY, 0, 0};
	if (!options) {
		uintptr_t end = regions[parent].base + regions[parent].size;
		for (;;) {
			if (size > end - regions[parent].base)
				return (hyper_call_result_t){HYPER_NATIVE_STATUS_NO_MEMORY, 0, 0};
			base = end - size;
			uintptr_t next = end;
			for (size_t i = 1; i < 128; ++i) {
				if (regions[i].size && regions[i].parent == parent &&
				    base < regions[i].base + regions[i].size &&
				    regions[i].base < end && regions[i].base < next)
					next = regions[i].base;
			}
			if (next == end)
				break;
			end = next;
		}
	}
	assert(base >= regions[parent].base && size <= regions[parent].size);
	assert(base - regions[parent].base <= regions[parent].size - size);
	for (size_t i = 1; i < 128; ++i) {
		if (regions[i].size && regions[i].parent == parent)
			assert(base + size <= regions[i].base ||
			       regions[i].base + regions[i].size <= base);
	}
	for (size_t i = 4; i < 128; ++i) {
		if (!regions[i].size) {
			regions[i] = (struct region){base, size, 0, parent};
			return (hyper_call_result_t){HYPER_NATIVE_STATUS_OK, i, base};
		}
	}
	return (hyper_call_result_t){HYPER_NATIVE_STATUS_NO_MEMORY, 0, 0};
}

hyper_call_result_t hyper_vmo_create(uint64_t size)
{
	assert(!backing_live);
	if (fail_vmo)
		return (hyper_call_result_t){HYPER_NATIVE_STATUS_NO_MEMORY, 0, 0};
	backing_live = 1;
	backing_size = size;
	return (hyper_call_result_t){HYPER_NATIVE_STATUS_OK, backing_handle, 0};
}

hyper_native_status_t hyper_handle_close(uint64_t handle)
{
	if (handle == 2) {
		assert(root_live);
		root_live = 0;
		return HYPER_NATIVE_STATUS_OK;
	}
	assert(handle == backing_handle && backing_live);
	backing_live = 0;
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_vmar_map(uint64_t vmar, uint64_t vmo, uint64_t offset, uintptr_t base,
				     size_t size, uint32_t permissions)
{
	assert(vmar < 128 && regions[vmar].size);
	assert(vmo == backing_handle && backing_live && offset == 0 && size == backing_size);
	assert(permissions ==
	       (HYPER_NATIVE_VMAR_PERMISSION_READ | HYPER_NATIVE_VMAR_PERMISSION_WRITE));
	assert(base >= regions[vmar].base + HYPER_NATIVE_PAGE_SIZE);
	assert(base + size <= regions[vmar].base + regions[vmar].size - HYPER_NATIVE_PAGE_SIZE);
	if (fail_map)
		return HYPER_NATIVE_STATUS_NO_MEMORY;
	size_t slot = 256;
	for (size_t i = 0; i < 256; ++i) {
		if (!mappings[i].size)
			slot = i;
		else
			assert(base + size <= mappings[i].base ||
			       mappings[i].base + mappings[i].size <= base);
	}
	assert(slot < 256);
	assert(mprotect((void *)base, size, PROT_READ | PROT_WRITE) == 0);
	memset((void *)base, 0, size);
	mappings[slot] = (struct mapping){base, size, vmar};
	regions[vmar].mapped += size;
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_vmar_unmap(uint64_t vmar, uintptr_t base, size_t size)
{
	assert(vmar < 128 && regions[vmar].size);
	if (fail_unmap)
		return HYPER_NATIVE_STATUS_NO_MEMORY;
	size_t removed = 0;
	for (size_t i = 0; i < 256; ++i) {
		if (mappings[i].size && mappings[i].vmar == vmar && mappings[i].base >= base &&
		    mappings[i].base + mappings[i].size <= base + size) {
			removed += mappings[i].size;
			mappings[i].size = 0;
		}
	}
	assert(removed == size && regions[vmar].mapped >= size);
	regions[vmar].mapped -= size;
	assert(mprotect((void *)base, size, PROT_NONE) == 0);
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_vmar_destroy(uint64_t vmar)
{
	assert(vmar >= 3 && vmar < 128 && regions[vmar].size && !regions[vmar].mapped);
	if (fail_destroy)
		return HYPER_NATIVE_STATUS_NO_MEMORY;
	for (size_t i = 1; i < 128; ++i)
		assert(!regions[i].size || regions[i].parent != vmar);
	memset(&regions[vmar], 0, sizeof(regions[vmar]));
	return HYPER_NATIVE_STATUS_OK;
}

static void guard_fault(uintptr_t address)
{
	pid_t child = fork();
	assert(child >= 0);
	if (child == 0) {
		struct rlimit limit = {0, 0};
		(void)setrlimit(RLIMIT_CORE, &limit);
		*(volatile unsigned char *)address = 7;
		_exit(0);
	}
	int status;
	assert(waitpid(child, &status, 0) == child);
	assert(WIFSIGNALED(status));
	assert(WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS);
}

static hyper_stack_info_t info(hyper_stack_t *stack)
{
	hyper_stack_info_t result;
	assert(hyper_stack_get_info(stack, &result) == HYPER_NATIVE_STATUS_OK);
	return result;
}

static void growth(hyper_stack_t *stack)
{
	hyper_stack_info_t before = info(stack);
	memset((void *)before.base, 0xa5, before.size);
	guard_fault(before.base - 1);
	guard_fault(before.top);
	size_t grown_size = before.size + HYPER_NATIVE_PAGE_SIZE;
	fail_vmo = 1;
	assert(hyper_stack_grow(stack, grown_size) == HYPER_NATIVE_STATUS_NO_MEMORY);
	fail_vmo = 0;
	assert(info(stack).base == before.base && !backing_live);
	fail_map = 1;
	assert(hyper_stack_grow(stack, grown_size) == HYPER_NATIVE_STATUS_NO_MEMORY);
	fail_map = 0;
	assert(info(stack).size == before.size && !backing_live);
	guard_fault(before.base - 1);
	assert(hyper_stack_grow(stack, grown_size) == HYPER_NATIVE_STATUS_OK);
	hyper_stack_info_t after = info(stack);
	assert(after.top == before.top && after.capacity == before.capacity);
	assert(after.size == grown_size && after.base + after.size == after.top);
	for (uintptr_t p = after.base; p < before.base; ++p)
		assert(*(unsigned char *)p == 0);
	for (uintptr_t p = before.base; p < before.top; ++p)
		assert(*(unsigned char *)p == 0xa5);
	guard_fault(after.base - 1);
	guard_fault(after.top);
	assert(hyper_stack_grow(stack, after.capacity + HYPER_NATIVE_PAGE_SIZE) !=
	       HYPER_NATIVE_STATUS_OK);
	assert(hyper_stack_grow(stack, SIZE_MAX) != HYPER_NATIVE_STATUS_OK);
	assert(info(stack).base == after.base);
	fail_vmo = 1;
	assert(hyper_stack_grow(stack, after.size) == HYPER_NATIVE_STATUS_OK);
	fail_vmo = 0;
	assert(hyper_stack_grow(stack, before.size) == HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
}

struct grow_request {
	hyper_stack_t *stack;
	size_t size;
};

static void *grow_same_extent(void *raw)
{
	const struct grow_request *request = raw;
	assert(hyper_stack_grow(request->stack, request->size) == HYPER_NATIVE_STATUS_OK);
	return NULL;
}

static void concurrent_growth(hyper_stack_t *stack)
{
	hyper_stack_info_t before = info(stack);
	struct grow_request request = {stack, before.size + HYPER_NATIVE_PAGE_SIZE};
	pthread_t workers[4];
	for (size_t i = 0; i < 4; ++i)
		assert(pthread_create(&workers[i], NULL, grow_same_extent, &request) == 0);
	for (size_t i = 0; i < 4; ++i)
		assert(pthread_join(workers[i], NULL) == 0);
	hyper_stack_info_t after = info(stack);
	assert(after.size == request.size && after.top == before.top && !backing_live);
}

int main(void)
{
	const size_t page = HYPER_NATIVE_PAGE_SIZE;
	const size_t arena_size = (size_t)4 * 1024 * 1024 * 1024;
	void *arena = mmap(NULL, arena_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(arena != MAP_FAILED);
	hyper_stack_test_base = (uintptr_t)arena;
	regions[1] = (struct region){hyper_stack_test_base, arena_size, 0, 0};
	const size_t main_capacity = 8 * 1024 * 1024;
	const uintptr_t main_base = hyper_stack_test_base + arena_size - main_capacity - 2 * page;
	regions[3] = (struct region){main_base, main_capacity + 2 * page, 2 * page, 1};
	const uintptr_t main_low = main_base + page + main_capacity - 2 * page;
	assert(mprotect((void *)main_low, 2 * page, PROT_READ | PROT_WRITE) == 0);
	mappings[0] = (struct mapping){main_low, 2 * page, 3};
	hyper_native_startup_handle_t handles[] = {
		{HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_ROOT_VMAR, 0, 1},
		{HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_INITIAL_STACK_VMAR, 0, 3},
	};
	hyper_auxiliary_entry_t auxiliary[] = {
		{HYPER_AUXV_INITIAL_STACK_BASE, main_base},
		{HYPER_AUXV_INITIAL_STACK_CAPACITY, main_capacity},
		{HYPER_AUXV_INITIAL_STACK_SIZE, 2 * page},
		{HYPER_AUXV_MAIN_STACK_SIZE, 3 * page + 1},
	};
	hyper_startup_t startup = {.handle_count = 2,
				   .handles = handles,
				   .auxiliary_count = 3,
				   .auxiliary = auxiliary};
	hyper_stack_t *initial = NULL;
	assert(hyper_stack_initialize(NULL, &initial) == HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
	assert(hyper_stack_initialize(&startup, NULL) == HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
	fail_duplicate = 1;
	assert(hyper_stack_initialize(&startup, &initial) == HYPER_NATIVE_STATUS_NO_MEMORY);
	fail_duplicate = 0;
	fail_allocate = 1;
	assert(hyper_stack_initialize(&startup, &initial) == HYPER_NATIVE_STATUS_NO_MEMORY);
	assert(!root_live);
	fail_allocate = 0;
	startup.auxiliary_count = 4;
	assert(hyper_stack_initialize(&startup, &initial) == HYPER_NATIVE_STATUS_OK && initial);
	current_stack = initial;
	assert(info(initial).base != main_low);
	assert(info(initial).capacity == 256 * 1024 * 1024);
	assert(info(initial).size == 4 * page);
	/* Initialization must not free a bootstrap stack it still executes on. */
	assert(regions[3].size && regions[3].mapped == 2 * page);
	assert(hyper_stack_destroy(initial) == HYPER_NATIVE_STATUS_BAD_STATE);
	growth(initial);
	concurrent_growth(initial);
	hyper_stack_t *adopted_again = NULL;
	assert(hyper_stack_initialize(&startup, &adopted_again) == HYPER_NATIVE_STATUS_OK);
	assert(adopted_again == initial);

	hyper_stack_t *stack = NULL;
	/* A sparse reservation larger than the removed 248 MiB arena. */
	assert(hyper_stack_create(page, 300 * 1024 * 1024, &stack) == HYPER_NATIVE_STATUS_OK);
	assert(info(stack).capacity == 300 * 1024 * 1024 && info(stack).size == page);
	guard_fault(info(stack).base - 1);
	guard_fault(info(stack).top);
	assert(hyper_stack_destroy(stack) == HYPER_NATIVE_STATUS_OK);
	stack = NULL;

	assert(hyper_stack_create(1, page, &stack) == HYPER_NATIVE_STATUS_OK);
	assert(info(stack).size == page && info(stack).capacity == 256 * 1024 * 1024);
	assert(hyper_stack_destroy(stack) == HYPER_NATIVE_STATUS_OK);
	stack = NULL;
	assert(hyper_stack_create(SIZE_MAX, 0, &stack) != HYPER_NATIVE_STATUS_OK);
	assert(hyper_stack_create(page, SIZE_MAX, &stack) != HYPER_NATIVE_STATUS_OK);
	assert(hyper_stack_create(4 * page, 2 * page, &stack) != HYPER_NATIVE_STATUS_OK);
	fail_allocate = 1;
	assert(hyper_stack_create(2 * page, 0, &stack) == HYPER_NATIVE_STATUS_NO_MEMORY);
	fail_allocate = 0;
	fail_vmo = 1;
	assert(hyper_stack_create(2 * page, 0, &stack) == HYPER_NATIVE_STATUS_NO_MEMORY);
	fail_vmo = 0;
	fail_map = 1;
	assert(hyper_stack_create(2 * page, 0, &stack) == HYPER_NATIVE_STATUS_NO_MEMORY);
	fail_map = 0;
	assert(!backing_live);
	// Failed construction must retain the reservation if VMAR destruction fails.
	fail_map = 1;
	fail_destroy = 1;
	assert(hyper_stack_create(2 * page, 0, &stack) == HYPER_NATIVE_STATUS_NO_MEMORY);
	assert(stack == NULL && !backing_live);
	fail_map = 0;
	fail_destroy = 0;
	// Creating again retries quarantined cleanup before reusing that address.
	assert(hyper_stack_create(2 * page, 0, &stack) == HYPER_NATIVE_STATUS_OK);
	assert(info(stack).capacity >= 1024 * 1024);
	concurrent_growth(stack);
	growth(stack);
	hyper_stack_info_t partial = info(stack);
	assert(hyper_stack_grow(stack, partial.capacity) == HYPER_NATIVE_STATUS_OK);
	hyper_stack_info_t full = info(stack);
	assert(full.size == full.capacity && full.top == partial.top);
	for (uintptr_t p = full.base; p < partial.base; ++p)
		assert(*(unsigned char *)p == 0);
	guard_fault(full.base - 1);
	guard_fault(full.top);
	current_stack = stack;
	assert(hyper_stack_destroy(stack) == HYPER_NATIVE_STATUS_BAD_STATE);
	current_stack = initial;
	hyper_stack_t *managed = NULL;
	assert(hyper_stack_create(page, 4 * page, &managed) == HYPER_NATIVE_STATUS_OK);
	hyper_stack_claim_runtime(managed);
	assert(hyper_stack_destroy(managed) == HYPER_NATIVE_STATUS_BAD_STATE);
	hyper_stack_release_runtime(managed); /* ownership transferred; no further access */
	fail_unmap = 1;
	assert(hyper_stack_destroy(stack) == HYPER_NATIVE_STATUS_NO_MEMORY);
	fail_unmap = 0;
	hyper_stack_info_t unavailable;
	assert(hyper_stack_get_info(stack, &unavailable) == HYPER_NATIVE_STATUS_BAD_STATE);
	assert(hyper_stack_grow(stack, full.capacity) == HYPER_NATIVE_STATUS_BAD_STATE);
	assert(*(unsigned char *)partial.base == 0); /* failed unmap preserved backing */
	fail_destroy = 1;
	assert(hyper_stack_destroy(stack) == HYPER_NATIVE_STATUS_NO_MEMORY);
	fail_destroy = 0;
	assert(hyper_stack_destroy(stack) == HYPER_NATIVE_STATUS_OK);
	assert(!backing_live);
	size_t live_regions = 0;
	for (size_t i = 1; i < 128; ++i)
		live_regions += regions[i].size != 0;
	assert(live_regions == 3); /* root, bootstrap and final main stack */
	/* Main and worker stacks follow exactly the same retirement path. */
	current_stack = NULL;
	fail_destroy = 1;
	hyper_stack_release_runtime(initial);
	assert(hyper_stack_get_info(initial, &unavailable) == HYPER_NATIVE_STATUS_BAD_STATE);
	assert(regions[4].size && !regions[4].mapped);
	fail_destroy = 0;
	assert(hyper_stack_create(page, 4 * page, &stack) == HYPER_NATIVE_STATUS_OK);
	assert(regions[3].size && regions[3].mapped == 2 * page);
	assert(hyper_stack_destroy(stack) == HYPER_NATIVE_STATUS_OK);
	assert(munmap(arena, arena_size) == 0);
	return 0;
}
