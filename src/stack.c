/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/launch.h>
#include <hyper/stack.h>
#include <hyper/syscall.h>
#include <hyper/system.h>
#include "stack-internal.h"
#include <stdatomic.h>
#include "mutex-internal.h"
#include <stdbool.h>
#include <stdlib.h>

static size_t page_size;
#define DEFAULT_CAPACITY ((size_t)256 * 1024 * 1024)

struct hyper_stack {
	uintptr_t top;
	size_t capacity;
	size_t size;
	hyper_native_handle_t vmar;
	bool managed;
	bool retiring;
	bool garbage;
	struct hyper_stack *next;
};

/* The registry owns stack descriptors and retirement retries. VMARs own the
 * address reservations, including guards and uncommitted capacity.
 * Slow VMAR operations may block; contending callers sleep on a Native atomic
 * wait. No thread termination is awaited while holding this mutex. */
static hyper_mutex_t registry_lock;
static atomic_bool initialized;
static hyper_native_handle_t root_vmar;
static uintptr_t stack_ceiling;
static hyper_stack_t *stacks;
/* Bootstrap handoff reference; ownership lives in the same registry as every
 * other stack. Initialization only runs before secondary threads exist. */
static hyper_stack_t *initial_stack;

static void close_owned(hyper_native_handle_t handle)
{
	if (hyper_handle_close(handle) != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
}

static inline bool page_round_up(size_t size, size_t *rounded)
{
	if (!page_size || !size || size > SIZE_MAX - (page_size - 1))
		return false;
	*rounded = (size + page_size - 1) & ~(page_size - 1);
	return true;
}

/* Register a runtime-created reservation. All descriptors have the same
 * allocation, registry and retirement rules. Caller
 * serializes publication (startup is single-threaded). */
static inline void register_stack(hyper_stack_t *stack, uintptr_t base, size_t capacity, size_t size,
			   hyper_native_handle_t vmar)
{
	*stack = (hyper_stack_t){.top = base + page_size + capacity,
				 .capacity = capacity,
				 .size = size,
				 .vmar = vmar,
				 .next = stacks};
	stacks = stack;
}

/* Publication is the successful map. No descriptor allocation follows it;
 * all failed map backing is released before the error returns. */
static hyper_native_status_t extend(hyper_stack_t *stack, size_t size)
{
	size_t added = size - stack->size;
	hyper_call_result_t backing = hyper_vmo_create(added);
	if (backing.status != HYPER_NATIVE_STATUS_OK)
		return backing.status;
	hyper_native_status_t status = hyper_vmar_map(
		stack->vmar, backing.value0, 0, stack->top - size, added,
		HYPER_NATIVE_VMAR_PERMISSION_READ | HYPER_NATIVE_VMAR_PERMISSION_WRITE);
	if (status == HYPER_NATIVE_STATUS_OK)
		stack->size = size;
	close_owned(backing.value0);
	return status;
}

/* A failed retirement retains its descriptor, VMAR and VA reservation. It may
 * already have unmapped the payload; it must never permit grow or VA reuse.
 * Internal cleanup retries these owned records on subsequent creation or runtime release. */
static hyper_native_status_t retire(hyper_stack_t *stack)
{
	stack->retiring = true;
	if (stack->size) {
		hyper_native_status_t status =
			hyper_vmar_unmap(stack->vmar, stack->top - stack->size, stack->size);
		if (status != HYPER_NATIVE_STATUS_OK)
			return status;
		stack->size = 0;
	}
	hyper_native_status_t status = hyper_vmar_destroy(stack->vmar);
	/* Successful VMAR destruction consumes its handle in the Native ABI. */
	return status;
}

static void reclaim_retired_stacks(void)
{
	hyper_stack_t **link = &stacks;
	while (*link) {
		hyper_stack_t *stack = *link;
		if (stack->garbage && retire(stack) == HYPER_NATIVE_STATUS_OK) {
			*link = stack->next;
			free(stack);
		} else {
			link = &stack->next;
		}
	}
}

static hyper_native_status_t create_stack(size_t size, size_t capacity, hyper_stack_t **result);

hyper_native_status_t hyper_stack_initialize(const hyper_startup_t *startup, hyper_stack_t **result)
{
	if (!startup || !result || (startup->auxiliary_count && !startup->auxiliary))
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	if (atomic_load_explicit(&initialized, memory_order_acquire)) {
		*result = initial_stack;
		return HYPER_NATIVE_STATUS_OK;
	}
	hyper_native_status_t query_status = hyper_page_size(&page_size);
	if (query_status != HYPER_NATIVE_STATUS_OK)
		return query_status;
	hyper_native_handle_t root;
	hyper_native_status_t status;
	status = hyper_startup_find_handle(startup, HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_ROOT_VMAR,
					   &root);
	if (status != HYPER_NATIVE_STATUS_OK)
		return status;
	/* The kernel provides only a disposable bootstrap stack. Final stack
	 * placement belongs to this runtime and uses the same allocator as workers. */
	hyper_call_result_t limits =
		hyper_system_config(HYPER_NATIVE_SYSTEM_CONFIG_APPLICATION_ADDRESS_LIMIT);
	if (limits.status != HYPER_NATIVE_STATUS_OK)
		return limits.status;
	if (limits.value1 || !limits.value0 || limits.value0 % page_size)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;

	/* PT_GNU_STACK is an application request, not a bootstrap-stack size. */
	size_t requested = 256 * 1024;
	for (size_t i = 0; i < startup->auxiliary_count; ++i) {
		if (startup->auxiliary[i].key == HYPER_AUXV_MAIN_STACK_SIZE &&
		    startup->auxiliary[i].value)
			requested = startup->auxiliary[i].value;
	}
	/* The main entry vector lives at the final top. Retain at least one page
	 * below it for the handoff/CRT frames, including tiny ELF stack requests. */
	if (startup->argument_count > 4096 || startup->environment_count > 4096 ||
	    startup->auxiliary_count > 256)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	size_t words = 3 + startup->argument_count + startup->environment_count +
		       2 * (startup->auxiliary_count + 1);
	size_t entry_bytes = (words * sizeof(uintptr_t) + 15) & ~(size_t)15;
	if (entry_bytes > SIZE_MAX - page_size)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	if (requested < entry_bytes + page_size)
		requested = entry_bytes + page_size;
	hyper_call_result_t owned_root = hyper_handle_duplicate(root, HYPER_NATIVE_RIGHT_MAP);
	if (owned_root.status != HYPER_NATIVE_STATUS_OK)
		return owned_root.status;
	root_vmar = owned_root.value0;
	stack_ceiling = limits.value0;
	hyper_stack_t *stack;
	status = create_stack(requested, 0, &stack);
	if (status != HYPER_NATIVE_STATUS_OK) {
		close_owned(root_vmar);
		root_vmar = 0;
		return status;
	}
	hyper_stack_claim_runtime(stack);
	initial_stack = stack;
	atomic_store_explicit(&initialized, true, memory_order_release);
	*result = initial_stack;
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_stack_create(size_t size, size_t capacity, hyper_stack_t **result)
{
	if (!result)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	*result = NULL;
	if (!atomic_load_explicit(&initialized, memory_order_acquire))
		return HYPER_NATIVE_STATUS_BAD_STATE;
	return create_stack(size, capacity, result);
}

static hyper_native_status_t create_stack(size_t size, size_t capacity, hyper_stack_t **result)
{
	if (!page_round_up(size, &size))
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	if (capacity && capacity < size)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	if (capacity < size)
		capacity = size;
	if (capacity < DEFAULT_CAPACITY)
		capacity = DEFAULT_CAPACITY;
	if (!page_round_up(capacity, &capacity) || size > capacity ||
	    capacity > SIZE_MAX - 2 * page_size)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	hyper_stack_t *stack = calloc(1, sizeof(*stack));
	if (!stack)
		return HYPER_NATIVE_STATUS_NO_MEMORY;
	hyper_mutex_lock(&registry_lock);
	reclaim_retired_stacks();
	size_t extent = capacity + 2 * page_size;
	hyper_call_result_t region = hyper_vmar_allocate(
		root_vmar, stack_ceiling > extent ? stack_ceiling - extent : 0, extent, 0);
	if (region.status != HYPER_NATIVE_STATUS_OK) {
		hyper_mutex_unlock(&registry_lock);
		free(stack);
		return region.status;
	}
	register_stack(stack, region.value1, capacity, 0, region.value0);
	hyper_native_status_t status = extend(stack, size);
	if (status == HYPER_NATIVE_STATUS_OK)
		*result = stack;
	else {
		stack->garbage = true;
		reclaim_retired_stacks();
	}
	hyper_mutex_unlock(&registry_lock);
	return status;
}

hyper_native_status_t hyper_stack_get_info(hyper_stack_t *stack, hyper_stack_info_t *result)
{
	if (!stack || !result)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	hyper_mutex_lock(&registry_lock);
	if (stack->retiring) {
		hyper_mutex_unlock(&registry_lock);
		return HYPER_NATIVE_STATUS_BAD_STATE;
	}
	*result = (hyper_stack_info_t){stack->top - stack->size, stack->top, stack->size,
				       stack->capacity};
	hyper_mutex_unlock(&registry_lock);
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_stack_grow(hyper_stack_t *stack, size_t size)
{
	if (!stack || !page_round_up(size, &size))
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	hyper_mutex_lock(&registry_lock);
	hyper_native_status_t status;
	if (stack->retiring)
		status = HYPER_NATIVE_STATUS_BAD_STATE;
	else if (size < stack->size || size > stack->capacity)
		status = HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	else
		status = size == stack->size ? HYPER_NATIVE_STATUS_OK : extend(stack, size);
	hyper_mutex_unlock(&registry_lock);
	return status;
}

hyper_native_status_t hyper_stack_destroy(hyper_stack_t *stack)
{
	if (!stack)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	hyper_mutex_lock(&registry_lock);
	if (stack->managed || stack == hyper_stack_current()) {
		hyper_mutex_unlock(&registry_lock);
		return HYPER_NATIVE_STATUS_BAD_STATE;
	}
	hyper_native_status_t status = retire(stack);
	if (status == HYPER_NATIVE_STATUS_OK) {
		hyper_stack_t **link = &stacks;
		while (*link != stack)
			link = &(*link)->next;
		*link = stack->next;
		free(stack);
	}
	hyper_mutex_unlock(&registry_lock);
	return status;
}

void hyper_stack_claim_runtime(hyper_stack_t *stack)
{
	hyper_mutex_lock(&registry_lock);
	stack->managed = true;
	hyper_mutex_unlock(&registry_lock);
}

void hyper_stack_release_runtime(hyper_stack_t *stack)
{
	/* Caller has observed kernel termination (or never published a thread).
	 * Cleanup failure keeps explicit ownership, not a dangling pool slot. */
	hyper_mutex_lock(&registry_lock);
	stack->managed = false;
	stack->garbage = true;
	reclaim_retired_stacks();
	hyper_mutex_unlock(&registry_lock);
}
