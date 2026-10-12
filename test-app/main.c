/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hyper/native.h>
#include <hyper/startup.h>
#include <hyper/syscall.h>
#include <string.h>

static int verify_memory_primitives(void)
{
	const unsigned char source[] = "HypeR";
	unsigned char destination[sizeof(source)] = {0};

	if (memcpy(destination, source, sizeof(source)) != destination) {
		return 1;
	}
	if (memcmp(destination, source, sizeof(source)) != 0) {
		return 1;
	}
	return strlen((const char *)destination) != sizeof(source) - 1;
}

int hyper_main(const hyper_startup_t *startup)
{
	const hyper_call_result_t abi = hyper_abi_query();

	if (startup == NULL || startup->argument_count == 0) {
		return 1;
	}
	if (abi.status != HYPER_NATIVE_STATUS_OK || abi.value0 != HYPER_NATIVE_ABI_REVISION ||
	    (abi.value1 & HYPER_NATIVE_FEATURE_CORE) == 0) {
		return 1;
	}
	if (verify_memory_primitives() != 0) {
		return 1;
	}
	return hyper_thread_yield() == HYPER_NATIVE_STATUS_OK ? 0 : 1;
}
