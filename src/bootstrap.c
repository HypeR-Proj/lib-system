/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/launch.h>
#include <hyper/startup.h>
#include <hyper/stack.h>
#include <hyper/syscall.h>
#include <hyper/system.h>
#include <string.h>

/* These records outlive the abandoned bootstrap C frames. Startup runs once,
 * before constructors or any secondary thread can execute. */
static hyper_native_handle_t bootstrap_vmar;
static uintptr_t bootstrap_base;
static size_t bootstrap_size;
static uintptr_t loader_base, loader_size;
static hyper_native_handle_t loader_root, startup_channel;
static void (*continuation)(const uintptr_t *);

extern _Noreturn void __hyper_runtime_switch_stack(uintptr_t sp, void (*entry)(const uintptr_t *));

static _Noreturn void finish(const uintptr_t *stack)
{
	/* No return address or live pointer refers to bootstrap storage now. */
	hyper_native_status_t status =
		hyper_vmar_unmap(bootstrap_vmar, bootstrap_base, bootstrap_size);
	if (status == HYPER_NATIVE_STATUS_OK)
		status = hyper_vmar_destroy(bootstrap_vmar);
	if (status != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(status);
	bootstrap_vmar = 0;
	if (loader_size) {
		status = hyper_vmar_unmap(loader_root, loader_base, loader_size);
		if (status)
			hyper_process_exit(status);
	}
	if (startup_channel) {
		hyper_launch_result_t result = {.status = 0};
		status = hyper_byte_channel_write(startup_channel, &result, sizeof(result));
		(void)hyper_handle_close(startup_channel);
		startup_channel = 0;
		if (status)
			hyper_process_exit(status);
	}
	continuation(stack);
	hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
}

_Noreturn void hyper_runtime_start(const uintptr_t *stack, void (*entry)(const uintptr_t *))
{
	hyper_startup_t original;
	size_t page;
	if (!entry || continuation ||
	    hyper_startup_parse(stack, &original) != HYPER_NATIVE_STATUS_OK ||
	    hyper_page_size(&page) != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(HYPER_NATIVE_STATUS_BAD_STATE);
	uintptr_t base = 0, capacity = 0;
	unsigned seen = 0;
	for (size_t i = 0; i < original.auxiliary_count; ++i) {
		hyper_auxiliary_entry_t item = original.auxiliary[i];
		unsigned bit = 0;
		if (item.key == HYPER_AUXV_INITIAL_STACK_BASE) {
			base = item.value;
			bit = 1;
		}
		if (item.key == HYPER_AUXV_INITIAL_STACK_CAPACITY) {
			capacity = item.value;
			bit = 2;
		}
		if (item.key == HYPER_AUXV_INITIAL_STACK_SIZE) {
			bootstrap_size = item.value;
			bit = 4;
		}
		if (item.key == HYPER_AUXV_LOADER_BASE)
			loader_base = item.value;
		if (item.key == HYPER_AUXV_LOADER_SIZE)
			loader_size = item.value;
		if (item.key == HYPER_AUXV_STARTUP_CHANNEL)
			startup_channel = item.value;
		if (seen & bit)
			hyper_process_exit(HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
		seen |= bit;
	}
	if (seen != 7 || !bootstrap_size || bootstrap_size > capacity ||
	    (base | capacity | bootstrap_size) % page || page > UINTPTR_MAX / 2 ||
	    capacity > UINTPTR_MAX - 2 * page || base > UINTPTR_MAX - capacity - 2 * page)
		hyper_process_exit(HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
	hyper_native_status_t status = hyper_startup_find_handle(
		&original, HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_INITIAL_STACK_VMAR, &bootstrap_vmar);
	if (status != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(status);
	status = hyper_startup_find_handle(&original, HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_ROOT_VMAR,
					   &loader_root);
	if (status)
		hyper_process_exit(status);
	status = hyper_runtime_initialize(stack);
	if (status != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(status);
	/* Keep only scalar ownership information about the old mapping. */
	bootstrap_base = base + page + capacity - bootstrap_size;
	const hyper_startup_t *startup = hyper_runtime_startup();
	hyper_stack_info_t info;
	status = hyper_stack_get_info(hyper_stack_current(), &info);
	if (status != HYPER_NATIVE_STATUS_OK)
		hyper_process_exit(status);
	size_t words = 1 + startup->argument_count + 1 + startup->environment_count + 1 +
		       2 * (startup->auxiliary_count + 1);
	size_t bytes = (words * sizeof(uintptr_t) + 15) & ~(size_t)15;
	if (bytes > SIZE_MAX - page || info.size < bytes + page)
		hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
	uintptr_t *out = (uintptr_t *)(info.top - bytes);
	uintptr_t *cursor = out;
	*cursor++ = startup->argument_count;
	for (size_t i = 0; i < startup->argument_count; ++i)
		*cursor++ = (uintptr_t)startup->arguments[i];
	*cursor++ = 0;
	for (size_t i = 0; i < startup->environment_count; ++i)
		*cursor++ = (uintptr_t)startup->environment[i];
	*cursor++ = 0;
	memcpy(cursor, startup->auxiliary,
	       (startup->auxiliary_count + 1) * sizeof(hyper_auxiliary_entry_t));
	continuation = entry;
	__hyper_runtime_switch_stack((uintptr_t)out, finish);
}
