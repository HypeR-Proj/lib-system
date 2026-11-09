/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/random.h>
#include <hyper/startup.h>
#include <hyper/syscall.h>
#include <assert.h>
#include <stdio.h>
static int available, fail_second;
static unsigned closed, original_closed;

hyper_native_status_t hyper_startup_find_handle(const hyper_startup_t *startup, uint32_t purpose,
						hyper_native_handle_t *handle)
{
	(void)startup;
	if (!available)
		return HYPER_NATIVE_STATUS_NOT_FOUND;
	if (purpose == HYPER_RANDOM_CLIENT)
		*handle = 100;
	else if (purpose == HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_ROOT_DIRECTORY && fail_second)
		*handle = 200;
	else
		return HYPER_NATIVE_STATUS_NOT_FOUND;
	return 0;
}

hyper_call_result_t hyper_handle_get_info(uint64_t handle, hyper_native_handle_info_t *output,
					  size_t capacity)
{
	assert((handle == 100 || handle == 200) && capacity == sizeof(hyper_native_handle_info_t));
	hyper_native_handle_info_t *info = output;
	info->rights = HYPER_NATIVE_RIGHT_WRITE | HYPER_NATIVE_RIGHT_WAIT |
		       HYPER_NATIVE_RIGHT_DUPLICATE | HYPER_NATIVE_RIGHT_TRANSFER;
	return (hyper_call_result_t){0, sizeof(*info), 0};
}

hyper_call_result_t hyper_handle_duplicate(uint64_t handle, uint64_t rights)
{
	assert(rights == (HYPER_NATIVE_RIGHT_WRITE | HYPER_NATIVE_RIGHT_WAIT |
			  HYPER_NATIVE_RIGHT_DUPLICATE | HYPER_NATIVE_RIGHT_TRANSFER));
	if (handle == 200)
		return (hyper_call_result_t){HYPER_NATIVE_STATUS_RESOURCE_LIMIT, 0, 0};
	assert(handle == 100);
	return (hyper_call_result_t){0, 101, 0};
}

hyper_native_status_t hyper_handle_close(uint64_t handle)
{
	if (handle == 100)
		original_closed++;
	else {
		assert(handle == 101);
		closed++;
	}
	return 0;
}

int main(void)
{
	hyper_startup_t startup = {0};
	assert(!hyper_runtime_capabilities_initialize(&startup));
	assert(!hyper_runtime_capability(HYPER_RANDOM_CLIENT));
	available = fail_second = 1;
	assert(hyper_runtime_capabilities_initialize(&startup) ==
	       HYPER_NATIVE_STATUS_RESOURCE_LIMIT);
	assert(!hyper_runtime_capability(HYPER_RANDOM_CLIENT) && closed == 1 && !original_closed);
	fail_second = 0;
	assert(!hyper_runtime_capabilities_initialize(&startup));
	assert(!hyper_handle_close(100));
	assert(hyper_runtime_capability(HYPER_RANDOM_CLIENT) == 101 && original_closed == 1);
	puts("Optional random capability retention and initialization rollback: PASS");
}
