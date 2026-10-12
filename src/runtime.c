/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hyper/launch.h>
#include <hyper/heap.h>
#include <hyper/thread.h>
#include <hyper/system.h>
#include "stack-internal.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

static hyper_startup_t process_startup;
static hyper_native_startup_handle_t application_handles[HYPER_NATIVE_STARTUP_MAX_HANDLES];
static atomic_bool initialized;

const hyper_startup_t *hyper_runtime_startup(void)
{
	return atomic_load_explicit(&initialized, memory_order_acquire) ? &process_startup : NULL;
}

/* Startup strings cannot remain in the disposable kernel bootstrap stack. */
static char **copy_strings(char *const *strings, size_t count)
{
	char **copy = calloc(count + 1, sizeof(*copy));
	if (!copy)
		return NULL;
	for (size_t i = 0; i < count; ++i) {
		size_t bytes = strlen(strings[i]) + 1;
		copy[i] = malloc(bytes);
		if (!copy[i]) {
			while (i)
				free(copy[--i]);
			free(copy);
			return NULL;
		}
		memcpy(copy[i], strings[i], bytes);
	}
	return copy;
}

/* Invoked through the relocated libhyper image, not the interpreter's static
 * copy of runtime primitives, so constructors and main share one heap. */
hyper_native_status_t hyper_runtime_initialize(const uintptr_t *initial_stack)
{
	if (atomic_load_explicit(&initialized, memory_order_acquire))
		return HYPER_NATIVE_STATUS_OK;
	hyper_startup_t startup;
	hyper_native_status_t status = hyper_startup_parse(initial_stack, &startup);
	if (status != HYPER_NATIVE_STATUS_OK) {
		return status;
	}
	status = hyper_heap_initialize(&startup);
	if (status != HYPER_NATIVE_STATUS_OK)
		return status;
	hyper_stack_t *stack;
	status = hyper_stack_initialize(&startup, &stack);
	if (status != HYPER_NATIVE_STATUS_OK)
		return status;
	status = hyper_runtime_thread_attach_stack(stack);
	if (status != HYPER_NATIVE_STATUS_OK)
		return status;
	/* Initial loader/CRT initialization runs before secondary threads exist. */
	status = hyper_runtime_capabilities_initialize(&startup);
	if (status != HYPER_NATIVE_STATUS_OK)
		return status;
	/* The one-way bootstrap handoff consumes this handle. Do not expose it
	 * to language startup owners, which may close every delegated handle. */
	size_t count = 0;
	for (size_t i = 0; i < startup.handle_count; ++i) {
		if (startup.handles[i].purpose !=
		    HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_INITIAL_STACK_VMAR)
			application_handles[count++] = startup.handles[i];
	}
	hyper_auxiliary_entry_t *auxiliary =
		calloc(startup.auxiliary_count + 1, sizeof(*auxiliary));
	if (!auxiliary)
		return HYPER_NATIVE_STATUS_NO_MEMORY;
	for (size_t i = 0; i < startup.auxiliary_count; ++i) {
		auxiliary[i] = startup.auxiliary[i];
		if (auxiliary[i].key == HYPER_AUXV_STARTUP_HANDLES)
			auxiliary[i].value = (uintptr_t)application_handles;
		if (auxiliary[i].key == HYPER_AUXV_STARTUP_HANDLE_COUNT)
			auxiliary[i].value = count;
	}
	char **arguments = copy_strings(startup.arguments, startup.argument_count);
	char **environment = copy_strings(startup.environment, startup.environment_count);
	if (!arguments || !environment)
		return HYPER_NATIVE_STATUS_NO_MEMORY;
	hyper_stack_info_t info;
	status = hyper_stack_get_info(stack, &info);
	if (status != HYPER_NATIVE_STATUS_OK)
		return status;
	size_t page;
	status = hyper_page_size(&page);
	if (status != HYPER_NATIVE_STATUS_OK)
		return status;
	for (size_t i = 0; i < startup.auxiliary_count; ++i) {
		if (auxiliary[i].key == HYPER_AUXV_INITIAL_STACK_BASE)
			auxiliary[i].value = info.top - info.capacity - page;
		if (auxiliary[i].key == HYPER_AUXV_INITIAL_STACK_CAPACITY)
			auxiliary[i].value = info.capacity;
		if (auxiliary[i].key == HYPER_AUXV_INITIAL_STACK_SIZE)
			auxiliary[i].value = info.size;
	}
	process_startup = startup;
	process_startup.arguments = arguments;
	process_startup.environment = environment;
	process_startup.auxiliary = auxiliary;
	process_startup.handles = application_handles;
	process_startup.handle_count = count;
	atomic_store_explicit(&initialized, 1, memory_order_release);
	return HYPER_NATIVE_STATUS_OK;
}
