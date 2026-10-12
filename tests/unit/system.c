/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/system.h>
#include <hyper/syscall.h>
#include <assert.h>
#include <stdlib.h>

static hyper_call_result_t reply;
static unsigned calls;

hyper_call_result_t hyper_system_config(uint64_t key)
{
	assert(key == HYPER_NATIVE_SYSTEM_CONFIG_PAGE_SIZE);
	++calls;
	return reply;
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	size_t value = 123;
	assert(hyper_page_size(NULL) == HYPER_NATIVE_STATUS_INVALID_ARGUMENT && !calls);
	reply.status = HYPER_NATIVE_STATUS_NOT_SUPPORTED;
	assert(hyper_page_size(&value) == reply.status && value == 123);
	reply.status = HYPER_NATIVE_STATUS_OK;
	uint64_t invalid[] = {0, 1, 4095, 12288, UINT64_MAX, UINT64_C(1) << 63};
	for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
		reply.value0 = invalid[i];
		assert(hyper_page_size(&value) == HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
		assert(value == 123);
	}
	reply.value0 = 0;
	for (const char *digit = argv[1]; *digit; ++digit) {
		assert(*digit >= '0' && *digit <= '9');
		reply.value0 = reply.value0 * 10 + (unsigned)(*digit - '0');
	}
	reply.value1 = 1;
	assert(hyper_page_size(&value) == HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
	reply.value1 = 0;
	assert(hyper_page_size(&value) == HYPER_NATIVE_STATUS_OK && value == reply.value0);
	unsigned before = calls;
	reply.status = HYPER_NATIVE_STATUS_INTERNAL;
	assert(hyper_page_size(&value) == HYPER_NATIVE_STATUS_OK && calls == before);
	return 0;
}
